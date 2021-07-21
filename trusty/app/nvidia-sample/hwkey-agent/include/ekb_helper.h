/*
 * Copyright (c) 2020, NVIDIA Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __EKB_HELPER_H__
#define __EKB_HELPER_H__

#include <stdint.h>

enum {
	EKB_USER_KEY_KERNEL_ENCRYPTION,
	EKB_USER_KEY_DISK_ENCRYPTION,
	EKB_USER_KEYS_NUM,
};

/*
 * @brief verify the EKB blob
 *
 * @param *ekb_ak	[in]  base address of the EKB_AK
 * @param *ekb_ek	[in]  base address of the EKB_EK
 *
 * @return NO_ERROR if successful
 */
int ekb_verification(uint8_t *ekb_ak, uint8_t *ekb_ek);

/*
 * @brief get the key from EKB
 *
 * @return key addr if successful
 */
uint8_t *ekb_get_key(uint8_t);

#endif /* __EKB_HELPER_H__ */
