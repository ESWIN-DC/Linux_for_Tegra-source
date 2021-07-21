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

#ifndef __CRYPTO_SERVICE_H__
#define __CRYPTO_SERVICE_H__

#include <common.h>
#include <stdint.h>
#include <trusty_ipc.h>

#define CRYPTO_SRV_PAYLOAD_SIZE 2048

enum crypto_srv_cmd {
	CRYPTO_SRV_CMD_ENCRYPT = 1,
	CRYPTO_SRV_CMD_DECRYPT
};

typedef struct crypto_srv_cmd_msg {
    enum crypto_srv_cmd cmd;
    uint8_t iv[AES_KEY_128_SIZE];
    uint32_t payload_len;
    uint8_t payload[];			/* CRYPTO_SRV_PAYLOAD_SIZE */
} crypto_srv_msg_t;

void crypto_srv_process_req(iovec_t *ipc_msg, int len);

#endif /* __CRYPTO_SERVICE_H__ */
