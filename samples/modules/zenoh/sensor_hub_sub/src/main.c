
/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/pmci/mctp/mctp.h>
#include <libmctp.h>
#include <zenoh-pico.h>

#include <zephyr/pmci/mctp/mctp_i2c_gpio_target.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sensor_hub_sub);

MCTP_I2C_GPIO_CONTROLLER_DT_DEFINE(mctp_i2c_ctrl, DT_NODELABEL(mctp_i2c));

#define MODE "peer"
#define LOCATOR "mctp/11#"

#define KEYEXPR "demo/example/**"

void data_handler(z_loaned_sample_t *sample, void *arg) {
    z_view_string_t keystr;
    z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
    z_owned_string_t value;
    z_bytes_to_string(z_sample_payload(sample), &value);
    LOG_INF(" >> [Subscriber handler] Received ('%.*s': '%.*s')", (int)z_string_len(z_loan(keystr)),
           z_string_data(z_loan(keystr)), (int)z_string_len(z_loan(value)), z_string_data(z_loan(value)));
    z_drop(z_move(value));
}

int main(int argc, char **argv) {
    int rc;

    
    LOG_INF("Sensor Hub Sub: MCTP Endpoint ID[%d] on Board [%s]", CONFIG_MCTP_ENDPOINT_ID, CONFIG_BOARD_TARGET);
    zephyr_mctp_register_bus(&mctp_i2c_ctrl.binding);

    // Initialize Zenoh Session and other parameters
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, MODE);
    if (strcmp(LOCATOR, "") != 0) {
    
	    zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, LOCATOR);
    }
    // Open Zenoh session
    LOG_INF("Opening Zenoh Session...");
    z_owned_session_t s;
    while (z_open(&s, z_move(config), NULL) < 0) {
        LOG_INF("Unable to open session, retrying in a few...\n");
        k_msleep(5000);
    }
    LOG_INF("Starting read+lease tasks");
    // Start the receive and the session lease loop for zenoh-pico
    zp_start_read_task(z_loan_mut(s), NULL);
    zp_start_lease_task(z_loan_mut(s), NULL);
    LOG_INF("Declaring Subscriber on '%s'...", KEYEXPR);
    z_owned_closure_sample_t callback;
    z_closure(&callback, data_handler, NULL, NULL);
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str_unchecked(&ke, KEYEXPR);
    z_owned_subscriber_t sub;
    if (z_declare_subscriber(z_loan(s), &sub, z_loan(ke), z_move(callback), NULL) < 0) {
        LOG_ERR("Unable to declare subscriber.");
        return -1;
    }
    LOG_INF("Looping for eternity!");
    while (1) {
        sleep(1);
    }
    LOG_INF("Closing Zenoh Session...");
    z_drop(z_move(sub));
    z_drop(z_move(s));
    LOG_INF("Done!");
    return rc;
}
