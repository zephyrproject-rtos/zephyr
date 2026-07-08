/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_etm_alloc.h>

#include <zephyr/kernel.h>

#include <soc/soc_caps.h>
#include <hal/etm_ll.h>

#if defined(CONFIG_SOC_GPIO_SUPPORT_ETM)
#include <hal/gpio_etm_ll.h>
#include <hal/gpio_caps.h>
#define ETM_GPIO_NUM_EVENTS _GPIO_ETM_EVENT_CHANNELS_PER_GROUP
#endif

#define ETM_GROUP_ID  0
#define ETM_NUM_CHANS ETM_LL_CHANS_PER_INST

static struct k_spinlock etm_lock;
static uint32_t etm_chan_mask[DIV_ROUND_UP(ETM_NUM_CHANS, 32)];
static bool etm_initialized;

#if defined(CONFIG_SOC_GPIO_SUPPORT_ETM)
static uint32_t etm_gpio_evt_mask;
#endif

static void esp_etm_hw_init(void)
{
	if (etm_initialized) {
		return;
	}

	/* The ETM group clock stays enabled once initialized; channels are the
	 * managed resource.
	 */
	etm_ll_enable_bus_clock(ETM_GROUP_ID, true);
	etm_ll_reset_register(ETM_GROUP_ID);
	etm_ll_enable_function_clock(ETM_GROUP_ID, true);

	etm_initialized = true;
}

int esp_etm_channel_alloc(esp_etm_chan_t *out_chan)
{
	if (out_chan == NULL) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&etm_lock);

	esp_etm_hw_init();

	for (int i = 0; i < ETM_NUM_CHANS; i++) {
		if (!(etm_chan_mask[i / 32] & BIT(i % 32))) {
			etm_chan_mask[i / 32] |= BIT(i % 32);
			k_spin_unlock(&etm_lock, key);
			*out_chan = (esp_etm_chan_t)i;
			return 0;
		}
	}

	k_spin_unlock(&etm_lock, key);
	return -ENOMEM;
}

int esp_etm_channel_connect(esp_etm_chan_t chan, uint32_t event, uint32_t task)
{
	if (chan < 0 || chan >= ETM_NUM_CHANS) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&etm_lock);

	etm_ll_channel_set_event(&SOC_ETM, (uint32_t)chan, event);
	etm_ll_channel_set_task(&SOC_ETM, (uint32_t)chan, task);

	k_spin_unlock(&etm_lock, key);

	return 0;
}

int esp_etm_channel_enable(esp_etm_chan_t chan)
{
	if (chan < 0 || chan >= ETM_NUM_CHANS) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&etm_lock);

	etm_ll_enable_channel(&SOC_ETM, (uint32_t)chan);

	k_spin_unlock(&etm_lock, key);

	return 0;
}

int esp_etm_channel_disable(esp_etm_chan_t chan)
{
	if (chan < 0 || chan >= ETM_NUM_CHANS) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&etm_lock);

	etm_ll_disable_channel(&SOC_ETM, (uint32_t)chan);

	k_spin_unlock(&etm_lock, key);

	return 0;
}

int esp_etm_channel_free(esp_etm_chan_t chan)
{
	if (chan < 0 || chan >= ETM_NUM_CHANS) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&etm_lock);

	etm_chan_mask[chan / 32] &= ~BIT(chan % 32);

	k_spin_unlock(&etm_lock, key);

	return 0;
}

#if defined(CONFIG_SOC_GPIO_SUPPORT_ETM)
int esp_etm_gpio_event_alloc(uint32_t gpio_num, enum esp_etm_gpio_edge edge,
			     esp_etm_chan_t *out_chan, uint32_t *out_event_id)
{
	gpio_etm_dev_t *hw = GPIO_LL_ETM_GET_HW();
	esp_etm_chan_t ch = ESP_ETM_CHAN_NONE;

	if (out_chan == NULL || out_event_id == NULL) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&etm_lock);

	for (int i = 0; i < ETM_GPIO_NUM_EVENTS; i++) {
		if (!(etm_gpio_evt_mask & BIT(i))) {
			etm_gpio_evt_mask |= BIT(i);
			ch = (esp_etm_chan_t)i;
			break;
		}
	}

	k_spin_unlock(&etm_lock, key);

	if (ch == ESP_ETM_CHAN_NONE) {
		return -ENOMEM;
	}

	gpio_ll_etm_event_channel_set_gpio(hw, (uint32_t)ch, gpio_num);
	gpio_ll_etm_enable_event_channel(hw, (uint32_t)ch, true);

	switch (edge) {
	case ESP_ETM_GPIO_EDGE_POS:
		*out_event_id = GPIO_LL_ETM_EVENT_ID_POS_EDGE((uint32_t)ch);
		break;
	case ESP_ETM_GPIO_EDGE_NEG:
		*out_event_id = GPIO_LL_ETM_EVENT_ID_NEG_EDGE((uint32_t)ch);
		break;
	case ESP_ETM_GPIO_EDGE_ANY:
		*out_event_id = GPIO_LL_ETM_EVENT_ID_ANY_EDGE((uint32_t)ch);
		break;
	default:
		esp_etm_gpio_event_free(ch);
		return -EINVAL;
	}

	*out_chan = ch;
	return 0;
}

int esp_etm_gpio_event_free(esp_etm_chan_t chan)
{
	gpio_etm_dev_t *hw = GPIO_LL_ETM_GET_HW();

	if (chan < 0 || chan >= ETM_GPIO_NUM_EVENTS) {
		return -EINVAL;
	}

	gpio_ll_etm_enable_event_channel(hw, (uint32_t)chan, false);

	k_spinlock_key_t key = k_spin_lock(&etm_lock);

	etm_gpio_evt_mask &= ~BIT(chan);

	k_spin_unlock(&etm_lock, key);

	return 0;
}
#endif /* CONFIG_SOC_GPIO_SUPPORT_ETM */
