/*
 * Copyright (c) 2026 Testo SE & Co. KGaA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "emul_bq25620.h"
#include "mfd_bq25620.h"

#define CHARGER_NODE DT_NODELABEL(charger)
#define MFD_NODE     DT_NODELABEL(bq25620)

static const struct gpio_dt_spec int_gpio = GPIO_DT_SPEC_GET(MFD_NODE, int_gpios);
static const struct gpio_dt_spec ce_gpio = GPIO_DT_SPEC_GET(CHARGER_NODE, ce_gpios);

struct bq25620_fixture {
	const struct device *dev;
	const struct emul *target;
	/* Registers after the driver initialization */
	uint8_t init_regs[BQ25620_REG_COUNT];
};

static enum charger_status notified_status;
static enum charger_online notified_online;
static int status_notifications;
static int online_notifications;

static void status_notifier(enum charger_status status)
{
	notified_status = status;
	status_notifications++;
}

static void online_notifier(enum charger_online online)
{
	notified_online = online;
	online_notifications++;
}

static void *bq25620_setup(void)
{
	static struct bq25620_fixture fixture;

	fixture.dev = DEVICE_DT_GET(CHARGER_NODE);
	fixture.target = EMUL_DT_GET(MFD_NODE);

	zassert_true(device_is_ready(fixture.dev), "Charger not ready");

	for (uint8_t reg = 0; reg < BQ25620_REG_COUNT; reg++) {
		fixture.init_regs[reg] = emul_bq25620_get_reg(fixture.target, reg);
	}

	return &fixture;
}

static void bq25620_before(void *f)
{
	struct bq25620_fixture *fixture = f;

	emul_bq25620_set_reg(fixture->target, BQ25620_REG_CHG_STAT_0, 0);
	emul_bq25620_set_reg(fixture->target, BQ25620_REG_CHG_STAT_1, 0);
	emul_bq25620_set_reg(fixture->target, BQ25620_REG_FAULT_STAT_0, 0);
	zassert_ok(charger_charge_enable(fixture->dev, false));
}

static uint32_t get_uint(const struct device *dev, charger_prop_t prop)
{
	union charger_propval val = {};

	zassert_ok(charger_get_prop(dev, prop, &val), "Reading property %u failed", prop);

	/* All limit properties share the uint32_t representation */
	return val.const_charge_current_ua;
}

static int set_uint(const struct device *dev, charger_prop_t prop, uint32_t value)
{
	union charger_propval val = {.const_charge_current_ua = value};

	return charger_set_prop(dev, prop, &val);
}

static void set_vbus_chg_stat(const struct emul *target, uint8_t vbus_stat, uint8_t chg_stat)
{
	emul_bq25620_set_reg(target, BQ25620_REG_CHG_STAT_1,
			     FIELD_PREP(BQ25620_CHG_STAT_1_VBUS_STAT, vbus_stat) |
				     FIELD_PREP(BQ25620_CHG_STAT_1_CHG_STAT, chg_stat));
}

static enum charger_status get_status(const struct device *dev)
{
	union charger_propval val = {};

	zassert_ok(charger_get_prop(dev, CHARGER_PROP_STATUS, &val));

	return val.status;
}

static enum charger_online get_online(const struct device *dev)
{
	union charger_propval val = {};

	zassert_ok(charger_get_prop(dev, CHARGER_PROP_ONLINE, &val));

	return val.online;
}

static enum charger_charge_type get_charge_type(const struct device *dev)
{
	union charger_propval val = {};

	zassert_ok(charger_get_prop(dev, CHARGER_PROP_CHARGE_TYPE, &val));

	return val.charge_type;
}

static enum charger_health get_health(const struct device *dev)
{
	union charger_propval val = {};

	zassert_ok(charger_get_prop(dev, CHARGER_PROP_HEALTH, &val));

	return val.health;
}

static uint16_t init_reg16(const struct bq25620_fixture *fixture, uint8_t reg)
{
	return fixture->init_regs[reg] | (fixture->init_regs[reg + 1] << 8);
}

ZTEST_F(bq25620, test_init_limits)
{
	/* 2 A charge current, 80 mA steps */
	zassert_equal(init_reg16(fixture, BQ25620_REG_ICHG), 25 << BQ25620_ICHG_SHIFT);
	/* 4.35 V charge voltage, 10 mV steps */
	zassert_equal(init_reg16(fixture, BQ25620_REG_VREG), 435 << BQ25620_VREG_SHIFT);
	/* 80 mA pre-charge current, 20 mA steps */
	zassert_equal(init_reg16(fixture, BQ25620_REG_IPRECHG), 4 << BQ25620_IPRECHG_SHIFT);
	/* 120 mA termination current, 10 mA steps */
	zassert_equal(init_reg16(fixture, BQ25620_REG_ITERM), 12 << BQ25620_ITERM_SHIFT);
	/* 500 mA input current limit, 20 mA steps */
	zassert_equal(init_reg16(fixture, BQ25620_REG_IINDPM), 25 << BQ25620_IINDPM_SHIFT);
	/* 4.4 V input voltage limit, 40 mV steps */
	zassert_equal(init_reg16(fixture, BQ25620_REG_VINDPM), 110 << BQ25620_VINDPM_SHIFT);
	/* 3.12 V minimal system voltage, 80 mV steps */
	zassert_equal(init_reg16(fixture, BQ25620_REG_VSYSMIN), 39 << BQ25620_VSYSMIN_SHIFT);
}

ZTEST_F(bq25620, test_init_ctrl)
{
	/* Q1_FULLON, EN_TERM (reset value), VRECHG 200 mV, no VINDPM tracking */
	zassert_equal(fixture->init_regs[BQ25620_REG_CHG_CTRL_0], 0x85);
	/* EN_AUTO_INDET, EN_DCP_BIAS, EN_SAFETY_TMRS (reset values), 0.62 h, 28 h */
	zassert_equal(fixture->init_regs[BQ25620_REG_TIMER_CTRL], 0x57);
	/* EN_CHG (reset value), watchdog and auto battery discharge disabled */
	zassert_equal(fixture->init_regs[BQ25620_REG_CHG_CTRL_1], BQ25620_CHG_CTRL_1_EN_CHG);
	/* TREG 60 C, 1.65 MHz, normal drive strength, 6.3 V OVP, reserved bit 1 */
	zassert_equal(fixture->init_regs[BQ25620_REG_CHG_CTRL_2], 0x26);
	/* IBAT_PK 6 A, 2C */
	zassert_equal(fixture->init_regs[BQ25620_REG_CHG_CTRL_4], 0x81);
	/* TS_IGNORE on top of the reset value */
	zassert_equal(fixture->init_regs[BQ25620_REG_NTC_CTRL_0], 0xbd);
}

ZTEST(bq25620, test_init_defaults)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(charger_min));
	const struct emul *target = EMUL_DT_GET(DT_NODELABEL(bq25620_min));
	union charger_propval val = {};

	zassert_false(device_is_ready(dev));

	/* Settings left by a previous boot, the device registers are not reset */
	emul_bq25620_set_reg(target, BQ25620_REG_IINDPM, 0x90);
	emul_bq25620_set_reg(target, BQ25620_REG_IINDPM + 1, 0x01);
	emul_bq25620_set_reg(target, BQ25620_REG_IPRECHG, 0xf0);
	emul_bq25620_set_reg(target, BQ25620_REG_IPRECHG + 1, 0x01);
	emul_bq25620_set_reg(target, BQ25620_REG_ITERM, 0xf8);
	emul_bq25620_set_reg(target, BQ25620_REG_ITERM + 1, 0x01);
	emul_bq25620_set_reg(target, BQ25620_REG_VINDPM, 0x80);
	emul_bq25620_set_reg(target, BQ25620_REG_VINDPM + 1, 0x34);
	emul_bq25620_set_reg(target, BQ25620_REG_VSYSMIN, 0x00);
	emul_bq25620_set_reg(target, BQ25620_REG_VSYSMIN + 1, 0x08);
	emul_bq25620_set_reg(target, BQ25620_REG_CHG_CTRL_0, 0xc5);
	emul_bq25620_set_reg(target, BQ25620_REG_TIMER_CTRL, 0x53);
	emul_bq25620_set_reg(target, BQ25620_REG_CHG_CTRL_1, 0x00);
	emul_bq25620_set_reg(target, BQ25620_REG_CHG_CTRL_2, 0x26);
	emul_bq25620_set_reg(target, BQ25620_REG_CHG_CTRL_4, 0x83);
	emul_bq25620_set_reg(target, BQ25620_REG_NTC_CTRL_0, 0xbd);

	zassert_ok(device_init(dev));

	/* Charge current and voltage from the devicetree */
	zassert_equal(emul_bq25620_get_reg(target, BQ25620_REG_ICHG), 0x40);
	zassert_equal(emul_bq25620_get_reg(target, BQ25620_REG_ICHG + 1), 0x03);
	zassert_equal(emul_bq25620_get_reg(target, BQ25620_REG_VREG), 0x20);
	zassert_equal(emul_bq25620_get_reg(target, BQ25620_REG_VREG + 1), 0x0d);

	/* The input current limit is left to the adapter detection */
	zassert_equal(emul_bq25620_get_reg(target, BQ25620_REG_IINDPM), 0x90);
	zassert_equal(emul_bq25620_get_reg(target, BQ25620_REG_IINDPM + 1), 0x01);

	/* All other settings are back at their reset values */
	zassert_equal(emul_bq25620_get_reg(target, BQ25620_REG_IPRECHG), 0x50);
	zassert_equal(emul_bq25620_get_reg(target, BQ25620_REG_IPRECHG + 1), 0x00);
	zassert_equal(emul_bq25620_get_reg(target, BQ25620_REG_ITERM), 0x30);
	zassert_equal(emul_bq25620_get_reg(target, BQ25620_REG_ITERM + 1), 0x00);
	zassert_equal(emul_bq25620_get_reg(target, BQ25620_REG_VINDPM), 0x60);
	zassert_equal(emul_bq25620_get_reg(target, BQ25620_REG_VINDPM + 1), 0x0e);
	zassert_equal(emul_bq25620_get_reg(target, BQ25620_REG_VSYSMIN), 0x00);
	zassert_equal(emul_bq25620_get_reg(target, BQ25620_REG_VSYSMIN + 1), 0x0b);
	zassert_equal(emul_bq25620_get_reg(target, BQ25620_REG_CHG_CTRL_0), 0x06);
	zassert_equal(emul_bq25620_get_reg(target, BQ25620_REG_TIMER_CTRL), 0x5c);
	/* EN_CHG and EN_AUTO_IBATDIS set, watchdog disabled by the MFD */
	zassert_equal(emul_bq25620_get_reg(target, BQ25620_REG_CHG_CTRL_1), 0xa0);
	zassert_equal(emul_bq25620_get_reg(target, BQ25620_REG_CHG_CTRL_2), 0x4f);
	zassert_equal(emul_bq25620_get_reg(target, BQ25620_REG_CHG_CTRL_4), 0xc0);
	zassert_equal(emul_bq25620_get_reg(target, BQ25620_REG_NTC_CTRL_0), 0x3d);

	/* Without a charge enable pin, charging is enabled after initialization */
	set_vbus_chg_stat(target, 0x3, BQ25620_CHG_STAT_CC);
	zassert_equal(get_status(dev), CHARGER_STATUS_CHARGING);

	/* Notifications need the interrupt pin */
	val.status_notification = status_notifier;
	zassert_equal(charger_set_prop(dev, CHARGER_PROP_STATUS_NOTIFICATION, &val), -ENOTSUP);
	val.online_notification = online_notifier;
	zassert_equal(charger_set_prop(dev, CHARGER_PROP_ONLINE_NOTIFICATION, &val), -ENOTSUP);
}

ZTEST_F(bq25620, test_init_masks)
{
	/* Only the VBUS and charge status events used by the charger are unmasked */
	zassert_equal(fixture->init_regs[BQ25620_REG_CHG_MASK_0], BQ25620_CHG_MASK_0_ALL);
	zassert_equal(fixture->init_regs[BQ25620_REG_CHG_MASK_1], 0x00);
	zassert_equal(fixture->init_regs[BQ25620_REG_FAULT_MASK_0], BQ25620_FAULT_MASK_0_ALL);
}

ZTEST_F(bq25620, test_set_limits)
{
	/* Values are rounded down to the step size */
	zassert_ok(set_uint(fixture->dev, CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA, 1000000));
	zassert_equal(get_uint(fixture->dev, CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA), 960000);

	zassert_ok(set_uint(fixture->dev, CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV, 4205000));
	zassert_equal(get_uint(fixture->dev, CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV), 4200000);

	zassert_ok(set_uint(fixture->dev, CHARGER_PROP_PRECHARGE_CURRENT_UA, 620000));
	zassert_equal(get_uint(fixture->dev, CHARGER_PROP_PRECHARGE_CURRENT_UA), 620000);

	zassert_ok(set_uint(fixture->dev, CHARGER_PROP_CHARGE_TERM_CURRENT_UA, 10000));
	zassert_equal(get_uint(fixture->dev, CHARGER_PROP_CHARGE_TERM_CURRENT_UA), 10000);

	zassert_ok(set_uint(fixture->dev, CHARGER_PROP_INPUT_REGULATION_CURRENT_UA, 3200000));
	zassert_equal(get_uint(fixture->dev, CHARGER_PROP_INPUT_REGULATION_CURRENT_UA), 3200000);

	zassert_ok(set_uint(fixture->dev, CHARGER_PROP_INPUT_REGULATION_VOLTAGE_UV, 16800000));
	zassert_equal(get_uint(fixture->dev, CHARGER_PROP_INPUT_REGULATION_VOLTAGE_UV), 16800000);
}

ZTEST_F(bq25620, test_set_limits_out_of_range)
{
	/* The charge current and voltage are limited by the devicetree */
	zassert_equal(set_uint(fixture->dev, CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA, 2080000),
		      -EINVAL);
	zassert_equal(set_uint(fixture->dev, CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA, 0), -EINVAL);
	zassert_equal(set_uint(fixture->dev, CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV, 4360000),
		      -EINVAL);
	zassert_equal(set_uint(fixture->dev, CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV, 3490000),
		      -EINVAL);

	/* The other limits by the device */
	zassert_equal(set_uint(fixture->dev, CHARGER_PROP_PRECHARGE_CURRENT_UA, 640000), -EINVAL);
	zassert_equal(set_uint(fixture->dev, CHARGER_PROP_CHARGE_TERM_CURRENT_UA, 0), -EINVAL);
	zassert_equal(set_uint(fixture->dev, CHARGER_PROP_INPUT_REGULATION_CURRENT_UA, 90000),
		      -EINVAL);
	zassert_equal(set_uint(fixture->dev, CHARGER_PROP_INPUT_REGULATION_VOLTAGE_UV, 16840000),
		      -EINVAL);
}

ZTEST_F(bq25620, test_unsupported_props)
{
	union charger_propval val = {};

	zassert_equal(charger_get_prop(fixture->dev, CHARGER_PROP_PRESENT, &val), -ENOTSUP);
	zassert_equal(charger_get_prop(fixture->dev, CHARGER_PROP_MAX, &val), -ENOTSUP);
	zassert_equal(charger_set_prop(fixture->dev, CHARGER_PROP_ONLINE, &val), -ENOTSUP);
	zassert_equal(charger_set_prop(fixture->dev, CHARGER_PROP_MAX, &val), -ENOTSUP);
}

ZTEST_F(bq25620, test_charge_enable)
{
	uint8_t ctrl;

	/* Charging stays disabled by the charge enable pin until it is enabled */
	zassert_equal(gpio_emul_output_get(ce_gpio.port, ce_gpio.pin), 1);

	zassert_ok(charger_charge_enable(fixture->dev, true));
	zassert_equal(gpio_emul_output_get(ce_gpio.port, ce_gpio.pin), 0);
	ctrl = emul_bq25620_get_reg(fixture->target, BQ25620_REG_CHG_CTRL_1);
	zassert_not_equal(ctrl & BQ25620_CHG_CTRL_1_EN_CHG, 0);

	zassert_ok(charger_charge_enable(fixture->dev, false));
	zassert_equal(gpio_emul_output_get(ce_gpio.port, ce_gpio.pin), 1);
	ctrl = emul_bq25620_get_reg(fixture->target, BQ25620_REG_CHG_CTRL_1);
	zassert_equal(ctrl & BQ25620_CHG_CTRL_1_EN_CHG, 0);
}

ZTEST_F(bq25620, test_online)
{
	set_vbus_chg_stat(fixture->target, 0x0, BQ25620_CHG_STAT_NOT_CHARGING);
	zassert_equal(get_online(fixture->dev), CHARGER_ONLINE_OFFLINE);

	/* USB SDP, CDP, DCP, unknown, non-standard and HVDCP adapters */
	for (uint8_t vbus_stat = 0x1; vbus_stat <= 0x6; vbus_stat++) {
		set_vbus_chg_stat(fixture->target, vbus_stat, BQ25620_CHG_STAT_NOT_CHARGING);
		zassert_equal(get_online(fixture->dev), CHARGER_ONLINE_FIXED, "VBUS_STAT %u",
			      vbus_stat);
	}

	set_vbus_chg_stat(fixture->target, BQ25620_VBUS_STAT_OTG, BQ25620_CHG_STAT_NOT_CHARGING);
	zassert_equal(get_online(fixture->dev), CHARGER_ONLINE_OFFLINE);
}

ZTEST_F(bq25620, test_status)
{
	set_vbus_chg_stat(fixture->target, 0x0, BQ25620_CHG_STAT_NOT_CHARGING);
	zassert_equal(get_status(fixture->dev), CHARGER_STATUS_DISCHARGING);

	set_vbus_chg_stat(fixture->target, BQ25620_VBUS_STAT_OTG, BQ25620_CHG_STAT_NOT_CHARGING);
	zassert_equal(get_status(fixture->dev), CHARGER_STATUS_DISCHARGING);

	/* Charging disabled */
	set_vbus_chg_stat(fixture->target, 0x3, BQ25620_CHG_STAT_CC);
	zassert_equal(get_status(fixture->dev), CHARGER_STATUS_NOT_CHARGING);

	zassert_ok(charger_charge_enable(fixture->dev, true));
	zassert_equal(get_status(fixture->dev), CHARGER_STATUS_CHARGING);

	set_vbus_chg_stat(fixture->target, 0x3, BQ25620_CHG_STAT_CV);
	zassert_equal(get_status(fixture->dev), CHARGER_STATUS_CHARGING);

	set_vbus_chg_stat(fixture->target, 0x3, BQ25620_CHG_STAT_TOP_OFF);
	zassert_equal(get_status(fixture->dev), CHARGER_STATUS_CHARGING);

	set_vbus_chg_stat(fixture->target, 0x3, BQ25620_CHG_STAT_NOT_CHARGING);
	zassert_equal(get_status(fixture->dev), CHARGER_STATUS_NOT_CHARGING);
}

ZTEST_F(bq25620, test_charge_type)
{
	set_vbus_chg_stat(fixture->target, 0x3, BQ25620_CHG_STAT_CC);
	zassert_equal(get_charge_type(fixture->dev), CHARGER_CHARGE_TYPE_NONE);

	zassert_ok(charger_charge_enable(fixture->dev, true));
	zassert_equal(get_charge_type(fixture->dev), CHARGER_CHARGE_TYPE_FAST);

	set_vbus_chg_stat(fixture->target, 0x3, BQ25620_CHG_STAT_CV);
	zassert_equal(get_charge_type(fixture->dev), CHARGER_CHARGE_TYPE_STANDARD);

	set_vbus_chg_stat(fixture->target, 0x3, BQ25620_CHG_STAT_TOP_OFF);
	zassert_equal(get_charge_type(fixture->dev), CHARGER_CHARGE_TYPE_TRICKLE);

	set_vbus_chg_stat(fixture->target, 0x3, BQ25620_CHG_STAT_NOT_CHARGING);
	zassert_equal(get_charge_type(fixture->dev), CHARGER_CHARGE_TYPE_NONE);
}

ZTEST_F(bq25620, test_health_ts)
{
	static const struct {
		uint8_t ts_stat;
		enum charger_health health;
	} cases[] = {
		{BQ25620_TS_STAT_NORMAL, CHARGER_HEALTH_GOOD},
		{BQ25620_TS_STAT_COLD, CHARGER_HEALTH_COLD},
		{BQ25620_TS_STAT_HOT, CHARGER_HEALTH_HOT},
		{BQ25620_TS_STAT_COOL, CHARGER_HEALTH_COOL},
		{BQ25620_TS_STAT_WARM, CHARGER_HEALTH_WARM},
		{BQ25620_TS_STAT_PRECOOL, CHARGER_HEALTH_COOL},
		{BQ25620_TS_STAT_PREWARM, CHARGER_HEALTH_WARM},
		{BQ25620_TS_STAT_BIAS_FAULT, CHARGER_HEALTH_UNSPEC_FAILURE},
	};

	ARRAY_FOR_EACH(cases, i) {
		emul_bq25620_set_reg(fixture->target, BQ25620_REG_FAULT_STAT_0, cases[i].ts_stat);
		zassert_equal(get_health(fixture->dev), cases[i].health, "TS_STAT %u",
			      cases[i].ts_stat);
	}
}

ZTEST_F(bq25620, test_health_faults)
{
	emul_bq25620_set_reg(fixture->target, BQ25620_REG_CHG_STAT_0, BQ25620_CHG_STAT_0_WD_STAT);
	zassert_equal(get_health(fixture->dev), CHARGER_HEALTH_WATCHDOG_TIMER_EXPIRE);

	emul_bq25620_set_reg(fixture->target, BQ25620_REG_CHG_STAT_0,
			     BQ25620_CHG_STAT_0_SAFETY_TMR_STAT);
	zassert_equal(get_health(fixture->dev), CHARGER_HEALTH_SAFETY_TIMER_EXPIRE);

	emul_bq25620_set_reg(fixture->target, BQ25620_REG_FAULT_STAT_0,
			     BQ25620_FAULT_STAT_0_VBUS_FAULT);
	zassert_equal(get_health(fixture->dev), CHARGER_HEALTH_UNSPEC_FAILURE);

	emul_bq25620_set_reg(fixture->target, BQ25620_REG_FAULT_STAT_0,
			     BQ25620_FAULT_STAT_0_BAT_FAULT);
	zassert_equal(get_health(fixture->dev), CHARGER_HEALTH_OVERVOLTAGE);

	/* Thermal shutdown takes precedence over all other conditions */
	emul_bq25620_set_reg(fixture->target, BQ25620_REG_FAULT_STAT_0,
			     BQ25620_FAULT_STAT_0_TSHUT | BQ25620_FAULT_STAT_0_BAT_FAULT |
				     BQ25620_TS_STAT_HOT);
	zassert_equal(get_health(fixture->dev), CHARGER_HEALTH_OVERHEAT);
}

static void trigger_interrupt(const struct emul *target, uint8_t chg_flag_1)
{
	emul_bq25620_set_reg(target, BQ25620_REG_CHG_FLAG_1, chg_flag_1);

	/* The interrupt pin is active low, a falling edge raises the interrupt */
	zassert_ok(gpio_emul_input_set(int_gpio.port, int_gpio.pin, 1));
	zassert_ok(gpio_emul_input_set(int_gpio.port, int_gpio.pin, 0));

	k_sleep(K_MSEC(10));
}

ZTEST_F(bq25620, test_notifications)
{
	union charger_propval val;

	val.status_notification = status_notifier;
	zassert_ok(charger_set_prop(fixture->dev, CHARGER_PROP_STATUS_NOTIFICATION, &val));
	val.online_notification = online_notifier;
	zassert_ok(charger_set_prop(fixture->dev, CHARGER_PROP_ONLINE_NOTIFICATION, &val));

	status_notifications = 0;
	online_notifications = 0;

	/* Adapter plugged in */
	zassert_ok(charger_charge_enable(fixture->dev, true));
	set_vbus_chg_stat(fixture->target, 0x3, BQ25620_CHG_STAT_CC);
	trigger_interrupt(fixture->target, BIT(0));

	zassert_equal(status_notifications, 1);
	zassert_equal(notified_status, CHARGER_STATUS_CHARGING);
	zassert_equal(online_notifications, 1);
	zassert_equal(notified_online, CHARGER_ONLINE_FIXED);

	/* The flags have been read and cleared */
	zassert_equal(emul_bq25620_get_reg(fixture->target, BQ25620_REG_CHG_FLAG_1), 0);

	/* Charge status change only, the online state is not notified */
	set_vbus_chg_stat(fixture->target, 0x3, BQ25620_CHG_STAT_NOT_CHARGING);
	trigger_interrupt(fixture->target, BIT(3));

	zassert_equal(status_notifications, 2);
	zassert_equal(notified_status, CHARGER_STATUS_NOT_CHARGING);
	zassert_equal(online_notifications, 1);

	/* Adapter removed */
	set_vbus_chg_stat(fixture->target, 0x0, BQ25620_CHG_STAT_NOT_CHARGING);
	trigger_interrupt(fixture->target, BIT(0));

	zassert_equal(status_notifications, 3);
	zassert_equal(notified_status, CHARGER_STATUS_DISCHARGING);
	zassert_equal(online_notifications, 2);
	zassert_equal(notified_online, CHARGER_ONLINE_OFFLINE);

	val.status_notification = NULL;
	zassert_ok(charger_set_prop(fixture->dev, CHARGER_PROP_STATUS_NOTIFICATION, &val));
	val.online_notification = NULL;
	zassert_ok(charger_set_prop(fixture->dev, CHARGER_PROP_ONLINE_NOTIFICATION, &val));
}

ZTEST_SUITE(bq25620, NULL, bq25620_setup, bq25620_before, NULL, NULL);
