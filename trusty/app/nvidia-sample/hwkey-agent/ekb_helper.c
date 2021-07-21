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
#include <lib/trusty/ioctl.h>
#include <openssl/aes.h>
#include <openssl/cmac.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <trusty_std.h>
#include <ekb_helper.h>

/*
 * The symmetric key in EKB.
 */
typedef uint8_t sym_key[AES_KEY_128_SIZE];

sym_key *sym_keys[EKB_USER_KEYS_NUM];

struct ekb_content {
	uint8_t ekb_cmac[16];
	uint8_t random_iv[16];
	uint8_t ekb_ciphertext[16];
};

/* Facilitates the IOCTL_MAP_EKS_TO_USER ioctl() */
union ptr_to_int_bridge {
	uint32_t val;
	void *ptr;
};

uint8_t *ekb_get_key(uint8_t idx)
{
	if (idx >= EKB_USER_KEYS_NUM) {
		return NULL;
	}
	return (uint8_t *)sym_keys[idx];
}

/*
 * @brief copies EKB contents to TA memory
 *
 * @param ekb_base [out] pointer to base of ekb content buffer
 * @param ekb_size [out] length of ekb content buffer
 *
 * @return NO_ERROR if successful
 */
static int get_ekb(uint8_t **ekb_base, uint32_t *ekb_size)
{
	int rc = NO_ERROR;
	void *nsdram_ekb_map_base = NULL;
	uint32_t nsdram_ekb_map_size = 0;
	void *nsdram_ekb_base = NULL;
	uint32_t nsdram_ekb_size = 0;

	union ptr_to_int_bridge params[4] = {
		{.ptr = &nsdram_ekb_base},
		{.ptr = &nsdram_ekb_size},
		{.ptr = &nsdram_ekb_map_base},
		{.ptr = &nsdram_ekb_map_size}
	};

	if ((ekb_base == NULL) || (ekb_size == NULL)) {
		TLOGE("%s: Invalid arguments\n", __func__);
		return ERR_INVALID_ARGS;
	}

	/* Map the NSDRAM region containing EKB */
	rc = ioctl(3, IOCTL_MAP_EKS_TO_USER, params);
	if (rc != NO_ERROR) {
		TLOGE("%s: failed to map EKB memory (%d)\n", __func__, rc);
		return ERR_GENERIC;
	}

	/* Enforce a reasonable bound on the EKB size */
	if (nsdram_ekb_size > MIN_HEAP_SIZE) {
		rc = ERR_TOO_BIG;
		goto err;
	}

	/* Copy EKB contents out of NSDRAM */
	*ekb_size = nsdram_ekb_size;

	*ekb_base = malloc(*ekb_size);
	if (*ekb_base == NULL) {
		TLOGE("%s: malloc failed\n", __func__);
		rc = ERR_NO_MEMORY;
		goto err;
	}
	memcpy(*ekb_base, nsdram_ekb_base, *ekb_size);

err:
	/* Unmap the NSDRAM region containing EKB */
	if (munmap(nsdram_ekb_map_base, nsdram_ekb_map_size) != 0) {
		TLOGE("%s: failed to unmap EKB\n", __func__);
		return ERR_GENERIC;
	}

	return rc;
}

int ekb_verification(uint8_t *ekb_ak, uint8_t *ekb_ek)
{
	int rc = NO_ERROR;
	uint8_t *ekb_base = NULL;
	size_t ekb_size = 0;
	struct ekb_content *ekb;
	CMAC_CTX *cmac = NULL;
	uint8_t ekb_cmac_verify[16];
	size_t cmac_len;
	AES_KEY decrypt_key;
	int i;
	uint8_t *data;

	/* Get EKB */
	rc = get_ekb(&ekb_base, &ekb_size);
	if ((rc != NO_ERROR) || (ekb_base == NULL) || (ekb_size == 0)) {
		TLOGE("%s: failed to get EKB (%d). Exiting\n", __func__, rc);
		goto ekb_verify_error;
	}

	/* Convert to EKB layout */
	ekb = (struct ekb_content*)ekb_base;

	/* Caculate the EKB_cmac */
	cmac = CMAC_CTX_new();
	if (cmac == NULL) {
		TLOGE("%s: CMAC_CTX_new failed.\n", __func__);
		rc = ERR_NO_MEMORY;
		goto ekb_verify_error;
	}

	CMAC_Init(cmac, ekb_ak, AES_KEY_128_SIZE, EVP_aes_128_cbc(), NULL);
	CMAC_Update(cmac, ekb->random_iv, sizeof(ekb->random_iv) +
				sizeof(ekb->ekb_ciphertext) * EKB_USER_KEYS_NUM);
	CMAC_Final(cmac, ekb_cmac_verify, &cmac_len);
	CMAC_CTX_free(cmac);

	/* Verify the CMAC */
	if (memcmp(ekb->ekb_cmac, ekb_cmac_verify, cmac_len)) {
		TLOGE("%s: EKB_CMAC verification is not match.\n", __func__);
		rc = ERR_NOT_VALID;
		goto ekb_verify_error;
	}

	/* Decrypt the EKB ciphertext */
	rc = AES_set_decrypt_key(ekb_ek, AES_KEY_128_SIZE * 8, &decrypt_key);
	if (rc < 0) {
		TLOGE("%s: AES set decrypt key failed.\n", __func__);
		goto ekb_verify_error;
	}

	for (i = 0; i < EKB_USER_KEYS_NUM; ++i) {
		data = malloc(AES_KEY_128_SIZE);
		if (data == NULL) {
			TLOGE("%s: malloc failed\n", __func__);
			/* free memory that was previously allocated */
			while (--i >= 0) {
				if (sym_keys[i])
					free((uint8_t *)sym_keys[i]);
			}
			rc = ERR_NO_MEMORY;
			goto ekb_verify_error;
		}
		sym_keys[i] = (sym_key *)data;
		AES_cbc_encrypt(ekb->ekb_ciphertext + (AES_KEY_128_SIZE * i), (uint8_t *)sym_keys[i],
						AES_KEY_128_SIZE, &decrypt_key, ekb->random_iv, AES_DECRYPT);
	}

ekb_verify_error:
	if (ekb_base != NULL)
		free(ekb_base);

	return rc;
}
