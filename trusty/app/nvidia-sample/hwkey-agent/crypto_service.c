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

#include <crypto_service.h>
#include <ekb_helper.h>
#include <openssl/aes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void crypto_srv_process_req(iovec_t *ipc_msg, int len)
{
	crypto_srv_msg_t *msg = ipc_msg[0].base;
	uint8_t *payload = ipc_msg[1].base;
	int payload_len = msg->payload_len;
	uint8_t *data = NULL;
	uint8_t *key_in_ekb;
	AES_KEY aes_key;
	int aes_enc_flag;
	int idx;

	key_in_ekb = ekb_get_key(EKB_USER_KEY_DISK_ENCRYPTION);
	if (key_in_ekb == NULL) {
		TLOGE("%s: get key in ekb failed\n", __func__);
		return;
	}

	data = malloc(CRYPTO_SRV_PAYLOAD_SIZE);
	if (data == NULL) {
		TLOGE("%s: malloc failed\n", __func__);
		return;
	}
	memcpy(data, payload, payload_len);

	if (msg->cmd == CRYPTO_SRV_CMD_ENCRYPT) {
		AES_set_encrypt_key(key_in_ekb, AES_KEY_128_SIZE * 8, &aes_key);
		aes_enc_flag = AES_ENCRYPT;
	} else {
		AES_set_decrypt_key(key_in_ekb, AES_KEY_128_SIZE * 8, &aes_key);
		aes_enc_flag = AES_DECRYPT;
	}

	for (idx = 0; idx < payload_len; idx += AES_BLOCK_SIZE)
		AES_cbc_encrypt(data + idx, payload + idx, AES_BLOCK_SIZE,
				&aes_key, msg->iv, aes_enc_flag);

	free(data);
}
