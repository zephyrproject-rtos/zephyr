/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_s32_psi5_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/drivers/psi5/psi5.h>

#define PSI5_NODE DT_INST(0, nxp_s32_psi5)

/* Use pin B2 for simulate PSI5 signal from sensor to board */

#define GPIO_BASE     0x40521700
#define MSRC_BASE     0x40520280
#define GPIO_PIN      2
#define GPIO_SET(val) sys_write16(BIT(15 - val), GPIO_BASE)

void timing_delay_2_5_us(void)
{
	uint64_t start_cycles = arm_arch_timer_count();
	uint64_t cycles_to_wait = 20;

	for (;;) {
		uint64_t current_cycles = arm_arch_timer_count();

		if ((current_cycles - start_cycles) >= cycles_to_wait) {
			break;
		}
	}
}

/* Simulate a PSI5 message with 2 start bits, 16 data bits (0xAD2C), and 3 CRC bits (0x4). */
void data_sensor_to_board_simulation(void)
{
	int key;

	sys_write32(BIT(21), MSRC_BASE + GPIO_PIN * 4);
	key = irq_lock();

	GPIO_SET(0);
	timing_delay_2_5_us();
	timing_delay_2_5_us();
	timing_delay_2_5_us();
	timing_delay_2_5_us();

	/* s1, s2 */
	GPIO_SET(0);
	timing_delay_2_5_us();
	GPIO_SET(GPIO_PIN);
	timing_delay_2_5_us();

	GPIO_SET(0);
	timing_delay_2_5_us();
	GPIO_SET(GPIO_PIN);
	timing_delay_2_5_us();

	/* 0011 */
	GPIO_SET(0);
	timing_delay_2_5_us();
	GPIO_SET(GPIO_PIN);
	timing_delay_2_5_us();

	GPIO_SET(0);
	timing_delay_2_5_us();
	GPIO_SET(GPIO_PIN);
	timing_delay_2_5_us();

	GPIO_SET(GPIO_PIN);
	timing_delay_2_5_us();
	GPIO_SET(0);
	timing_delay_2_5_us();

	GPIO_SET(GPIO_PIN);
	timing_delay_2_5_us();
	GPIO_SET(0);
	timing_delay_2_5_us();

	/* 0100 */
	GPIO_SET(0);
	timing_delay_2_5_us();
	GPIO_SET(GPIO_PIN);
	timing_delay_2_5_us();

	GPIO_SET(GPIO_PIN);
	timing_delay_2_5_us();
	GPIO_SET(0);
	timing_delay_2_5_us();

	GPIO_SET(0);
	timing_delay_2_5_us();
	GPIO_SET(GPIO_PIN);
	timing_delay_2_5_us();

	GPIO_SET(0);
	timing_delay_2_5_us();
	GPIO_SET(GPIO_PIN);
	timing_delay_2_5_us();

	/* 1011 */
	GPIO_SET(GPIO_PIN);
	timing_delay_2_5_us();
	GPIO_SET(0);
	timing_delay_2_5_us();

	GPIO_SET(0);
	timing_delay_2_5_us();
	GPIO_SET(GPIO_PIN);
	timing_delay_2_5_us();

	GPIO_SET(GPIO_PIN);
	timing_delay_2_5_us();
	GPIO_SET(0);
	timing_delay_2_5_us();

	GPIO_SET(GPIO_PIN);
	timing_delay_2_5_us();
	GPIO_SET(0);
	timing_delay_2_5_us();

	/* 0101 */
	GPIO_SET(0);
	timing_delay_2_5_us();
	GPIO_SET(GPIO_PIN);
	timing_delay_2_5_us();

	GPIO_SET(GPIO_PIN);
	timing_delay_2_5_us();
	GPIO_SET(0);
	timing_delay_2_5_us();

	GPIO_SET(0);
	timing_delay_2_5_us();
	GPIO_SET(GPIO_PIN);
	timing_delay_2_5_us();

	GPIO_SET(GPIO_PIN);
	timing_delay_2_5_us();
	GPIO_SET(0);
	timing_delay_2_5_us();

	/* CRC */
	GPIO_SET(GPIO_PIN);
	timing_delay_2_5_us();
	GPIO_SET(0);
	timing_delay_2_5_us();

	GPIO_SET(0);
	timing_delay_2_5_us();
	GPIO_SET(GPIO_PIN);
	timing_delay_2_5_us();

	GPIO_SET(0);
	timing_delay_2_5_us();
	GPIO_SET(GPIO_PIN);
	timing_delay_2_5_us();

	GPIO_SET(0);
	irq_unlock(key);
}

void tx_cb(const struct device *dev, uint8_t channel_id, enum psi5_state state, void *user_data)
{
	LOG_INF("Tx channel %d completed\n", channel_id);
}

void rx_cb(const struct device *dev, uint8_t channel_id, struct psi5_frame *frame,
	   enum psi5_state state, void *user_data)
{
	if (state == PSI5_STATE_MSG_RECEIVED) {
		LOG_INF("Rx channel %d completed data (0x%x)\n", channel_id, frame->msg.data);
	} else {
		LOG_INF("Rx channel %d err (0x%x)\n", channel_id, state);
	}
}

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(PSI5_NODE);
	uint64_t send_data = 0x1234;

	/* test receive data */
	psi5_add_rx_callback(dev, 1, rx_cb, NULL);

	psi5_start(dev, 1);

	/* delay generate data after pusle sync */
	k_busy_wait(100);

	data_sensor_to_board_simulation();

	k_sleep(K_MSEC(1));

	psi5_stop(dev, 1);

	/* test send data */
	psi5_start(dev, 2);

	psi5_send(dev, 2, send_data, K_MSEC(100), tx_cb, NULL);

	return 0;
}
