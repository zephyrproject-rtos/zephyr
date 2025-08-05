/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <platform-zephyr.h>
#include <openthread/border_agent.h>
#include <openthread/cli.h>
#include <openthread/dns.h>
#include <openthread/thread.h>
#include <openthread/verhoeff_checksum.h>
#include <openthread/platform/entropy.h>
#include "common/code_utils.hpp"
#include "openthread_border_router.h"
#include <zephyr/sys/byteorder.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

static struct otInstance *ot_instance_ptr;
static bool border_agent_is_init;

#if defined(CONFIG_OPENTHREAD_BORDER_AGENT_EPHEMERAL_KEY_ENABLE)
/* Byte values, 9 bytes for the key, one for null terminator. */
static uint8_t ephemeral_key_string[10];

static uint32_t ephemeral_key_timeout;
static bool epskc_active;

static otError generate_ephemeral_key(void);
static void handle_border_agent_ephemeral_key_callback(void *context);
static otError border_agent_enable_epskc_service(uint32_t timeout);

/* shell related functions */
static int border_agent_eph_key_shell_cmd(const struct shell *sh, size_t argc, char **argv);
static int border_agent_eph_key_run(size_t argc, char **argv);

#endif /* CONFIG_OPENTHREAD_BORDER_AGENT_EPHEMERAL_KEY_ENABLE */

static void append_vendor_txt_data(uint8_t *txt_data, uint16_t *txt_data_len);

otError border_agent_init(otInstance *instance)
{
	otError error = OT_ERROR_NONE;

	ot_instance_ptr = instance;

	if (!border_agent_is_init) {
		uint8_t txt_buffer[128] = {0};
		uint16_t txt_buffer_len = 0;

		VerifyOrExit(otBorderAgentSetMeshCoPServiceBaseName(instance,
								    otbr_base_service_instance_name)
			     == OT_ERROR_NONE, error = OT_ERROR_FAILED);
		append_vendor_txt_data(txt_buffer, &txt_buffer_len);
		otBorderAgentSetVendorTxtData(instance, txt_buffer, txt_buffer_len);
#if defined(CONFIG_OPENTHREAD_BORDER_AGENT_EPHEMERAL_KEY_ENABLE)
		otBorderAgentEphemeralKeySetEnabled(instance, true);
		otBorderAgentEphemeralKeySetCallback(instance,
						     handle_border_agent_ephemeral_key_callback,
						     instance);
#endif /* CONFIG_OPENTHREAD_BORDER_AGENT_EPHEMERAL_KEY_ENABLE */

		border_agent_is_init = true;
	}
exit:
	return error;
}

void border_agent_deinit(void)
{
#if defined(CONFIG_OPENTHREAD_BORDER_AGENT_EPHEMERAL_KEY_ENABLE)
	otBorderAgentEphemeralKeySetEnabled(ot_instance_ptr, false);
	epskc_active = false;
#endif /* CONFIG_OPENTHREAD_BORDER_AGENT_EPHEMERAL_KEY_ENABLE */
	border_agent_is_init = false;
}

static void append_vendor_txt_data(uint8_t *txt_data, uint16_t *txt_data_len)
{
	*txt_data_len = 0;
	size_t i = 0;
	otDnsTxtEntry txt_entries[] = {
				       {.mKey = "vn", .mValue = (uint8_t *)otbr_vendor_name,
					.mValueLength = strlen(otbr_vendor_name)},
				       {.mKey = "mn", .mValue = (uint8_t *)otbr_model_name,
					.mValueLength = strlen(otbr_model_name)}};

	for (i = 0; i < ARRAY_SIZE(txt_entries); ++i) {
		const otDnsTxtEntry *entry    = &txt_entries[i];
		uint8_t key_len   = (uint8_t)strlen(entry->mKey);
		uint8_t total_len = key_len + 1 + entry->mValueLength; /* +1 for '=' */

		txt_data[(*txt_data_len)++] = total_len;
		memcpy(txt_data + *txt_data_len, entry->mKey, key_len);
		*txt_data_len += key_len;

		txt_data[(*txt_data_len)++] = '=';
		memcpy(txt_data + *txt_data_len, entry->mValue, entry->mValueLength);
		*txt_data_len += entry->mValueLength;
	}

}

#if defined(CONFIG_OPENTHREAD_BORDER_AGENT_EPHEMERAL_KEY_ENABLE)

/** Defining functions below as weak allows applications to implement their specific behaviour,
 *  i.e., custom messages/used print function.
 */
__weak void print_ephemeral_key(const char *ephemeral_key, uint32_t timeout)
{
	otCliOutputFormat("\r\n Use this passcode to enable an additional device to administer "
			  "and manage your Thread network, including adding new devices to it. "
			  "\r\nThis passcode is not required for an app to communicate with "
			  "existing devices on your Thread network.");
	otCliOutputFormat("\r\n\nePSKc : %s", ephemeral_key);
	otCliOutputFormat("\r\n\nValid for %" PRIu32 " seconds.\r\n", timeout);
}

__weak void print_ephemeral_key_expired_message(void)
{
	otCliOutputFormat("\r\nEphemeral Key disabled.\r\n");
}

static otError generate_ephemeral_key(void)
{
	otError error = OT_ERROR_NONE;
	uint8_t i = 0;
	uint32_t random;
	char verhoeff_checksum;

	memset(ephemeral_key_string, 0, sizeof(ephemeral_key_string));

	VerifyOrExit(otPlatEntropyGet((uint8_t *)&random, sizeof(random)) == OT_ERROR_NONE,
		     error = OT_ERROR_FAILED);
	random %= 100000000;
	i += snprintf((char *)ephemeral_key_string, sizeof(ephemeral_key_string),
		      "%08u", random);
	VerifyOrExit(otVerhoeffChecksumCalculate((const char *)ephemeral_key_string,
		     &verhoeff_checksum) == OT_ERROR_NONE,
		     error = OT_ERROR_FAILED);
	snprintf((char *)&ephemeral_key_string[i], sizeof(ephemeral_key_string) - i,
		 "%c", verhoeff_checksum);
exit:
	return error;
}

static void handle_border_agent_ephemeral_key_callback(void *context)
{
	char formatted_epskc[12] = {0};
	otBorderAgentEphemeralKeyState eph_key_state;

	eph_key_state = otBorderAgentEphemeralKeyGetState((otInstance *)context);

	switch (eph_key_state) {
	case OT_BORDER_AGENT_STATE_STOPPED:
		if (epskc_active) {
			epskc_active = false;
			print_ephemeral_key_expired_message();
		}
		break;

	case OT_BORDER_AGENT_STATE_STARTED:
		snprintf(formatted_epskc, sizeof(formatted_epskc), "%.3s %.3s %.3s",
			 ephemeral_key_string, ephemeral_key_string + 3,
			 ephemeral_key_string + 6);
		print_ephemeral_key(formatted_epskc, (uint32_t)(ephemeral_key_timeout / 1000UL));
		epskc_active = true;
		break;

	case OT_BORDER_AGENT_STATE_CONNECTED:
		/* connected to */
	case OT_BORDER_AGENT_STATE_ACCEPTED:
	default:
		break;
	}
}
static otError border_agent_enable_epskc_service(uint32_t timeout)
{
	otError error = OT_ERROR_NONE;

	VerifyOrExit(border_agent_is_init, error = OT_ERROR_INVALID_STATE);

	ephemeral_key_timeout = (timeout &&
				   timeout >= OT_BORDER_AGENT_DEFAULT_EPHEMERAL_KEY_TIMEOUT &&
				   timeout <= OT_BORDER_AGENT_MAX_EPHEMERAL_KEY_TIMEOUT) ?
				   timeout : OT_BORDER_AGENT_DEFAULT_EPHEMERAL_KEY_TIMEOUT;

	VerifyOrExit((generate_ephemeral_key() == OT_ERROR_NONE), error = OT_ERROR_FAILED);
	error = otBorderAgentEphemeralKeyStart(ot_instance_ptr,
					       (const char *)ephemeral_key_string,
					       ephemeral_key_timeout, 0);

exit:
	return error;
}

static int border_agent_eph_key_run(size_t argc, char **argv)
{
	if (strcmp(argv[1], "enable") == 0) {
		uint32_t timeout = (argc >= 1) ? strtoul(argv[2], NULL, 10) : 0;

		if (border_agent_enable_epskc_service(timeout) != OT_ERROR_NONE) {
			otCliOutputFormat("Invalid state\r\n");
		}
		return 0;
	}

	if (strcmp(argv[1], "help") == 0) {
		otCliOutputFormat("ephkey enable [timeout-in-msec]\r\n");
	}

	return 0;
}

static int border_agent_eph_key_shell_cmd(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_help(sh);
		return -ENOEXEC;
	}

	border_agent_eph_key_run(argc, argv);

	return 0;
}
#endif /* CONFIG_OPENTHREAD_BORDER_AGENT_EPHEMERAL_KEY_ENABLE */

#if defined(CONFIG_OPENTHREAD_BORDER_AGENT_EPHEMERAL_KEY_ENABLE)
SHELL_CMD_ARG_REGISTER(ephkey, NULL, "Ephemeral key support", border_agent_eph_key_shell_cmd, 2,
		       CONFIG_SHELL_ARGC_MAX);
#endif /* CONFIG_OPENTHREAD_BORDER_AGENT_EPHEMERAL_KEY_ENABLE */
