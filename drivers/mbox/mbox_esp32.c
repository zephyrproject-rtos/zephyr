/*
 * Copyright (c) 2024 Felipe Neves.
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_mbox_esp32

/*
 * SoCs where the HP and LP cores communicate through the PMU software
 * interrupt build either the HP or the LP image separately.
 */
#if defined(CONFIG_SOC_ESP32C5_LPCORE) || defined(CONFIG_SOC_ESP32C6_LPCORE) ||                    \
	defined(CONFIG_SOC_ESP32P4_LPCORE)
#define MBOX_ESP32_LPCORE 1
#endif

#if defined(CONFIG_SOC_ESP32C5_HPCORE) || defined(CONFIG_SOC_ESP32C6_HPCORE) ||                    \
	defined(CONFIG_SOC_ESP32P4_HPCORE)
#define MBOX_ESP32_HPCORE 1
#endif

#if !defined(MBOX_ESP32_HPCORE) && !defined(MBOX_ESP32_LPCORE)
#include "soc/dport_reg.h"
#else
#include <ulp_lp_core.h>
#include <soc/pmu_reg.h>
#include <ulp_lp_core_utils.h>
#include <ulp_lp_core_interrupts.h>
#if defined(MBOX_ESP32_HPCORE)
#include <esp_sleep.h>
#endif
#if defined(MBOX_ESP32_LPCORE)
#include <hal/pmu_ll.h>
#include <soc/pmu_struct.h>
#endif
#endif

#include "soc/gpio_periph.h"

#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/sys/barrier.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mbox_esp32, CONFIG_MBOX_LOG_LEVEL);

struct esp32_mbox_control {
	volatile uint32_t busy[2];
};

/*
 * Stamped into the LP core's busy slot once its interrupt is enabled, distinct
 * from the 0/1 the slot holds in use. The slot is reused instead of a dedicated
 * field because the control block ends at the top of LP RAM on every SoC.
 */
#define ESP32_MBOX_LP_READY_MAGIC 0x4c505244

struct esp32_mbox_memory {
	uint8_t *pro_cpu_shm;
	uint8_t *app_cpu_shm;
};

struct esp32_mbox_config {
	int irq_source_pro_cpu;
	int irq_priority_pro_cpu;
	int irq_flags_pro_cpu;
	int irq_source_app_cpu;
	int irq_priority_app_cpu;
	int irq_flags_app_cpu;
};

struct esp32_mbox_data {
	mbox_callback_t cb;
	void *user_data;
	uint32_t this_core_id;
	uint32_t other_core_id;
	uint32_t shm_size;
	bool lp_ready;
	struct esp32_mbox_memory shm;
	struct esp32_mbox_control *control;
};

static void esp32_mbox_isr(const struct device *dev);

#if defined(MBOX_ESP32_LPCORE)
static const struct device *s_mbox_dev;

/*
 * LP_CORE_ISR_ATTR marks this as an interrupt handler where required. On SoCs
 * with a multi-vectored LP-core interrupt controller (e.g. esp32p4) the
 * hardware jumps here directly, so the handler must save/restore registers and
 * return with mret; the attribute expands to __attribute__((interrupt)) there.
 * On single-vector SoCs (c5/c6) an assembly trampoline does this and the
 * attribute is empty.
 */
LP_CORE_ISR_ATTR void ulp_lp_core_lp_pmu_intr_handler(void)
{
#if defined(CONFIG_SOC_SERIES_ESP32P4)
	bool triggered = PMU.lp_ext.int_st.hp_sw_trigger;
#else
	bool triggered = PMU.lp_ext.int_st.sw_trigger;
#endif

	if (triggered) {
		pmu_ll_lp_clear_sw_intr_status(&PMU);
		if (s_mbox_dev != NULL) {
			esp32_mbox_isr(s_mbox_dev);
		}
	}
}
#endif

IRAM_ATTR static void esp32_mbox_isr(const struct device *dev)
{
	struct esp32_mbox_data *dev_data = (struct esp32_mbox_data *)dev->data;
	struct mbox_msg msg;
	uint32_t core_id = dev_data->this_core_id;

	/* clear interrupt flag */
	if (core_id == 0) {
#if defined(CONFIG_SOC_SERIES_ESP32)
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_0_REG, 0);
#elif defined(CONFIG_SOC_SERIES_ESP32S3)
		WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_0_REG, 0);
#elif defined(MBOX_ESP32_HPCORE)
		SET_PERI_REG_MASK(PMU_HP_INT_CLR_REG, PMU_SW_INT_CLR);
#endif
	} else {
#if defined(CONFIG_SOC_SERIES_ESP32)
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_1_REG, 0);
#elif defined(CONFIG_SOC_SERIES_ESP32S3)
		WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_1_REG, 0);
#elif defined(MBOX_ESP32_LPCORE)
		ulp_lp_core_sw_intr_clear();
#endif
	}

	if (dev_data->cb) {
		msg.data = (dev_data->this_core_id == 0) ? (const void *)dev_data->shm.pro_cpu_shm
							 : (const void *)dev_data->shm.app_cpu_shm;
		msg.size = dev_data->shm_size;

		dev_data->cb(dev, dev_data->other_core_id, dev_data->user_data, &msg);
	}

	barrier_dsync_fence_full();
	dev_data->control->busy[dev_data->this_core_id] = 0;
}

#if defined(MBOX_ESP32_HPCORE)
/* A deep-sleep wakeup leaves the LP core running with its stamp still valid. */
static bool esp32_mbox_lp_core_kept_running(void)
{
	return (esp_sleep_get_wakeup_causes() & BIT(ESP_SLEEP_WAKEUP_ULP)) != 0;
}

/* Consume the ready stamp once the LP core has posted it. Does not block. */
static bool esp32_mbox_lp_is_ready(struct esp32_mbox_data *dev_data)
{
	volatile uint32_t *slot = &dev_data->control->busy[dev_data->other_core_id];

	if (dev_data->lp_ready) {
		return true;
	}

	if (*slot != ESP32_MBOX_LP_READY_MAGIC) {
		return false;
	}

	*slot = 0;
	barrier_dsync_fence_full();
	dev_data->lp_ready = true;

	return true;
}

/*
 * Hold the application back until the LP core can service the mailbox, so a
 * first send from main() does not race it. The LP core is only started by the
 * SoC late init hook, after device init, so this runs at APPLICATION level.
 * Bounded so an LP image that never uses the mailbox still boots; mbox_send()
 * keeps reporting -EBUSY until ready.
 */
static int esp32_mbox_wait_lp_ready(void)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	struct esp32_mbox_data *dev_data = (struct esp32_mbox_data *)dev->data;

	if (dev_data->this_core_id == 0 &&
	    !WAIT_FOR(esp32_mbox_lp_is_ready(dev_data), CONFIG_MBOX_ESP32_LP_READY_TIMEOUT_US,
		      k_busy_wait(100))) {
		LOG_WRN("%s: remote core is not servicing the mailbox", dev->name);
	}

	return 0;
}

SYS_INIT(esp32_mbox_wait_lp_ready, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif

static int esp32_mbox_send(const struct device *dev, mbox_channel_id_t channel,
			   const struct mbox_msg *msg)
{
	struct esp32_mbox_data *dev_data = (struct esp32_mbox_data *)dev->data;
	uint32_t mtu = dev_data->shm_size;

	if (channel > 0xFFFF) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	if (msg != NULL && msg->data != NULL && msg->size > mtu) {
		LOG_ERR("Message size %d exceeds shared memory region %d", msg->size, mtu);
		return -EMSGSIZE;
	}

#if defined(MBOX_ESP32_HPCORE)
	if (!esp32_mbox_lp_is_ready(dev_data)) {
		return -EBUSY;
	}
#endif

	uint32_t key = irq_lock();

	if (dev_data->control->busy[dev_data->other_core_id]) {
		irq_unlock(key);
		return -EBUSY;
	}
	dev_data->control->busy[dev_data->other_core_id] = 1;

	/* Copy data into the other core's receive region */
	if (msg != NULL && msg->data != NULL) {
		uint8_t *dest = (dev_data->other_core_id == 0) ? dev_data->shm.pro_cpu_shm
							       : dev_data->shm.app_cpu_shm;
		memcpy(dest, msg->data, msg->size);
	}

	barrier_dsync_fence_full();

	/* Generate interrupt in the remote core */
	if (dev_data->this_core_id == 0) {
		LOG_DBG("Generating interrupt on remote CPU 1 from CPU 0");
#if defined(CONFIG_SOC_SERIES_ESP32)
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_1_REG, DPORT_CPU_INTR_FROM_CPU_1);
#elif defined(CONFIG_SOC_SERIES_ESP32S3)
		WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_1_REG, SYSTEM_CPU_INTR_FROM_CPU_1);
#elif defined(MBOX_ESP32_HPCORE)
		ulp_lp_core_sw_intr_trigger();
#endif
	} else {
		LOG_DBG("Generating interrupt on remote CPU 0 from CPU 1");
#if defined(CONFIG_SOC_SERIES_ESP32)
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_0_REG, DPORT_CPU_INTR_FROM_CPU_0);
#elif defined(CONFIG_SOC_SERIES_ESP32S3)
		WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_0_REG, SYSTEM_CPU_INTR_FROM_CPU_0);
#elif defined(MBOX_ESP32_LPCORE)
		ulp_lp_core_wakeup_main_processor();
#endif
	}

	irq_unlock(key);

	return 0;
}

static int esp32_mbox_register_callback(const struct device *dev, mbox_channel_id_t channel,
					mbox_callback_t cb, void *user_data)
{
	ARG_UNUSED(channel);

	struct esp32_mbox_data *data = (struct esp32_mbox_data *)dev->data;

	if (!cb) {
		LOG_ERR("Must provide callback");
		return -EINVAL;
	}

	uint32_t key = irq_lock();

	data->cb = cb;
	data->user_data = user_data;

	irq_unlock(key);

	return 0;
}

static int esp32_mbox_mtu_get(const struct device *dev)
{
	struct esp32_mbox_data *data = (struct esp32_mbox_data *)dev->data;

	return data->shm_size;
}

static uint32_t esp32_mbox_max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 1;
}

static int esp32_mbox_set_enabled(const struct device *dev, mbox_channel_id_t channel, bool enable)
{
	/* The esp32 MBOX is always enabled
	 * but rpmsg backend needs MBOX set enabled to be
	 * implemented so just return success here
	 */

	ARG_UNUSED(dev);
	ARG_UNUSED(enable);
	ARG_UNUSED(channel);

	return 0;
}

static int esp32_mbox_init(const struct device *dev)
{
	struct esp32_mbox_data *data = (struct esp32_mbox_data *)dev->data;
#if !defined(MBOX_ESP32_LPCORE)
	struct esp32_mbox_config *cfg = (struct esp32_mbox_config *)dev->config;
	int ret;
#endif

#if defined(MBOX_ESP32_LPCORE)
	data->this_core_id = 1;
#else
	data->this_core_id = esp_core_id();
#endif
	data->other_core_id = (data->this_core_id == 0) ? 1 : 0;

	if (data->this_core_id == 0) {
		data->control->busy[0] = 0;
#if defined(MBOX_ESP32_HPCORE)
		/*
		 * Clear a stale stamp left in unretained LP RAM, but keep a
		 * valid one from a deep-sleep wakeup: the LP core kept running
		 * and will not post it again.
		 */
		if (!esp32_mbox_lp_core_kept_running()) {
			data->control->busy[1] = 0;
		}
#else
		data->control->busy[1] = 0;
#endif
		barrier_dsync_fence_full();
#if !defined(MBOX_ESP32_LPCORE)
		ret = esp_intr_alloc(cfg->irq_source_pro_cpu,
				     ESP_PRIO_TO_FLAGS(cfg->irq_priority_pro_cpu) |
					     ESP_INT_FLAGS_CHECK(cfg->irq_flags_pro_cpu) |
					     ESP_INTR_FLAG_IRAM,
				     (intr_handler_t)esp32_mbox_isr, (void *)dev, NULL);
#endif
#if defined(MBOX_ESP32_HPCORE)
		SET_PERI_REG_MASK(PMU_HP_INT_ENA_REG, PMU_SW_INT_ENA);
#endif
	} else {
#if defined(MBOX_ESP32_LPCORE)
		s_mbox_dev = dev;
		ulp_lp_core_intr_enable();
		ulp_lp_core_sw_intr_enable(true);

		data->control->busy[data->this_core_id] = ESP32_MBOX_LP_READY_MAGIC;
		barrier_dsync_fence_full();
#else
		ret = esp_intr_alloc(cfg->irq_source_app_cpu,
				     ESP_PRIO_TO_FLAGS(cfg->irq_priority_app_cpu) |
					     ESP_INT_FLAGS_CHECK(cfg->irq_flags_app_cpu) |
					     ESP_INTR_FLAG_IRAM,
				     (intr_handler_t)esp32_mbox_isr, (void *)dev, NULL);
#endif
	}

#if defined(MBOX_ESP32_LPCORE)
	return 0;
#else
	return ret;
#endif
}

static DEVICE_API(mbox, esp32_mbox_driver_api) = {
	.send = esp32_mbox_send,
	.register_callback = esp32_mbox_register_callback,
	.mtu_get = esp32_mbox_mtu_get,
	.max_channels_get = esp32_mbox_max_channels_get,
	.set_enabled = esp32_mbox_set_enabled,
};

#define ESP32_MBOX_SHM_SIZE_BY_IDX(idx) DT_INST_PROP(idx, shared_memory_size)

#define ESP32_MBOX_SHM_ADDR_BY_IDX(idx) DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(idx), shared_memory))

#define ESP32_MBOX_INIT(idx)                                                                       \
	static struct esp32_mbox_config esp32_mbox_device_cfg_##idx = {                            \
		.irq_source_pro_cpu = DT_INST_IRQ_BY_IDX(idx, 0, irq),                             \
		.irq_priority_pro_cpu = DT_INST_IRQ_BY_IDX(idx, 0, priority),                      \
		.irq_flags_pro_cpu = DT_INST_IRQ_BY_IDX(idx, 0, flags),                            \
		.irq_source_app_cpu = DT_INST_IRQ_BY_IDX(idx, 1, irq),                             \
		.irq_priority_app_cpu = DT_INST_IRQ_BY_IDX(idx, 1, priority),                      \
		.irq_flags_app_cpu = DT_INST_IRQ_BY_IDX(idx, 1, flags),                            \
	};                                                                                         \
	static struct esp32_mbox_data esp32_mbox_device_data_##idx = {                             \
		.shm_size = ESP32_MBOX_SHM_SIZE_BY_IDX(idx) / 2,                                   \
		.shm.pro_cpu_shm = (uint8_t *)ESP32_MBOX_SHM_ADDR_BY_IDX(idx),                     \
		.shm.app_cpu_shm = (uint8_t *)ESP32_MBOX_SHM_ADDR_BY_IDX(idx) +                    \
				   ESP32_MBOX_SHM_SIZE_BY_IDX(idx) / 2,                            \
		.control = (struct esp32_mbox_control *)DT_INST_REG_ADDR(idx),                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, &esp32_mbox_init, NULL, &esp32_mbox_device_data_##idx,          \
			      &esp32_mbox_device_cfg_##idx, PRE_KERNEL_2,                          \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &esp32_mbox_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_MBOX_INIT);
