/*
 * Copyright (c) 2020-2021, NVIDIA Corporation. All Rights Reserved.
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
static uint8_t demo_256_rk[AES_KEY_256_SIZE] = { 0 };

/*
 * Derived keys from NIST-SP-800-108.
 */
static uint8_t ekb_ek[AES_KEY_128_SIZE] = { 0 };
static uint8_t ekb_ak[AES_KEY_128_SIZE] = { 0 };
static uint8_t ssk_dk[AES_KEY_128_SIZE] = { 0 };

/*
 * @brief NIST-SP-800-108 KDF
 *
 * @param *key		[in]  input key for derivation
 * @param key_len	[in]  input key length (byte)
 * @param *context	[in]  context string
 * @param *label	[in]  label string
 * @param dk_len	[in]  length of the derived key (byte)
 * @param *out_dk 	[out] output of derived key
 *
 * @return NO_ERROR if successful
 */
static int nist_sp_800_108_with_cmac(uint8_t *key,
				     uint32_t key_len,
				     char const *context,
				     char const *label,
				     uint32_t dk_len,
				     uint8_t *out_dk)
{
	uint8_t *message = NULL, *mptr;
	uint8_t counter[] = { 1 }, zero_byte[] = { 0 };
	uint32_t L[] = { __builtin_bswap32(dk_len * 8) };
	int msg_len;
	int rc = NO_ERROR;
	CMAC_CTX *cmac = NULL;
	size_t cmact_len;
	int i, n;

	if ((key_len != AES_KEY_128_SIZE) && (key_len != AES_KEY_256_SIZE))
		return ERR_INVALID_ARGS;

	if ((dk_len % AES_BLOCK_SIZE) != 0)
		return ERR_INVALID_ARGS;

	if (!key || !context || !label || !out_dk)
		return ERR_INVALID_ARGS;

	/*
	 *  Regarding to NIST-SP-800-108
	 *  message = counter || label || 0 || context || L
	 *
	 *  A || B = The concatenation of binary strings A and B.
	 */
	msg_len = strlen(context) + strlen(label) + 2 + sizeof(L);
	message = malloc(msg_len);
	if (message == NULL) {
		TLOGE("%s: malloc failed.\n", __func__);
		return ERR_NO_MEMORY;
	}

	/* AES-CMAC */
	cmac = CMAC_CTX_new();
	if (cmac == NULL) {
		TLOGE("%s: CMAC_CTX_new failed.\n", __func__);
		rc = ERR_NO_MEMORY;
		goto kdf_error;
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
	mptr += strlen(context);
	memcpy(mptr, L, sizeof(L));

	/* n: iterations of the PRF count */
	n = dk_len / AES_BLOCK_SIZE;

	for (i = 0; i < n; i++) {
		/* Update the counter */
		message[0] = i + 1;

		/* re-init initial vector for each iteration */
		if (key_len == AES_KEY_128_SIZE)
			CMAC_Init(cmac, key, AES_KEY_128_SIZE, EVP_aes_128_cbc(), NULL);
		else
			CMAC_Init(cmac, key, AES_KEY_256_SIZE, EVP_aes_256_cbc(), NULL);

		CMAC_Update(cmac, message, msg_len);
		CMAC_Final(cmac, (out_dk + (i * AES_BLOCK_SIZE)), &cmact_len);
	}

	CMAC_CTX_free(cmac);

kdf_error:
	free(message);
	return rc;
}

static int key_mgnt_derive_root_keys(void)
{
	int rc = NO_ERROR;

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

	/*
	 * Derive 256-bit root key from KEK256 SE keyslot.
	 *
	 * To support this, you need to modify the BR BCT file
	 * e.g. "tegra194-br-bct-sdmmc.cfg" (or "tegra194-br-bct-qspi.cfg").
	 * Add "BctKEKKeySelect = 1" into the BR BCT file.
	 * The BootRom will check the BCT to load KEK0 and KEK1 as a single
	 * 256-bit fuse into KEK256 SE keyslot.
	 */
	rc = se_nist_sp_800_108_with_cmac(SE_AES_KEYSLOT_KEK256, AES_KEY_256_SIZE,
					  "Derived 256-bit root key", "256-bit key",
					  AES_KEY_256_SIZE, demo_256_rk);
	if (rc != NO_ERROR)
		TLOGE("%s: failed to derive 256-bit root key (%d)\n", __func__, rc);

root_key_fail:
	/* Clear keys from SE keyslots */
	rc = se_clear_aes_keyslots();
	if (rc != NO_ERROR)
		TLOGE("%s: failed to clear SE keyslots (%d)\n", __func__, rc);

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
	return se_write_keyslot(key_in_ekb, AES_KEY_128_SIZE, AES_QUAD_KEYS, keyslot);
}

static int tegra_se_cmac_self_test(void)
{
	se_cmac_ctx *se_cmac = NULL;
	uint8_t test_key_256[] = {
		0x72, 0xd1, 0x1f, 0x8b, 0x1c, 0x01, 0xe1, 0x5c,
		0x49, 0x86, 0x07, 0x2a, 0xe5, 0x63, 0x42, 0x21,
		0x65, 0x3f, 0x2e, 0x7f, 0x22, 0xfd, 0x05, 0x4c,
		0x60, 0xc9, 0x76, 0xa6, 0xf4, 0x3a, 0x93, 0xfe,
	};
	char test_msg[] = "SE_aes_cmac_test_string";
	uint8_t openssl_cmac_digest[AES_BLOCK_SIZE] = { 0 };
	uint8_t se_cmac_digest[AES_BLOCK_SIZE] = {0};
	size_t cmac_len;

	/* OpenSSL AES-CMAC */
	CMAC_CTX *cmac = NULL;

	cmac = CMAC_CTX_new();
	if (cmac == NULL)
		return ERR_NO_MEMORY;
	CMAC_Init(cmac, test_key_256, AES_KEY_256_SIZE, EVP_aes_256_cbc(), NULL);

	CMAC_Update(cmac, test_msg, sizeof(test_msg));
	CMAC_Final(cmac, openssl_cmac_digest, &cmac_len);
	CMAC_CTX_free(cmac);

	/* Write key into keyslot */
	se_write_keyslot(test_key_256, AES_KEY_256_SIZE, AES_QUAD_KEYS_256, SE_AES_KEYSLOT_KEK256);

	/* SE AES-CMAC */
	se_cmac = tegra_se_cmac_new();
	if (se_cmac == NULL)
		return ERR_NO_MEMORY;

	tegra_se_cmac_init(se_cmac, SE_AES_KEYSLOT_KEK256, AES_KEY_256_SIZE);
	tegra_se_cmac_update(se_cmac, test_msg, sizeof(test_msg));
	tegra_se_cmac_final(se_cmac, se_cmac_digest, &cmac_len);

	tegra_se_cmac_free(se_cmac);

	/* Verify the result */
	if (memcmp(openssl_cmac_digest, se_cmac_digest, cmac_len)) {
		TLOGE("%s: Tegra SE AES-CMAC verification is not match.\n", __func__);
		return ERR_GENERIC;
	}

	return NO_ERROR;
}

static int tegra_se_nist_800_108_kdf_self_test(void)
{
	uint8_t test_key_256[] = {
		0xc0, 0x3c, 0x15, 0x4e, 0xe5, 0x6c, 0xb5, 0x69,
		0x1b, 0x27, 0xd9, 0x2e, 0x7f, 0x34, 0xfb, 0x8a,
		0x88, 0x6c, 0x0c, 0x40, 0xf9, 0x51, 0x66, 0xe0,
		0x1d, 0x43, 0x5b, 0xba, 0xa3, 0x90, 0x47, 0x32,
	};
	const char context_str[] = "nist sp 800-108 KDF verification";
	const char label_str[] = "KDF comparison";
	uint8_t sw_derived_key[AES_KEY_256_SIZE] = { 0 };
	uint8_t hw_derived_key[AES_KEY_256_SIZE] = { 0 };

	/* SW-based NIST SP 800-108 KDF */
	nist_sp_800_108_with_cmac(test_key_256, AES_KEY_256_SIZE,
				  context_str, label_str,
				  AES_KEY_256_SIZE, sw_derived_key);

	/* Write key into keyslot */
	se_write_keyslot(test_key_256, AES_KEY_256_SIZE, AES_QUAD_KEYS_256, SE_AES_KEYSLOT_KEK256);

	/* HW-based NIST SP 800-108 KDF */
	se_nist_sp_800_108_with_cmac(SE_AES_KEYSLOT_KEK256, AES_KEY_256_SIZE,
				     context_str, label_str,
				     AES_KEY_256_SIZE, hw_derived_key);

	/* Verify the result */
	if (memcmp(sw_derived_key, hw_derived_key, AES_KEY_256_SIZE)) {
		TLOGE("%s: Tegra SE NIST 800-108 KDF verification is not match.\n", __func__);
		return ERR_GENERIC;
	}

	return NO_ERROR;
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
	rc = nist_sp_800_108_with_cmac(kek2_rk_for_ekb, AES_KEY_128_SIZE,
				       "ekb", "encryption",
				       AES_KEY_128_SIZE, ekb_ek);
	if (rc != NO_ERROR)
		goto err_key_mgnt;

	/* Derive EKB_AK */
	rc = nist_sp_800_108_with_cmac(kek2_rk_for_ekb, AES_KEY_128_SIZE,
				       "ekb", "authentication",
				       AES_KEY_128_SIZE, ekb_ak);
	if (rc != NO_ERROR)
		goto err_key_mgnt;

	/*
	 * Derive SSK_DK
	 *
	 * This demos how to derive a key from SE keyslot. So developers
	 * can follow the same scenario to derive keys for different
	 * security purposes.
	 */
	rc = nist_sp_800_108_with_cmac(ssk_rk, AES_KEY_128_SIZE,
				       "ssk", "derivedkey",
				       AES_KEY_128_SIZE, ssk_dk);
	if (rc != NO_ERROR)
		goto err_key_mgnt;

	/* Verify EKB */
	rc = ekb_verification(ekb_ak, ekb_ek);
	if (rc != NO_ERROR)
		goto err_key_mgnt;

	/* Set ekb key to SBK key slot to support for cboot crypto operation */
	rc = set_ekb_key_to_keyslot(SE_AES_KEYSLOT_SBK, EKB_USER_KEY_KERNEL_ENCRYPTION);

	/* Tegra Security Engine AES-CMAC self test */
	if (tegra_se_cmac_self_test())
		return ERR_GENERIC;
	/* Tegra Security Engine NIST 800-108 KDF self test */
	if (tegra_se_nist_800_108_kdf_self_test())
		return ERR_GENERIC;
err_key_mgnt:
	if (rc != NO_ERROR)
		TLOGE("%s: failed (%d)\n", __func__, rc);
	return rc;
}
