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

/**
 * @file
 * <b>AES-256 hardware key definition functions</b>
 *
 * @b Description: This file specifies AES-256 hardware key definition functions.
 *
 * There are two groups of functions:
 * - Hardware-based AES-CMAC functions, for use only at boot time
 * - NIST-SP 800-108 key definition functions, for use
 *   (with the respective function) at either boot time or run time.
 */

#ifndef __TEGRA_SE_H__
#define __TEGRA_SE_H__

#include <tegra_se_internal.h>

/**
 * @defgroup trusty_aes_cmac_group Hardware-Based AES-CMAC Functions
 *
 * Specifies an implementation of the hardware-based AES-CMAC function,
 * very similar to the
 * <a href="https://man.archlinux.org/man/community/libressl/libressl-CMAC_Init.3.en">
 *   OpenSSL CMAC
 * </a>
 * implementation, and based on the same concepts.
 *
 * If you are not familiar with the OpenSSL implementation of CMAC,
 * the reference above will help you understand it.
 * Each AES-CMAC function corresponds to an OpenSSL CMAC function
 * with a similar name and usage. To use AES-CMAC,
 * follow the same sequence of operations as for OpenSSL CMAC,
 * using the AES-CMAC functions
 * instead of the OpenSSL CMAC ones.key definition functions.
 *
 * <table>
 *   <tr>
 *     <th>OpenSSL CMAC function</th>
 *     <th>Corresponding hardware-based<br/> AES-CMAC function</th>
 *   </tr>
 *   <tr>
 *     <td>CMAC_CTX_new()</td>
 *     <td>tegra_se_cmac_new()</td>
 *   </tr>
 *   <tr>
 *     <td>CMAC_Init()</td>
 *     <td>tegra_se_cmac_init()</td>
 *   </tr>
 *   <tr>
 *     <td>CMAC_Update()</td>
 *     <td>tegra_se_cmac_update()</td>
 *   </tr>
 *   <tr>
 *     <td>CMAC_Final()</td>
 *     <td>tegra_se_cmac_final()</td>
 *   </tr>
 *   <tr>
 *     <td>CMAC_CTX_free()</td>
 *     <td>tegra_se_cmac_free()</td>
 *   </tr>
 * </table>
 *
 * @note  To prevent security issues, the SE keyslots must be cleared
 *   after the hardware-based KDF process has finished.
 *   Then the untrusted rich OS (Jetson Linux) cannot use these keyslots
 *   in the non-secure world.
 *   <p>The hardware-based KDF may only be used at boot time to avoid
 *   a runtime conflict with SE hardware usage by the SE driver
 *   in the Linux kernel. A run time, use the software-based KDF instead.</p>
 *
 * @section trusty_aes_cmac_group_example Example
 *
 * The following code shows examples of how the API functions can be used.
 * @code
 * se_cmac_ctx *se_cmac = NULL;
 * uint8_t test_key_256[] = {
 *        .0x72, 0xd1, 0x1f, 0x8b, 0x1c, 0x01, 0xe1, 0x5c,
 *        .0x49, 0x86, 0x07, 0x2a, 0xe5, 0x63, 0x42, 0x21,
 *        .0x65, 0x3f, 0x2e, 0x7f, 0x22, 0xfd, 0x05, 0x4c,
 *        .0x60, 0xc9, 0x76, 0xa6, 0xf4, 0x3a, 0x93, 0xfe,
 * };
 * char test_msg[] = "SE_aes_cmac_test_string";
 * uint8_t openssl_cmac_digest[AES_BLOCK_SIZE] = { 0 };
 * uint8_t se_cmac_digest[AES_BLOCK_SIZE] = {0};
 * size_t cmac_len;
 *
 * // OpenSSL AES-CMAC
 * CMAC_CTX *cmac = NULL;
 *
 * cmac = CMAC_CTX_new();
 * CMAC_Init(cmac, test_key_256, AES_KEY_256_SIZE, EVP_aes_256_cbc(), NULL);
 *
 * CMAC_Update(cmac, test_msg, sizeof(test_msg));
 * CMAC_Final(cmac, openssl_cmac_digest, &cmac_len);
 * CMAC_CTX_free(cmac);
 *
 * // Write key into keyslot
 * se_write_keyslot(test_key_256, AES_KEY_256_SIZE, AES_QUAD_KEYS_256,
 *                  SE_AES_KEYSLOT_KEK256);
 *
 * // SE AES-CMAC
 * se_cmac = tegra_se_cmac_new();
 * if (se_cmac == NULL)
 *     return;
 *
 * tegra_se_cmac_init(se_cmac, SE_AES_KEYSLOT_KEK256, AES_KEY_256_SIZE);
 * tegra_se_cmac_update(se_cmac, test_msg, sizeof(test_msg));
 * tegra_se_cmac_final(se_cmac, se_cmac_digest, &cmac_len);
 *
 * tegra_se_cmac_free(se_cmac);
 *
 * // Verify the result
 * if (memcmp(openssl_cmac_digest, se_cmac_digest, cmac_len))
 *     TLOGE("%s: Tegra SE AES-CMAC verification is not match.\n", __func__);
 * @endcode
 *
 * @ingroup trusty_key_generation_group
 * @{
 */

/*
 * @brief acquires SE hardware mutex and initializes SE driver
 *
 * @return NO_ERROR if successful
 *
 * @note This function should ALWAYS be called BEFORE interacting
 *       with SE
 */
uint32_t se_acquire(void);

/*
 * @brief releases SE hardware
 *
 * @return NO_ERROR if successful
 *
 * @note This function should ALWAYS be called AFTER interacting
 *       with SE
 */
void se_release(void);

/*
 * @brief derives root key from SE keyslot
 *
 * @param *root_key    [out] root key will be written to this buffer
 * @param root_key_len [in]  length of root_key buffer
 * @param *fv          [in]  base address of fixed vector (fv)
 * @param fv_len       [in]  length of fixed vector
 * @param keyslot      [in]  keyslot index of the root key source
 *
 * @return NO_ERROR if successful
 */
uint32_t se_derive_root_key(uint8_t *root_key, size_t root_key_len, uint8_t *fv,
			    size_t fv_len, uint32_t keyslot);

/*
 * @brief: Write a key into a SE keyslot
 *
 * @param *key_in      [in] base address of the key
 * @param keylen       [in] key length
 * @param key_quad_sel [in] key QUAD selection
 * @param keyslot      [in] keyslot index
 *
 * @return NO_ERROR if successful
 */
int se_write_keyslot(uint8_t *key_in, uint32_t keylen, uint32_t key_quad_sel, uint32_t keyslot);

/*
 * @brief Clear SE keyslots that hold secret keys
 *
 * @return NO_ERROR if successful
 *
 * @note This function should ALWAYS be called so secret keys do
 *       not persist in SE keyslots.
 */
uint32_t se_clear_aes_keyslots(void);

typedef struct tegra_se_cmac_context se_cmac_ctx;

/**
 * @brief  Creates an SE CMAC context.
 *
 * @return  A pointer to the SE CMAC context if successful, or NULL otherwise.
 */
se_cmac_ctx *tegra_se_cmac_new(void);

/**
 * @brief  Frees an SE CMAC context.
 *
 * @param [in]  *se_cmac    A pointer to the SE CMAC context.
 */
void tegra_se_cmac_free(se_cmac_ctx *se_cmac);

/**
 * @brief Initialize the SE CMAC from a user-provided key.
 *
 * @param [in]  *se_cmac    A pointer to the SE CMAC context.
 * @param [in]  *keyslot    A pointer to an SE keyslot
 *                           containing the user-provided key.
 * @param [in]  *keylen     Length of the user-provided key.
 *
 * @retval  NO_ERROR if successful.
 * @retval  ERR_INVALID_ARGS if any of the arguments is invalid.
 * @retval  ERR_NO_MEMORY if no memory is available.
 */
int tegra_se_cmac_init(se_cmac_ctx *se_cmac, se_aes_keyslot_t keyslot,
		       uint32_t keylen);
/**
 * @brief Caches input data in an SE CMAC.
 *
 * This function may be called multiple times to cache additional data.
 *
 * @param [in]  *se_cmac    A pointer to the SE CMAC context.
 * @param [in]  *data   	A pointer to input data.
 * @param [in]  dlen        Length of the input data.
 * @retval  NO_ERROR if successful.
 * @retval  ERR_INVALID_ARGS if any of the arguments is invalid.
 * @retval  ERR_NO_MEMORY if no memory is available.
 */
int tegra_se_cmac_update(se_cmac_ctx *se_cmac, void *data, uint32_t dlen);

/**
 * @brief  Finalizes a SE CMAC.
 *
 * Call this function after the input has been processed
 * and the output has been used.
 *
 * @param [in]  *se_cmac    A pointer to the SE CMAC context.
 * @param [out] *out        A pointer to an output buffer.
 *                          The function places the derived key here.
 * @param [out] *poutlen    A pointer to the derived key length. The function
 *                           places the length of the derived key here.
 * @return  NO_ERROR if successful,
 *   or ERR_INVALID_ARGS if any of the arguments is invalid.
 */
int tegra_se_cmac_final(se_cmac_ctx *se_cmac, uint8_t *out, uint32_t *poutlen);

/** @} */


/**
 * @defgroup trusty_nist_800_108_group NIST 800-108 Key Definition Functions
 *
 * Specifies an API for NIST 800-108 key definition functions.
 *
 * Jetson Linux provides two functions that implement the counter-mode KDF
 * as defined in NIST-SP 800-108. One is hardware-based,
 ( the other software-based. Both are for use only at run time,
 * in contrast to the AES-CMAC functions, which are for use only at boot time.
 *
 * For more information about the architecture of NIST-SP 800-108
 * and the concepts it uses, see NIST Special Publication 800-108,
 * <a href="https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-108.pdf">
 *    Recommendation for Key Derivation Using Pseudorandom Functions
 * </a>.
 *
 * @ingroup trusty_key_generation_group
 * @{
 */

/**
 * @brief  A hardware-based NIST-SP-800-108 KDF; derives keys from the
 *         SE keyslot.
 *
 * @note  Use this function only during Trusty initialization at boot time
 *   (the device boot stage). To derive keys from a key buffer at run time,
 *   use nist_sp_800_with_cmac().
 *
 * @param [in]  keyslot     A pointer to a 128-bit input key (an SE keyslot).
 * @param [in]  key_len     Length in bytes of the input key.
 * @param [in]  *context    A pointer to a NIST-SP-800-108 context string.
 * @param [in]  *label      A pointer to a NIST-SP-800-108 label string.
 * @param [in]  dk_len      Length of the derived key in bytes;
 *                           may be 16 (128 bits) or any multiple of 16.
 * @param [out] *out_dk     A pointer to the derived key. The function stores
 *                           its result in this location.
 *
 * @return NO_ERROR if successful, or ERR_NO_MEMORY if no memory is available.
 */
int se_nist_sp_800_108_with_cmac(se_aes_keyslot_t keyslot,
				 uint32_t key_len,
				 char const *context,
				 char const *label,
				 uint32_t dk_len,
				 uint8_t *out_dk);

 /** @} */

#endif /* __TEGRA_SE_H__ */
