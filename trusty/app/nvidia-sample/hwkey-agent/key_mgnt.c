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
#include <ekb_helper.h>
#include <err.h>
#include <fuse.h>
#include <openssl/cmac.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tegra_se.h>
#include <tegra_se_internal.h>
#include <trusty_std.h>
#include <tegra_se.h>

/*
 * Random fixed vector for EKB.
 *
 * Note: This vector MUST match the 'fv' vector used for EKB binary
 * generation process.
 * ba d6 6e b4 48 49 83 68 4b 99 2f e5 4a 64 8b b8
 */
static uint8_t fv_for_ekb[] = {
	0xba, 0xd6, 0x6e, 0xb4, 0x48, 0x49, 0x83, 0x68,
	0x4b, 0x99, 0x2f, 0xe5, 0x4a, 0x64, 0x8b, 0xb8,
};

/*
 * Random fixed vector used to derive SSK_DK (Derived Key).
 *
 * e4 20 f5 8d 1d ea b5 24 c2 70 d8 d2 3e ca 45 e8
 */
static uint8_t fv_for_ssk_dk[] = {
	0xe4, 0x20, 0xf5, 0x8d, 0x1d, 0xea, 0xb5, 0x24,
	0xc2, 0x70, 0xd8, 0xd2, 0x3e, 0xca, 0x45, 0xe8,
};

/*
 * Root keys derived from SE keyslots.
 */
static uint8_t kek2_rk_for_ekb[AES_KEY_128_SIZE] = { 0 };
static uint8_t ssk_rk[AES_KEY_128_SIZE] = { 0 };

/*
 * Derived keys from NIST-SP-800-108.
 */
static uint8_t ekb_ek[AES_KEY_128_SIZE] = { 0 };
static uint8_t ekb_ak[AES_KEY_128_SIZE] = { 0 };
static uint8_t ssk_dk[AES_KEY_128_SIZE] = { 0 };

/*
 * @brief NIST-SP-800-108 KDF
 *
 * @param *key		[in]  input key (128 bit) for derivation
 * @param *context	[in]  context string
 * @param *label	[in]  label string
 * @param *out_dk 	[out] output of derived key
 *
 * @return NO_ERROR if successful
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
	msg_len = strlen(context) + strlen(label) + 2;
	message = malloc(msg_len);
	if (message == NULL) {
		TLOGE("%s: malloc failed.\n", __func__);
		return ERR_NO_MEMORY;
	}

	/* Concatenate the messages */
	mptr = message;
	memcpy(mptr , counter, sizeof(counter));
	mptr++;
	memcpy(mptr, label, strlen(label));
	mptr += strlen(label);
	memcpy(mptr, zero_byte, sizeof(zero_byte));
	mptr++;
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

static int key_mgnt_derive_root_keys(void)
{
	int rc = NO_ERROR;

	/*
	 * Initialize Security Engine (SE) and acquire SE mutex.
	 * The mutex MUST be acquired before interacting with SE.
	 */
	rc = se_acquire();
	if (rc != NO_ERROR) {
		TLOGE("%s: failed to initialize SE (%d). Exiting\n", __func__, rc);
		return rc;
	}

	/*
	 * Derive root keys by performing AES-ECB encryption with the fixed
	 * vector (fv) and the key in the KEK2 and SSK SE keyslot.
	 */
	rc = se_derive_root_key(kek2_rk_for_ekb, sizeof(kek2_rk_for_ekb),
				fv_for_ekb, sizeof(fv_for_ekb),
				SE_AES_KEYSLOT_KEK2_128B);
	if (rc != NO_ERROR) {
		TLOGE("%s: failed to derive KEK2 root key (%d)\n", __func__, rc);
		goto root_key_fail;
	}

	rc = se_derive_root_key(ssk_rk, sizeof(ssk_rk),
				fv_for_ssk_dk, sizeof(fv_for_ssk_dk),
				SE_AES_KEYSLOT_SSK);
	if (rc != NO_ERROR) {
		TLOGE("%s: failed to derive SSK root key (%d)\n", __func__, rc);
		goto root_key_fail;
	}

root_key_fail:
	/* Clear keys from SE keyslots */
	rc = se_clear_aes_keyslots();
	if (rc != NO_ERROR)
		TLOGE("%s: failed to clear SE keyslots (%d)\n", __func__, rc);

	/* Release SE mutex */
	se_release();

	return rc;
}

static int set_ekb_key_to_keyslot(uint32_t keyslot, uint8_t key_index)
{
	uint8_t *key_in_ekb;

	key_in_ekb = ekb_get_key(key_index);
	if (key_in_ekb == NULL) {
		return ERR_NOT_VALID;
	}

	TLOGE("Setting EKB key %d to slot %d\n", key_index, keyslot);
	return se_write_keyslot(key_in_ekb, keyslot);
}

int key_mgnt_processing(void)
{
	int rc = NO_ERROR;

	TLOGE("%s .......\n", __func__);

	/* Query ECID */
	fuse_query_ecid();

	/* Derive root keys from SE keyslots. */
	rc = key_mgnt_derive_root_keys();
	if (rc != NO_ERROR)
		goto err_key_mgnt;

	/* Derive EKB_EK */
	rc = nist_sp_800_108_with_cmac(kek2_rk_for_ekb, "ekb", "encryption",
				       ekb_ek);
	if (rc != NO_ERROR)
		goto err_key_mgnt;

	/* Derive EKB_AK */
	rc = nist_sp_800_108_with_cmac(kek2_rk_for_ekb, "ekb", "authentication",
				       ekb_ak);
	if (rc != NO_ERROR)
		goto err_key_mgnt;

	/*
	 * Derive SSK_DK
	 *
	 * This demos how to derive a key from SE keyslot. So developers
	 * can follow the same scenario to derive keys for different
	 * security purposes.
	 */
	rc = nist_sp_800_108_with_cmac(ssk_rk, "ssk", "derivedkey", ssk_dk);
	if (rc != NO_ERROR)
		goto err_key_mgnt;

	/* Verify EKB */
	rc = ekb_verification(ekb_ak, ekb_ek);
	if (rc != NO_ERROR)
		goto err_key_mgnt;

	/* Set ekb key to SBK key slot to support for cboot crypto operation */
	rc = set_ekb_key_to_keyslot(SE_AES_KEYSLOT_SBK, EKB_USER_KEY_KERNEL_ENCRYPTION);

err_key_mgnt:
	if (rc != NO_ERROR)
		TLOGE("%s: failed (%d)\n", __func__, rc);
	return rc;
}
