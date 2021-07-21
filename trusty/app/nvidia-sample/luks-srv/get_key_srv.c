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

#include <err.h>
#include <get_key_srv.h>
#include <lib/trusty/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <trusty_std.h>

static int transceive_get_key_srv_cmd_msg(handle_t session,
					  get_key_srv_cmd_msg_t *msg)
{
	int rc;
	iovec_t iov = {
		.base = msg,
		.len = sizeof(*msg),
	};
	ipc_msg_t ipc_msg = {
		.iov = &iov,
		.num_iov = 1,
	};
	uevent_t uevt;
	ipc_msg_info_t ipc_info;

	rc = send_msg(session, &ipc_msg);
	if (rc < 0)
		goto err_fail;

	if ((size_t)rc != sizeof(*msg)) {
		rc = ERR_IO;
		goto err_fail;
	}

	rc = wait(session, &uevt, INFINITE_TIME);
	if (rc != NO_ERROR)
		goto err_fail;

	rc = get_msg(session, &ipc_info);
	if (rc != NO_ERROR)
		goto err_fail;

	rc = read_msg(session, ipc_info.id, 0, &ipc_msg);
	put_msg(session, ipc_info.id);
	if (rc < 0)
		goto err_fail;

	if ((size_t)rc != sizeof(*msg)) {
		rc = ERR_IO;
		goto err_fail;
	}

	return 0;

err_fail:
	TLOGE("%s: failed (%d)\n", __func__, rc);
	return rc;
}

handle_t get_key_srv_open(void)
{
	return connect(GET_KEY_SRV_PORT, IPC_CONNECT_WAIT_FOR_PORT);
}

int get_key_srv_query_ekb_key(handle_t session, get_key_srv_cmd_msg_t *msg)
{
	msg->cmd = GET_KEY_SRV_QUERY_EKB_KEY;
	return transceive_get_key_srv_cmd_msg(session, msg);
}

void get_key_srv_close(handle_t session)
{
	close(session);
}
