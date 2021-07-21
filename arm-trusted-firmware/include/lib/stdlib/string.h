/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)string.h	8.1 (Berkeley) 6/2/93
 * $FreeBSD$
 */

/*
 * Portions copyright (c) 2013-2017, ARM Limited and Contributors.
 * All rights reserved.
 */

#ifndef _STRING_H_
#define	_STRING_H_

#include <sys/cdefs.h>
#include <sys/_null.h>
#include <sys/_types.h>

#ifndef _SIZE_T_DECLARED
typedef	__size_t	size_t;
#define	_SIZE_T_DECLARED
#endif

__BEGIN_DECLS

void	*memchr(const void *src, int c, size_t len) __pure;
int	 memcmp(const void *s1, const void *s2, size_t len) __pure;
void	*memcpy(void * __restrict, const void * __restrict, size_t);
void	*memcpy16(void * __restrict, const void * __restrict, size_t);
void	*memmove(void *dst, const void *src, size_t len);
void	*memset(void *dst, int val, size_t count);

char	*strchr(const char *p, int ch) __pure;
int	 strcmp(const char *s1, const char *s2) __pure;
size_t	 strlen(const char *str) __pure;
int	 strncmp(const char *s1, const char *s2, size_t n) __pure;
size_t	 strnlen(const char *s, size_t maxlen) __pure;
int	 strcasecmp(const char *s1, const char *s2);
int	 timingsafe_bcmp(const void *b1, const void *b2, size_t n);

__END_DECLS

#endif /* _STRING_H_ */
