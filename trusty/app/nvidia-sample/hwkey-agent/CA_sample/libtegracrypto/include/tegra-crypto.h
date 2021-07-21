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

#ifndef __TEGRA_CRYPTO_H__
#define __TEGRA_CRYPTO_H__

#define TEGRA_CRYPTO_ENCRYPT 1
#define TEGRA_CRYPTO_DECRYPT 0

/*
 * @brief proceed Tegra crypto operation
 *
 * @param *in			[in] input pointer of the data for crypto op
 * @param *out			[in] output pointer of the result
 * @param len			[in] the data length
 * @param *iv			[in] the pointer of initial vector
 * @param iv_len		[in] the length of initial vector
 * @param encrypt		[in] TEGRA_CRYPTO_ENCRYPT or TEGRA_CRYPTO_DECRYPT
 * @param crypto_op_mode	[in] crypto op mode i.e. TEGRA_CRYPTO_CBC
 * @param close			[in] indicate the driver to release SE
 *
 * @return 0 means success
 */
int tegra_crypto_op(unsigned char *in, unsigned char *out, int len,
		    unsigned char *iv, int iv_len, int encrypt,
		    unsigned int crypto_op_mode, bool close);

/*
 * @brief close Tegra crypto op
 */
void tegra_crypto_op_close(void);

#endif /* __TEGRA_CRYPTO_H__ */
