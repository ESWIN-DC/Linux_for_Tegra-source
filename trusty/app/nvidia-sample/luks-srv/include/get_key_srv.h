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

#ifndef __GET_KEY_SRV_H__
#define __GET_KEY_SRV_H__

#include <common.h>
#include <stdint.h>
#include <trusty_ipc.h>

#define GET_KEY_SRV_PORT "hwkey-agent.srv.get-key-srv"

#define GET_KEY_SRV_QUERY_EKB_KEY	1

typedef struct get_key_srv_cmd_msg {
	uint32_t cmd;
	uint8_t key[AES_KEY_128_SIZE];
	uint32_t ecid[4];
} get_key_srv_cmd_msg_t;

/**
 * @brief Opens a trusty get-key-srv session.
 *
 * @return A handle_t > 0 on success, or an error code < 0 on
 * failure.
 */
handle_t get_key_srv_open(void);

/**
 * @brief Queries the EKB key.
 * @param session	current session id
 * @param msg		get_key_srv_cmd_msg
 * @return 0 if successful.
 */
int get_key_srv_query_ekb_key(handle_t session, get_key_srv_cmd_msg_t *msg);

/**
 * @brief Closes the trusty get-key-srv session.
 */
void get_key_srv_close(handle_t session);

#endif /* __GET_KEY_SRV_H__ */
