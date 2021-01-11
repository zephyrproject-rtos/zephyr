/*
 * Copyright (c) 2021 Golden Bits Software, Inc.
 *
 * main.c - Application main entry point
 *          Sample authenticating over a UART link
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>
#include <device.h>
#include <drivers/uart.h>


#include <auth/auth_lib.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>


#if defined(CONFIG_AUTH_DTLS)
#include "../../../certs/auth_certs.h"
#endif

LOG_MODULE_REGISTER(auth_serial_server, CONFIG_AUTH_LOG_LEVEL);



static struct uart_config uart_cfg = {
	.baudrate = 115200,
	.parity = UART_CFG_PARITY_NONE,
	.stop_bits = UART_CFG_STOP_BITS_1,
	.data_bits = UART_CFG_DATA_BITS_8,
	.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
};

#if defined(CONFIG_AUTH_DTLS)
/* The Root and Intermediate Certs in a single CA chain.
 * plus the server cert. All in PEM format.*/
static const uint8_t auth_cert_ca_chain[] = AUTH_ROOTCA_CERT_PEM AUTH_INTERMEDIATE_CERT_PEM;
static const uint8_t auth_dev_client_cert[] = AUTH_CLIENT_CERT_PEM;
static const uint8_t auth_client_privatekey[] = AUTH_CLIENT_PRIVATE_KEY_PEM;

static struct auth_optional_param tls_certs_param = {
	.param_id = AUTH_DTLS_PARAM,
	.param_body = {
		.dtls_certs = {
			.server_ca_chain_pem = {
				.cert = auth_cert_ca_chain,
				.cert_size = sizeof(auth_cert_ca_chain),
			},

			.device_cert_pem = {
				.cert = auth_dev_client_cert,
				.cert_size = sizeof(auth_dev_client_cert),
				.priv_key = auth_client_privatekey,
				.priv_key_size = sizeof(auth_client_privatekey)
			}
		}
	}
};
#endif

#if defined(CONFIG_AUTH_CHALLENGE_RESPONSE)
#define NEW_SHARED_KEY_LEN          (32u)

/* Use a different key than default */
static uint8_t chal_resp_sharedkey[NEW_SHARED_KEY_LEN] = {
	0x21, 0x8e, 0x37, 0x42, 0x1e, 0xe1, 0x2a, 0x22, 0x7c, 0x4b, 0x3f, 0x3f, 0x07, 0x5e, 0x8a, 0xd8,
	0x24, 0xdf, 0xca, 0xf4, 0x04, 0xd0, 0x3e, 0x22, 0x61, 0x9f, 0x24, 0xa3, 0xc7, 0xf6, 0x5d, 0x66
};


static struct auth_optional_param chal_resp_param = {
	.param_id = AUTH_CHALRESP_PARAM,
	.param_body = {
		.chal_resp = {
			.shared_key = chal_resp_sharedkey,
		},
	}
};
#endif

static const struct device *uart_dev;

/* Authentication connection info */
static struct authenticate_conn auth_conn_serial;

/**
 * Authentication status callback.
 *
 * @param auth_conn   The authentication connection.
 * @param instance    Authentication instance.
 * @param status      Status
 * @param context     Optional context.
 */
static void auth_status_callback(struct authenticate_conn *auth_conn,
				 enum auth_instance_id instance,
				 enum auth_status status, void *context)
{
	printk("Authentication instance (%d) status: %s\n", instance,
	       auth_lib_getstatus_str(status));

#if defined(CONFIG_AUTH_DTLS)
	if (status == AUTH_STATUS_IN_PROCESS) {
		printk("     DTLS may take 30-60 seconds.\n");
	}
#endif

	if ((status == AUTH_STATUS_FAILED) ||
	    (status == AUTH_STATUS_AUTHENTICATION_FAILED) ||
	    (status == AUTH_STATUS_SUCCESSFUL)) {
		/* Authentication has finished */
		auth_lib_deinit(auth_conn);
	}
}

static void process_log_msgs(void)
{
	while (log_process(false)) {
		; /* intentionally empty statement */
	}
}

static void idle_process(void)
{
	/* Just spin while the BT modules handle the connection. */
	while (true) {

		process_log_msgs();

		/* Let the handshake thread run */
		k_yield();
	}
}

/**
 *  Configure the UART params.
 *
 * @return 0 on success, else negative error value.
 */
static int config_uart(void)
{
	struct auth_xp_serial_params xp_params;

	uart_dev = device_get_binding(DT_LABEL(DT_NODELABEL(uart0)));

	int err = uart_configure(uart_dev, &uart_cfg);

	if (err) {
		LOG_ERR("Failed to configure UART, err: %d", err);
		return err;
	}

	/* If successful,then init lower transport layer. */
	xp_params.uart_dev = uart_dev;

	err = auth_xport_init(&auth_conn_serial.xport_hdl,
			      auth_conn_serial.instance,
			      AUTH_XP_TYPE_SERIAL, &xp_params);

	if (err) {
		LOG_ERR("Failed to initialize authentication transport, error: %d", err);
	}

	return err;
}


void main(void)
{
	struct auth_optional_param *opt_parms = NULL;

	log_init();

#if defined(CONFIG_AUTH_DTLS) && defined(CONFIG_AUTH_CHALLENGE_RESPONSE)
#error Invalid authentication config, either DTLS or Challenge-Response, not both.
#endif

	uint32_t flags = AUTH_CONN_SERVER;


#if defined(CONFIG_AUTH_DTLS)
	flags |= AUTH_CONN_DTLS_AUTH_METHOD;

	/* set TLS certs */
	opt_parms = &tls_certs_param;
	printk("Using DTLS authentication method.\n");
#endif


#if defined(CONFIG_AUTH_CHALLENGE_RESPONSE)
	flags |= AUTH_CONN_CHALLENGE_AUTH_METHOD;

	/* Use different shared key */
	opt_parms = &chal_resp_param;
	printk("Using Challenge-Response authentication method.\n");
#endif


	/* init authentication library */
	int err = auth_lib_init(&auth_conn_serial, AUTH_INST_1_ID,
				auth_status_callback, NULL,
				opt_parms, flags);

	/* If successful, then configure the UAR and start the
	 * authentication process */
	if (!err) {

		/* configure the UART and init the lower serial transport */
		err = config_uart();

		/* start authentication */
		if (!err) {
			err = auth_lib_start(&auth_conn_serial);

			if (err) {
				LOG_ERR("Failed to start authentication, err: %d", err);
			}
		}

	}

	/* does not return */
	idle_process();

	/* should not reach here */
}
