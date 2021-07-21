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

#include <common.h>
#include <err.h>
#include <get_key_srv.h>
#include <luks_srv.h>
#include <openssl/cmac.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <trusty_std.h>

/*
 * Derived keys from NIST-SP-800-108.
 */
static uint8_t luks_key_unique[AES_KEY_128_SIZE] = { 0 };
static uint8_t luks_key_generic[AES_KEY_128_SIZE] = { 0 };

/*
 * @brief NIST-SP-800-108 KDF.
 *
 * @param *key		[in]  input key (128 bits) for derivation
 * @param *context	[in]  context string
 * @param *label	[in]  label string
 * @param *out_dk 	[out] output of derived key (128 bits)
 *
 * @return NO_ERROR if successful.
 */
static int nist_sp_800_108_with_cmac(uint8_t *key, char const *context,
				     char const *label,
				     uint8_t *out_dk)
{
	uint8_t *message = NULL, *mptr;
	uint8_t counter[] = { 1 }, zero_byte[] = { 0 };
	int msg_len;
	int rc = NO_ERROR;
	CMAC_CTX *cmac = NULL;
	size_t cmact_len;

	/*
	 *  Regarding to NIST-SP-800-108
	 *  message = counter || label || 0 || context
	 *
	 *  A || B = The concatenation of binary strings A and B.
	 */
	msg_len = (context != NULL ? strnlen(context, LUKS_SRV_CONTEXT_STR_LEN) : 0) +
		  (label != NULL ? strlen(label) : 0) + 2;
	message = malloc(msg_len);
	if (message == NULL) {
		TLOGE("%s: malloc failed.\n", __func__);
		return ERR_NO_MEMORY;
	}

	/* Concatenate the messages */
	mptr = message;
	memcpy(mptr , counter, sizeof(counter));
	mptr++;
	if (label != NULL) {
		memcpy(mptr, label, strlen(label));
		mptr += strlen(label);
	}
	memcpy(mptr, zero_byte, sizeof(zero_byte));
	mptr++;
	if (context != NULL)
		memcpy(mptr, context, strlen(context));

	/* AES-CMAC */
	cmac = CMAC_CTX_new();
	if (cmac == NULL) {
		TLOGE("%s: CMAC_CTX_new failed.\n", __func__);
		rc = ERR_NO_MEMORY;
		goto kdf_error;
	}

	CMAC_Init(cmac, key, AES_KEY_128_SIZE, EVP_aes_128_cbc(), NULL);
	CMAC_Update(cmac, message, msg_len);
	CMAC_Final(cmac, out_dk, &cmact_len);

	CMAC_CTX_free(cmac);

kdf_error:
	free(message);
	return rc;
}

void luks_srv_get_generic_pass(luks_srv_cmd_msg_t *msg)
{
	nist_sp_800_108_with_cmac(luks_key_generic, msg->context_str,
				  "luks-srv-passphrase-generic",
				  (uint8_t*)msg->output_passphrase);
}

void luks_srv_get_unique_pass(luks_srv_cmd_msg_t *msg)
{
	nist_sp_800_108_with_cmac(luks_key_unique, msg->context_str,
				  "luks-srv-passphrase-unique",
				  (uint8_t*)msg->output_passphrase);
}

int luks_srv_key_mgnt_processing(void)
{
	int rc = NO_ERROR;
	handle_t session;
	get_key_srv_cmd_msg_t msg;
	char ecid[40];

	rc = get_key_srv_open();
	if (rc < 0)
		goto err_key_mgnt;
	session = rc;

	/* Query raw keys */
	rc = get_key_srv_query_ekb_key(session, &msg);
	if (rc != 0)
		goto err_key_mgnt;
	get_key_srv_close(session);

	sprintf(ecid, "%08x%08x%08x%08x", msg.ecid[3], msg.ecid[2],
		msg.ecid[1], msg.ecid[0]);

	/* Derive unique LUKS key */
	rc = nist_sp_800_108_with_cmac(msg.key, ecid, "luks-srv-ecid",
				       luks_key_unique);

	/* Derive generic LUKS key */
	rc = nist_sp_800_108_with_cmac(msg.key, "generic-key", "luks-srv-generic",
				       luks_key_generic);

err_key_mgnt:
	if (rc != NO_ERROR)
		TLOGE("%s: failed (%d)\n", __func__, rc);
	return rc;
}
