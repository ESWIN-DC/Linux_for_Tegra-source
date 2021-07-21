/*
 * Copyright (c) 2008-2009,2012-2014 Travis Geiselbrecht
 * Copyright (c) 2009 Corey Tabaka
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <debug.h>
#include <trace.h>
#include <assert.h>
#include <err.h>
#include <list.h>
#include <rand.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <kernel/spinlock.h>
#include <lib/heap.h>

#define LOCAL_TRACE 0

#define DEBUG_HEAP 0
#define ALLOC_FILL 0x99
#define FREE_FILL 0x77
#define PADDING_FILL 0x55
#define PADDING_SIZE 64

#define HEAP_MAGIC 'HEAP'

#if WITH_KERNEL_VM

#include <kernel/vm.h>
/* we will use kalloc routines to back our heap */
#if !defined(HEAP_GROW_SIZE)
#define HEAP_GROW_SIZE (4 * 1024 * 1024) /* size the heap grows by when it runs out of memory */
#endif

STATIC_ASSERT(IS_PAGE_ALIGNED(HEAP_GROW_SIZE));

#elif WITH_STATIC_HEAP

#if !defined(HEAP_START) || !defined(HEAP_LEN)
#error WITH_STATIC_HEAP set but no HEAP_START or HEAP_LEN defined
#endif

#else
/* not a static vm, not using the kernel vm */
extern int _end;
extern int _end_of_ram;

/* default to using up the rest of memory after the kernel ends */
/* may be modified by other parts of the system */
uintptr_t _heap_start = (uintptr_t)&_end;
uintptr_t _heap_end = (uintptr_t)&_end_of_ram;

#define HEAP_START ((uintptr_t)_heap_start)
#define HEAP_LEN ((uintptr_t)_heap_end - HEAP_START)
#endif

struct free_heap_chunk {
	struct list_node node;
	size_t len;
};

struct heap {
	void *base;
	size_t len;
	size_t remaining;
	size_t low_watermark;
	mutex_t lock;
	struct list_node free_list;
	struct list_node delayed_free_list;
	spin_lock_t delayed_free_lock;
};

// heap static vars
static struct heap theheap;

// structure placed at the beginning every allocation
struct alloc_struct_begin {
#if LK_DEBUGLEVEL > 1
	unsigned int magic;
#endif
	void *ptr;
	size_t size;
#if DEBUG_HEAP
	void *padding_start;
	size_t padding_size;
#endif
};

static ssize_t heap_grow(size_t len);

// try to insert this free chunk into the free list, consuming the chunk by merging it with
// nearby ones if possible. Returns base of whatever chunk it became in the list.
static struct free_heap_chunk *heap_insert_free_chunk(struct free_heap_chunk *chunk)
{
#if LK_DEBUGLEVEL > INFO
	vaddr_t chunk_end = (vaddr_t)chunk + chunk->len;
#endif

	LTRACEF("chunk ptr %p, size 0x%zx\n", chunk, chunk->len);

	struct free_heap_chunk *next_chunk;
	struct free_heap_chunk *last_chunk;

	mutex_acquire(&theheap.lock);

	theheap.remaining += chunk->len;

	// walk through the list, finding the node to insert before
	list_for_every_entry(&theheap.free_list, next_chunk, struct free_heap_chunk, node) {
		if (chunk < next_chunk) {
			DEBUG_ASSERT(chunk_end <= (vaddr_t)next_chunk);

			list_add_before(&next_chunk->node, &chunk->node);

			goto try_merge;
		}
	}

	// walked off the end of the list, add it at the tail
	list_add_tail(&theheap.free_list, &chunk->node);

	// try to merge with the previous chunk
try_merge:
	last_chunk = list_prev_type(&theheap.free_list, &chunk->node, struct free_heap_chunk, node);
	if (last_chunk) {
		if ((vaddr_t)last_chunk + last_chunk->len == (vaddr_t)chunk) {
			// easy, just extend the previous chunk
			last_chunk->len += chunk->len;

			// remove ourself from the list
			list_delete(&chunk->node);

			// set the chunk pointer to the newly extended chunk, in case
			// it needs to merge with the next chunk below
			chunk = last_chunk;
		}
	}

	// try to merge with the next chunk
	if (next_chunk) {
		if ((vaddr_t)chunk + chunk->len == (vaddr_t)next_chunk) {
			// extend our chunk
			chunk->len += next_chunk->len;

			// remove them from the list
			list_delete(&next_chunk->node);
		}
	}

	mutex_release(&theheap.lock);

	return chunk;
}

static struct free_heap_chunk *heap_create_free_chunk(void *ptr, size_t len, bool allow_debug)
{
	DEBUG_ASSERT((len % sizeof(void *)) == 0); // size must be aligned on pointer boundary

#if DEBUG_HEAP
	if (allow_debug)
		memset(ptr, FREE_FILL, len);
#endif

	struct free_heap_chunk *chunk = (struct free_heap_chunk *)ptr;
	chunk->len = len;

	return chunk;
}

static void heap_free_delayed_list(void)
{
	struct list_node list;

	list_initialize(&list);

	spin_lock_saved_state_t state;
	spin_lock_irqsave(&theheap.delayed_free_lock, state);

	struct free_heap_chunk *chunk;
	while ((chunk = list_remove_head_type(&theheap.delayed_free_list, struct free_heap_chunk, node))) {
		list_add_head(&list, &chunk->node);
	}
	spin_unlock_irqrestore(&theheap.delayed_free_lock, state);

	while ((chunk = list_remove_head_type(&list, struct free_heap_chunk, node))) {
		LTRACEF("freeing chunk %p\n", chunk);
		heap_insert_free_chunk(chunk);
	}
}

void *heap_alloc(size_t size, unsigned int alignment)
{
	void *ptr;
#if DEBUG_HEAP
	size_t original_size = size;
#endif

	LTRACEF("size %zd, align %d\n", size, alignment);

	// deal with the pending free list
	if (unlikely(!list_is_empty(&theheap.delayed_free_list))) {
		heap_free_delayed_list();
	}

	// alignment must be power of 2
	if (alignment & (alignment - 1))
		return NULL;

	// we always put a size field + base pointer + magic in front of the allocation
	size += sizeof(struct alloc_struct_begin);
#if DEBUG_HEAP
	size += PADDING_SIZE;
#endif

	// make sure we allocate at least the size of a struct free_heap_chunk so that
	// when we free it, we can create a struct free_heap_chunk struct and stick it
	// in the spot
	if (size < sizeof(struct free_heap_chunk))
		size = sizeof(struct free_heap_chunk);

	// round up size to a multiple of native pointer size
	size = ROUNDUP(size, sizeof(void *));

	// deal with nonzero alignments
	if (alignment > 0) {
		if (alignment < 16)
			alignment = 16;

		// add alignment for worst case fit
		size += alignment;
	}

#if WITH_KERNEL_VM
	int retry_count = 0;
retry:
#endif
	mutex_acquire(&theheap.lock);

	// walk through the list
	ptr = NULL;
	struct free_heap_chunk *chunk;
	list_for_every_entry(&theheap.free_list, chunk, struct free_heap_chunk, node) {
		DEBUG_ASSERT((chunk->len % sizeof(void *)) == 0); // len should always be a multiple of pointer size

		// is it big enough to service our allocation?
		if (chunk->len >= size) {
			ptr = chunk;

			// remove it from the list
			struct list_node *next_node = list_next(&theheap.free_list, &chunk->node);
			list_delete(&chunk->node);

			if (chunk->len > size + sizeof(struct free_heap_chunk)) {
				// there's enough space in this chunk to create a new one after the allocation
				struct free_heap_chunk *newchunk = heap_create_free_chunk((uint8_t *)ptr + size, chunk->len - size, true);

				// truncate this chunk
				chunk->len -= chunk->len - size;

				// add the new one where chunk used to be
				if (next_node)
					list_add_before(next_node, &newchunk->node);
				else
					list_add_tail(&theheap.free_list, &newchunk->node);
			}

			// the allocated size is actually the length of this chunk, not the size requested
			DEBUG_ASSERT(chunk->len >= size);
			size = chunk->len;

#if DEBUG_HEAP
			memset(ptr, ALLOC_FILL, size);
#endif

			ptr = (void *)((addr_t)ptr + sizeof(struct alloc_struct_begin));

			// align the output if requested
			if (alignment > 0) {
				ptr = (void *)ROUNDUP((addr_t)ptr, (addr_t)alignment);
			}

			struct alloc_struct_begin *as = (struct alloc_struct_begin *)ptr;
			as--;
#if LK_DEBUGLEVEL > 1
			as->magic = HEAP_MAGIC;
#endif
			as->ptr = (void *)chunk;
			as->size = size;
			theheap.remaining -= size;

			if (theheap.remaining < theheap.low_watermark) {
				theheap.low_watermark = theheap.remaining;
			}
#if DEBUG_HEAP
			as->padding_start = ((uint8_t *)ptr + original_size);
			as->padding_size = (((addr_t)chunk + size) - ((addr_t)ptr + original_size));
//			printf("padding start %p, size %u, chunk %p, size %u\n", as->padding_start, as->padding_size, chunk, size);

			memset(as->padding_start, PADDING_FILL, as->padding_size);
#endif

			break;
		}
	}

	mutex_release(&theheap.lock);

#if WITH_KERNEL_VM
	/* try to grow the heap if we can */
	if (ptr == NULL && retry_count == 0) {
		size_t growby = MAX(HEAP_GROW_SIZE, ROUNDUP(size, PAGE_SIZE));

		ssize_t err = heap_grow(growby);
		if (err >= 0) {
			retry_count++;
			goto retry;
		}
	}
#endif

	LTRACEF("returning ptr %p\n", ptr);

	return ptr;
}

void heap_free(void *ptr)
{
	if (ptr == 0)
		return;

	LTRACEF("ptr %p\n", ptr);

	// check for the old allocation structure
	struct alloc_struct_begin *as = (struct alloc_struct_begin *)ptr;
	as--;

	DEBUG_ASSERT(as->magic == HEAP_MAGIC);

#if DEBUG_HEAP
	{
		uint i;
		uint8_t *pad = (uint8_t *)as->padding_start;

		for (i = 0; i < as->padding_size; i++) {
			if (pad[i] != PADDING_FILL) {
				printf("free at %p scribbled outside the lines:\n", ptr);
				hexdump(pad, as->padding_size);
				panic("die\n");
			}
		}
	}
#endif

	LTRACEF("allocation was %zd bytes long at ptr %p\n", as->size, as->ptr);

	// looks good, create a free chunk and add it to the pool
	heap_insert_free_chunk(heap_create_free_chunk(as->ptr, as->size, true));
}

void heap_delayed_free(void *ptr)
{
	LTRACEF("ptr %p\n", ptr);

	// check for the old allocation structure
	struct alloc_struct_begin *as = (struct alloc_struct_begin *)ptr;
	as--;

	DEBUG_ASSERT(as->magic == HEAP_MAGIC);

	struct free_heap_chunk *chunk = heap_create_free_chunk(as->ptr, as->size, false);

	spin_lock_saved_state_t state;
	spin_lock_irqsave(&theheap.delayed_free_lock, state);
	list_add_head(&theheap.delayed_free_list, &chunk->node);
	spin_unlock_irqrestore(&theheap.delayed_free_lock, state);
}

void heap_get_stats(struct heap_stats *ptr)
{
	struct free_heap_chunk *chunk;

	if ((struct heap_stats*)NULL==ptr) {
		return;
	}
	//flush the delayed free list
	if (unlikely(!list_is_empty(&theheap.delayed_free_list))) {
		heap_free_delayed_list();
	}

	ptr->heap_start = theheap.base;
	ptr->heap_len = theheap.len;
	ptr->heap_free=0;
	ptr->heap_max_chunk = 0;

	mutex_acquire(&theheap.lock);

	list_for_every_entry(&theheap.free_list, chunk, struct free_heap_chunk, node) {
		ptr->heap_free += chunk->len;

		if (chunk->len > ptr->heap_max_chunk) {
			ptr->heap_max_chunk = chunk->len;
		}
	}

	ptr->heap_low_watermark = theheap.low_watermark;

	mutex_release(&theheap.lock);
}

static ssize_t heap_grow(size_t size)
{
#if WITH_KERNEL_VM
	size = ROUNDUP(size, PAGE_SIZE);

	void *ptr = pmm_alloc_kpages(size / PAGE_SIZE, NULL);
	if (!ptr) {
		TRACEF("failed to grow kernel heap by 0x%zx bytes\n", size);
		return ERR_NO_MEMORY;
	}

	LTRACEF("growing heap by 0x%zx bytes, new ptr %p\n", size, ptr);

	heap_insert_free_chunk(heap_create_free_chunk(ptr, size, true));

	/* change the heap start and end variables */
	if ((uintptr_t)ptr < (uintptr_t)theheap.base)
		theheap.base = ptr;

	uintptr_t endptr = (uintptr_t)ptr + size;
	if (endptr > (uintptr_t)theheap.base + theheap.len) {
		theheap.len = (uintptr_t)endptr - (uintptr_t)theheap.base;
	}

	return size;
#else
	return ERR_NO_MEMORY;
#endif
}

void heap_init(void)
{
	LTRACE_ENTRY;

	// create a mutex
	mutex_init(&theheap.lock);

	// initialize the free list
	list_initialize(&theheap.free_list);

	// initialize the delayed free list
	list_initialize(&theheap.delayed_free_list);
	spin_lock_init(&theheap.delayed_free_lock);

	// set the heap range
#if WITH_KERNEL_VM
	theheap.base = pmm_alloc_kpages(HEAP_GROW_SIZE / PAGE_SIZE, NULL);
	theheap.len = HEAP_GROW_SIZE;

	if (theheap.base == 0) {
		panic("HEAP: error allocating initial heap size\n");
	}
#else
	theheap.base = (void *)HEAP_START;
	theheap.len = HEAP_LEN;
#endif
	theheap.remaining = 0; // will get set by heap_insert_free_chunk()
	theheap.low_watermark = theheap.len;
	LTRACEF("base %p size %zd bytes\n", theheap.base, theheap.len);

	// create an initial free chunk
	heap_insert_free_chunk(heap_create_free_chunk(theheap.base, theheap.len, false));
}

/* add a new block of memory to the heap */
void heap_add_block(void *ptr, size_t len)
{
	heap_insert_free_chunk(heap_create_free_chunk(ptr, len, false));
}

#if LK_DEBUGLEVEL > 1

static void dump_free_chunk(struct free_heap_chunk *chunk)
{
	dprintf(INFO, "\t\tbase %p, end 0x%lx, len 0x%zx\n", chunk, (vaddr_t)chunk + chunk->len, chunk->len);
}

static void heap_dump(void)
{
	dprintf(INFO, "Heap dump:\n");
	dprintf(INFO, "\tbase %p, len 0x%zx\n", theheap.base, theheap.len);
	dprintf(INFO, "\tfree list:\n");

	mutex_acquire(&theheap.lock);

	struct free_heap_chunk *chunk;
	list_for_every_entry(&theheap.free_list, chunk, struct free_heap_chunk, node) {
		dump_free_chunk(chunk);
	}
	mutex_release(&theheap.lock);

	dprintf(INFO, "\tdelayed free list:\n");
	spin_lock_saved_state_t state;
	spin_lock_irqsave(&theheap.delayed_free_lock, state);
	list_for_every_entry(&theheap.delayed_free_list, chunk, struct free_heap_chunk, node) {
		dump_free_chunk(chunk);
	}
	spin_unlock_irqrestore(&theheap.delayed_free_lock, state);
}

static void heap_test(void)
{
	void *ptr[16];

	ptr[0] = heap_alloc(8, 0);
	ptr[1] = heap_alloc(32, 0);
	ptr[2] = heap_alloc(7, 0);
	ptr[3] = heap_alloc(0, 0);
	ptr[4] = heap_alloc(98713, 0);
	ptr[5] = heap_alloc(16, 0);

	heap_free(ptr[5]);
	heap_free(ptr[1]);
	heap_free(ptr[3]);
	heap_free(ptr[0]);
	heap_free(ptr[4]);
	heap_free(ptr[2]);

	heap_dump();

	int i;
	for (i=0; i < 16; i++)
		ptr[i] = 0;

	for (i=0; i < 32768; i++) {
		unsigned int index = (unsigned int)rand() % 16;

		if ((i % (16*1024)) == 0)
			printf("pass %d\n", i);

//		printf("index 0x%x\n", index);
		if (ptr[index]) {
//			printf("freeing ptr[0x%x] = %p\n", index, ptr[index]);
			heap_free(ptr[index]);
			ptr[index] = 0;
		}
		unsigned int align = 1 << ((unsigned int)rand() % 8);
		ptr[index] = heap_alloc((unsigned int)rand() % 32768, align);
//		printf("ptr[0x%x] = %p, align 0x%x\n", index, ptr[index], align);

		DEBUG_ASSERT(((addr_t)ptr[index] % align) == 0);
//		heap_dump();
	}

	for (i=0; i < 16; i++) {
		if (ptr[i])
			heap_free(ptr[i]);
	}

	heap_dump();
}

#if WITH_LIB_CONSOLE

#include <lib/console.h>

static int cmd_heap(int argc, const cmd_args *argv);

STATIC_COMMAND_START
STATIC_COMMAND("heap", "heap debug commands", &cmd_heap)
STATIC_COMMAND_END(heap);

static int cmd_heap(int argc, const cmd_args *argv)
{
	if (argc < 2) {
notenoughargs:
		printf("not enough arguments\n");
usage:
		printf("usage:\n");
		printf("\t%s info\n", argv[0].str);
		printf("\t%s alloc <size> [alignment]\n", argv[0].str);
		printf("\t%s free <address>\n", argv[0].str);
		return -1;
	}

	if (strcmp(argv[1].str, "info") == 0) {
		heap_dump();
	} else if (strcmp(argv[1].str, "alloc") == 0) {
		if (argc < 3) goto notenoughargs;

		void *ptr = heap_alloc(argv[2].u, (argc >= 3) ? argv[3].u : 0);
		printf("heap_alloc returns %p\n", ptr);
	} else if (strcmp(argv[1].str, "free") == 0) {
		if (argc < 2) goto notenoughargs;

		heap_free((void *)argv[2].u);
	} else {
		printf("unrecognized command\n");
		goto usage;
	}

	return 0;
}

#endif
#endif

/* vim: set ts=4 sw=4 noexpandtab: */

