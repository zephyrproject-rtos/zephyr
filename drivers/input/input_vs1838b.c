/*
 * Copyright (c) 2025 Bastien Jauny <bastien.jauny@smile.fr>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vishay_vs1838b

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(input_vs1838b, CONFIG_INPUT_LOG_LEVEL);

/* A NEC packet is defined by:
 * - a lead burst (2 edges)
 * - an 8-bit address followed by its logical inverse
 * - an 8-bit command followed by its logical inverse
 * - a trailing burst
 */

/* Constants used for parsing the edges buffer for NEC protocol */
#define NEC_LEAD_PULSE_EDGE_OFFSET 0
#define NEC_LEAD_PULSE_EDGE_WIDTH  2

#define NEC_ADDRESS_BYTE_EDGE_OFFSET (NEC_LEAD_PULSE_EDGE_OFFSET + NEC_LEAD_PULSE_EDGE_WIDTH)
#define NEC_ADDRESS_BYTE_EDGE_WIDTH  (2 * BITS_PER_BYTE)

#define NEC_REVERSE_ADDRESS_BYTE_EDGE_OFFSET                                                       \
	(NEC_ADDRESS_BYTE_EDGE_OFFSET + NEC_ADDRESS_BYTE_EDGE_WIDTH)
#define NEC_REVERSE_ADDRESS_BYTE_EDGE_WIDTH (2 * BITS_PER_BYTE)

#define NEC_COMMAND_BYTE_EDGE_OFFSET                                                               \
	(NEC_REVERSE_ADDRESS_BYTE_EDGE_OFFSET + NEC_REVERSE_ADDRESS_BYTE_EDGE_WIDTH)
#define NEC_COMMAND_BYTE_EDGE_WIDTH (2 * BITS_PER_BYTE)

#define NEC_REVERSE_COMMAND_BYTE_EDGE_OFFSET                                                       \
	(NEC_COMMAND_BYTE_EDGE_OFFSET + NEC_COMMAND_BYTE_EDGE_WIDTH)
#define NEC_REVERSE_COMMAND_BYTE_EDGE_WIDTH (2 * BITS_PER_BYTE)

#define NEC_SINGLE_COMMAND_EDGES_COUNT                                                             \
	(NEC_REVERSE_COMMAND_BYTE_EDGE_OFFSET + NEC_REVERSE_COMMAND_BYTE_EDGE_WIDTH + 2)

/* NEC protocol values */
#define NEC_LEAD_PULSE_PERIOD_ON_USEC  9000
#define NEC_LEAD_PULSE_PERIOD_OFF_USEC 4500
#define NEC_BIT_DETECT_PERIOD_NSEC     562500
#define NEC_BIT_DETECT_PERIOD_USEC     (NEC_BIT_DETECT_PERIOD_NSEC / NSEC_PER_USEC)
#define NEC_BIT_0_TOTAL_PERIOD_USEC    1125
#define NEC_BIT_1_TOTAL_PERIOD_USEC    2250
/* Total delay between a command and a repeat code is 108ms
 * and total time of a command is 67.5ms
 */
#define NEC_TIMEOUT_REPEAT_CODE_MSEC   (108 - 67)

/* Macros to define tick ranges based on a millisecond tolerance */
#define VS1838B_MIN_TICK(usec, tol)                                                                \
	((((usec) - (tol)) * CONFIG_SYS_CLOCK_TICKS_PER_SEC) / USEC_PER_SEC)
#define VS1838B_MAX_TICK(usec, tol)                                                                \
	((((usec) + (tol)) * CONFIG_SYS_CLOCK_TICKS_PER_SEC) / USEC_PER_SEC)

/* Empiric tolerance values. Might be a good idea to put them in the Kconfig? */
#define VS1838B_NEC_LEAD_PULSE_PERIOD_TOLERANCE_USEC 400
#define VS1838B_NEC_BIT_DETECT_PERIOD_TOLERANCE_USEC 150
#define VS1838B_NEC_BIT_0_TOTAL_TOLERANCE_USEC       200
#define VS1838B_NEC_BIT_1_TOTAL_TOLERANCE_USEC       200

/* Tick ranges for the NEC elements */
#define VS1838B_NEC_LEAD_PULSE_ON_MIN_TICK                                                         \
	VS1838B_MIN_TICK(NEC_LEAD_PULSE_PERIOD_ON_USEC,                                            \
			 VS1838B_NEC_LEAD_PULSE_PERIOD_TOLERANCE_USEC)
#define VS1838B_NEC_LEAD_PULSE_ON_MAX_TICK                                                         \
	VS1838B_MAX_TICK(NEC_LEAD_PULSE_PERIOD_ON_USEC,                                            \
			 VS1838B_NEC_LEAD_PULSE_PERIOD_TOLERANCE_USEC)

#define VS1838B_NEC_LEAD_PULSE_OFF_MIN_TICK                                                        \
	VS1838B_MIN_TICK(NEC_LEAD_PULSE_PERIOD_OFF_USEC,                                           \
			 VS1838B_NEC_LEAD_PULSE_PERIOD_TOLERANCE_USEC)
#define VS1838B_NEC_LEAD_PULSE_OFF_MAX_TICK                                                        \
	VS1838B_MAX_TICK(NEC_LEAD_PULSE_PERIOD_OFF_USEC,                                           \
			 VS1838B_NEC_LEAD_PULSE_PERIOD_TOLERANCE_USEC)

#define VS1838B_NEC_BIT_DETECT_MIN_TICK                                                            \
	VS1838B_MIN_TICK(NEC_BIT_DETECT_PERIOD_USEC, VS1838B_NEC_BIT_DETECT_PERIOD_TOLERANCE_USEC)
#define VS1838B_NEC_BIT_DETECT_MAX_TICK                                                            \
	VS1838B_MAX_TICK(NEC_BIT_DETECT_PERIOD_USEC, VS1838B_NEC_BIT_DETECT_PERIOD_TOLERANCE_USEC)

#define VS1838B_NEC_BIT_0_TOTAL_MIN_TICK                                                           \
	VS1838B_MIN_TICK(NEC_BIT_0_TOTAL_PERIOD_USEC, VS1838B_NEC_BIT_0_TOTAL_TOLERANCE_USEC)
#define VS1838B_NEC_BIT_0_TOTAL_MAX_TICK                                                           \
	VS1838B_MAX_TICK(NEC_BIT_0_TOTAL_PERIOD_USEC, VS1838B_NEC_BIT_0_TOTAL_TOLERANCE_USEC)

#define VS1838B_NEC_BIT_1_TOTAL_MIN_TICK                                                           \
	VS1838B_MIN_TICK(NEC_BIT_1_TOTAL_PERIOD_USEC, VS1838B_NEC_BIT_1_TOTAL_TOLERANCE_USEC)
#define VS1838B_NEC_BIT_1_TOTAL_MAX_TICK                                                           \
	VS1838B_MAX_TICK(NEC_BIT_1_TOTAL_PERIOD_USEC, VS1838B_NEC_BIT_1_TOTAL_TOLERANCE_USEC)

struct vs1838b_data {
	struct device const *dev;
	struct gpio_callback input_cb;
	struct k_work_delayable decode_work;
	int64_t edges_ticks[NEC_SINGLE_COMMAND_EDGES_COUNT];
	uint8_t edges_count;
	struct k_sem decode_sem;
};

struct vs1838b_config {
	struct gpio_dt_spec input;
};

static inline bool is_within_range(k_ticks_t const ticks, k_ticks_t const min, k_ticks_t const max)
{
	return (ticks <= max) && (ticks >= min);
}

static bool read_byte_from(int64_t *const edges_ticks, uint8_t const offset, uint8_t *byte)
{
	/* Make sure we add bits from 0 */
	uint8_t temp_byte = 0;
	k_ticks_t ticks_on;
	k_ticks_t ticks_total;

	/* Bytes are transmitted LSB first */
	for (uint8_t i = 0; i < BITS_PER_BYTE; ++i) {
		/*
		 * To detect bits and their values we analyze:
		 * - the initial pulse width
		 * - the total period
		 */
		ticks_on = edges_ticks[(2 * i) + offset + 1] - edges_ticks[(2 * i) + offset];
		ticks_total = edges_ticks[(2 * i) + offset + 2] - edges_ticks[(2 * i) + offset];

		LOG_DBG("ticks_on %lld", ticks_on);
		LOG_DBG("ticks_total %lld", ticks_total);
		if (is_within_range(ticks_on, VS1838B_NEC_BIT_DETECT_MIN_TICK,
				    VS1838B_NEC_BIT_DETECT_MAX_TICK)) {
			if (is_within_range(ticks_total, VS1838B_NEC_BIT_0_TOTAL_MIN_TICK,
					    VS1838B_NEC_BIT_0_TOTAL_MAX_TICK)) {
				/* 0 detected */
			} else if (is_within_range(ticks_total, VS1838B_NEC_BIT_1_TOTAL_MIN_TICK,
						   VS1838B_NEC_BIT_1_TOTAL_MAX_TICK)) {
				/* 1 detected */
				temp_byte += BIT(i);
			} else {
				LOG_WRN("Failed to identify detected bit at position %u", i);
				return false;
			}
		} else {
			LOG_WRN("Failed to detect a valid bit at position %u", i);
			return false;
		}
	}

	*byte = temp_byte;
	return true;
}

static bool detect_leading_burst(int64_t *const edges_ticks)
{
	/* Detect leading pulse using the first 3 edges */
	int64_t lead_ticks_on = edges_ticks[NEC_LEAD_PULSE_EDGE_OFFSET + 1] -
				edges_ticks[NEC_LEAD_PULSE_EDGE_OFFSET];
	int64_t lead_ticks_off = edges_ticks[NEC_LEAD_PULSE_EDGE_OFFSET + 2] -
				 edges_ticks[NEC_LEAD_PULSE_EDGE_OFFSET + 1];

	/* Manage the corner case of an overflow */
	if ((lead_ticks_on < 0) || (lead_ticks_off < 0)) {
		LOG_ERR("Ticks overflow: %lld - %lld - %lld",
			edges_ticks[NEC_LEAD_PULSE_EDGE_OFFSET],
			edges_ticks[NEC_LEAD_PULSE_EDGE_OFFSET + 1],
			edges_ticks[NEC_LEAD_PULSE_EDGE_OFFSET + 2]);
		return false;
	}

	LOG_DBG("Read %lld ticks on and %lld ticks off", lead_ticks_on, lead_ticks_off);

	return is_within_range(lead_ticks_on, VS1838B_NEC_LEAD_PULSE_ON_MIN_TICK,
			       VS1838B_NEC_LEAD_PULSE_ON_MAX_TICK) &&
	       is_within_range(lead_ticks_off, VS1838B_NEC_LEAD_PULSE_OFF_MIN_TICK,
			       VS1838B_NEC_LEAD_PULSE_OFF_MAX_TICK);
}

static bool read_redundant_byte(int64_t *const edges_ticks, uint8_t *const byte,
				uint32_t const offset)
{
	uint8_t temp_byte;
	uint8_t reverse_byte;

	if (read_byte_from(edges_ticks, offset, &temp_byte) &&
	    read_byte_from(edges_ticks, offset + (2 * BITS_PER_BYTE), &reverse_byte)) {
		if (temp_byte == (uint8_t)(~reverse_byte)) {
			*byte = temp_byte;
		} else {
			LOG_ERR("Error while decoding byte");
			return false;
		}
	} else {
		LOG_ERR("Error while reading bytes");
		return false;
	}

	return true;
}

static bool read_address_byte(int64_t *const edges_ticks, uint8_t *const address)
{
	return read_redundant_byte(edges_ticks, address, NEC_ADDRESS_BYTE_EDGE_OFFSET);
}

static bool read_command_byte(int64_t *const edges_ticks, uint8_t *const command)
{
	return read_redundant_byte(edges_ticks, command, NEC_COMMAND_BYTE_EDGE_OFFSET);
}

static bool detect_last_burst(int64_t *const edges_ticks)
{
	/* Detect leading pulse using the last 3 edges */
	int64_t burst_length = edges_ticks[NEC_SINGLE_COMMAND_EDGES_COUNT - 1] -
			       edges_ticks[NEC_SINGLE_COMMAND_EDGES_COUNT - 2];

	/* Manage the corner case of an overflow */
	if (burst_length < 0) {
		LOG_ERR("Ticks overflow: %lld - %lld",
			edges_ticks[NEC_SINGLE_COMMAND_EDGES_COUNT - 1],
			edges_ticks[NEC_SINGLE_COMMAND_EDGES_COUNT - 2]);
		return false;
	}

	LOG_DBG("Read %lld ticks in the last burst", burst_length);

	return is_within_range(burst_length, VS1838B_NEC_BIT_DETECT_MIN_TICK,
			       VS1838B_NEC_BIT_DETECT_MAX_TICK);
}

static bool get_address_and_command(int64_t *const edges_ticks, uint8_t *const address,
				    uint8_t *const command)
{
	if (!detect_leading_burst(edges_ticks)) {
		LOG_DBG("No lead detected");
		return false;
	}

	if (!read_address_byte(edges_ticks, address)) {
		LOG_DBG("No address decoded");
		return false;
	}

	if (!read_command_byte(edges_ticks, command)) {
		LOG_DBG("No command decoded");
		return false;
	}
	if (!detect_last_burst(edges_ticks)) {
		LOG_DBG("No trailing edge detected");
		return false;
	}

	return true;
}

/*
 * Management of the decoding
 */
static void vs1838b_decode_work_handler(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	struct vs1838b_data *data = CONTAINER_OF(dwork, struct vs1838b_data, decode_work);

	if (k_sem_take(&data->decode_sem, K_FOREVER) == 0) {
		uint8_t address_byte;
		uint8_t command_byte;

		if (get_address_and_command(data->edges_ticks, &address_byte, &command_byte)) {
			LOG_DBG("Address: [0x%X] | Command: [0x%X]", address_byte, command_byte);
			if (input_report(data->dev, INPUT_EV_DEVICE, INPUT_MSC_SCAN,
					 (address_byte << 8) | command_byte, true, K_FOREVER) < 0) {
				LOG_ERR("Message failed to be enqueued");
			}
		}
	}

	/* Reset the record */
	data->edges_count = 0;
	k_sem_give(&data->decode_sem);
}

/*
 * Internal callback
 */
static void vs1838b_input_callback(struct device const *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	/*
	 * We want to:
	 * - register the timestamps of interrupts
	 * - try and decode the received bits when we reach the appropriate threshold
	 */
	int64_t const tick = k_uptime_ticks();
	struct vs1838b_data *data = CONTAINER_OF(cb, struct vs1838b_data, input_cb);

	/* If we already schedule a decode, we need to cancel it. */
	if (k_work_cancel_delayable(&data->decode_work) != 0) {
		LOG_WRN("Decoding not cancelled!");
	}

	if (k_sem_take(&data->decode_sem, K_NO_WAIT) != 0) {
		/* Decoding might be pending */
		return;
	}

	/* If more interrupts are received, they're likely to be repeat codes
	 * and we choose to ignore them.
	 */
	if (data->edges_count < NEC_SINGLE_COMMAND_EDGES_COUNT) {
		data->edges_ticks[data->edges_count++] = tick;
	}

	/* If the first 3 edges do not match a leading burst,
	 * shift left the edges_ticks to get rid of leading noises.
	 */
	if ((data->edges_count == 3) && !detect_leading_burst(data->edges_ticks)) {
		data->edges_ticks[0] = data->edges_ticks[1];
		data->edges_ticks[1] = data->edges_ticks[2];
		data->edges_count = 2;
	}

	if (data->edges_count == NEC_SINGLE_COMMAND_EDGES_COUNT) {
		/* There's a candidate!
		 * If nothing gets in during the grace period
		 * it *should* be an entire command.
		 */
		k_work_schedule(&data->decode_work, K_MSEC(NEC_TIMEOUT_REPEAT_CODE_MSEC));
	}
	k_sem_give(&data->decode_sem);
}

static int vs1838b_init(struct device const *dev)
{
	struct vs1838b_config const *config = dev->config;
	struct gpio_dt_spec const *data_input = &config->input;
	struct vs1838b_data *data = dev->data;

	data->dev = dev;

	if (!gpio_is_ready_dt(data_input)) {
		LOG_ERR("GPIO input pin is not ready");
		return -ENODEV;
	}

	/*
	 * Setup the input as an interrupt source
	 * and register an associated callback.
	 */
	gpio_pin_configure_dt(data_input, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(data_input, GPIO_INT_EDGE_BOTH);
	gpio_init_callback(&data->input_cb, vs1838b_input_callback, BIT(data_input->pin));
	gpio_add_callback_dt(data_input, &data->input_cb);

	k_sem_init(&data->decode_sem, 1, 1);
	k_work_init_delayable(&data->decode_work, vs1838b_decode_work_handler);

	return 0;
}

#define VS1838B_DEFINE(inst)                                                                       \
	static struct vs1838b_data vs1838b_data_##inst;                                            \
                                                                                                   \
	static struct vs1838b_config const vs1838b_config_##inst = {                               \
		.input = GPIO_DT_SPEC_INST_GET(inst, data_gpios),                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, vs1838b_init, NULL, &vs1838b_data_##inst,                      \
			      &vs1838b_config_##inst, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,     \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(VS1838B_DEFINE)
