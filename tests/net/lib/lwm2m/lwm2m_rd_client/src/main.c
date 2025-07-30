/*
 * Copyright (c) 2022 grandcentrix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stubs.h"

#include <zephyr/fff.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>
#if defined(CONFIG_NATIVE_SIM_SLOWDOWN_TO_REAL_TIME)
#include "nsi_timer_model.h"
#endif
#include <lwm2m_rd_client.h>

LOG_MODULE_REGISTER(lwm2m_rd_client_test);

DEFINE_FFF_GLOBALS;

/* Maximum number of iterations within the state machine of RD Client
 * service that is waited for until a possible event occurs
 */
#define RD_CLIENT_MAX_LOOKUP_ITERATIONS 500

FAKE_VOID_FUNC(show_lwm2m_event, enum lwm2m_rd_client_event);
FAKE_VOID_FUNC(show_lwm2m_observe, enum lwm2m_observe_event);

static int next_event;
static struct lwm2m_ctx ctx;

bool expect_lwm2m_rd_client_event(uint8_t expected_val)
{
	int max_service_iterations = RD_CLIENT_MAX_LOOKUP_ITERATIONS;
	bool match = false;

	while (max_service_iterations > 0) {
		for (int i = next_event; i < show_lwm2m_event_fake.call_count; i++) {
			match = show_lwm2m_event_fake.arg0_history[i] == expected_val;
			if (match) {
				next_event = i + 1;
				break;
			}
		}

		if (match) {
			break;
		}
		wait_for_service(1);
		max_service_iterations--;
	}

	if (!match) {
		LOG_INF("Expecting event  %d, events:", expected_val);
		for (int i = next_event; i < show_lwm2m_event_fake.call_count; i++) {
			LOG_INF("[%d] = %d", i, show_lwm2m_event_fake.arg0_history[i]);
		}
	}

	return match;
}

static void lwm2m_event_cb(struct lwm2m_ctx *client, enum lwm2m_rd_client_event client_event)
{
	ARG_UNUSED(client);

	switch (client_event) {
	case LWM2M_RD_CLIENT_EVENT_NONE:
		LOG_INF("*** LWM2M_RD_CLIENT_EVENT_NONE");
		break;
	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE:
		LOG_INF("*** LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE");
		break;
	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_COMPLETE:
		LOG_INF("*** LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_COMPLETE");
		break;
	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_TRANSFER_COMPLETE:
		LOG_INF("*** LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_TRANSFER_COMPLETE");
		break;
	case LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE:
		LOG_INF("*** LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE");
		break;
	case LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE:
		LOG_INF("*** LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE");
		break;
	case LWM2M_RD_CLIENT_EVENT_REG_TIMEOUT:
		LOG_INF("*** LWM2M_RD_CLIENT_EVENT_REG_TIMEOUT");
		break;
	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE:
		LOG_INF("*** LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE");
		break;
	case LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE:
		LOG_INF("*** LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE");
		break;
	case LWM2M_RD_CLIENT_EVENT_DISCONNECT:
		LOG_INF("*** LWM2M_RD_CLIENT_EVENT_DISCONNECT");
		break;
	case LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF:
		LOG_INF("*** LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF");
		break;
	case LWM2M_RD_CLIENT_EVENT_SERVER_DISABLED:
		LOG_INF("*** LWM2M_RD_CLIENT_EVENT_SERVER_DISABLED");
		break;
	case LWM2M_RD_CLIENT_EVENT_ENGINE_SUSPENDED:
		LOG_INF("*** LWM2M_RD_CLIENT_EVENT_ENGINE_SUSPENDED");
		break;
	case LWM2M_RD_CLIENT_EVENT_NETWORK_ERROR:
		LOG_INF("*** LWM2M_RD_CLIENT_EVENT_NETWORK_ERROR");
		break;
	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE:
		LOG_INF("*** LWM2M_RD_CLIENT_EVENT_REG_UPDATE");
		break;
	case LWM2M_RD_CLIENT_EVENT_DEREGISTER:
		LOG_INF("*** LWM2M_RD_CLIENT_EVENT_DEREGISTER");
		break;
	}

	show_lwm2m_event(client_event);
}

static void lwm2m_observe_cb(enum lwm2m_observe_event event, struct lwm2m_obj_path *path,
			     void *user_data)
{
	switch (event) {
	case LWM2M_OBSERVE_EVENT_OBSERVER_ADDED:
		LOG_INF("**** LWM2M_OBSERVE_EVENT_OBSERVER_ADDED");
		break;
	case LWM2M_OBSERVE_EVENT_NOTIFY_TIMEOUT:
		LOG_INF("**** LWM2M_OBSERVE_EVENT_NOTIFY_TIMEOUT");
		break;
	case LWM2M_OBSERVE_EVENT_OBSERVER_REMOVED:
		LOG_INF("**** LWM2M_OBSERVE_EVENT_OBSERVER_REMOVED");
		break;
	case LWM2M_OBSERVE_EVENT_NOTIFY_ACK:
		LOG_INF("**** LWM2M_OBSERVE_EVENT_NOTIFY_ACK");
		break;
	default:
		break;
	}

	show_lwm2m_observe(event);
}

#define FFF_FAKES_LIST(FAKE)

static void my_suite_before(void *data)
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

	RESET_FAKE(show_lwm2m_event);
	RESET_FAKE(show_lwm2m_observe);
	next_event = 0;

	/* Common stubs for all tests */
	get_u32_val = CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME;
	lwm2m_get_u32_fake.custom_fake = lwm2m_get_u32_val;
	lwm2m_get_bool_fake.custom_fake = lwm2m_get_bool_fake_default;
	lwm2m_sprint_ip_addr_fake.custom_fake = lwm2m_sprint_ip_addr_fake_default;
	lwm2m_init_message_fake.custom_fake = lwm2m_init_message_fake_default;
	coap_header_get_code_fake.custom_fake = coap_header_get_code_fake_created;
	coap_packet_append_option_fake.custom_fake = NULL;
	stub_lwm2m_server_disable(false);
}

static void my_suite_after(void *data)
{
	test_lwm2m_engine_stop_service();
}

void message_reply_cb_default(struct lwm2m_message *msg)
{
	struct coap_packet response;
	struct coap_reply reply;
	struct sockaddr from;

	memset(&response, 0, sizeof(struct coap_packet));
	memset(&reply, 0, sizeof(struct coap_reply));
	memset(&from, 0, sizeof(struct sockaddr));

	msg->reply_cb(&response, &reply, &from);
}

void message_reply_timeout_cb_default(struct lwm2m_message *msg)
{
	msg->message_timeout_cb(msg);
}

ZTEST_SUITE(lwm2m_rd_client, NULL, NULL, my_suite_before, my_suite_after, NULL);

ZTEST(lwm2m_rd_client, test_start_registration_ok)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_cb_default);
	zassert(lwm2m_rd_client_ctx() == &ctx, "");
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);
	zassert_true(lwm2m_rd_client_is_registred(&ctx), NULL);

	coap_header_get_code_fake.custom_fake = coap_header_get_code_fake_deleted;
	zassert_true(lwm2m_rd_client_stop(&ctx, lwm2m_event_cb, true) == 0, NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_DEREGISTER), NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_DISCONNECT), NULL);
	zassert_true(!lwm2m_rd_client_is_registred(&ctx), NULL);
}

ZTEST(lwm2m_rd_client, test_register_update_too_small_lifetime_to_default)
{
	get_u32_val = CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME / 2;
	lwm2m_get_u32_fake.custom_fake = lwm2m_get_u32_val;

	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_cb_default);
	zassert(lwm2m_rd_client_ctx() == &ctx, "");
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);
	zassert_equal(lwm2m_set_u32_fake.arg1_val, CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME);
}

ZTEST(lwm2m_rd_client, test_timeout_resume_registration)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();

	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_cb_default);
	zassert(lwm2m_rd_client_ctx() == &ctx, "");
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);

	zassert(lwm2m_rd_client_timeout(&ctx) == 0, "");
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);

}

ZTEST(lwm2m_rd_client, test_start_registration_timeout)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_timeout_cb_default);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REG_TIMEOUT), NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REG_TIMEOUT), NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REG_TIMEOUT), NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_NETWORK_ERROR), NULL);
}

ZTEST(lwm2m_rd_client, test_start_registration_fail)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	lwm2m_get_bool_fake.custom_fake = lwm2m_get_bool_fake_default;
	lwm2m_sprint_ip_addr_fake.custom_fake = lwm2m_sprint_ip_addr_fake_default;
	lwm2m_init_message_fake.custom_fake = lwm2m_init_message_fake_default;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_cb_default);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE),
		     NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE),
		     NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE),
		     NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_NETWORK_ERROR),
		     NULL);
}

ZTEST(lwm2m_rd_client, test_start_registration_update)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_cb_default);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);

	lwm2m_rd_client_update();
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE),
		     NULL);
}

ZTEST(lwm2m_rd_client, test_rx_off)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_cb_default);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);

	/* Should not go to RX_OFF while ongoing traffic */
	lwm2m_rd_client_hint_socket_state(&ctx, LWM2M_SOCKET_STATE_ONGOING);
	engine_update_tx_time();
	k_sleep(K_SECONDS(15));
	zassert_false(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF),
		     NULL);

	/* Should not go to RX_OFF while waiting for response */
	lwm2m_rd_client_hint_socket_state(&ctx, LWM2M_SOCKET_STATE_ONE_RESPONSE);
	engine_update_tx_time();
	k_sleep(K_SECONDS(15));
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE),
		     NULL);
	zassert_false(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF),
		     NULL);

	/* Should go to RX_OFF after response to a registration request */
	lwm2m_rd_client_hint_socket_state(&ctx, LWM2M_SOCKET_STATE_LAST);
	engine_update_tx_time();
	k_sleep(K_SECONDS(15));
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE),
		     NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF),
		     NULL);

	/* Should go to RX_OFF normally */
	lwm2m_rd_client_hint_socket_state(&ctx, LWM2M_SOCKET_STATE_NO_DATA);
	engine_update_tx_time();
	k_sleep(K_SECONDS(15));
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE),
		     NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF),
		     NULL);
}

ZTEST(lwm2m_rd_client, test_start_registration_update_fail)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_cb_default);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);

	RESET_FAKE(coap_header_get_code);

	lwm2m_rd_client_update();
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE),
		     NULL);
}

ZTEST(lwm2m_rd_client, test_registration_update_timeout)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_cb_default);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);
	test_prepare_pending_message_cb(&message_reply_timeout_cb_default);
	ctx.sock_fd = 100;
	lwm2m_rd_client_update();
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REG_UPDATE));
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REG_TIMEOUT),
		     NULL);
	zassert_true(lwm2m_engine_stop_fake.call_count >= 1);
	zassert_equal(lwm2m_engine_stop_fake.arg0_val, &ctx);

	test_prepare_pending_message_cb(&message_reply_cb_default);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);
}

ZTEST(lwm2m_rd_client, test_deregistration_timeout)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_cb_default);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);

	test_prepare_pending_message_cb(&message_reply_timeout_cb_default);
	zassert_true(lwm2m_rd_client_stop(&ctx, lwm2m_event_cb, true) == 0, NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_DEREGISTER), NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE));
}

ZTEST(lwm2m_rd_client, test_error_on_registration_update)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	coap_packet_append_option_fake.custom_fake = NULL;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_cb_default);

	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);

	coap_packet_append_option_fake.custom_fake = coap_packet_append_option_fake_err;
	lwm2m_rd_client_update();
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);
}

ZTEST(lwm2m_rd_client, test_network_error_on_registration)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;

	coap_packet_append_option_fake.custom_fake = coap_packet_append_option_fake_err;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_NETWORK_ERROR), NULL);
}

ZTEST(lwm2m_rd_client, test_suspend_resume_registration)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_cb_default);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);
	zassert_true(!lwm2m_rd_client_is_suspended(&ctx), NULL);

	zassert_true(lwm2m_rd_client_pause() == 0, NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_ENGINE_SUSPENDED), NULL);
	zassert_true(lwm2m_rd_client_is_suspended(&ctx), NULL);

	zassert_true(lwm2m_rd_client_resume() == 0, NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE),
		     NULL);
	zassert_true(!lwm2m_rd_client_is_suspended(&ctx), NULL);

	zassert_true(lwm2m_rd_client_pause() == 0, NULL);
	k_sleep(K_SECONDS(CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME));
	zassert_true(lwm2m_rd_client_resume() == 0, NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);
}

ZTEST(lwm2m_rd_client, test_suspend_stop_resume)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	coap_header_get_code_fake.custom_fake = coap_header_get_code_fake_created;
	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_cb_default);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);
	zassert_true(lwm2m_rd_client_pause() == 0, NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_ENGINE_SUSPENDED), NULL);

	zassert_equal(lwm2m_rd_client_stop(&ctx, lwm2m_event_cb, false), 0);
	zassert_true(lwm2m_rd_client_resume() == 0, NULL);
	zassert_false(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_DEREGISTER), NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_DISCONNECT), NULL);
}

ZTEST(lwm2m_rd_client, test_socket_error)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	coap_header_get_code_fake.custom_fake = coap_header_get_code_fake_created;
	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_cb_default);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);

	ctx.fault_cb(EIO);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REG_UPDATE), NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE),
		     NULL);
}

ZTEST(lwm2m_rd_client, test_socket_error_on_stop)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	coap_header_get_code_fake.custom_fake = coap_header_get_code_fake_created;
	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_cb_default);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);

	test_prepare_pending_message_cb(NULL);
	zassert_equal(lwm2m_rd_client_stop(&ctx, lwm2m_event_cb, true), 0);
	k_msleep(1000);
	ctx.fault_cb(EIO);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE),
		     NULL);
}

ZTEST(lwm2m_rd_client, test_no_context)
{
	lwm2m_rd_client_init();
	zassert_equal(lwm2m_rd_client_stop(&ctx, NULL, false), -EPERM);
	zassert_equal(lwm2m_rd_client_pause(), -EPERM);
	zassert_equal(lwm2m_rd_client_resume(), -EPERM);
	zassert_equal(lwm2m_rd_client_connection_resume(&ctx), -EPERM);
	zassert_equal(lwm2m_rd_client_timeout(&ctx), -EPERM);
}

ZTEST(lwm2m_rd_client, test_engine_trigger_bootstrap)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_cb_default);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);
	lwm2m_get_bool_fake.custom_fake = lwm2m_get_bool_fake_true;
	coap_header_get_code_fake.custom_fake = coap_header_get_code_fake_changed;
	zassert_equal(engine_trigger_bootstrap(), 0);
	zassert_equal(engine_trigger_bootstrap(), -EPERM);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_COMPLETE),
		     NULL);
	engine_bootstrap_finish();
	zassert_equal(
		expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_TRANSFER_COMPLETE),
		true);
}

ZTEST(lwm2m_rd_client, test_bootstrap_timeout)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	lwm2m_get_bool_fake.custom_fake = lwm2m_get_bool_fake_true;
	lwm2m_sprint_ip_addr_fake.custom_fake = lwm2m_sprint_ip_addr_fake_default;
	lwm2m_init_message_fake.custom_fake = lwm2m_init_message_fake_default;
	coap_header_get_code_fake.custom_fake = coap_header_get_code_fake_created;
	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 1, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_timeout_cb_default);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE),
		     NULL);
}

ZTEST(lwm2m_rd_client, test_bootstrap_fail)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	lwm2m_get_bool_fake.custom_fake = lwm2m_get_bool_fake_true;
	lwm2m_sprint_ip_addr_fake.custom_fake = lwm2m_sprint_ip_addr_fake_default;
	lwm2m_init_message_fake.custom_fake = lwm2m_init_message_fake_default;
	coap_header_get_code_fake.custom_fake = coap_header_get_code_fake_bad_request;
	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 1, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_cb_default);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE),
		     NULL);

}

ZTEST(lwm2m_rd_client, test_bootstrap_no_srv)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 1, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_cb_default);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE),
		     NULL);
}

ZTEST(lwm2m_rd_client, test_disable_server)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_cb_default);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);
	coap_header_get_code_fake.custom_fake = coap_header_get_code_fake_deleted;
	stub_lwm2m_server_disable(true);
	lwm2m_rd_client_server_disabled(0);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_SERVER_DISABLED),
		     NULL);
}

ZTEST(lwm2m_rd_client, test_disable_server_stop)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_cb_default);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);
	coap_header_get_code_fake.custom_fake = coap_header_get_code_fake_deleted;
	stub_lwm2m_server_disable(true);
	lwm2m_rd_client_server_disabled(0);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_SERVER_DISABLED),
		     NULL);
	wait_for_service(1);
	zassert_true(lwm2m_rd_client_stop(&ctx, lwm2m_event_cb, true) == 0, NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_DISCONNECT), NULL);
}

ZTEST(lwm2m_rd_client, test_disable_server_connect)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_cb_default);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);
	coap_header_get_code_fake.custom_fake = coap_header_get_code_fake_deleted;
	stub_lwm2m_server_disable(true);
	lwm2m_rd_client_server_disabled(0);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_SERVER_DISABLED),
		     NULL);

	wait_for_service(500);

	coap_header_get_code_fake.custom_fake = coap_header_get_code_fake_created;
	stub_lwm2m_server_disable(false);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);
}

ZTEST(lwm2m_rd_client, test_fallback_to_bootstrap)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	lwm2m_get_bool_fake.custom_fake = lwm2m_get_bool_fake_true;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_timeout_cb_default);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REG_TIMEOUT), NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REG_TIMEOUT), NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REG_TIMEOUT), NULL);

	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE),
		     NULL);
}

ZTEST(lwm2m_rd_client, test_no_srv_fallback_to_bootstrap)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	coap_header_get_code_fake.custom_fake = coap_header_get_code_fake_changed;
	lwm2m_get_bool_fake.custom_fake = lwm2m_get_bool_fake_true;
	stub_lwm2m_server_disable(true);
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	test_prepare_pending_message_cb(&message_reply_cb_default);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_COMPLETE),
		     NULL);
	coap_header_get_code_fake.custom_fake = coap_header_get_code_fake_created;
	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	stub_lwm2m_server_disable(false);
	engine_bootstrap_finish();
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);
}

ZTEST(lwm2m_rd_client, test_start_stop_ignore_engine_fault)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	test_prepare_pending_message_cb(&message_reply_cb_default);

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	lwm2m_engine_context_init_fake.custom_fake = lwm2m_engine_context_init_fake1;
	lwm2m_get_bool_fake.custom_fake = lwm2m_get_bool_fake_default;
	lwm2m_sprint_ip_addr_fake.custom_fake = lwm2m_sprint_ip_addr_fake_default;
	lwm2m_init_message_fake.custom_fake = lwm2m_init_message_fake_default;
	coap_header_get_code_fake.custom_fake = coap_header_get_code_fake_created;
	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);

	coap_header_get_code_fake.custom_fake = coap_header_get_code_fake_deleted;
	zassert_true(lwm2m_rd_client_stop(&ctx, lwm2m_event_cb, true) == 0, NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_DISCONNECT), NULL);

	int c = show_lwm2m_event_fake.call_count;

	test_throw_network_error_from_engine(EIO);
	wait_for_service(10);
	zassert_equal(show_lwm2m_event_fake.call_count, c,
		      "Should not enter any other state and throw an event");
}

ZTEST(lwm2m_rd_client, test_start_suspend_ignore_engine_fault)
{
	(void)memset(&ctx, 0x0, sizeof(ctx));

	test_prepare_pending_message_cb(&message_reply_cb_default);

	lwm2m_rd_client_init();
	test_lwm2m_engine_start_service();
	wait_for_service(1);

	lwm2m_engine_context_init_fake.custom_fake = lwm2m_engine_context_init_fake1;
	lwm2m_get_bool_fake.custom_fake = lwm2m_get_bool_fake_default;
	lwm2m_sprint_ip_addr_fake.custom_fake = lwm2m_sprint_ip_addr_fake_default;
	lwm2m_init_message_fake.custom_fake = lwm2m_init_message_fake_default;
	coap_header_get_code_fake.custom_fake = coap_header_get_code_fake_created;
	coap_find_options_fake.custom_fake = coap_find_options_do_registration_reply_cb_ok;
	zassert_true(lwm2m_rd_client_start(&ctx, "Test", 0, lwm2m_event_cb, lwm2m_observe_cb) == 0,
		     NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE),
		     NULL);

	coap_header_get_code_fake.custom_fake = coap_header_get_code_fake_deleted;
	zassert_true(lwm2m_rd_client_pause() == 0, NULL);
	zassert_true(expect_lwm2m_rd_client_event(LWM2M_RD_CLIENT_EVENT_ENGINE_SUSPENDED), NULL);

	int c = show_lwm2m_event_fake.call_count;

	test_throw_network_error_from_engine(EIO);
	wait_for_service(10);
	zassert_equal(show_lwm2m_event_fake.call_count, c,
		      "Should not enter any other state and throw an event");
}
