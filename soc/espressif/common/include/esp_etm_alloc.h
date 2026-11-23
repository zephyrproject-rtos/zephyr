/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ESPRESSIF_COMMON_ESP_ETM_ALLOC_H_
#define ZEPHYR_SOC_ESPRESSIF_COMMON_ESP_ETM_ALLOC_H_

#include <stdint.h>
#include <stdbool.h>

/*
 * Thin Event Task Matrix (ETM) channel allocator shared by Espressif
 * peripheral drivers. The matrix routes a peripheral event to a
 * peripheral task entirely in hardware, with no CPU involvement at the
 * trigger instant. Consumers acquire a channel, connect an event source
 * id to a task id, then enable the channel.
 *
 * Event and task ids come from the per-SoC soc_etm_source.h tables and
 * from the peripheral _ll helpers (e.g. GPIO event ids, GPTimer task
 * ids).
 */

/** Opaque ETM channel handle. Negative value means "not allocated". */
typedef int esp_etm_chan_t;

#define ESP_ETM_CHAN_NONE (-1)

/**
 * @brief Acquire a free ETM channel.
 *
 * @param out_chan Filled with the acquired channel handle on success.
 *
 * @retval 0 on success.
 * @retval -EINVAL if out_chan is NULL.
 * @retval -ENOMEM if no free channel is available.
 */
int esp_etm_channel_alloc(esp_etm_chan_t *out_chan);

/**
 * @brief Connect an event source to a task on a channel.
 *
 * @param chan  Channel handle from esp_etm_channel_alloc().
 * @param event ETM event source id.
 * @param task  ETM task id.
 *
 * @retval 0 on success, negative errno otherwise.
 */
int esp_etm_channel_connect(esp_etm_chan_t chan, uint32_t event, uint32_t task);

/**
 * @brief Enable a previously connected channel.
 */
int esp_etm_channel_enable(esp_etm_chan_t chan);

/**
 * @brief Disable a channel without releasing it.
 */
int esp_etm_channel_disable(esp_etm_chan_t chan);

/**
 * @brief Release a channel back to the pool.
 */
int esp_etm_channel_free(esp_etm_chan_t chan);

/** Edge selection for a GPIO ETM event. */
enum esp_etm_gpio_edge {
	ESP_ETM_GPIO_EDGE_POS,
	ESP_ETM_GPIO_EDGE_NEG,
	ESP_ETM_GPIO_EDGE_ANY,
};

/**
 * @brief Acquire a GPIO ETM event channel and bind a pad and edge to it.
 *
 * The returned event id can be passed as the @p event argument of
 * esp_etm_channel_connect().
 *
 * The GPIO event channel handle belongs to a pool distinct from the matrix
 * channel pool. It must only be passed to esp_etm_gpio_event_free(), never to
 * esp_etm_channel_free() and vice versa.
 *
 * @param gpio_num Pad number to watch.
 * @param edge     Edge that produces the event.
 * @param out_chan Filled with the acquired GPIO event channel handle.
 * @param out_event_id Filled with the ETM event source id for the pad.
 *
 * @retval 0 on success, negative errno otherwise.
 */
int esp_etm_gpio_event_alloc(uint32_t gpio_num, enum esp_etm_gpio_edge edge,
			     esp_etm_chan_t *out_chan, uint32_t *out_event_id);

/**
 * @brief Release a GPIO ETM event channel.
 */
int esp_etm_gpio_event_free(esp_etm_chan_t chan);

#endif /* ZEPHYR_SOC_ESPRESSIF_COMMON_ESP_ETM_ALLOC_H_ */
