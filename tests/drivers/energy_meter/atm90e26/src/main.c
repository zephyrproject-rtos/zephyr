/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/energy_meter.h>
#include <zephyr/drivers/energy_meter/atm90e26.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/ztest.h>

#ifdef CONFIG_SHELL_BACKEND_DUMMY
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#endif

#include "atm90e26.h"
#include "emul_atm90e26.h"

#define SPI_NODE      DT_NODELABEL(meter_spi)
#define UART_NODE     DT_NODELABEL(meter_uart)
#define DEFERRED_NODE DT_NODELABEL(meter_deferred)

#define IRQ_PIN DT_GPIO_PIN(SPI_NODE, irq_gpios)
#define CF1_PIN DT_GPIO_PIN(SPI_NODE, cf1_gpios)

#define LINE_A     ENERGY_METER_LINE_A
#define LINE_N     ENERGY_METER_LINE_N
#define LINE_TOTAL ENERGY_METER_LINE_TOTAL

struct meter {
	const char *name;
	const struct device *dev;
	const struct emul *emul;
};

static const struct meter meters[] = {
	{"spi", DEVICE_DT_GET(SPI_NODE), EMUL_DT_GET(SPI_NODE)},
	{"uart", DEVICE_DT_GET(UART_NODE), EMUL_DT_GET(UART_NODE)},
};

static int64_t get(const struct meter *m, uint8_t quantity, uint8_t line)
{
	int64_t value;

	zassert_ok(energy_meter_get_one(m->dev, quantity, line, &value), "%s quantity %u line %u",
		   m->name, quantity, line);

	return value;
}

ZTEST(atm90e26, test_init_applies_devicetree)
{
	static const uint16_t meter_cal[ATM90E26_METER_CAL_LEN] = {
		0x0015, 0xD175, 0x1234, 0, 0, 0, 0x08BD, 0, 0x0AEC, 0, 0x9423,
	};
	static const uint16_t meas_cal[ATM90E26_MEAS_CAL_LEN] = {
		0x6000, 0x7A13, 0x7530, 0, 0, 0, 0, 0, 0, 0x0042,
	};
	const struct emul *emul = EMUL_DT_GET(SPI_NODE);

	zassert_true(atm90e26_emul_reset_count(emul) >= 1U, "no soft reset at init");
	zassert_equal(atm90e26_emul_get_reg(emul, ATM90E26_REG_FUNC_EN), 0x0008);

	for (size_t i = 0; i < ARRAY_SIZE(meter_cal); i++) {
		zassert_equal(atm90e26_emul_get_reg(emul, ATM90E26_REG_PL_CONST_H + i),
			      meter_cal[i], "metering register 0x%02x",
			      ATM90E26_REG_PL_CONST_H + i);
	}

	for (size_t i = 0; i < ARRAY_SIZE(meas_cal); i++) {
		zassert_equal(atm90e26_emul_get_reg(emul, ATM90E26_REG_U_GAIN + i), meas_cal[i],
			      "measurement register 0x%02x", ATM90E26_REG_U_GAIN + i);
	}

	zassert_equal(atm90e26_emul_get_reg(emul, ATM90E26_REG_CS1),
		      atm90e26_checksum(meter_cal, ARRAY_SIZE(meter_cal)));
	zassert_equal(atm90e26_emul_get_reg(emul, ATM90E26_REG_CS2),
		      atm90e26_checksum(meas_cal, ARRAY_SIZE(meas_cal)));
	zassert_equal(atm90e26_emul_get_reg(emul, ATM90E26_REG_CAL_START), ATM90E26_CAL_CHECK_CMD);
	zassert_equal(atm90e26_emul_get_reg(emul, ATM90E26_REG_ADJ_START), ATM90E26_CAL_CHECK_CMD);

	/* The UART instance went through the same sequence */
	emul = EMUL_DT_GET(UART_NODE);
	zassert_equal(atm90e26_emul_get_reg(emul, ATM90E26_REG_MMODE), 0x9423);
	zassert_equal(atm90e26_emul_get_reg(emul, ATM90E26_REG_U_GAIN), 0x6000);
	zassert_equal(atm90e26_emul_get_reg(emul, ATM90E26_REG_ADJ_START), ATM90E26_CAL_CHECK_CMD);
}

ZTEST(atm90e26, test_checksum_of_power_on_values)
{
	/* Power-on values of 21h-2Bh: byte sum 0x3CB, byte XOR 0x55 */
	static const uint16_t regs[] = {
		0x0015, 0xD174, 0, 0, 0, 0, 0x08BD, 0, 0x0AEC, 0, 0x9422,
	};

	zassert_equal(atm90e26_checksum(regs, ARRAY_SIZE(regs)), 0x55CB);
}

ZTEST(atm90e26, test_info)
{
	struct energy_meter_info info;

	zassert_ok(energy_meter_get_info(meters[0].dev, &info));
	zassert_equal(info.phases, 1);
	zassert_true((info.quantities[LINE_A] & BIT(ENERGY_METER_VOLTAGE)) != 0U);
	zassert_true((info.quantities[LINE_N] & BIT(ENERGY_METER_VOLTAGE)) == 0U);
	zassert_true((info.quantities[LINE_N] & BIT(ENERGY_METER_ACTIVE_POWER)) != 0U);
	zassert_true((info.quantities[LINE_TOTAL] & BIT(ENERGY_METER_FREQUENCY)) != 0U);
	zassert_true((info.quantities[LINE_TOTAL] & BIT(ENERGY_METER_ACTIVE_ENERGY_IMPORT)) != 0U);
	zassert_equal(info.quantities[ENERGY_METER_LINE_B], 0U);
	zassert_equal(info.quantities[ENERGY_METER_LINE_C], 0U);
}

ZTEST(atm90e26, test_measurements)
{
	ARRAY_FOR_EACH_PTR(meters, m) {
		atm90e26_emul_set_reg(m->emul, ATM90E26_REG_URMS, 23012);
		zassert_equal(get(m, ENERGY_METER_VOLTAGE, LINE_A), 230120000, "%s", m->name);

		atm90e26_emul_set_reg(m->emul, ATM90E26_REG_IRMS2, 5123);
		zassert_equal(get(m, ENERGY_METER_CURRENT, LINE_N), 5123000, "%s", m->name);

		atm90e26_emul_set_reg(m->emul, ATM90E26_REG_PMEAN, (uint16_t)-1500);
		zassert_equal(get(m, ENERGY_METER_ACTIVE_POWER, LINE_A), -1500000000LL, "%s",
			      m->name);

		atm90e26_emul_set_reg(m->emul, ATM90E26_REG_SMEAN2, 2300);
		zassert_equal(get(m, ENERGY_METER_APPARENT_POWER, LINE_N), 2300000000LL, "%s",
			      m->name);

		atm90e26_emul_set_reg(m->emul, ATM90E26_REG_FREQ, 5002);
		zassert_equal(get(m, ENERGY_METER_FREQUENCY, LINE_TOTAL), 50020000, "%s", m->name);
	}
}

ZTEST(atm90e26, test_power_factor_and_angle_are_sign_magnitude)
{
	ARRAY_FOR_EACH_PTR(meters, m) {
		/* Sign bit set, magnitude 500: -0.500 */
		atm90e26_emul_set_reg(m->emul, ATM90E26_REG_POWERF, 0x81F4);
		zassert_equal(get(m, ENERGY_METER_POWER_FACTOR, LINE_A), -500000, "%s", m->name);

		atm90e26_emul_set_reg(m->emul, ATM90E26_REG_POWERF2, 0x03E8);
		zassert_equal(get(m, ENERGY_METER_POWER_FACTOR, LINE_N), 1000000, "%s", m->name);

		/* Sign bit set, magnitude 900: -90.0 degrees */
		atm90e26_emul_set_reg(m->emul, ATM90E26_REG_PANGLE, 0x8384);
		zassert_equal(get(m, ENERGY_METER_PHASE_ANGLE, LINE_A), -90000000, "%s", m->name);

		/* -180.0 degrees is the same angle as 180.0, the end of the API range */
		atm90e26_emul_set_reg(m->emul, ATM90E26_REG_PANGLE, 0x8708);
		zassert_equal(get(m, ENERGY_METER_PHASE_ANGLE, LINE_A), 180000000, "%s", m->name);
	}
}

ZTEST(atm90e26, test_energy_accumulates_across_reads)
{
	ARRAY_FOR_EACH_PTR(meters, m) {
		int64_t before = get(m, ENERGY_METER_ACTIVE_ENERGY_IMPORT, LINE_TOTAL);

		/* 32 counts of 0.1 CF at 3200 imp/kWh is 1 Wh */
		atm90e26_emul_set_reg(m->emul, ATM90E26_REG_AP_ENERGY, 32);
		zassert_equal(get(m, ENERGY_METER_ACTIVE_ENERGY_IMPORT, LINE_TOTAL) - before,
			      1000000, "%s", m->name);
		zassert_equal(atm90e26_emul_get_reg(m->emul, ATM90E26_REG_AP_ENERGY), 0,
			      "%s: energy register not cleared by the read", m->name);

		atm90e26_emul_set_reg(m->emul, ATM90E26_REG_AP_ENERGY, 16);
		zassert_equal(get(m, ENERGY_METER_ACTIVE_ENERGY_IMPORT, LINE_TOTAL) - before,
			      1500000, "%s", m->name);
	}
}

ZTEST(atm90e26, test_every_energy_value_has_its_own_register)
{
	static const uint8_t quantities[ATM90E26_ENERGY_COUNT] = {
		ENERGY_METER_ACTIVE_ENERGY_IMPORT,       ENERGY_METER_ACTIVE_ENERGY_EXPORT,
		ENERGY_METER_ATM90E26_ACTIVE_ENERGY_ABS, ENERGY_METER_REACTIVE_ENERGY_IMPORT,
		ENERGY_METER_REACTIVE_ENERGY_EXPORT,     ENERGY_METER_ATM90E26_REACTIVE_ENERGY_ABS,
	};

	ARRAY_FOR_EACH_PTR(meters, m) {
		int64_t before[ATM90E26_ENERGY_COUNT];

		for (int i = 0; i < ATM90E26_ENERGY_COUNT; i++) {
			before[i] = get(m, quantities[i], LINE_TOTAL);
			atm90e26_emul_set_reg(m->emul, ATM90E26_REG_AP_ENERGY + i, 32 * (i + 1));
		}

		for (int i = 0; i < ATM90E26_ENERGY_COUNT; i++) {
			zassert_equal(get(m, quantities[i], LINE_TOTAL) - before[i],
				      1000000 * (i + 1), "%s energy %d", m->name, i);
		}
	}
}

ZTEST(atm90e26, test_unsupported_channels)
{
	int64_t value;

	zassert_equal(energy_meter_get_one(meters[0].dev, ENERGY_METER_VOLTAGE, ENERGY_METER_LINE_B,
					   &value),
		      -ENOTSUP);
	zassert_equal(
		energy_meter_get_one(meters[0].dev, ENERGY_METER_TEMPERATURE, LINE_TOTAL, &value),
		-ENOTSUP);
}

ZTEST(atm90e26, test_snapshot)
{
	struct energy_meter_snapshot snap;

	atm90e26_emul_set_reg(meters[1].emul, ATM90E26_REG_URMS, 22000);
	atm90e26_emul_set_reg(meters[1].emul, ATM90E26_REG_IRMS2, 250);

	zassert_ok(energy_meter_get_snapshot(meters[1].dev, &snap));
	zassert_equal(snap.phases, 1);
	zassert_equal(snap.line[LINE_A].voltage, 220000000);
	zassert_equal(snap.line[LINE_N].current, 250000);
	zassert_equal(snap.valid[ENERGY_METER_LINE_B], 0U);
}

ZTEST(atm90e26, test_reset_energy)
{
	ARRAY_FOR_EACH_PTR(meters, m) {
		atm90e26_emul_set_reg(m->emul, ATM90E26_REG_RP_ENERGY, 64);
		zassert_true(get(m, ENERGY_METER_REACTIVE_ENERGY_IMPORT, LINE_TOTAL) > 0);

		atm90e26_emul_set_reg(m->emul, ATM90E26_REG_RP_ENERGY, 64);
		zassert_ok(energy_meter_reset_energy(m->dev));
		zassert_equal(atm90e26_emul_get_reg(m->emul, ATM90E26_REG_RP_ENERGY), 0);
		zassert_equal(get(m, ENERGY_METER_REACTIVE_ENERGY_IMPORT, LINE_TOTAL), 0, "%s",
			      m->name);
	}
}

ZTEST(atm90e26, test_sys_status_is_cleared_by_read)
{
	ARRAY_FOR_EACH_PTR(meters, m) {
		atm90e26_emul_set_reg(m->emul, ATM90E26_REG_SYS_STATUS, 0x0082);

		zassert_equal(get(m, ENERGY_METER_ATM90E26_SYS_STATUS, LINE_TOTAL), 0x0082, "%s",
			      m->name);
		zassert_equal(atm90e26_emul_get_reg(m->emul, ATM90E26_REG_SYS_STATUS), 0);
	}
}

ZTEST(atm90e26, test_corrupted_uart_read_keeps_energy)
{
	const struct meter *m = &meters[1];
	int64_t before = get(m, ENERGY_METER_ATM90E26_REACTIVE_ENERGY_ABS, LINE_TOTAL);
	int64_t value;

	/* The UART checksum catches the corruption without read verification */
	atm90e26_emul_set_reg(m->emul, ATM90E26_REG_RT_ENERGY, 64);
	atm90e26_emul_corrupt_reads(m->emul, 1);
	zassert_equal(get(m, ENERGY_METER_ATM90E26_REACTIVE_ENERGY_ABS, LINE_TOTAL) - before,
		      2000000);

	/* With every copy corrupted the read fails instead of returning garbage */
	atm90e26_emul_corrupt_reads(m->emul, 3);
	zassert_equal(energy_meter_get_one(m->dev, ENERGY_METER_FREQUENCY, LINE_TOTAL, &value),
		      -EIO);
}

ZTEST(atm90e26, test_corrupted_spi_read_keeps_energy)
{
	const struct meter *m = &meters[0];
	int64_t before = get(m, ENERGY_METER_ACTIVE_ENERGY_EXPORT, LINE_TOTAL);

	Z_TEST_SKIP_IFNDEF(CONFIG_ATM90E26_VERIFY_READ);

	atm90e26_emul_set_reg(m->emul, ATM90E26_REG_AN_ENERGY, 64);
	atm90e26_emul_corrupt_reads(m->emul, 1);
	zassert_equal(get(m, ENERGY_METER_ACTIVE_ENERGY_EXPORT, LINE_TOTAL) - before, 2000000);
}

ZTEST(atm90e26, test_uart_timeout)
{
	const struct meter *m = &meters[1];
	int64_t value;

	atm90e26_emul_set_present(m->emul, false);
	zassert_equal(energy_meter_get_one(m->dev, ENERGY_METER_VOLTAGE, LINE_A, &value),
		      -ETIMEDOUT);
	atm90e26_emul_set_present(m->emul, true);
	zassert_ok(energy_meter_get_one(m->dev, ENERGY_METER_VOLTAGE, LINE_A, &value));
}

ZTEST(atm90e26, test_init_failures)
{
	const struct device *dev = DEVICE_DT_GET(DEFERRED_NODE);
	const struct emul *emul = EMUL_DT_GET(DEFERRED_NODE);

	zassert_false(device_is_ready(dev));

	atm90e26_emul_set_present(emul, false);
	zassert_equal(device_init(dev), -ENODEV);
	atm90e26_emul_set_present(emul, true);

	/*
	 * The unverified soft reset takes the first dropped write; the next three exhaust the
	 * retries of the first verified write.
	 */
	atm90e26_emul_drop_writes(emul, 4);
	zassert_equal(device_init(dev), -EIO);

	/* Two dropped writes are recovered by retrying */
	atm90e26_emul_drop_writes(emul, 2);
	zassert_ok(device_init(dev));
	zassert_true(device_is_ready(dev));
	zassert_equal(device_init(dev), -EALREADY);
}

ZTEST(atm90e26, test_current_scale)
{
	const struct device *scaled = DEVICE_DT_GET(DT_NODELABEL(meter_scaled));
	const struct emul *scaled_emul = EMUL_DT_GET(DT_NODELABEL(meter_scaled));
	int64_t before;
	int64_t value;

	/* 60 A read by the chip is 600 A, beyond the 65.535 A register range */
	atm90e26_emul_set_reg(scaled_emul, ATM90E26_REG_IRMS, 60000);
	zassert_ok(energy_meter_get_one(scaled, ENERGY_METER_CURRENT, LINE_A, &value));
	zassert_equal(value, 600000000);

	/* 13.8 kW read by the chip is 138 kW, beyond the 32.767 kW register range */
	atm90e26_emul_set_reg(scaled_emul, ATM90E26_REG_PMEAN, 13800);
	zassert_ok(energy_meter_get_one(scaled, ENERGY_METER_ACTIVE_POWER, LINE_A, &value));
	zassert_equal(value, 138000000000LL);

	atm90e26_emul_set_reg(scaled_emul, ATM90E26_REG_QMEAN2, (uint16_t)-1500);
	zassert_ok(energy_meter_get_one(scaled, ENERGY_METER_REACTIVE_POWER, LINE_N, &value));
	zassert_equal(value, -15000000000LL);

	/* 1 Wh counted by the chip is 10 Wh */
	zassert_ok(energy_meter_get_one(scaled, ENERGY_METER_ACTIVE_ENERGY_IMPORT, LINE_TOTAL,
					&before));
	atm90e26_emul_set_reg(scaled_emul, ATM90E26_REG_AP_ENERGY, 32);
	zassert_ok(energy_meter_get_one(scaled, ENERGY_METER_ACTIVE_ENERGY_IMPORT, LINE_TOTAL,
					&value));
	zassert_equal(value - before, 10000000);

	/* Voltage and power factor do not depend on the current calibration */
	atm90e26_emul_set_reg(scaled_emul, ATM90E26_REG_URMS, 23012);
	zassert_ok(energy_meter_get_one(scaled, ENERGY_METER_VOLTAGE, LINE_A, &value));
	zassert_equal(value, 230120000);

	atm90e26_emul_set_reg(scaled_emul, ATM90E26_REG_POWERF, 0x81F4);
	zassert_ok(energy_meter_get_one(scaled, ENERGY_METER_POWER_FACTOR, LINE_A, &value));
	zassert_equal(value, -500000);
}

struct event_record {
	struct k_sem sem;
	const struct device *dev;
	struct energy_meter_event event;
};

static void event_cb(const struct device *dev, const struct energy_meter_event *event,
		     void *user_data)
{
	struct event_record *rec = user_data;

	rec->dev = dev;
	rec->event = *event;
	k_sem_give(&rec->sem);
}

static void pulse(const struct device *gpio, gpio_pin_t pin)
{
	zassert_ok(gpio_emul_input_set(gpio, pin, 0));
	zassert_ok(gpio_emul_input_set(gpio, pin, 1));
	zassert_ok(gpio_emul_input_set(gpio, pin, 0));
}

ZTEST(atm90e26, test_events)
{
	const struct device *dev = meters[0].dev;
	const struct device *gpio = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	static struct event_record rec;

	if (!IS_ENABLED(CONFIG_ATM90E26_EVENTS)) {
		zassert_equal(energy_meter_set_event_callback(dev, event_cb, &rec), -ENOSYS);
		ztest_test_skip();
	}

	k_sem_init(&rec.sem, 0, 1);
	zassert_ok(energy_meter_set_event_callback(dev, event_cb, &rec));

	pulse(gpio, IRQ_PIN);
	zassert_ok(k_sem_take(&rec.sem, K_MSEC(100)), "IRQ event not reported");
	zassert_equal_ptr(rec.dev, dev);
	zassert_equal(rec.event.type, ENERGY_METER_EVENT_ALARM);
	zassert_equal(rec.event.index, 0);

	pulse(gpio, CF1_PIN);
	zassert_ok(k_sem_take(&rec.sem, K_MSEC(100)), "CF1 event not reported");
	zassert_equal(rec.event.type, ENERGY_METER_EVENT_PULSE);
	zassert_equal(rec.event.index, 0);

	zassert_ok(energy_meter_set_event_callback(dev, NULL, NULL));
	pulse(gpio, IRQ_PIN);
	zassert_equal(k_sem_take(&rec.sem, K_MSEC(50)), -EAGAIN);
}

#ifdef CONFIG_SHELL_BACKEND_DUMMY
static const char *run(const char *cmd, int expected)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	size_t size;

	WAIT_FOR(shell_ready(sh), 20000, k_msleep(1));
	shell_backend_dummy_clear_output(sh);
	zassert_equal(shell_execute_cmd(sh, cmd), expected, "%s", cmd);

	return shell_backend_dummy_get_output(sh, &size);
}
#endif

ZTEST(atm90e26, test_register_shell)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_ATM90E26_SHELL);

#ifdef CONFIG_ATM90E26_SHELL
	const struct emul *emul = EMUL_DT_GET(UART_NODE);
	uint16_t meas_cal[ATM90E26_MEAS_CAL_LEN];
	const char *out;

	out = run("atm90e26 read atm90e26 0x2b", 0);
	zassert_not_null(strstr(out, "0x2b: 0x9423"), "%s", out);

	/* A calibration register is written with its whole block and a new checksum */
	run("atm90e26 write atm90e26 0x33 0x7531", 0);
	for (size_t i = 0; i < ARRAY_SIZE(meas_cal); i++) {
		meas_cal[i] = atm90e26_emul_get_reg(emul, ATM90E26_REG_U_GAIN + i);
	}
	zassert_equal(meas_cal[2], 0x7531);
	zassert_equal(meas_cal[0], 0x6000, "the rest of the block must be kept");
	zassert_equal(atm90e26_emul_get_reg(emul, ATM90E26_REG_CS2),
		      atm90e26_checksum(meas_cal, ARRAY_SIZE(meas_cal)));
	zassert_equal(atm90e26_emul_get_reg(emul, ATM90E26_REG_ADJ_START), ATM90E26_CAL_CHECK_CMD);

	run("atm90e26 read atm90e26 0x80", -EINVAL);
#endif
}

ZTEST(atm90e26, test_energy_meter_shell)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_ENERGY_METER_SHELL);

#ifdef CONFIG_ENERGY_METER_SHELL
	const char *out;

	atm90e26_emul_set_reg(EMUL_DT_GET(SPI_NODE), ATM90E26_REG_URMS, 23012);

	out = run("energy_meter get atm90e26@0", 0);
	zassert_not_null(strstr(out, "230.120"), "%s", out);
	zassert_not_null(strstr(out, "frequency [Hz]"), "%s", out);
#endif
}

static void *atm90e26_setup(void)
{
	ARRAY_FOR_EACH_PTR(meters, m) {
		zassert_true(device_is_ready(m->dev), "%s meter not ready", m->name);
	}

	return NULL;
}

ZTEST_SUITE(atm90e26, NULL, atm90e26_setup, NULL, NULL, NULL);
