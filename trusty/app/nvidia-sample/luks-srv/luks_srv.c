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

#include <key_mgnt.h>
#include <luks_srv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool no_pass_response = false;

void luks_srv_process_req(iovec_t *ipc_msg)
{
	luks_srv_cmd_msg_t *msg = ipc_msg->base;

	if (no_pass_response)
		return;

	switch (msg->luks_srv_cmd) {
	case LUKS_GET_UNIQUE_PASS:
		luks_srv_get_unique_pass(msg);
		break;
	case LUKS_GET_GENERIC_PASS:
		luks_srv_get_generic_pass(msg);
		break;
	case LUKS_NO_PASS_RESPONSE:
		no_pass_response = true;
		break;
	default:
		return;
	}
}
