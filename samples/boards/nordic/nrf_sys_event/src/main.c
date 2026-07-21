/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrf_sys_event.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/cache.h>
#include <stdio.h>

#ifdef CONFIG_NRF_SYS_EVENT_IRQ_LATENCY
#define ALARM_CH 0
#define TIMEOUT_US 100

enum rramc_mode {
	/* Default mode where RRAMC goes to low power state and had approx.15 us wake up time. */
	RRAMC_DEFAULT,
	/* Using nrf_sys_event API to schedule a PPI wake up before expected interrupt. */
	RRAMC_PPI_WAKEUP,
	/* Using nrf_sys_event API to change the power mode of RRAMC to 0 us wake up time. */
	RRAMC_POWER_MODE,
};

static void counter_handler(const struct device *counter_dev, uint8_t ch_id,
			    uint32_t ticks, void *user_data)
{
	k_sem_give((struct k_sem *)user_data);
}

static uint32_t counter_alarm_execute(const struct device *counter_dev,
				      struct counter_alarm_cfg *alarm_cfg, k_timeout_t timeout)
{
	struct k_sem sem;
	uint32_t now;
	int err;

	k_sem_init(&sem, 0, 1);
	alarm_cfg->user_data = &sem;

	now = k_cycle_get_32();
	err = counter_set_channel_alarm(counter_dev, ALARM_CH, alarm_cfg);
	if (err < 0) {
		printf("Failed to set the counter alarm.\n");
		return 0;
	}
	err = k_sem_take(&sem, timeout);
	if (err < 0) {
		printf("Failed waiting for counter alarm.\n");
		return 0;
	}

	return k_cycle_get_32() - now;
}

static void sys_event_irq_latency_run(enum rramc_mode mode)
{
	const struct device *counter = DEVICE_DT_GET(DT_NODELABEL(sample_counter));
	struct counter_alarm_cfg alarm_cfg;
	uint32_t delay = TIMEOUT_US;
	uint32_t delay_adj = 4;
	uint32_t rpt = 10;
	uint32_t cyc;
	int event_handle;
	const char *mode_str = (mode == RRAMC_DEFAULT) ? "default RRAMC mode" :
		(mode == RRAMC_PPI_WAKEUP) ? "RRAMC waken by PPI" : "RRAMC Standby mode";

	counter_start(counter);
	alarm_cfg.flags = 0;
	alarm_cfg.ticks = counter_us_to_ticks(counter, delay);
	alarm_cfg.callback = counter_handler;

	cyc = 0;
	for (int i = 0; i < rpt; i++) {
		sys_cache_instr_invd_all();
		if (mode != RRAMC_DEFAULT) {
			uint32_t t = (mode == RRAMC_PPI_WAKEUP) ? (delay + delay_adj) : 0;

			event_handle = nrf_sys_event_register(t, true);
			if (mode == RRAMC_PPI_WAKEUP && event_handle == 32) {
				printk("err\n");
			}
		}

		cyc += counter_alarm_execute(counter, &alarm_cfg, K_USEC(delay + 100));
		if (mode != RRAMC_DEFAULT) {
			(void)nrf_sys_event_unregister(event_handle, false);
		}
	}

	cyc /= rpt;
	printf("Alarm set for %d us, execution took:%d (%s)\n", delay, cyc, mode_str);

	counter_stop(counter);
}

static void sys_event_irq_latency(void)
{
	sys_event_irq_latency_run(RRAMC_DEFAULT);
	sys_event_irq_latency_run(RRAMC_POWER_MODE);
	sys_event_irq_latency_run(RRAMC_PPI_WAKEUP);
}
#endif /* CONFIG_NRF_SYS_EVENT_IRQ_LATENCY */

int main(void)
{
	printf("request global constant latency mode\n");
	if (nrf_sys_event_request_global_constlat()) {
		printf("failed to request global constant latency mode\n");
		return 0;
	}
	printf("constant latency mode enabled\n");

	printf("request global constant latency mode again\n");
	if (nrf_sys_event_request_global_constlat()) {
		printf("failed to request global constant latency mode\n");
		return 0;
	}

	printf("release global constant latency mode\n");
	printf("constant latency mode will remain enabled\n");
	if (nrf_sys_event_release_global_constlat()) {
		printf("failed to release global constant latency mode\n");
		return 0;
	}

	printf("release global constant latency mode again\n");
	printf("constant latency mode will be disabled\n");
	if (nrf_sys_event_release_global_constlat()) {
		printf("failed to release global constant latency mode\n");
		return 0;
	}

	printf("constant latency mode disabled\n");

#ifdef CONFIG_NRF_SYS_EVENT_IRQ_LATENCY
	sys_event_irq_latency();

	printf("All done\n");
#endif
	return 0;
}
