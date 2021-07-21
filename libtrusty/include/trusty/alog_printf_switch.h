/*
 * Copyright (c) 2016 NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>

/* adapter for ALOG* macros from android. redefine them as printf's in linux */
#ifdef NV_EMBEDDED_BUILD
#undef ALOGI
#define ALOGI(fmt, ...) do { \
		fprintf(stdout, (fmt), ##__VA_ARGS__); \
	} while (0)

#undef ALOGE
#define ALOGE(fmt, ...) do { \
		fprintf(stderr, (fmt), ##__VA_ARGS__); \
	} while (0)

#undef ALOGV
#define ALOGV(fmt, ...) do { \
	} while (0)

#undef ALOGW
#define ALOGW(fmt, ...) do { \
		fprintf(stderr, (fmt), ##__VA_ARGS__); \
	} while (0)

#else
#include <cutils/log.h>
#endif
