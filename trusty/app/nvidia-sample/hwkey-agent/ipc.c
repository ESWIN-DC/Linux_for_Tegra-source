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

#include <assert.h>
#include <common.h>
#include <crypto_service.h>
#include <err.h>
#include <get_key_srv.h>
#include <stdio.h>
#include <stdlib.h>
#include <trusty_std.h>

#define MAX_PORT_BUF_SIZE	4096	/* max size of per port buffer */

#define SRV_PATH_BASE	"hwkey-agent"
#define SRV_NAME(name)	SRV_PATH_BASE ".srv." name

typedef void (*event_handler_proc_t) (const uevent_t *ev);

typedef struct tipc_event_handler {
	event_handler_proc_t proc;
	void *priv;

} tipc_event_handler_t;

typedef struct tipc_srv {
	const char *name;
	uint   msg_num;
	size_t msg_size;
	uint   port_flags;
	event_handler_proc_t port_handler;
	event_handler_proc_t chan_handler;
} tipc_srv_t;

typedef struct tipc_srv_state {
	const struct tipc_srv *service;
	handle_t port;
	void *priv;
	tipc_event_handler_t handler;
} tipc_srv_state_t;

typedef struct chan_state {
	struct tipc_event_handler handler;
} chan_state_t;

static void common_port_handler(const uevent_t *evt);
static void crypto_srv_chan_handler(const uevent_t *evt);
static void get_key_srv_chan_handler(const uevent_t *evt);

static const struct tipc_srv _services[] =
{
	{
		.name = SRV_NAME("crypto-srv"),
		.msg_num = 1,
		.msg_size = MAX_PORT_BUF_SIZE,
		.port_flags = IPC_PORT_ALLOW_NS_CONNECT,
		.port_handler = common_port_handler,
		.chan_handler = crypto_srv_chan_handler,
	},
	{
		.name = SRV_NAME("get-key-srv"),
		.msg_num = 1,
		.msg_size = MAX_PORT_BUF_SIZE,
		.port_flags = IPC_PORT_ALLOW_TA_CONNECT,
		.port_handler = common_port_handler,
		.chan_handler = get_key_srv_chan_handler,
	},
};

static struct tipc_srv_state _srv_states[countof(_services)] = {
	[0 ... (countof(_services) - 1)] = {
		.port = INVALID_IPC_HANDLE,
	},
};

static struct tipc_srv_state *get_srv_state(const uevent_t *ev)
{
	return containerof(ev->cookie, struct tipc_srv_state, handler);
}

static void _destroy_service(struct tipc_srv_state *state)
{
	if (!state) {
		TLOGI("non-null state expected\n");
		return;
	}

	/* Close port */
	if (state->port != INVALID_IPC_HANDLE) {
		int rc = close(state->port);
		if (rc != NO_ERROR) {
			TLOGI("Failed (%d) to close port %d\n",
			      rc, state->port);
		}
		state->port = INVALID_IPC_HANDLE;
	}

	/* Reset handler */
	state->service = NULL;
	state->handler.proc = NULL;
	state->handler.priv = NULL;
	state->priv = NULL;
}

/*
 *  Create service
 */
static int _create_service(const struct tipc_srv *srv,
                           struct tipc_srv_state *state)
{
	if (!srv || !state) {
		TLOGI("null services specified\n");
		return ERR_INVALID_ARGS;
	}

	/* Create port */
	int rc = port_create(srv->name, srv->msg_num, srv->msg_size,
			     srv->port_flags);
	if (rc < 0) {
		TLOGI("Failed (%d) to create port\n", rc);
		return rc;
	}

	/* Setup port state  */
	state->port = (handle_t)rc;
	state->handler.proc = srv->port_handler;
	state->handler.priv = state;
	state->service = srv;
	state->priv = NULL;

	/* Attach handler to port handle */
	rc = set_cookie(state->port, &state->handler);
	if (rc < 0) {
		TLOGI("Failed (%d) to set cookie on port %d\n",
		      rc, state->port);
		goto err_set_cookie;
	}

	return NO_ERROR;

err_set_cookie:
	_destroy_service(state);
	return rc;
}

int init_hwkey_agent_srv(void)
{
	TLOGI ("Init hweky-agent services!!\n");

	for (uint i = 0; i < countof(_services); i++) {
		int rc = _create_service(&_services[i], &_srv_states[i]);
		if (rc < 0) {
			TLOGI("Failed (%d) to create service %s\n",
			      rc, _services[i].name);
			return rc;
		}
	}

	return 0;
}

void kill_hwkey_agent_srv(void)
{
	TLOGI ("Terminating hwkey-agent services.\n");

	/* Close any opened ports */
	for (uint i = 0; i < countof(_services); i++) {
		_destroy_service(&_srv_states[i]);
	}
}

static int wait_to_send(handle_t session, struct ipc_msg *msg) {
	int rc;
	struct uevent evt = UEVENT_INITIAL_VALUE(evt);

	rc = wait(session, &evt, INFINITE_TIME);
	if (rc < 0) {
		TLOGE("failed to wait for outgoing queue to free up\n");
		return rc;
	}

	if (evt.event & IPC_HANDLE_POLL_SEND_UNBLOCKED)
		return send_msg(session, msg);

	if (evt.event & IPC_HANDLE_POLL_MSG)
		return ERR_BUSY;

	if (evt.event & IPC_HANDLE_POLL_HUP)
		return ERR_CHANNEL_CLOSED;

	return rc;
}

static int crypto_srv_handle_msg(const uevent_t *evt)
{
	int rc;
	ipc_msg_t msg;
	ipc_msg_info_t msg_info;
	uint8_t* _msg_buf = NULL;
	crypto_srv_msg_t cmd;

	rc = get_msg(evt->handle, &msg_info);
	if (rc == ERR_NO_MSG)
		return rc; /* no new messages */

	if (rc != NO_ERROR) {
		TLOGI("failed (%d) to get_msg for chan (%d)\n",
		      rc, evt->handle);
		return rc;
	}

	_msg_buf = malloc(CRYPTO_SRV_PAYLOAD_SIZE);
	if (_msg_buf == NULL) {
		TLOGI("chan (%d) failed: out of memory\n",
		      evt->handle);
		rc = ERR_NO_MEMORY;
		return rc;
	}

	iovec_t iov_base = {
		.base = _msg_buf,
		.len = CRYPTO_SRV_PAYLOAD_SIZE
	};

	iovec_t iov[2] = {
		{ .base = &cmd, .len = sizeof(cmd) },
		iov_base
	};

	/*
	 * Handle all messages in queue
	 * init message structure
	 */
	msg.num_iov = 2;
	msg.iov     = iov;
	msg.num_handles = 0;
	msg.handles  = NULL;

	/* Read msg content */
	rc = read_msg(evt->handle, msg_info.id, 0, &msg);
	if (rc < 0) {
		free(_msg_buf);
		TLOGI("failed (%d) to read_msg for chan (%d)\n",
		      rc, evt->handle);
		return rc;
	}

	crypto_srv_process_req(msg.iov, rc);

	/* And send it back */
	rc = send_msg(evt->handle, &msg);
	if (rc == ERR_NOT_ENOUGH_BUFFER)
		rc = wait_to_send(evt->handle, &msg);

	if (rc < 0) {
		free(_msg_buf);
		TLOGI("failed (%d) to send_msg for chan (%d)\n",
		      rc, evt->handle);
		return rc;
	}

	/* Retire original message */
	rc = put_msg(evt->handle, msg_info.id);
	if (rc != NO_ERROR) {
		free(_msg_buf);
		TLOGI("failed (%d) to put_msg for chan (%d)\n",
		      rc, evt->handle);
		return rc;
	}

	free(_msg_buf);

	return NO_ERROR;
}

static int get_key_srv_handle_msg(const uevent_t *evt)
{
	int rc;
	ipc_msg_t msg;
	ipc_msg_info_t msg_info;
	get_key_srv_cmd_msg_t cmd;

	rc = get_msg(evt->handle, &msg_info);
	if (rc == ERR_NO_MSG)
		return rc; /* no new messages */

	if (rc != NO_ERROR) {
		TLOGI("failed (%d) to get_msg for chan (%d)\n",
		      rc, evt->handle);
		return rc;
	}

	iovec_t iov = {
		.base = &cmd,
		.len = sizeof(cmd),
	};

	/*
	 * Handle all messages in queue
	 * init message structure
	 */
	msg.num_iov = 1;
	msg.iov = &iov;
	msg.num_handles = 0;
	msg.handles  = NULL;

	/* Read msg content */
	rc = read_msg(evt->handle, msg_info.id, 0, &msg);
	if (rc < 0) {
		TLOGI("failed (%d) to read_msg for chan (%d)\n",
		      rc, evt->handle);
		return rc;
	}

	get_key_srv_process_req(msg.iov);

	/* And send it back */
	rc = send_msg(evt->handle, &msg);
	if (rc == ERR_NOT_ENOUGH_BUFFER)
		rc = wait_to_send(evt->handle, &msg);

	if (rc < 0) {
		TLOGI("failed (%d) to send_msg for chan (%d)\n",
		      rc, evt->handle);
		return rc;
	}

	/* Retire original message */
	rc = put_msg(evt->handle, msg_info.id);
	if (rc != NO_ERROR) {
		TLOGI("failed (%d) to put_msg for chan (%d)\n",
		      rc, evt->handle);
		return rc;
	}

	return NO_ERROR;
}

/*
 *  service channel handler
 */
static void crypto_srv_chan_handler(const uevent_t *evt)
{
	struct chan_state *chan_st;

	if ((evt->event & IPC_HANDLE_POLL_ERROR)
	    || (evt->event & IPC_HANDLE_POLL_HUP))
		goto close_it;

	if (evt->event & (IPC_HANDLE_POLL_MSG)) {
		if (crypto_srv_handle_msg(evt) != 0) {
			TLOGI("error event (0x%x) for chan (%d)\n",
			      evt->event, evt->handle);
			goto close_it;
		}
	}

	return;

close_it:
	chan_st = containerof(evt->cookie, struct chan_state, handler);
	free(chan_st);
	close(evt->handle);
}

static void get_key_srv_chan_handler(const uevent_t *evt)
{
	struct chan_state *chan_st;

	if ((evt->event & IPC_HANDLE_POLL_ERROR)
	    || (evt->event & IPC_HANDLE_POLL_HUP))
		goto close_it;

	if (evt->event & (IPC_HANDLE_POLL_MSG)) {
		if (get_key_srv_handle_msg(evt) != 0) {
			TLOGI("error event (0x%x) for chan (%d)\n",
			      evt->event, evt->handle);
			goto close_it;
		}
	}

	return;

close_it:
	chan_st = containerof(evt->cookie, struct chan_state, handler);
	free(chan_st);
	close(evt->handle);
}

static int restart_service(struct tipc_srv_state *state)
{
	if (!state) {
		TLOGI("non-null state expected\n");
		return ERR_INVALID_ARGS;
	}

	const struct tipc_srv *srv = state->service;
	_destroy_service(state);

	return _create_service(srv, state);
}

/*
 *  Handle common port errors
 */
static bool handle_port_errors(const uevent_t *evt)
{
	if ((evt->event & IPC_HANDLE_POLL_ERROR) ||
	    (evt->event & IPC_HANDLE_POLL_HUP) ||
	    (evt->event & IPC_HANDLE_POLL_MSG) ||
	    (evt->event & IPC_HANDLE_POLL_SEND_UNBLOCKED)) {
		/* Should never happen with port handles */
		TLOGI("error event (0x%x) for port (%d)\n",
		      evt->event, evt->handle);

		/* Recreate service */
		restart_service(get_srv_state(evt));
		return true;
	}

	return false;
}

/*
 *   service port event handler
 */
static void common_port_handler(const uevent_t *evt)
{
	uuid_t peer_uuid;
	struct chan_state *chan_st;
	const struct tipc_srv *srv = get_srv_state(evt)->service;

	if (handle_port_errors(evt))
		return;

	if (evt->event & IPC_HANDLE_POLL_READY) {
		handle_t chan;

		/* Incomming connection: accept it */
		int rc = accept(evt->handle, &peer_uuid);
		if (rc < 0) {
			TLOGI("failed (%d) to accept on port %d\n",
			      rc, evt->handle);
			return;
		}
		chan = (handle_t)rc;

		chan_st = malloc(sizeof(struct chan_state));
		if (!chan_st) {
			TLOGI("failed (%d) to callocate state for chan %d\n",
			      rc, chan);
			close(chan);
			return;
		}

		/* Init state */
		chan_st->handler.proc = srv->chan_handler;
		chan_st->handler.priv = chan_st;

		/* Attach it to handle */
		rc = set_cookie(chan, &chan_st->handler);
		if (rc) {
			TLOGI("failed (%d) to set_cookie on chan %d\n",
			      rc, chan);
			free(chan_st);
			close(chan);
			return;
		}
	}
}

void dispatch_hwkey_agent_srv_event(const uevent_t *evt)
{
	assert(evt);

	if (evt->event == IPC_HANDLE_POLL_NONE) {
		/* Not really an event, do nothing */
		TLOGI("got an empty event\n");
		return;
	}

	if (evt->handle == INVALID_IPC_HANDLE) {
		/* Not a valid handle  */
		TLOGI("got an event (0x%x) with invalid handle (%d)",
		      evt->event, evt->handle);
		return;
	}

	/* Check if we have handler */
	struct tipc_event_handler *handler = evt->cookie;
	if (handler && handler->proc) {
		/* Invoke it */
		handler->proc(evt);
		return;
	}

	/* No handler? close it */
	TLOGI("no handler for event (0x%x) with handle %d\n",
	      evt->event, evt->handle);
	close(evt->handle);

	return;
}
