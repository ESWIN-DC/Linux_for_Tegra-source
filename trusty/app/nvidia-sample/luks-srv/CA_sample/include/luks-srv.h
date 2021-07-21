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

#ifndef __LUKS_SRV_H__
#define __LUKS_SRV_H__

#define TIPC_DEFAULT_NODE "/dev/trusty-ipc-dev0"
#define TA_LUKS_SRV_CHAL "luks-srv.srv.passphrase-gen"

#define LUKS_GET_UNIQUE_PASS	1
#define LUKS_GET_GENERIC_PASS	2
#define LUKS_NO_PASS_RESPONSE	10

#define LUKS_SRV_CONTEXT_STR_LEN	40
#define LUKS_SRV_PASSPHRASE_LEN		16

typedef struct luks_srv_cmd_msg {
	uint32_t luks_srv_cmd;
	char context_str[LUKS_SRV_CONTEXT_STR_LEN];
	char output_passphrase[LUKS_SRV_PASSPHRASE_LEN];
} luks_srv_cmd_msg_t;

#endif /* __LUKS_SRV_H__ */
