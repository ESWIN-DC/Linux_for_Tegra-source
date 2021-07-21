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

#include <ekb_helper.h>
#include <fuse.h>
#include <get_key_srv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void get_key_srv_process_req(iovec_t *ipc_msg)
{
	get_key_srv_cmd_msg_t *msg = ipc_msg->base;
	uint8_t *key_in_ekb;

	switch (msg->cmd) {
	case GET_KEY_SRV_QUERY_EKB_KEY:
		key_in_ekb = ekb_get_key(EKB_USER_KEY_DISK_ENCRYPTION);
		if (key_in_ekb == NULL) {
			TLOGE("%s: get key in ekb failed\n", __func__);
			return;
		}
		memcpy(msg->key, key_in_ekb, AES_KEY_128_SIZE);
		break;
	default:
		return;
	}

	memcpy(msg->ecid, fuse_get_queried_ecid(), sizeof(uint32_t) * 4);
}
