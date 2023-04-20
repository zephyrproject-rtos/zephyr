/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <stubs.h>

LOG_MODULE_DECLARE(lwm2m_engine_test);

DEFINE_FAKE_VALUE_FUNC(int, lwm2m_rd_client_pause);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_rd_client_resume);
DEFINE_FAKE_VALUE_FUNC(struct lwm2m_message *, find_msg, struct coap_pending *,
		       struct coap_reply *);
DEFINE_FAKE_VOID_FUNC(coap_pending_clear, struct coap_pending *);
DEFINE_FAKE_VOID_FUNC(lwm2m_reset_message, struct lwm2m_message *, bool);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_send_message_async, struct lwm2m_message *);
DEFINE_FAKE_VOID_FUNC(lwm2m_registry_lock);
DEFINE_FAKE_VOID_FUNC(lwm2m_registry_unlock);
DEFINE_FAKE_VALUE_FUNC(bool, coap_pending_cycle, struct coap_pending *);
DEFINE_FAKE_VALUE_FUNC(int, generate_notify_message, struct lwm2m_ctx *, struct observe_node *,
		       void *);
DEFINE_FAKE_VALUE_FUNC(int64_t, engine_observe_shedule_next_event, struct observe_node *, uint16_t,
		       const int64_t);
DEFINE_FAKE_VALUE_FUNC(int, handle_request, struct coap_packet *, struct lwm2m_message *);
DEFINE_FAKE_VOID_FUNC(lwm2m_udp_receive, struct lwm2m_ctx *, uint8_t *, uint16_t, struct sockaddr *,
		      udp_request_handler_cb_t);
DEFINE_FAKE_VALUE_FUNC(bool, lwm2m_rd_client_is_registred, struct lwm2m_ctx *);
DEFINE_FAKE_VOID_FUNC(lwm2m_engine_context_close, struct lwm2m_ctx *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_get_res_buf, const struct lwm2m_obj_path *, void **, uint16_t *,
		       uint16_t *, uint8_t *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_parse_peerinfo, char *, struct lwm2m_ctx *, bool);
DEFINE_FAKE_VALUE_FUNC(int, tls_credential_add, sec_tag_t, enum tls_credential_type, const void *,
		       size_t);
DEFINE_FAKE_VALUE_FUNC(int, tls_credential_delete, sec_tag_t, enum tls_credential_type);
DEFINE_FAKE_VALUE_FUNC(struct lwm2m_engine_obj_field *, lwm2m_get_engine_obj_field,
		       struct lwm2m_engine_obj *, int);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_get_bool, const struct lwm2m_obj_path *, bool *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_delete_obj_inst, uint16_t, uint16_t);
DEFINE_FAKE_VOID_FUNC(lwm2m_clear_block_contexts);

static sys_slist_t obs_obj_path_list = SYS_SLIST_STATIC_INIT(&obs_obj_path_list);
sys_slist_t *lwm2m_obs_obj_path_list(void)
{
	return &obs_obj_path_list;
}

static sys_slist_t engine_obj_inst_list = SYS_SLIST_STATIC_INIT(&engine_obj_inst_list);
sys_slist_t *lwm2m_engine_obj_inst_list(void) { return &engine_obj_inst_list; }

struct zsock_pollfd {
	int fd;
	short events;
	short revents;
};

static short my_events;

void set_socket_events(short events)
{
	my_events |= events;
}

void clear_socket_events(void)
{
	my_events = 0;
}

int z_impl_zsock_socket(int family, int type, int proto)
{
	return 0;
}

int z_impl_zsock_close(int sock)
{
	return 0;
}

DEFINE_FAKE_VALUE_FUNC(int, z_impl_zsock_connect, int, const struct sockaddr *, socklen_t);

ssize_t z_impl_zsock_sendto(int sock, const void *buf, size_t len, int flags,
			    const struct sockaddr *dest_addr, socklen_t addrlen)
{
	k_sleep(K_MSEC(1));
	if (my_events & ZSOCK_POLLOUT) {
		my_events = 0;
	}
	return 1;
}

ssize_t z_impl_zsock_recvfrom(int sock, void *buf, size_t max_len, int flags,
			      struct sockaddr *src_addr, socklen_t *addrlen)
{
	k_sleep(K_MSEC(1));
	if (my_events & ZSOCK_POLLIN) {
		my_events = 0;
		return 1;
	}
	errno = EWOULDBLOCK;
	return -1;
}

int z_impl_zsock_poll(struct zsock_pollfd *fds, int nfds, int poll_timeout)
{
	k_sleep(K_MSEC(poll_timeout));
	fds->revents = my_events;
	return 0;
}

int z_impl_zsock_setsockopt(int sock, int level, int optname, const void *optval, socklen_t optlen)
{
	return 0;
}

int z_impl_zsock_fcntl(int sock, int cmd, int flags)
{
	return 0;
}
