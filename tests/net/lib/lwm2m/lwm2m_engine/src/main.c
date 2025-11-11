/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>
#include <zephyr/net/socket.h>

#include "lwm2m_engine.h"
#include "lwm2m_rd_client.h"

#include "stubs.h"
#if defined(CONFIG_NATIVE_SIM_SLOWDOWN_TO_REAL_TIME)
#include "nsi_timer_model.h"
#endif

#define LOG_LEVEL	LOG_LEVEL_DBG
LOG_MODULE_REGISTER(lwm2m_engine_test);

DEFINE_FFF_GLOBALS;
#define FFF_FAKES_LIST(FAKE)

static uint8_t my_buf[256];
static uint16_t my_data_len = 1;
static struct lwm2m_message my_msg;
static struct lwm2m_engine_obj_field my_obj_field;

static int lwm2m_get_res_buf_custom_fake(const struct lwm2m_obj_path *path, void **buffer_ptr,
					 uint16_t *buffer_len, uint16_t *data_len,
					 uint8_t *data_flags)
{
	if (buffer_ptr) {
		*buffer_ptr = my_buf;
	}
	if (buffer_len) {
		*buffer_len = sizeof(my_buf);
	}
	if (data_len) {
		*data_len = my_data_len;
	}

	return 0;
}

static struct lwm2m_message *find_msg_custom_fake(struct coap_pending *pending,
						  struct coap_reply *reply)
{
	return &my_msg;
}

static struct lwm2m_engine_obj_field *
lwm2m_get_engine_obj_field_custom_fake(struct lwm2m_engine_obj *obj, int res_id)
{
	return &my_obj_field;
}

static int lwm2m_get_bool_custom_fake(const struct lwm2m_obj_path *path, bool *value)
{
	*value = false;

	return 0;
}

static void test_service(struct k_work *work)
{
	k_sleep(K_MSEC(10));
}

static void setup(void *data)
{
#if defined(CONFIG_NATIVE_SIM_SLOWDOWN_TO_REAL_TIME)
	/* It is enough that some slow-down is happening on sleeps, it does not have to be
	 * real time
	 */
	hwtimer_set_rt_ratio(100.0);
#endif
	/* Register resets */
	DO_FOREACH_FAKE(RESET_FAKE);

	/* reset common FFF internal structures */
	FFF_RESET_HISTORY();

	clear_socket_events();
	lwm2m_get_res_buf_fake.custom_fake = lwm2m_get_res_buf_custom_fake;
	find_msg_fake.custom_fake = find_msg_custom_fake;
	lwm2m_get_engine_obj_field_fake.custom_fake = lwm2m_get_engine_obj_field_custom_fake;
	lwm2m_get_bool_fake.custom_fake = lwm2m_get_bool_custom_fake;
}

ZTEST_SUITE(lwm2m_engine, NULL, NULL, setup, NULL, NULL);

ZTEST(lwm2m_engine, test_start_stop)
{
	int ret;
	struct lwm2m_ctx ctx;
	char host_name[10] = "my_host";

	(void)memset(&ctx, 0x0, sizeof(ctx));

	ctx.remote_addr.sa_family = AF_INET;
	ctx.sock_fd = -1;
	ctx.load_credentials = NULL;
	ctx.desthostname = host_name;
	ctx.desthostnamelen = strlen(host_name);
	ctx.use_dtls = true;

	ret = lwm2m_engine_start(&ctx);
	zassert_equal(ret, 0);

	struct lwm2m_ctx **eng_ctx = lwm2m_sock_ctx();
	int nfds = lwm2m_sock_nfds();

	zassert_not_null(eng_ctx);
	zassert_true(nfds > 0);
	zassert_equal(eng_ctx[0], &ctx);

	/* wait for socket receive thread */
	k_sleep(K_MSEC(1000));
	ret = lwm2m_engine_stop(&ctx);
	zassert_equal(ret, 0);
}

ZTEST(lwm2m_engine, test_pause_resume)
{
	int ret;
	struct lwm2m_ctx ctx;

	(void)memset(&ctx, 0x0, sizeof(ctx));

	ctx.remote_addr.sa_family = AF_INET;
	ctx.sock_fd = -1;
	ctx.load_credentials = NULL;

	ret = lwm2m_engine_start(&ctx);
	zassert_equal(ret, 0);
	ret = lwm2m_engine_resume();
	zassert_equal(ret, -EPERM);
	ret = lwm2m_engine_pause();
	zassert_equal(ret, 0);
	ret = lwm2m_engine_pause();
	zassert_equal(ret, 0);
	ret = lwm2m_engine_resume();
	zassert_equal(ret, 0);
	ret = lwm2m_engine_stop(&ctx);
	zassert_equal(ret, 0);
}

ZTEST(lwm2m_engine, test_engine_add_service)
{
	int ret;
	struct lwm2m_ctx ctx;

	(void)memset(&ctx, 0x0, sizeof(ctx));

	ctx.remote_addr.sa_family = AF_INET;
	ctx.load_credentials = NULL;

	ret = lwm2m_engine_start(&ctx);
	zassert_equal(ret, 0);
	ret = lwm2m_engine_add_service(test_service, 1000);
	zassert_equal(ret, 0);
	/* wait for socket receive thread */
	k_sleep(K_MSEC(1500));
	ret = lwm2m_engine_update_service_period(test_service, 500);
	zassert_equal(ret, 0);
	ret = lwm2m_engine_stop(&ctx);
	zassert_equal(ret, 0);
}

ZTEST(lwm2m_engine, test_no_sa_family)
{
	int ret;
	struct lwm2m_ctx ctx;

	(void)memset(&ctx, 0x0, sizeof(ctx));

	ctx.sock_fd = -1;
	ctx.load_credentials = NULL;

	ret = lwm2m_engine_start(&ctx);
	zassert_equal(ret, -EPROTONOSUPPORT);
	lwm2m_engine_stop(&ctx);
}

ZTEST(lwm2m_engine, test_connect_fail)
{
	int ret;
	struct lwm2m_ctx ctx;

	(void)memset(&ctx, 0x0, sizeof(ctx));

	ctx.sock_fd = -1;
	ctx.load_credentials = NULL;
	ctx.remote_addr.sa_family = AF_INET;

	errno = ENETDOWN;
	z_impl_zsock_connect_fake.return_val = -1;
	ret = lwm2m_engine_start(&ctx);
	zassert_equal(ret, -ENETDOWN);
	lwm2m_engine_stop(&ctx);
}

ZTEST(lwm2m_engine, test_socket_suspend_resume_connection)
{
	int ret;
	struct lwm2m_ctx ctx;

	(void)memset(&ctx, 0x0, sizeof(ctx));

	ctx.sock_fd = -1;
	ctx.load_credentials = NULL;
	ctx.remote_addr.sa_family = AF_INET;

	ret = lwm2m_engine_start(&ctx);
	zassert_equal(ret, 0);
	ret = lwm2m_socket_suspend(&ctx);
	zassert_equal(ret, 0);
	zassert_equal(ctx.connection_suspended, true);
	ret = lwm2m_engine_connection_resume(&ctx);
	zassert_equal(ret, 0);
	zassert_equal(ctx.connection_suspended, false);
	lwm2m_engine_stop(&ctx);
}

ZTEST(lwm2m_engine, test_check_notifications)
{
	int ret;
	struct lwm2m_ctx ctx;
	struct observe_node obs;

	(void)memset(&ctx, 0x0, sizeof(ctx));

	ctx.sock_fd = -1;
	ctx.load_credentials = NULL;
	ctx.remote_addr.sa_family = AF_INET;
	sys_slist_init(&ctx.observer);

	obs.last_timestamp = k_uptime_get();
	obs.event_timestamp = k_uptime_get() + 1000U;
	obs.resource_update = false;
	obs.active_notify = NULL;

	sys_slist_append(&ctx.observer, &obs.node);

	lwm2m_rd_client_is_registred_fake.return_val = true;
	ret = lwm2m_engine_start(&ctx);
	zassert_equal(ret, 0);
	/* wait for socket receive thread */
	k_sleep(K_MSEC(2000));
	ret = lwm2m_engine_stop(&ctx);
	zassert_equal(ret, 0);
	zassert_equal(generate_notify_message_fake.call_count, 1, "Notify message not generated");
	zassert_equal(engine_observe_shedule_next_event_fake.call_count, 1,
		      "Next observe event not scheduled");
}

ZTEST(lwm2m_engine, test_push_queued_buffers)
{
	int ret;
	struct lwm2m_ctx ctx;
	struct lwm2m_message msg;
	struct coap_pending pending;

	(void)memset(&ctx, 0x0, sizeof(ctx));

	sys_slist_init(&ctx.queued_messages);
	msg.ctx = &ctx;
	msg.pending = &pending;
	sys_slist_append(&ctx.queued_messages, &msg.node);
	ret = lwm2m_push_queued_buffers(&ctx);
	zassert_equal(ret, 0);
}

ZTEST(lwm2m_engine, test_validate_write_access)
{
	int ret;
	struct lwm2m_ctx ctx;
	struct lwm2m_message msg;
	struct lwm2m_engine_res resources;
	struct lwm2m_engine_obj_inst obj_inst;
	struct lwm2m_engine_obj_field *obj_field = NULL;

	(void)memset(&ctx, 0x0, sizeof(ctx));

	ctx.bootstrap_mode = true;
	msg.ctx = &ctx;
	msg.path = LWM2M_OBJ(LWM2M_OBJECT_SECURITY_ID, 0);
	obj_inst.resources = &resources;
	obj_inst.resource_count = 1U;
	ret = lwm2m_engine_validate_write_access(&msg, &obj_inst, &obj_field);
	zassert_equal(ret, 0);

	obj_inst.resource_count = 0U;
	ret = lwm2m_engine_validate_write_access(&msg, &obj_inst, &obj_field);
	zassert_equal(ret, -EINVAL);

	msg.path = LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0);
	ret = lwm2m_engine_validate_write_access(&msg, &obj_inst, &obj_field);
	zassert_equal(ret, -EPERM);
}

ZTEST(lwm2m_engine, test_bootstrap_delete)
{
	int ret;
	struct lwm2m_message msg;

	msg.path = LWM2M_OBJ(LWM2M_OBJECT_SECURITY_ID, 0, 0);
	ret = bootstrap_delete(&msg);
	zassert_equal(ret, -EPERM);

	msg.path = LWM2M_OBJ(LWM2M_OBJECT_SECURITY_ID, 1);
	ret = bootstrap_delete(&msg);
	zassert_equal(ret, 0);
	zassert_equal(0, lwm2m_delete_obj_inst_fake.arg0_history[0]);
	zassert_equal(1, lwm2m_delete_obj_inst_fake.arg1_history[0]);


	struct lwm2m_engine_obj sec_obj = {.obj_id = 0};
	struct lwm2m_engine_obj_inst sec_inst = {
		.obj_inst_id = 2,
		.obj = &sec_obj
	};
	sys_slist_append(lwm2m_engine_obj_inst_list(), &sec_inst.node);

	msg.path = LWM2M_OBJ(LWM2M_OBJECT_SECURITY_ID);
	ret = bootstrap_delete(&msg);
	zassert_equal(ret, 0);
	zassert_equal(0, lwm2m_delete_obj_inst_fake.arg0_history[1]);
	zassert_equal(2, lwm2m_delete_obj_inst_fake.arg1_history[1]);

	msg.path = LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0);
	ret = bootstrap_delete(&msg);
	zassert_equal(ret, -EPERM);
}

ZTEST(lwm2m_engine, test_retransmit_request)
{
	int ret;
	struct lwm2m_ctx ctx;
	struct coap_pending pending_1;
	struct coap_pending pending_2;

	(void)memset(&ctx, 0x0, sizeof(ctx));

	ctx.sock_fd = -1;
	ctx.load_credentials = NULL;
	ctx.remote_addr.sa_family = AF_INET;

	pending_1.t0 = k_uptime_get();
	pending_1.timeout = 200U;
	pending_1.retries = 0;
	ctx.pendings[0] = pending_1;

	pending_2.t0 = k_uptime_get();
	pending_2.timeout = 200U;
	pending_2.retries = 1;
	ctx.pendings[1] = pending_2;

	ret = lwm2m_engine_start(&ctx);
	zassert_equal(ret, 0);
	/* wait for socket receive thread */
	k_sleep(K_MSEC(500));
	ret = lwm2m_engine_stop(&ctx);
	zassert_equal(ret, 0);
	zassert_not_equal(lwm2m_reset_message_fake.call_count, 0, "Message was not reseted");
	zassert_not_equal(lwm2m_send_message_async_fake.call_count, 0, "Message was not sent");
}

ZTEST(lwm2m_engine, test_socket_recv)
{
	int ret;
	struct lwm2m_ctx ctx;

	(void)memset(&ctx, 0x0, sizeof(ctx));

	ctx.remote_addr.sa_family = AF_INET;
	ctx.sock_fd = -1;

	set_socket_events(ZSOCK_POLLIN);

	ret = lwm2m_engine_start(&ctx);
	zassert_equal(ret, 0);
	/* wait for socket receive thread */
	k_sleep(K_MSEC(1000));
	ret = lwm2m_engine_stop(&ctx);
	zassert_equal(ret, 0);
	zassert_true(lwm2m_udp_receive_fake.call_count > 0);
}

ZTEST(lwm2m_engine, test_socket_send)
{
	int ret;
	struct lwm2m_ctx ctx;
	struct lwm2m_message msg;
	struct coap_pending pending;

	(void)memset(&ctx, 0x0, sizeof(ctx));

	ctx.remote_addr.sa_family = AF_INET;
	ctx.sock_fd = -1;
	sys_slist_init(&ctx.queued_messages);
	msg.ctx = &ctx;
	msg.pending = &pending;
	msg.type = COAP_TYPE_CON;
	sys_slist_append(&ctx.queued_messages, &msg.node);

	ret = lwm2m_push_queued_buffers(&ctx);
	zassert_equal(ret, 0);

	set_socket_events(ZSOCK_POLLOUT);

	ret = lwm2m_engine_start(&ctx);
	zassert_equal(ret, 0);
	/* wait for socket receive thread */
	k_sleep(K_MSEC(2000));
	ret = lwm2m_engine_stop(&ctx);
	zassert_equal(ret, 0);
	LOG_INF("Count %d", coap_pending_cycle_fake.call_count);
	zassert_equal(coap_pending_cycle_fake.call_count, 1, "coap_pending_cycle not called");
}

ZTEST(lwm2m_engine, test_security)
{
	struct lwm2m_ctx ctx;
	char host_name[10] = "my_host";

	(void)memset(&ctx, 0x0, sizeof(ctx));
	my_data_len = snprintk(my_buf, sizeof(my_buf), "-----BEGIN SOMETHING");

	ctx.remote_addr.sa_family = AF_INET;
	ctx.sock_fd = -1;
	ctx.load_credentials = NULL;
	ctx.desthostname = host_name;
	ctx.desthostnamelen = strlen(host_name);
	ctx.use_dtls = false;

	lwm2m_security_mode_fake.return_val = LWM2M_SECURITY_NOSEC;

	zassert_equal(lwm2m_engine_start(&ctx), 0);
	zassert_equal(lwm2m_engine_stop(&ctx), 0);

	ctx.use_dtls = true;
	zassert_equal(lwm2m_engine_start(&ctx), -EINVAL);
	zassert_equal(lwm2m_engine_stop(&ctx), 0);

	RESET_FAKE(z_impl_zsock_setsockopt);
	lwm2m_security_mode_fake.return_val = LWM2M_SECURITY_PSK;
	zassert_equal(lwm2m_engine_start(&ctx), 0);
	zassert_equal(z_impl_zsock_setsockopt_fake.arg2_history[0], TLS_SEC_TAG_LIST);
	zassert_equal(z_impl_zsock_setsockopt_fake.arg2_history[1], TLS_PEER_VERIFY);
	zassert_equal(z_impl_zsock_setsockopt_fake.arg2_history[2], TLS_CIPHERSUITE_LIST);
	zassert_true(tls_credential_delete_fake.call_count > 3);
	zassert_true(tls_credential_add_fake.call_count == 2);
	zassert_equal(tls_credential_add_fake.arg1_history[0], TLS_CREDENTIAL_PSK_ID);
	zassert_equal(tls_credential_add_fake.arg1_history[1], TLS_CREDENTIAL_PSK);
	zassert_equal(lwm2m_engine_stop(&ctx), 0);

	RESET_FAKE(z_impl_zsock_setsockopt);
	RESET_FAKE(tls_credential_add);
	lwm2m_security_mode_fake.return_val = LWM2M_SECURITY_CERT;
	ctx.desthostname = NULL;
	zassert_equal(lwm2m_engine_start(&ctx), 0);
	zassert_equal(z_impl_zsock_setsockopt_fake.arg2_history[0], TLS_SEC_TAG_LIST);
	zassert_equal(z_impl_zsock_setsockopt_fake.arg2_history[1], TLS_PEER_VERIFY);
	zassert_equal(z_impl_zsock_setsockopt_fake.arg2_history[2], TLS_CIPHERSUITE_LIST);
	zassert_true(tls_credential_add_fake.call_count == 3);
	zassert_equal(tls_credential_add_fake.arg1_history[0], TLS_CREDENTIAL_PUBLIC_CERTIFICATE);
	zassert_equal(tls_credential_add_fake.arg1_history[1], TLS_CREDENTIAL_PRIVATE_KEY);
	zassert_equal(tls_credential_add_fake.arg1_history[2], TLS_CREDENTIAL_CA_CERTIFICATE);
	zassert_equal(lwm2m_engine_stop(&ctx), 0);
}

static enum lwm2m_socket_states last_state;

static void socket_state(int fd, enum lwm2m_socket_states state)
{
	(void) fd;
	last_state = state;
}

ZTEST(lwm2m_engine, test_socket_state)
{
	int ret;
	struct lwm2m_ctx ctx = {
		.remote_addr.sa_family = AF_INET,
		.sock_fd = -1,
		.set_socket_state = socket_state,
	};
	struct lwm2m_message msg1 = {
		.ctx = &ctx,
		.type = COAP_TYPE_CON,
	};
	struct lwm2m_message msg2 = msg1;
	struct lwm2m_message ack = {
		.ctx = &ctx,
		.type = COAP_TYPE_ACK,
	};

	sys_slist_init(&ctx.pending_sends);
	ret = lwm2m_engine_start(&ctx);
	zassert_equal(ret, 0);

	/* One confimable in queue, should cause ONE_RESPONSE status */
	coap_pendings_count_fake.return_val = 1;
	sys_slist_append(&ctx.pending_sends, &msg1.node);
	set_socket_events(ZSOCK_POLLOUT);
	k_sleep(K_MSEC(100));
	zassert_equal(last_state, LWM2M_SOCKET_STATE_ONE_RESPONSE);

	/* More than one messages in queue, not empty, should cause ONGOING */
	coap_pendings_count_fake.return_val = 2;
	sys_slist_append(&ctx.pending_sends, &msg1.node);
	sys_slist_append(&ctx.pending_sends, &msg2.node);
	set_socket_events(ZSOCK_POLLOUT);
	k_sleep(K_MSEC(100));
	zassert_equal(last_state, LWM2M_SOCKET_STATE_ONGOING);

	/* Last out, while waiting for ACK to both, should still cause ONGOING */
	coap_pendings_count_fake.return_val = 2;
	set_socket_events(ZSOCK_POLLOUT);
	k_sleep(K_MSEC(100));
	zassert_equal(last_state, LWM2M_SOCKET_STATE_ONGOING);

	/* Only one Ack transmiting, nothing expected back -> LAST */
	coap_pendings_count_fake.return_val = 0;
	sys_slist_append(&ctx.pending_sends, &ack.node);
	set_socket_events(ZSOCK_POLLOUT);
	k_sleep(K_MSEC(100));
	zassert_equal(last_state, LWM2M_SOCKET_STATE_LAST);

	/* Socket suspended (as in QUEUE_RX_OFF), should cause NO_DATA */
	ret = lwm2m_socket_suspend(&ctx);
	zassert_equal(ret, 0);
	zassert_equal(last_state, LWM2M_SOCKET_STATE_NO_DATA);

	ret = lwm2m_engine_stop(&ctx);
	zassert_equal(ret, 0);
}
