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
#include <ipc.h>
#include <key_mgnt.h>
#include <stdio.h>
#include <trusty_std.h>

/*
 * @brief hwkey-agent TA
 *
 * @return NO_ERROR if successful
 */
int main(void)
{
	int rc = NO_ERROR;
	uevent_t event;

	TLOGI("hwkey-agent is running!!\n");

	rc = key_mgnt_processing();
	if (rc != NO_ERROR) {
		TLOGE("%s: Failed to verify or extract EKB (%d).\n", __func__, rc);
		return rc;
	}

	/* Initialize IPC service */
	rc = init_hwkey_agent_srv();
	if (rc != NO_ERROR ) {
		TLOGI("Failed (%d) to init IPC service", rc);
		kill_hwkey_agent_srv();
		return -1;
	}

	/* Handle IPC service events */
	do {
		event.handle = INVALID_IPC_HANDLE;
		event.event  = 0;
		event.cookie = NULL;

		rc = wait_any(&event, -1);
		if (rc < 0) {
			TLOGI("wait_any failed (%d)", rc);
			continue;
		}

		if (rc == NO_ERROR) { /* got an event */
			dispatch_hwkey_agent_srv_event(&event);
		}
	} while(1);

	return rc;
}
