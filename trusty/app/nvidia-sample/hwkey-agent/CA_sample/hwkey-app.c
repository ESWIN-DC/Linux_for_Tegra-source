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

#include <argp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "tegra-crypto.h"
#include "tegra-cryptodev.h"
#include "tipc.h"

/*
 * Note that, the default_iv can be a input factor of the CA program.
 */
static uint8_t default_iv[AES_BLOCK_SIZE] = {
	0x36, 0xeb, 0x39, 0xfe, 0x3a, 0xcf, 0x1a, 0xf5,
	0x68, 0xc1, 0xb8, 0xe6, 0xf4, 0x8e, 0x5c, 0x79,
};

/*
 * Please enable Tegra SE via makefile and rebuild it.
 *
 * Before enabling the Tegra SE, please notice that the Tegra SE can be shared
 * by other clients or programs, which means it could not be available when
 * 'hwkey-app' tries to use that.
 */
#if (ENABLE_TEGRA_SE == 1)
static char args_doc[] = "-e [-d] -i <file> -o <out-file> -t|[s]";
#else
static char args_doc[] = "-e [-d] -i <file> -o <out-file> -t";
#endif

static struct argp_option options[] = {
	{ 0, 'e', 0, 0, "Encryption mode"},
	{ 0, 'd', 0, 0, "Decryption mode"},
	{"in", 'i', "file", 0, "Input file for encrypt/decrypt"},
	{"out", 'o', "outfile", 0, "Output file" },
	{"trusty", 't', 0, 0, "Encrypt using Trusty"},
#if (ENABLE_TEGRA_SE == 1)
	{"tegracrypto", 's', 0, 0, "Encrypt using SE via /dev/tegra-crypto"},
#endif
	{ 0 }
};

struct arguments {
	bool encryption;
	char* in_file;
	char* out_file;
	bool trusty;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	struct arguments *argus = state->input;

	switch (key) {
	case 'e':
		argus->encryption = true;
		break;
	case 'd':
		argus->encryption = false;
		break;
	case 'i':
		argus->in_file = strdup(arg);
		break;
	case 'o':
		argus->out_file = strdup(arg);
		break;
	case 't':
		argus->trusty = true;
		break;
#if (ENABLE_TEGRA_SE == 1)
	case 's':
		argus->trusty = false;
		break;
#endif
	case ARGP_KEY_ARG:
		if (state->argc <= 1)
			argp_usage(state);
	case ARGP_KEY_END:
		if (!argus->in_file || !argus->out_file || state->argc <= 6)
			argp_usage(state);
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, 0, 0, 0, 0 };
static int crypto_srv_fd = -1;
static FILE *infptr = NULL, *outfptr = NULL;

static void fail_handler(int f)
{
	tipc_close(crypto_srv_fd);
	tegra_crypto_op_close();

	if (infptr != NULL)
		fclose(infptr);
	if (outfptr != NULL)
		fclose(outfptr);

	exit(f);
}

static int get_file_size(FILE *fptr)
{
	size_t file_len;

	fseek(fptr, 0, SEEK_END);
	file_len = ftell(fptr);
	rewind(fptr);

	return file_len;
}

static uint8_t iv[AES_BLOCK_SIZE];
static int crypto_srv_handler(struct arguments *argus, uint8_t *in_data,
			      int data_len, FILE *outfptr, bool first_packet)
{
	int rc = 0;

	if (first_packet)
		memcpy(iv, default_iv, AES_BLOCK_SIZE);

	if (argus->trusty) {
		crypto_srv_msg_t *msg = NULL;
		size_t msg_size;
		fd_set rfds;
		struct timeval tv;

		if (crypto_srv_fd < 0) {
			crypto_srv_fd = tipc_connect(TIPC_DEFAULT_NODE,
						     TA_CRYPTO_SRV_CHAL);
			if (crypto_srv_fd < 0)
				fail_handler(1);
		}

		msg_size = sizeof(crypto_srv_msg_t) + CRYPTO_SRV_PAYLOAD_SIZE;
		msg = calloc(1, msg_size);
		if (msg == NULL) {
			LOG("Failed to allocate crypto_srv_msg buffer\n");
			fail_handler(1);
		}

		msg->payload_len = data_len;
		memcpy(msg->payload, in_data, data_len);
		memcpy(msg->iv, iv, AES_BLOCK_SIZE);
		if (argus->encryption)
			msg->cmd = CRYPTO_SRV_CMD_ENCRYPT;
		else
			msg->cmd = CRYPTO_SRV_CMD_DECRYPT;

re_send:
		write(crypto_srv_fd, msg, msg_size);

		FD_ZERO(&rfds);
		FD_SET(crypto_srv_fd, &rfds);
		tv.tv_sec = 0;
		tv.tv_usec = 500000;
		if (select(crypto_srv_fd + 1, &rfds, NULL, NULL, &tv)) {
			read(crypto_srv_fd, msg, msg_size);
			fwrite(msg->payload, data_len, 1, outfptr);
		} else {
			LOG("Unexpected packet lost in TIPC\n");
			goto re_send;
		}

		/* update IV for next chunk */
		if (argus->encryption)
			memcpy(iv, msg->payload, AES_BLOCK_SIZE);
		else
			memcpy(iv, in_data, AES_BLOCK_SIZE);

		free(msg);
	} else {
#if (ENABLE_TEGRA_SE == 1)
		uint8_t *input_buff = NULL;
		uint8_t *output_buff = NULL;

		input_buff = calloc(1, data_len);
		output_buff = calloc(1, data_len);
		if (input_buff == NULL) {
			LOG("Failed to allocate crypto_srv_msg buffer\n");
			fail_handler(1);
		}

		memcpy(input_buff, in_data, data_len);

		rc = tegra_crypto_op(input_buff, output_buff, data_len,
				     iv, AES_BLOCK_SIZE, argus->encryption,
				     TEGRA_CRYPTO_CBC, false);
		if (rc < 0) {
			LOG("Tegra-crypto failed\n");
			fail_handler(1);
		}

		fwrite(output_buff, data_len, 1, outfptr);

		/* update IV for next chunk */
		if (argus->encryption)
			memcpy(iv, output_buff, AES_BLOCK_SIZE);
		else
			memcpy(iv, in_data, AES_BLOCK_SIZE);

		free(input_buff);
		free(output_buff);
#endif
	}

	return rc;
}

int main(int argc, char *argv[]) {
	struct arguments argus;
	int inf_size, total_len = 0;

	/* Handle the break signal */
	signal(SIGINT, fail_handler);

	/* Handle the input parameters */
	argp_parse(&argp, argc, argv, 0, 0, &argus);

	/* Create and open the input/output files */
	infptr = fopen(argus.in_file, "rb");
	if (infptr == NULL) {
		LOG("Fail to open the input file: %s\n", argus.in_file);
		fail_handler(1);
	}

	outfptr = fopen(argus.out_file, "wb");
	if (outfptr == NULL) {
		LOG("Fail to open the output file: %s\n", argus.out_file);
		fail_handler(1);
	}

	inf_size = get_file_size(infptr);

	/*
	 * Input file size checking
	 *
	 * This CA doesn't support padding scheme like pkcs7 or zero padding.
	 * So the input file size must be mutiple of AES_BLOCK_SIZE (16 byte).
	 */
	if (inf_size % AES_BLOCK_SIZE != 0) {
		printf("The input file size must be multiple of AES_BLOCK_SIZE(16 bytes).\n");
		fail_handler(1);
	}

	do {
		int f_rc;
		uint8_t buff[CRYPTO_SRV_PAYLOAD_SIZE];
		bool first_packet = false;
		bool last_packet = false;

		memset(buff, 0, CRYPTO_SRV_PAYLOAD_SIZE);

		f_rc = fread(buff, 1, CRYPTO_SRV_PAYLOAD_SIZE, infptr);
		if (f_rc < 0) {
			LOG("Unexpected failure when reading input file\n");
			fail_handler(1);
		}

		total_len += f_rc;

		if (total_len <= CRYPTO_SRV_PAYLOAD_SIZE)
			first_packet = true;
		else
			first_packet = false;

		if (total_len >= inf_size)
			last_packet = true;

		/* Handle crypto service */
		crypto_srv_handler(&argus, buff, f_rc, outfptr, first_packet);

		if (last_packet)
			break;
	} while(1);

	if (argus.trusty)
		tipc_close(crypto_srv_fd);
	else
		tegra_crypto_op(NULL, NULL, 0, NULL, 0, argus.encryption,
				TEGRA_CRYPTO_CBC, true);

	fclose(infptr);
	fclose(outfptr);

	return 0;
}
