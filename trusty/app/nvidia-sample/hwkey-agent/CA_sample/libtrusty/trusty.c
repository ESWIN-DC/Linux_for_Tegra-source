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

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "common.h"
#include "tipc_ioctl.h"
#include "tipc.h"

int tipc_connect(const char *dev_name, const char *srv_name)
{
	int fd;
	int rc;

	fd = open(dev_name, O_RDWR);
	if (fd < 0) {
		rc = -errno;
		LOG("%s: cannot open tipc device \"%s\": %s\n",
		      __func__, dev_name, strerror(errno));
		return rc < 0 ? rc : -1;
	}

	rc = ioctl(fd, TIPC_IOC_CONNECT, srv_name);
	if (rc < 0) {
		rc = -errno;
		LOG("%s: can't connect to tipc service \"%s\" (err=%d)\n",
		      __func__, srv_name, errno);
		close(fd);
		return rc < 0 ? rc : -1;
	}

	return fd;
}

void tipc_close(int fd)
{
	if (fd >= 0)
		close(fd);
}
