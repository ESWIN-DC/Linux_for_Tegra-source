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

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "tegra-crypto.h"
#include "tegra-cryptodev.h"

static int fd = -1;

int tegra_crypto_op(unsigned char *in, unsigned char *out, int len,
		    unsigned char *iv, int iv_len, int encrypt,
		    unsigned int crypto_op_mode, bool close)
{
	struct tegra_crypt_req crypt_req;
	int rc = 0;

	if (fd == -1)
		fd = open("/dev/tegra-crypto", O_RDWR);

	if (fd < 0) {
		LOG("%s: /dev/tegra-crypto open fail\n", __func__);
		return -1;
	}

	crypt_req.skip_exit = !close;
	crypt_req.op = crypto_op_mode;
	crypt_req.encrypt = encrypt;

	memset(crypt_req.key, 0, AES_KEYSIZE_128);
	crypt_req.keylen = AES_KEYSIZE_128;
	memcpy(crypt_req.iv, iv, iv_len);
	crypt_req.ivlen = iv_len;
	crypt_req.plaintext = in;
	crypt_req.plaintext_sz = len;
	crypt_req.result = out;
	crypt_req.skip_key = 0;
	crypt_req.skip_iv = 0;

	rc = ioctl(fd, TEGRA_CRYPTO_IOCTL_NEED_SSK, 1);
	if (rc < 0) {
		LOG("tegra_crypto ioctl error: TEGRA_CRYPTO_IOCTL_NEED_SSK\n");
		goto err;
	}

	rc = ioctl(fd, TEGRA_CRYPTO_IOCTL_PROCESS_REQ, &crypt_req);
	if (rc < 0) {
		LOG("tegra_crypto ioctl error: TEGRA_CRYPTO_IOCTL_PROCESS_REQ\n");
		goto err;
	}

	if (close)
		tegra_crypto_op_close();

err:
	return rc;
}

void tegra_crypto_op_close(void)
{
	if (fd >= 0) {
		close(fd);
		fd = -1;
	}
}
