/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/energy_meter.h>
#include <zephyr/drivers/energy_meter/atm90e32as.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/pm/device.h>
#include <zephyr/ztest.h>

#ifdef CONFIG_SHELL_BACKEND_DUMMY
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#endif

#include "atm90e32as.h"
#include "emul_atm90e32as.h"

#define METER_NODE DT_NODELABEL(meter)
#define GPIO_NODE  DT_NODELABEL(gpio0)

#define PM0_PIN  DT_GPIO_PIN(METER_NODE, pm0_gpios)
#define PM1_PIN  DT_GPIO_PIN(METER_NODE, pm1_gpios)
#define IRQ0_PIN DT_GPIO_PIN(METER_NODE, irq0_gpios)
#define CF1_PIN  DT_GPIO_PIN(METER_NODE, cf1_gpios)

#define LINE_A     ENERGY_METER_LINE_A
#define LINE_B     ENERGY_METER_LINE_B
#define LINE_C     ENERGY_METER_LINE_C
#define LINE_N     ENERGY_METER_LINE_N
#define LINE_TOTAL ENERGY_METER_LINE_TOTAL

static const struct device *const meter = DEVICE_DT_GET(METER_NODE);
static const struct emul *const emul = EMUL_DT_GET(METER_NODE);

static int64_t get(uint8_t quantity, uint8_t line)
{
	int64_t value;

	zassert_ok(energy_meter_get_one(meter, quantity, line, &value), "quantity %u line %u",
		   quantity, line);

	return value;
}

ZTEST(atm90e32as, test_init_applies_devicetree)
{
	zassert_true(atm90e32as_emul_reset_count(emul) >= 1U, "no soft reset at init");
	zassert_equal(atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_MMODE0), 0x0185);
	zassert_equal(atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_F0), 6000);
	zassert_equal(atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_T0), 25);
	zassert_equal(atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_P_START_TH), 0x0123);
	zassert_equal(atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_PL_CONST_H), 0x0861);
	zassert_equal(atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_PL_CONST_L), 0xC468);
	zassert_equal(atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_U_GAIN_A), 0x8000);
	zassert_equal(
		atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_U_GAIN_A + ATM90E32AS_MEAS_CAL_STRIDE),
		0x7000);
	zassert_equal(atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_I_GAIN_A +
							    2 * ATM90E32AS_MEAS_CAL_STRIDE),
		      0x4000);
	zassert_equal(atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_EMM_INT_EN0), 0x0001);
	zassert_equal(atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_EMM_INT_EN1), 0x0002);
	zassert_equal(atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_METER_EN),
		      ATM90E32AS_METER_EN_ON, "metering not enabled");
	zassert_equal(atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_CFG_REG_ACC_EN),
		      ATM90E32AS_CFG_ACCESS_DISABLE, "configuration access left open");
}

ZTEST(atm90e32as, test_info)
{
	struct energy_meter_info info;

	zassert_ok(energy_meter_get_info(meter, &info));
	zassert_equal(info.phases, 3);

	for (uint8_t line = LINE_A; line <= LINE_C; line++) {
		zassert_true((info.quantities[line] & BIT(ENERGY_METER_VOLTAGE)) != 0U);
		zassert_true((info.quantities[line] & BIT(ENERGY_METER_PHASE_ANGLE)) != 0U);
		zassert_true((info.quantities[line] & BIT(ENERGY_METER_APPARENT_ENERGY)) != 0U);
	}

	zassert_equal(info.quantities[LINE_N], BIT(ENERGY_METER_CURRENT));
	zassert_true((info.quantities[LINE_TOTAL] & BIT(ENERGY_METER_FREQUENCY)) != 0U);
	zassert_true((info.quantities[LINE_TOTAL] & BIT(ENERGY_METER_TEMPERATURE)) != 0U);
	zassert_true((info.quantities[LINE_TOTAL] & BIT(ENERGY_METER_VOLTAGE)) == 0U);
	zassert_true((info.quantities[LINE_A] & BIT(ENERGY_METER_VOLTAGE_LL)) == 0U);
}

ZTEST(atm90e32as, test_rms_voltage_and_current)
{
	/* 230.12 V plus 0x80 / 256 of 0.01 V */
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_URMS_A, 23012);
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_URMS_A_LSB, 0x8000);
	zassert_equal(get(ENERGY_METER_VOLTAGE, LINE_A), 230125000);

	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_URMS_A + 2, 21000);
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_URMS_A_LSB + 2, 0);
	zassert_equal(get(ENERGY_METER_VOLTAGE, LINE_C), 210000000);

	/* 5.123 A plus 0x40 / 256 of 0.001 A */
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_IRMS_A + 1, 5123);
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_IRMS_A_LSB + 1, 0x4000);
	zassert_equal(get(ENERGY_METER_CURRENT, LINE_B), 5123250);

	/* The neutral current has no lower word */
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_IRMS_N, 1500);
	zassert_equal(get(ENERGY_METER_CURRENT, LINE_N), 1500000);
}

ZTEST(atm90e32as, test_power_is_signed_32_bit)
{
	/* 0x00010000 * 0.00032 W */
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_PMEAN_T, 0x0001);
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_PMEAN_T_LSB, 0x0000);
	zassert_equal(get(ENERGY_METER_ACTIVE_POWER, LINE_TOTAL), 20971520);

	/* -10000 * 0.00032 var */
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_QMEAN_T + 1, 0xFFFF);
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_QMEAN_T_LSB + 1, 0xD8F0);
	zassert_equal(get(ENERGY_METER_REACTIVE_POWER, LINE_A), -3200000);

	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_PMEAN_TH + 3, 0x0000);
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_PMEAN_TH_LSB + 3, 1000);
	zassert_equal(get(ENERGY_METER_ATM90E32AS_HARM_POWER, LINE_C), 320000);
}

ZTEST(atm90e32as, test_power_factor_angle_frequency_temperature)
{
	int64_t value;

	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_PFMEAN_T, (uint16_t)-1000);
	zassert_equal(get(ENERGY_METER_POWER_FACTOR, LINE_TOTAL), -1000000);

	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_PFMEAN_T + 1, 500);
	zassert_equal(get(ENERGY_METER_POWER_FACTOR, LINE_A), 500000);

	/* The chip reports 0 to 360 degrees; the API wants -180 to 180 */
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_PANGLE_A + 1, 3599);
	zassert_equal(get(ENERGY_METER_PHASE_ANGLE, LINE_B), -100000);
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_PANGLE_A + 1, 1800);
	zassert_equal(get(ENERGY_METER_PHASE_ANGLE, LINE_B), 180000000);
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_PANGLE_A + 1, 1801);
	zassert_equal(get(ENERGY_METER_PHASE_ANGLE, LINE_B), -179900000);
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_PANGLE_A, 3600);
	zassert_equal(get(ENERGY_METER_PHASE_ANGLE, LINE_A), 0);

	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_UANGLE_A + 2, 2400);
	zassert_equal(get(ENERGY_METER_VOLTAGE_ANGLE, LINE_C), 240000000);
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_UANGLE_A + 2, 3600);
	zassert_equal(get(ENERGY_METER_VOLTAGE_ANGLE, LINE_C), 0);

	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_FREQ, 5002);
	zassert_equal(get(ENERGY_METER_FREQUENCY, LINE_TOTAL), 50020000);

	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_TEMP, (uint16_t)-5);
	zassert_equal(get(ENERGY_METER_TEMPERATURE, LINE_TOTAL), -5000000);

	/* Before the first measurement after a reset */
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_TEMP, 0xFF00);
	zassert_equal(energy_meter_get_one(meter, ENERGY_METER_TEMPERATURE, LINE_TOTAL, &value),
		      -EAGAIN);
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_TEMP, 25);
	zassert_equal(get(ENERGY_METER_TEMPERATURE, LINE_TOTAL), 25000000);
}

ZTEST(atm90e32as, test_peaks_use_devicetree_gains)
{
	/* 8125 * 0x7000 / (100 * 2^13) V */
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_UPEAK_A + 1, 8125);
	zassert_equal(get(ENERGY_METER_ATM90E32AS_VOLTAGE_PEAK, LINE_B), 284375000);

	/* 4096 * 0x4000 / (1000 * 2^13) A */
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_IPEAK_A + 2, 4096);
	zassert_equal(get(ENERGY_METER_ATM90E32AS_CURRENT_PEAK, LINE_C), 8192000);
}

ZTEST(atm90e32as, test_energy_accumulates_across_reads)
{
	int64_t before = get(ENERGY_METER_ACTIVE_ENERGY_IMPORT, LINE_TOTAL);

	/* 3200 counts of 0.01 CF at 3200 imp/kWh is 10 Wh */
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_AP_ENERGY_T, 3200);
	zassert_equal(get(ENERGY_METER_ACTIVE_ENERGY_IMPORT, LINE_TOTAL) - before, 10000000);
	zassert_equal(atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_AP_ENERGY_T), 0,
		      "energy register not cleared by the read");

	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_AP_ENERGY_T, 160);
	zassert_equal(get(ENERGY_METER_ACTIVE_ENERGY_IMPORT, LINE_TOTAL) - before, 10500000);

	/* A read with nothing new keeps the total */
	zassert_equal(get(ENERGY_METER_ACTIVE_ENERGY_IMPORT, LINE_TOTAL) - before, 10500000);
}

ZTEST(atm90e32as, test_every_energy_value_has_its_own_register)
{
	static const uint8_t lines[] = {LINE_TOTAL, LINE_A, LINE_B, LINE_C};
	int64_t before[ARRAY_SIZE(lines)];

	for (size_t i = 0; i < ARRAY_SIZE(lines); i++) {
		before[i] = get(ENERGY_METER_ATM90E32AS_HARM_ENERGY_EXPORT, lines[i]);
		atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_AN_ENERGY_TH + i, 320 * (i + 1));
	}

	for (size_t i = 0; i < ARRAY_SIZE(lines); i++) {
		zassert_equal(get(ENERGY_METER_ATM90E32AS_HARM_ENERGY_EXPORT, lines[i]) - before[i],
			      1000000 * (i + 1));
	}
}

ZTEST(atm90e32as, test_batch_and_unsupported_channels)
{
	static const struct energy_meter_channel channels[] = {
		{ENERGY_METER_VOLTAGE, LINE_A},
		{ENERGY_METER_FREQUENCY, LINE_TOTAL},
	};
	static const struct energy_meter_channel bad[] = {
		{ENERGY_METER_VOLTAGE, LINE_A},
		{ENERGY_METER_VOLTAGE, LINE_N},
	};
	int64_t values[2] = {-1, -1};

	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_URMS_A, 22000);
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_URMS_A_LSB, 0);
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_FREQ, 6000);

	zassert_ok(energy_meter_get(meter, channels, values, ARRAY_SIZE(channels)));
	zassert_equal(values[0], 220000000);
	zassert_equal(values[1], 60000000);

	values[0] = -1;
	zassert_equal(energy_meter_get(meter, bad, values, ARRAY_SIZE(bad)), -ENOTSUP);
	zassert_equal(values[0], -1, "no value is read when a channel is not supported");
	zassert_equal(energy_meter_get_one(meter, ENERGY_METER_VOLTAGE_LL, LINE_A, &values[0]),
		      -ENOTSUP);
	zassert_equal(energy_meter_get_one(meter, ENERGY_METER_ATM90E32AS_QUANTITY_COUNT,
					   LINE_TOTAL, &values[0]),
		      -ENOTSUP);
}

ZTEST(atm90e32as, test_snapshot)
{
	struct energy_meter_snapshot snap;

	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_URMS_A + 1, 23100);
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_URMS_A_LSB + 1, 0);
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_FREQ, 4998);

	zassert_ok(energy_meter_get_snapshot(meter, &snap));
	zassert_equal(snap.phases, 3);
	zassert_equal(snap.line[LINE_B].voltage, 231000000);
	zassert_equal(snap.line[LINE_TOTAL].frequency, 49980000);
	zassert_equal(snap.valid[LINE_N], BIT(ENERGY_METER_CURRENT));
}

ZTEST(atm90e32as, test_reset_energy)
{
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_AP_ENERGY_T + 2, 320);
	zassert_true(get(ENERGY_METER_ACTIVE_ENERGY_IMPORT, LINE_B) > 0);

	/* Energy the chip has not reported yet is dropped too */
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_AP_ENERGY_T + 2, 640);
	zassert_ok(energy_meter_reset_energy(meter));
	zassert_equal(atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_AP_ENERGY_T + 2), 0);
	zassert_equal(get(ENERGY_METER_ACTIVE_ENERGY_IMPORT, LINE_B), 0);
}

ZTEST(atm90e32as, test_interrupt_status_is_cleared)
{
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_EMM_INT_STATE0, 0x8001);
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_EMM_INT_STATE1, 0x0002);

	/* The state registers are read without side effects */
	(void)get(ENERGY_METER_ATM90E32AS_EMM_STATE, LINE_TOTAL);
	zassert_equal(atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_EMM_INT_STATE0), 0x8001);

	zassert_equal(get(ENERGY_METER_ATM90E32AS_EMM_INT_STATE, LINE_TOTAL), 0x80010002);
	zassert_equal(atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_EMM_INT_STATE0), 0);
	zassert_equal(atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_EMM_INT_STATE1), 0);
}

ZTEST(atm90e32as, test_corrupted_read_keeps_energy)
{
	int64_t before = get(ENERGY_METER_ACTIVE_ENERGY_IMPORT, LINE_A);
	int64_t value;

	Z_TEST_SKIP_IFNDEF(CONFIG_ATM90E32AS_VERIFY_READ);

	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_AP_ENERGY_T + 1, 320);
	atm90e32as_emul_corrupt_reads(emul, 1);
	zassert_equal(get(ENERGY_METER_ACTIVE_ENERGY_IMPORT, LINE_A) - before, 1000000);

	/* With every copy corrupted the read fails instead of returning garbage */
	atm90e32as_emul_corrupt_reads(emul, 3);
	zassert_equal(energy_meter_get_one(meter, ENERGY_METER_FREQUENCY, LINE_TOTAL, &value),
		      -EIO);
}

ZTEST(atm90e32as, test_pm_suspend_resume)
{
	const struct device *gpio = DEVICE_DT_GET(GPIO_NODE);
	int64_t before = get(ENERGY_METER_ACTIVE_ENERGY_IMPORT, LINE_C);
	int64_t value;
	uint32_t resets;

	Z_TEST_SKIP_IFNDEF(CONFIG_PM_DEVICE);

	zassert_equal(gpio_emul_output_get(gpio, PM0_PIN), 1);
	zassert_equal(gpio_emul_output_get(gpio, PM1_PIN), 1);

	/* Energy counted before Idle mode must survive the reset that leaving it causes */
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_AP_ENERGY_T + 3, 640);
	zassert_ok(pm_device_action_run(meter, PM_DEVICE_ACTION_SUSPEND));
	zassert_equal(gpio_emul_output_get(gpio, PM0_PIN), 0);
	zassert_equal(gpio_emul_output_get(gpio, PM1_PIN), 0);

	zassert_equal(energy_meter_get_one(meter, ENERGY_METER_VOLTAGE, LINE_A, &value), -EBUSY);
	zassert_equal(energy_meter_reset_energy(meter), -EBUSY);

	/* Model the power-on reset of the chip */
	atm90e32as_emul_reset(emul);
	resets = atm90e32as_emul_reset_count(emul);

	zassert_ok(pm_device_action_run(meter, PM_DEVICE_ACTION_RESUME));
	zassert_equal(gpio_emul_output_get(gpio, PM0_PIN), 1);
	zassert_equal(gpio_emul_output_get(gpio, PM1_PIN), 1);
	zassert_true(atm90e32as_emul_reset_count(emul) > resets);
	zassert_equal(atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_MMODE0), 0x0185,
		      "configuration not restored on resume");
	zassert_equal(get(ENERGY_METER_ACTIVE_ENERGY_IMPORT, LINE_C) - before, 2000000);
}

ZTEST(atm90e32as, test_resume_fails_without_chip)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_PM_DEVICE);

	zassert_ok(pm_device_action_run(meter, PM_DEVICE_ACTION_SUSPEND));

	atm90e32as_emul_set_present(emul, false);
	zassert_equal(pm_device_action_run(meter, PM_DEVICE_ACTION_RESUME), -ENODEV);

	atm90e32as_emul_set_present(emul, true);
	zassert_ok(pm_device_action_run(meter, PM_DEVICE_ACTION_RESUME));
}

ZTEST(atm90e32as, test_write_retries)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_PM_DEVICE);

	/* Two dropped writes are recovered by retrying */
	zassert_ok(pm_device_action_run(meter, PM_DEVICE_ACTION_SUSPEND));
	atm90e32as_emul_drop_writes(emul, 2);
	zassert_ok(pm_device_action_run(meter, PM_DEVICE_ACTION_RESUME));
	zassert_equal(atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_MMODE0), 0x0185);

	/*
	 * The unverified soft reset takes the first dropped write; the next three exhaust the
	 * retries of the first verified write.
	 */
	zassert_ok(pm_device_action_run(meter, PM_DEVICE_ACTION_SUSPEND));
	atm90e32as_emul_drop_writes(emul, 4);
	zassert_equal(pm_device_action_run(meter, PM_DEVICE_ACTION_RESUME), -EIO);
	zassert_ok(pm_device_action_run(meter, PM_DEVICE_ACTION_RESUME));
}

ZTEST(atm90e32as, test_current_scale)
{
	const struct device *scaled = DEVICE_DT_GET(DT_NODELABEL(meter_scaled));
	const struct emul *scaled_emul = EMUL_DT_GET(DT_NODELABEL(meter_scaled));
	int64_t before;
	int64_t value;

	/* 60 A read by the chip is 150 A at a 5/2 ratio, beyond the 65.535 A register range */
	atm90e32as_emul_set_reg(scaled_emul, ATM90E32AS_REG_IRMS_A, 60000);
	atm90e32as_emul_set_reg(scaled_emul, ATM90E32AS_REG_IRMS_A_LSB, 0);
	zassert_ok(energy_meter_get_one(scaled, ENERGY_METER_CURRENT, LINE_A, &value));
	zassert_equal(value, 150000000);

	atm90e32as_emul_set_reg(scaled_emul, ATM90E32AS_REG_IRMS_A + 1, 5123);
	atm90e32as_emul_set_reg(scaled_emul, ATM90E32AS_REG_IRMS_A_LSB + 1, 0);
	zassert_ok(energy_meter_get_one(scaled, ENERGY_METER_CURRENT, LINE_B, &value));
	zassert_equal(value, 12807500);

	/* Power follows the current, including negative values */
	atm90e32as_emul_set_reg(scaled_emul, ATM90E32AS_REG_PMEAN_T, 0x0001);
	atm90e32as_emul_set_reg(scaled_emul, ATM90E32AS_REG_PMEAN_T_LSB, 0x0000);
	zassert_ok(energy_meter_get_one(scaled, ENERGY_METER_ACTIVE_POWER, LINE_TOTAL, &value));
	zassert_equal(value, 52428800);

	atm90e32as_emul_set_reg(scaled_emul, ATM90E32AS_REG_QMEAN_T + 1, 0xFFFF);
	atm90e32as_emul_set_reg(scaled_emul, ATM90E32AS_REG_QMEAN_T_LSB + 1, 0xD8F0);
	zassert_ok(energy_meter_get_one(scaled, ENERGY_METER_REACTIVE_POWER, LINE_A, &value));
	zassert_equal(value, -8000000);

	/* 4096 * 0x8000 / (1000 * 2^13) A, times 5/2 */
	atm90e32as_emul_set_reg(scaled_emul, ATM90E32AS_REG_IPEAK_A, 4096);
	zassert_ok(
		energy_meter_get_one(scaled, ENERGY_METER_ATM90E32AS_CURRENT_PEAK, LINE_A, &value));
	zassert_equal(value, 40960000);

	/* 10 Wh counted by the chip is 25 Wh */
	zassert_ok(energy_meter_get_one(scaled, ENERGY_METER_ACTIVE_ENERGY_IMPORT, LINE_TOTAL,
					&before));
	atm90e32as_emul_set_reg(scaled_emul, ATM90E32AS_REG_AP_ENERGY_T, 3200);
	zassert_ok(energy_meter_get_one(scaled, ENERGY_METER_ACTIVE_ENERGY_IMPORT, LINE_TOTAL,
					&value));
	zassert_equal(value - before, 25000000);

	/* Voltage and power factor do not depend on the current calibration */
	atm90e32as_emul_set_reg(scaled_emul, ATM90E32AS_REG_URMS_A, 23012);
	atm90e32as_emul_set_reg(scaled_emul, ATM90E32AS_REG_URMS_A_LSB, 0);
	zassert_ok(energy_meter_get_one(scaled, ENERGY_METER_VOLTAGE, LINE_A, &value));
	zassert_equal(value, 230120000);

	atm90e32as_emul_set_reg(scaled_emul, ATM90E32AS_REG_PFMEAN_T, 985);
	zassert_ok(energy_meter_get_one(scaled, ENERGY_METER_POWER_FACTOR, LINE_TOTAL, &value));
	zassert_equal(value, 985000);
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

ZTEST(atm90e32as, test_events)
{
	const struct device *gpio = DEVICE_DT_GET(GPIO_NODE);
	static struct event_record rec;

	if (!IS_ENABLED(CONFIG_ATM90E32AS_EVENTS)) {
		zassert_equal(energy_meter_set_event_callback(meter, event_cb, &rec), -ENOSYS);
		ztest_test_skip();
	}

	k_sem_init(&rec.sem, 0, 1);
	zassert_ok(energy_meter_set_event_callback(meter, event_cb, &rec));

	pulse(gpio, IRQ0_PIN);
	zassert_ok(k_sem_take(&rec.sem, K_MSEC(100)), "IRQ0 event not reported");
	zassert_equal_ptr(rec.dev, meter);
	zassert_equal(rec.event.type, ENERGY_METER_EVENT_ALARM);
	zassert_equal(rec.event.index, 0);

	pulse(gpio, CF1_PIN);
	zassert_ok(k_sem_take(&rec.sem, K_MSEC(100)), "CF1 event not reported");
	zassert_equal(rec.event.type, ENERGY_METER_EVENT_PULSE);
	zassert_equal(rec.event.index, 0);

	/* A NULL callback stops the events */
	zassert_ok(energy_meter_set_event_callback(meter, NULL, NULL));
	pulse(gpio, IRQ0_PIN);
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

ZTEST(atm90e32as, test_register_shell)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_ATM90E32AS_SHELL);

#ifdef CONFIG_ATM90E32AS_SHELL
	int64_t before = get(ENERGY_METER_ACTIVE_ENERGY_EXPORT, LINE_TOTAL);
	const char *out;

	out = run("atm90e32as read atm90e32as@0 0x33", 0);
	zassert_not_null(strstr(out, "0x33: 0x0185"), "%s", out);

	/* Configuration writes are unlocked and locked again around the write */
	run("atm90e32as write atm90e32as@0 0x33 0x0187", 0);
	zassert_equal(atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_MMODE0), 0x0187);
	zassert_equal(atm90e32as_emul_get_reg(emul, ATM90E32AS_REG_CFG_REG_ACC_EN),
		      ATM90E32AS_CFG_ACCESS_DISABLE);
	run("atm90e32as write atm90e32as@0 0x33 0x0185", 0);

	/* An energy register read from the shell still reaches the driver total */
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_AN_ENERGY_T, 320);
	out = run("atm90e32as read atm90e32as@0 0x84", 0);
	zassert_not_null(strstr(out, "0x84: 0x0140"), "%s", out);
	zassert_equal(get(ENERGY_METER_ACTIVE_ENERGY_EXPORT, LINE_TOTAL) - before, 1000000);

	out = run("atm90e32as read atm90e32as@0 0xfe 2", 0);
	zassert_not_null(strstr(out, "0xff:"), "%s", out);
	run("atm90e32as read atm90e32as@0 0xff 2", -EINVAL);
	run("atm90e32as write atm90e32as@0 0x33 0x10000", -EINVAL);
#endif
}

ZTEST(atm90e32as, test_energy_meter_shell)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_ENERGY_METER_SHELL);

#ifdef CONFIG_ENERGY_METER_SHELL
	const char *out;

	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_URMS_A, 23012);
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_URMS_A_LSB, 0x8000);

	out = run("energy_meter get atm90e32as@0", 0);
	zassert_not_null(strstr(out, "voltage [V]"), "%s", out);
	zassert_not_null(strstr(out, "230.125"), "%s", out);

	/* Device-specific quantities are read by number: 16 is the fundamental power */
	BUILD_ASSERT(ENERGY_METER_ATM90E32AS_FUND_POWER == 16);
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_PMEAN_TF, 0);
	atm90e32as_emul_set_reg(emul, ATM90E32AS_REG_PMEAN_TF_LSB, 1000);
	out = run("energy_meter read atm90e32as@0 16", 0);
	zassert_not_null(strstr(out, "320000 (0x4e200)"), "%s", out);
#endif
}

static void *atm90e32as_setup(void)
{
	zassert_true(device_is_ready(meter), "ATM90E32AS not ready");

	return NULL;
}

ZTEST_SUITE(atm90e32as, NULL, atm90e32as_setup, NULL, NULL, NULL);
