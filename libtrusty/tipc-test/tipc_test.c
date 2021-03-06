/*
 * Copyright (C) 2015-2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include <trusty/tipc.h>

#define TIPC_DEFAULT_DEVNAME "/dev/trusty-ipc-dev0"

static const char *dev_name = NULL;
static const char *test_name = NULL;

static const char *uuid_name = "com.android.ipc-unittest.srv.uuid";
static const char *echo_name = "com.android.ipc-unittest.srv.echo";
static const char *ta_only_name = "com.android.ipc-unittest.srv.ta_only";
static const char *ns_only_name = "com.android.ipc-unittest.srv.ns_only";
static const char *datasink_name = "com.android.ipc-unittest.srv.datasink";
static const char *closer1_name = "com.android.ipc-unittest.srv.closer1";
static const char *closer2_name = "com.android.ipc-unittest.srv.closer2";
static const char *closer3_name = "com.android.ipc-unittest.srv.closer3";
static const char *main_ctrl_name = "com.android.ipc-unittest.ctrl";

static const char *_sopts = "hsvD:t:r:m:b:";
static const struct option _lopts[] =  {
	{"help",    no_argument,       0, 'h'},
	{"silent",  no_argument,       0, 's'},
	{"variable",no_argument,       0, 'v'},
	{"dev",     required_argument, 0, 'D'},
	{"repeat",  required_argument, 0, 'r'},
	{"burst",   required_argument, 0, 'b'},
	{"msgsize", required_argument, 0, 'm'},
	{0, 0, 0, 0}
};

static const char *usage =
"Usage: %s [options]\n"
"\n"
"options:\n"
"  -h, --help            prints this message and exit\n"
"  -D, --dev name        device name\n"
"  -t, --test name       test to run\n"
"  -r, --repeat cnt      repeat count\n"
"  -m, --msgsize size    max message size\n"
"  -v, --variable        variable message size\n"
"  -s, --silent          silent\n"
"\n"
;

static const char *usage_long =
"\n"
"The following tests are available:\n"
"   connect      - connect to datasink service\n"
"   connect_foo  - connect to non existing service\n"
"   burst_write  - send messages to datasink service\n"
"   echo         - send/receive messages to echo service\n"
"   select       - test select call\n"
"   blocked_read - test blocked read\n"
"   closer1      - connection closed by remote (test1)\n"
"   closer2      - connection closed by remote (test2)\n"
"   closer3      - connection closed by remote (test3)\n"
"   ta2ta-ipc    - execute TA to TA unittest\n"
"   dev-uuid     - print device uuid\n"
"   ta-access    - test ta-access flags\n"
"\n"
;

static uint opt_repeat  = 1;
static uint opt_msgsize = 32;
static uint opt_msgburst = 32;
static bool opt_variable = false;
static bool opt_silent = false;

static void print_usage_and_exit(const char *prog, int code, bool verbose)
{
	fprintf (stderr, usage, prog);
	if (verbose)
		fprintf (stderr, "%s", usage_long);
	exit(code);
}

static void parse_options(int argc, char **argv)
{
	int c;
	int oidx = 0;

	while (1)
	{
		c = getopt_long (argc, argv, _sopts, _lopts, &oidx);
		if (c == -1)
			break; /* done */

		switch (c) {

		case 'D':
			dev_name = strdup(optarg);
		break;

		case 't':
			test_name = strdup(optarg);
		break;

		case 'v':
			opt_variable = true;
		break;

		case 'r':
			opt_repeat = atoi(optarg);
		break;

		case 'm':
			opt_msgsize = atoi(optarg);
		break;

		case 'b':
			opt_msgburst = atoi(optarg);
		break;

		case 's':
			opt_silent = true;
		break;

		case 'h':
		      print_usage_and_exit(argv[0], EXIT_SUCCESS, true);
		break;

		default:
		      print_usage_and_exit(argv[0], EXIT_FAILURE, false);
		}
	}
}

static int connect_test(uint repeat)
{
	uint i;
	int  echo_fd;
	int  dsink_fd;
	int  test_status = EXIT_SUCCESS;

	if (!opt_silent) {
		printf("%s: repeat = %u\n", __func__, repeat);
	}

	for (i = 0; i < repeat; i++) {
		echo_fd = tipc_connect(dev_name, echo_name);
		if (echo_fd < 0) {
			fprintf(stderr, "Failed to connect to '%s' service\n",
				"echo");
			test_status = EXIT_FAILURE;
		}
		dsink_fd = tipc_connect(dev_name, datasink_name);
		if (dsink_fd < 0) {
			fprintf(stderr, "Failed to connect to '%s' service\n",
				"datasink");
			test_status = EXIT_FAILURE;
		}

		if (echo_fd >= 0) {
			tipc_close(echo_fd);
		}
		if (dsink_fd >= 0) {
			tipc_close(dsink_fd);
		}
	}

	if (!opt_silent) {
		printf("%s: done\n", __func__);
	}

	return test_status;
}

static int connect_foo(uint repeat)
{
	uint i;
	int  fd;
	int  test_status = EXIT_SUCCESS;

	if (!opt_silent) {
		printf("%s: repeat = %u\n", __func__, repeat);
	}

	for (i = 0; i < repeat; i++) {
		fd = tipc_connect(dev_name, "foo");
		if (fd >= 0) {
			fprintf(stderr, "succeeded to connect to '%s' service\n",
				"foo");
			tipc_close(fd);
			test_status = EXIT_FAILURE;
		}
	}

	if (!opt_silent) {
		printf("%s: done\n", __func__);
	}

	return test_status;
}


static int closer1_test(uint repeat)
{
	uint i;
	int  fd;

	if (!opt_silent) {
		printf("%s: repeat = %u\n", __func__, repeat);
	}

	for (i = 0; i < repeat; i++) {
		fd = tipc_connect(dev_name, closer1_name);
		if (fd < 0) {
			fprintf(stderr, "Failed to connect to '%s' service\n",
				"closer1");
			continue;
		}
		if (!opt_silent) {
			printf("%s: connected\n", __func__);
		}
		tipc_close(fd);
	}

	if (!opt_silent) {
		printf("%s: done\n", __func__);
	}

	return 0;
}

static int closer2_test(uint repeat)
{
	uint i;
	int  fd;
	int  test_status = EXIT_SUCCESS;

	if (!opt_silent) {
		printf("%s: repeat = %u\n", __func__, repeat);
	}

	for (i = 0; i < repeat; i++) {
		fd = tipc_connect(dev_name, closer2_name);
		if (fd < 0) {
			if (!opt_silent) {
				printf("failed to connect to '%s' service\n", "closer2");
			}
		} else {
			/* this should always fail */
			fprintf(stderr, "connected to '%s' service\n", "closer2");
			tipc_close(fd);
			test_status = EXIT_FAILURE;
		}
	}

	if (!opt_silent) {
		printf("%s: done\n", __func__);
	}

	return test_status;
}

static int closer3_test(uint repeat)
{
	uint i, j;
	ssize_t rc;
	int  fd[4];
	char buf[64];
	int  test_status = EXIT_SUCCESS;

	if (!opt_silent) {
		printf("%s: repeat = %u\n", __func__, repeat);
	}

	for (i = 0; i < repeat; i++) {

		/* open 4 connections to closer3 service */
		for (j = 0; j < 4; j++) {
			fd[j] = tipc_connect(dev_name, closer3_name);
			if (fd[j] < 0) {
				fprintf(stderr, "fd[%d]: failed to connect to '%s' service\n", j, "closer3");
				test_status = EXIT_FAILURE;
			} else {
				if (!opt_silent) {
					printf("%s: fd[%d]=%d: connected\n", __func__, j, fd[j]);
				}
				memset(buf, i + j, sizeof(buf));
				rc = write(fd[j], buf, sizeof(buf));
				if (rc != sizeof(buf)) {
					if (!opt_silent) {
						printf("%s: fd[%d]=%d: write returned  = %zd\n",
							__func__, j, fd[j], rc);
					}
					perror("closer3_test: write");
					test_status = EXIT_FAILURE;
				}
			}
		}

		/* sleep a bit */
		sleep(1);

		/* It is expected that they will be closed by remote */
		for (j = 0; j < 4; j++) {
			if (fd[j] < 0)
				continue;
			/* write should always fail */
			rc = write(fd[j], buf, sizeof(buf));
			if (rc != sizeof(buf)) {
				if (!opt_silent) {
					printf("%s: fd[%d]=%d: write returned = %zd\n",
						__func__, j, fd[j], rc);
				}
				perror("closer3_test: write");
			} else {
				fprintf(stderr, "%s: fd[%d]=%d: write succeeded\n",
						__func__, j, fd[j]);
				test_status = EXIT_FAILURE;
			}
		}

		/* then they have to be closed by remote */
		for (j = 0; j < 4; j++) {
			if (fd[j] >= 0) {
				tipc_close(fd[j]);
			}
		}
	}

	if (!opt_silent) {
		printf("%s: done\n", __func__);
	}

	return test_status;
}


static int echo_test(uint repeat, uint msgsz, bool var)
{
	uint i;
	ssize_t rc;
	size_t  msg_len;
	int  echo_fd =-1;
	char tx_buf[msgsz];
	char rx_buf[msgsz];
	int  test_status = EXIT_SUCCESS;

	if (!opt_silent) {
		printf("%s: repeat %u: msgsz %u: variable %s\n",
			__func__, repeat, msgsz, var ? "true" : "false");
	}

	echo_fd = tipc_connect(dev_name, echo_name);
	if (echo_fd < 0) {
		test_status = EXIT_FAILURE;
		fprintf(stderr, "Failed to connect to service\n");
		return test_status;
	}

	for (i = 0; i < repeat; i++) {

		msg_len = msgsz;
		if (opt_variable && msgsz) {
			msg_len = rand() % msgsz;
		}

		memset(tx_buf, i + 1, msg_len);

		rc = write(echo_fd, tx_buf, msg_len);
		if ((size_t)rc != msg_len) {
			perror("echo_test: write");
			test_status = EXIT_FAILURE;
			break;
		}

		rc = read(echo_fd, rx_buf, msg_len);
		if (rc < 0) {
			perror("echo_test: read");
			test_status = EXIT_FAILURE;
			break;
		}

		if ((size_t)rc != msg_len) {
			fprintf(stderr, "data truncated (%zu vs. %zu)\n",
			                 rc, msg_len);
			test_status = EXIT_FAILURE;
			continue;
		}

		if (memcmp(tx_buf, rx_buf, (size_t) rc)) {
			fprintf(stderr, "data mismatch\n");
			test_status = EXIT_FAILURE;
			continue;
		}
	}

	tipc_close(echo_fd);

	if (!opt_silent) {
		printf("%s: done\n",__func__);
	}

	return test_status;
}

static int burst_write_test(uint repeat, uint msgburst, uint msgsz, bool var)
{
	int fd;
	uint i, j;
	ssize_t rc;
	size_t  msg_len;
	char tx_buf[msgsz];
	int  test_status = EXIT_SUCCESS;

	if (!opt_silent) {
		printf("%s: repeat %u: burst %u: msgsz %u: variable %s\n",
			__func__, repeat, msgburst, msgsz,
			var ? "true" : "false");
	}

	for (i = 0; i < repeat; i++) {

		fd = tipc_connect(dev_name, datasink_name);
		if (fd < 0) {
			fprintf(stderr, "Failed to connect to '%s' service\n",
				"datasink");
			test_status = EXIT_FAILURE;
			break;
		}

		for (j = 0; j < msgburst; j++) {
			msg_len = msgsz;
			if (var && msgsz) {
				msg_len = rand() % msgsz;
			}

			memset(tx_buf, i + 1, msg_len);
			rc = write(fd, tx_buf, msg_len);
			if ((size_t)rc != msg_len) {
				perror("burst_test: write");
				test_status = EXIT_FAILURE;
				break;
			}
		}

		tipc_close(fd);
	}

	if (!opt_silent) {
		printf("%s: done\n",__func__);
	}

	return test_status;
}


static int _wait_for_msg(int fd, uint msgsz, int timeout)
{
	int rc;
	fd_set rfds;
	uint msgcnt = 0;
	char rx_buf[msgsz];
	struct timeval tv;
	int  test_status = EXIT_SUCCESS;

	if (!opt_silent) {
		printf("waiting (%d) for msg\n", timeout);
	}

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);

	tv.tv_sec = timeout;
	tv.tv_usec = 0;

	for(;;) {
		rc = select(fd+1, &rfds, NULL, NULL, &tv);

		if (rc == 0) {
			if (!opt_silent) {
				printf("select timedout\n");
			}
			break;
		}

		if (rc == -1) {
			perror("select_test: select");
			test_status = EXIT_FAILURE;
			return test_status;
		}

		rc = read(fd, rx_buf, sizeof(rx_buf));
		if (rc < 0) {
			perror("select_test: read");
			test_status = EXIT_FAILURE;
			return test_status;
		} else {
			if (rc > 0) {
				msgcnt++;
			}
		}
	}

	if (!opt_silent) {
		printf("got %u messages\n", msgcnt);
	}

	return test_status;
}


static int select_test(uint repeat, uint msgburst, uint msgsz)
{
	int fd;
	uint i, j;
	ssize_t rc;
	char tx_buf[msgsz];
	int  test_status = EXIT_SUCCESS;

	if (!opt_silent) {
		printf("%s: repeat %u\n", __func__, repeat);
	}

	fd = tipc_connect(dev_name, echo_name);
	if (fd < 0) {
		fprintf(stderr, "Failed to connect to '%s' service\n",
			"echo");
		test_status = EXIT_FAILURE;
		return test_status;
	}

	for (i = 0; i < repeat; i++) {

		if (!opt_silent) {
			printf("sending burst: %u msg\n", msgburst);
		}

		for (j = 0; j < msgburst; j++) {
			memset(tx_buf, i + j, msgsz);
			rc = write(fd, tx_buf, msgsz);
			if ((size_t)rc != msgsz) {
				perror("burst_test: write");
				test_status = EXIT_FAILURE;
				break;
			}
		}

		_wait_for_msg(fd, msgsz, 1);
	}

	tipc_close(fd);

	if (!opt_silent) {
		printf("%s: done\n",__func__);
	}

	return test_status;
}

static int blocked_read_test(uint repeat)
{
	int fd;
	uint i;
	ssize_t rc;
	char rx_buf[512];

	if (!opt_silent) {
		printf("%s: repeat %u\n", __func__, repeat);
	}

	fd = tipc_connect(dev_name, echo_name);
	if (fd < 0) {
		fprintf(stderr, "Failed to connect to '%s' service\n",
			"echo");
		return fd;
	}

	for (i = 0; i < repeat; i++) {
		rc = read(fd, rx_buf, sizeof(rx_buf));
		if (rc < 0) {
			perror("select_test: read");
			break;
		} else {
			if (!opt_silent) {
				printf("got %zd bytes\n", rc);
			}
		}
	}

	tipc_close(fd);

	if (!opt_silent) {
		printf("%s: done\n",__func__);
	}

	return 0;
}

static int ta2ta_ipc_test(void)
{
	int fd;
	char rx_buf[64];
	int  test_status = EXIT_SUCCESS;
	ssize_t rc;
	char pass_str[] = "PASSED";

	if (!opt_silent) {
		printf("%s:\n", __func__);
	}

	fd = tipc_connect(dev_name, main_ctrl_name);
	if (fd < 0) {
		fprintf(stderr, "Failed to connect to '%s' service\n",
			"main_ctrl");
		test_status = EXIT_FAILURE;
		return test_status;
	}

	/* wait for test to complete */
	rc = read(fd, rx_buf, sizeof(pass_str));
	if (rc < 0) {
		perror("ta2ta_ipc_test: read");
		test_status = EXIT_FAILURE;
	} else {
		if (strncmp(rx_buf, pass_str, sizeof(pass_str)) != 0) {
			test_status = EXIT_FAILURE;
		}
	}

	tipc_close(fd);

	return test_status;
}

typedef struct uuid
{
	uint32_t time_low;
	uint16_t time_mid;
	uint16_t time_hi_and_version;
	uint8_t clock_seq_and_node[8];
} uuid_t;

static void print_uuid(const char *dev, uuid_t *uuid)
{
	printf("%s:", dev);
	printf("uuid: %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
	       uuid->time_low,
	       uuid->time_mid,
	       uuid->time_hi_and_version,
	       uuid->clock_seq_and_node[0],
	       uuid->clock_seq_and_node[1],
	       uuid->clock_seq_and_node[2],
	       uuid->clock_seq_and_node[3],
	       uuid->clock_seq_and_node[4],
	       uuid->clock_seq_and_node[5],
	       uuid->clock_seq_and_node[6],
	       uuid->clock_seq_and_node[7]
	       );
}

static int dev_uuid_test(void)
{
	int fd;
	ssize_t rc;
	uuid_t uuid;
	int  test_status = EXIT_SUCCESS;

	fd = tipc_connect(dev_name, uuid_name);
	if (fd < 0) {
		fprintf(stderr, "Failed to connect to '%s' service\n",
			"uuid");
		test_status = EXIT_FAILURE;
		return test_status;
	}

	/* wait for test to complete */
	rc = read(fd, &uuid, sizeof(uuid));
	if (rc < 0) {
		perror("dev_uuid_test: read");
		test_status = EXIT_FAILURE;
	} else if (rc != sizeof(uuid)) {
		fprintf(stderr, "unexpected uuid size (%d vs. %d)\n",
			(int)rc, (int)sizeof(uuid));
		test_status = EXIT_FAILURE;
	} else {
		print_uuid(dev_name, &uuid);
	}

	tipc_close(fd);

	return test_status;
}

static int ta_access_test(void)
{
	int fd;
	int  test_status = EXIT_SUCCESS;

	if (!opt_silent) {
		printf("%s:\n", __func__);
	}

	fd = tipc_connect(dev_name, ta_only_name);
	if (fd >= 0) {
		fprintf(stderr, "Succeed to connect to '%s' service\n",
			"ta_only");
		test_status = EXIT_FAILURE;
		tipc_close(fd);
	}

	fd = tipc_connect(dev_name, ns_only_name);
	if (fd < 0) {
		fprintf(stderr, "Failed to connect to '%s' service\n",
			"ns_only");
		test_status = EXIT_FAILURE;
		return test_status;
	}
	tipc_close(fd);

	if (!opt_silent) {
		printf("%s: done\n",__func__);
	}

	return test_status;
}


int main(int argc, char **argv)
{
	int rc = EXIT_SUCCESS;

	if (argc <= 1) {
		print_usage_and_exit(argv[0], EXIT_FAILURE, false);
	}

	parse_options(argc, argv);

	if (!dev_name) {
		dev_name = TIPC_DEFAULT_DEVNAME;
	}

	if (!test_name) {
		fprintf(stderr, "need a Test to run\n");
		print_usage_and_exit(argv[0], EXIT_FAILURE, true);
	}

	if (strcmp(test_name, "connect") == 0) {
		rc = connect_test(opt_repeat);
	} else if (strcmp(test_name, "connect_foo") == 0) {
		rc = connect_foo(opt_repeat);
	} else if (strcmp(test_name, "burst_write") == 0) {
		rc = burst_write_test(opt_repeat, opt_msgburst, opt_msgsize, opt_variable);
	} else if (strcmp(test_name, "select") == 0) {
		rc = select_test(opt_repeat, opt_msgburst,  opt_msgsize);
		return rc;
	} else if (strcmp(test_name, "blocked_read") == 0) {
		rc = blocked_read_test(opt_repeat);
	} else if (strcmp(test_name, "closer1") == 0) {
		rc = closer1_test(opt_repeat);
		return rc;
	} else if (strcmp(test_name, "closer2") == 0) {
		rc = closer2_test(opt_repeat);
	} else if (strcmp(test_name, "closer3") == 0) {
		rc = closer3_test(opt_repeat);
	} else if (strcmp(test_name, "echo") == 0) {
		rc = echo_test(opt_repeat, opt_msgsize, opt_variable);
	} else if(strcmp(test_name, "ta2ta-ipc") == 0) {
		rc = ta2ta_ipc_test();
	} else if (strcmp(test_name, "dev-uuid") == 0) {
		rc = dev_uuid_test();
	} else if (strcmp(test_name, "ta-access") == 0) {
		rc = ta_access_test();
	} else {
		fprintf(stderr, "Unrecognized test name '%s'\n", test_name);
		print_usage_and_exit(argv[0], EXIT_FAILURE, true);
	}

	if (rc == EXIT_SUCCESS) {
		printf("Testname: %s : Passed !!\n", test_name);
	} else {
		printf("Testname: %s : Failed !!\n", test_name);
	}

	return rc;
}
