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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "luks-srv.h"
#include "tipc.h"

static char args_doc[] = "-c <context string> -g|[u] -n";

static struct argp_option options[] = {
	{"context-string", 'c', "CONTEXT STRING", 0, "The context string for passphrase generation (Max length: 40)."},
	{"get-generic-pass", 'g', 0, 0, "Get generic passphrase."},
	{"get-unique-pass", 'u', 0 , 0, "Get unique passphrase."},
	{"no-pass-response", 'n', 0, 0, "No passphrase response after this command."},
	{ 0 }
};

struct arguments {
	char *context_str;
	bool unique_passphrase;
	bool no_pass_response;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	struct arguments *argus = state->input;

	switch (key) {
	case 'c':
		argus->context_str = strdup(arg);
		break;
	case 'g':
		argus->unique_passphrase = false;
		break;
	case 'u':
		argus->unique_passphrase = true;
		break;
	case 'n':
		argus->no_pass_response = true;
		break;
	case ARGP_KEY_ARG:
		if (state->argc < 1)
			argp_usage(state);
	case ARGP_KEY_END:
		if (!argus->context_str ||
		    strlen(argus->context_str) > LUKS_SRV_CONTEXT_STR_LEN)
			if (!argus->no_pass_response)
				argp_usage(state);
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, 0, 0, 0, 0 };

static void luks_srv_app_handler(struct arguments *argus) {
	int luks_srv_fd, i;
	luks_srv_cmd_msg_t *msg;
	uint32_t msg_size = sizeof(luks_srv_cmd_msg_t);

	luks_srv_fd = tipc_connect(TIPC_DEFAULT_NODE, TA_LUKS_SRV_CHAL);
	if (luks_srv_fd < 0) {
		LOG("LUKS_SRV: tipc connect fail.\n");
		return;
	}

	msg = calloc(1, msg_size);
	if (msg == NULL) {
		LOG("LUKS_SRV: memory alloc fail.\n");
		return;
	}

	if (argus->no_pass_response)
		msg->luks_srv_cmd = LUKS_NO_PASS_RESPONSE;
	else if (argus->unique_passphrase)
		msg->luks_srv_cmd = LUKS_GET_UNIQUE_PASS;
	else
		msg->luks_srv_cmd = LUKS_GET_GENERIC_PASS;

	if (argus->context_str != NULL)
		memcpy(msg->context_str, argus->context_str, strlen(argus->context_str));

	/* Send msg to TA */
	write(luks_srv_fd, msg, msg_size);

	/* Get response from TA */
	read(luks_srv_fd, msg, msg_size);

	for (i = 0; i < LUKS_SRV_PASSPHRASE_LEN; i ++)
		fprintf(stdout, "%02x", msg->output_passphrase[i]);
	fprintf(stdout, "\n");
}

int main(int argc, char *argv[]) {
	struct arguments argus = {
		.context_str = NULL,
		.unique_passphrase = false,
		.no_pass_response = false,
	};

	/* Handle the input parameters */
	argp_parse(&argp, argc, argv, 0, 0, &argus);

	luks_srv_app_handler(&argus);

	return 0;
}
