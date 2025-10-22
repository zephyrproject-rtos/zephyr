/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/audio/sf32lb_codec.h>

const struct device *dev;

static void tx_done(void)
{
    printf("Transmission complete\n");
}
static void rx_done(uint8_t *pbuf, uint32_t len)
{
    printf("Reception complete: %d bytes\n", len);
    sf32lb_codec_api_write(dev, pbuf, len);
}

int main(void)
{
	printf("Hello World! %s\n", CONFIG_BOARD_TARGET);
    
    printf("BF0 AUDCODEC example started");

    dev = sf32lb_codec_api_find();
    if (!device_is_ready(dev)) {
        printf("sf32lb codec device is not ready\n");
        return -1;
    }
    printf("sf32lb codec device is ready\n");

    struct sf32lb_codec_cfg cfg = {
        .dir = SF32LB_AUDIO_TX,
        .bit_width = 16,
        .channels = 1,
        .block_size = 320,
        .samplerate = 16000,
        .tx_done = tx_done,
        .rx_done = rx_done,
        .reserved = 0,
    };
    sf32lb_codec_api_config(dev, &cfg);
    printf("sf32lb codec configured\n");
    sf32lb_codec_api_start(dev, SF32LB_AUDIO_TX);
    printf("sf32lb codec started\n");
    k_sleep(K_MSEC(5000));
    sf32lb_codec_api_stop(dev, SF32LB_AUDIO_TX);  
    sf32lb_codec_api_set_dac_volume(dev, 15);
    
    cfg.dir = SF32LB_AUDIO_TXRX;

    sf32lb_codec_api_config(dev, &cfg);
    printf("sf32lb codec configured\n");
    sf32lb_codec_api_set_dac_volume(dev, 15);
    sf32lb_codec_api_start(dev, SF32LB_AUDIO_TXRX);
    printf("sf32lb codec started\n");
    k_sleep(K_MSEC(5000));
    sf32lb_codec_api_stop(dev, SF32LB_AUDIO_TXRX);  

    printf("BF0 AUDCODEC example completed\n");


	return 0;
}
