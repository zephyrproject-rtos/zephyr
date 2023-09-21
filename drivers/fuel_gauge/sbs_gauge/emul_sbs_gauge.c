/*
 * Copyright 2022 Google LLC
 * Copyright 2023 Microsoft Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for SBS 1.1 compliant smart battery fuel gauge.
 */

#ifdef CONFIG_FUEL_GAUGE
#define DT_DRV_COMPAT sbs_sbs_gauge_new_api
#else
#define DT_DRV_COMPAT sbs_sbs_gauge
#endif /* CONFIG_FUEL_GAUGE */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sbs_sbs_gauge);

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/emul_fuel_gauge.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/sys/util.h>

#include "sbs_gauge.h"

/** Run-time data used by the emulator */
struct sbs_gauge_emul_data {
	uint16_t mfr_acc;
	uint16_t remaining_capacity_alarm;
	uint16_t remaining_time_alarm;
	uint16_t mode;
	int16_t at_rate;
	/* Whether the battery cutoff or not */
	bool is_cutoff;
	/*
	 * Counts the number of times the cutoff payload has been sent to the designated
	 * register
	 */
	uint8_t cutoff_writes;
	struct {
		/* Non-register values associated with the state of the battery */
		/* Battery terminal voltage */
		uint32_t uV;
		/* Battery terminal current - Pos is charging, Neg is discharging */
		int uA;
	} batt_state;
};

/** Static configuration for the emulator */
struct sbs_gauge_emul_cfg {
	/** I2C address of emulator */
	uint16_t addr;
	bool cutoff_support;
	uint32_t cutoff_reg_addr;
	uint16_t cutoff_payload[SBS_GAUGE_CUTOFF_PAYLOAD_MAX_SIZE];
};

static void emul_sbs_gauge_maybe_do_battery_cutoff(const struct emul *target, int reg, int val)
{
	struct sbs_gauge_emul_data *data = target->data;
	const struct sbs_gauge_emul_cfg *cfg = target->cfg;

	/* Check if this is a cutoff write */
	if (cfg->cutoff_support && reg == cfg->cutoff_reg_addr) {
		__ASSERT_NO_MSG(ARRAY_SIZE(cfg->cutoff_payload) > 0);
		/*
		 * Calculate the next payload element value for a battery cutoff.
		 *
		 * We thoroughly check bounds elsewhere, so we can be confident we're not indexing
		 * past the end of the array.
		 */
		uint16_t target_payload_elem_val = cfg->cutoff_payload[data->cutoff_writes];

		if (target_payload_elem_val == val) {
			data->cutoff_writes++;
			__ASSERT_NO_MSG(data->cutoff_writes <= ARRAY_SIZE(cfg->cutoff_payload));
		} else {
			/* Wrong payload target value, reset cutoff sequence detection. */
			data->cutoff_writes = 0;
		}

		if (data->cutoff_writes == ARRAY_SIZE(cfg->cutoff_payload)) {
			data->is_cutoff = true;
			data->cutoff_writes = 0;
		}
	}
	/* Not a cutoff write, reset payload counter  */
	else {
		data->cutoff_writes = 0;
	}
}

static int emul_sbs_gauge_reg_write(const struct emul *target, int reg, int val)
{
	struct sbs_gauge_emul_data *data = target->data;

	LOG_INF("write %x = %x", reg, val);
	switch (reg) {
	case SBS_GAUGE_CMD_MANUFACTURER_ACCESS:
		data->mfr_acc = val;
		break;
	case SBS_GAUGE_CMD_REM_CAPACITY_ALARM:
		data->remaining_capacity_alarm = val;
		break;
	case SBS_GAUGE_CMD_REM_TIME_ALARM:
		data->remaining_time_alarm = val;
		break;
	case SBS_GAUGE_CMD_BATTERY_MODE:
		data->mode = val;
		break;
	case SBS_GAUGE_CMD_AR:
		data->at_rate = val;
		break;
	default:
		LOG_INF("Unknown write %x", reg);
		return -EIO;
	}

	/*
	 * One of the above registers is always designated as a "cutoff" register, usually it's
	 * MANUFACTURER ACCESS, but not always.
	 */
	emul_sbs_gauge_maybe_do_battery_cutoff(target, reg, val);

	return 0;
}

static int emul_sbs_gauge_reg_read(const struct emul *target, int reg, int *val)
{
	struct sbs_gauge_emul_data *data = target->data;

	switch (reg) {
	case SBS_GAUGE_CMD_MANUFACTURER_ACCESS:
		*val = data->mfr_acc;
		break;
	case SBS_GAUGE_CMD_REM_CAPACITY_ALARM:
		*val = data->remaining_capacity_alarm;
		break;
	case SBS_GAUGE_CMD_REM_TIME_ALARM:
		*val = data->remaining_time_alarm;
		break;
	case SBS_GAUGE_CMD_BATTERY_MODE:
		*val = data->mode;
		break;
	case SBS_GAUGE_CMD_AR:
		*val = data->at_rate;
		break;
	case SBS_GAUGE_CMD_VOLTAGE:
		*val = data->batt_state.uV / 1000;
		break;
	case SBS_GAUGE_CMD_CURRENT:
		*val = data->batt_state.uA / 1000;
		break;
	case SBS_GAUGE_CMD_AVG_CURRENT:
	case SBS_GAUGE_CMD_TEMP:
	case SBS_GAUGE_CMD_ASOC:
	case SBS_GAUGE_CMD_RSOC:
	case SBS_GAUGE_CMD_FULL_CAPACITY:
	case SBS_GAUGE_CMD_REM_CAPACITY:
	case SBS_GAUGE_CMD_NOM_CAPACITY:
	case SBS_GAUGE_CMD_AVG_TIME2EMPTY:
	case SBS_GAUGE_CMD_AVG_TIME2FULL:
	case SBS_GAUGE_CMD_RUNTIME2EMPTY:
	case SBS_GAUGE_CMD_CYCLE_COUNT:
	case SBS_GAUGE_CMD_DESIGN_VOLTAGE:
	case SBS_GAUGE_CMD_CHG_CURRENT:
	case SBS_GAUGE_CMD_CHG_VOLTAGE:
	case SBS_GAUGE_CMD_FLAGS:
	case SBS_GAUGE_CMD_ARTTF:
	case SBS_GAUGE_CMD_ARTTE:
	case SBS_GAUGE_CMD_AROK:
		/* Arbitrary stub value. */
		*val = 1;
		break;
	default:
		LOG_ERR("Unknown register 0x%x read", reg);
		return -EIO;
	}
	LOG_INF("read 0x%x = 0x%x", reg, *val);

	return 0;
}

static int emul_sbs_gauge_buffer_read(const struct emul *target, int reg, char *val)
{
	char mfg[] = "ACME";
	char dev[] = "B123456";
	char chem[] = "LiPO";
	struct sbs_gauge_manufacturer_name *mfg_name = (struct sbs_gauge_manufacturer_name *)val;
	struct sbs_gauge_device_name *dev_name = (struct sbs_gauge_device_name *)val;
	struct sbs_gauge_device_chemistry *dev_chem = (struct sbs_gauge_device_chemistry *)val;

	switch (reg) {
	case SBS_GAUGE_CMD_MANUFACTURER_NAME:
		mfg_name->manufacturer_name_length = sizeof(mfg);
		memcpy(mfg_name->manufacturer_name, mfg, mfg_name->manufacturer_name_length);
		break;
	case SBS_GAUGE_CMD_DEVICE_NAME:
		dev_name->device_name_length = sizeof(dev);
		memcpy(dev_name->device_name, dev, dev_name->device_name_length);
		break;

	case SBS_GAUGE_CMD_DEVICE_CHEMISTRY:
		dev_chem->device_chemistry_length = MIN(sizeof(chem),
							sizeof(dev_chem->device_chemistry));
		memcpy(dev_chem->device_chemistry, chem, dev_chem->device_chemistry_length);
		break;
	default:
		LOG_ERR("Unknown register 0x%x read", reg);
		return -EIO;
	}

	return 0;
}

static int sbs_gauge_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs,
				       int num_msgs, int addr)
{
	/* Largely copied from emul_bmi160.c */
	struct sbs_gauge_emul_data *data;
	unsigned int val;
	int reg;
	int rc;

	data = target->data;

	__ASSERT_NO_MSG(msgs && num_msgs);

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);
	switch (num_msgs) {
	case 2:
		if (msgs->flags & I2C_MSG_READ) {
			LOG_ERR("Unexpected read");
			return -EIO;
		}
		if (msgs->len != 1) {
			LOG_ERR("Unexpected msg0 length %d", msgs->len);
			return -EIO;
		}
		reg = msgs->buf[0];

		/* Now process the 'read' part of the message */
		msgs++;
		if (msgs->flags & I2C_MSG_READ) {
			switch (msgs->len) {
			case 2:
				rc = emul_sbs_gauge_reg_read(target, reg, &val);
				if (rc) {
					/* Return before writing bad value to message buffer */
					return rc;
				}

				/* SBS uses SMBus, which sends data in little-endian format. */
				sys_put_le16(val, msgs->buf);
				break;
					/* buffer properties */
			case (sizeof(struct sbs_gauge_manufacturer_name)):
			case (sizeof(struct sbs_gauge_device_chemistry)):
				rc = emul_sbs_gauge_buffer_read(target, reg, (char *)msgs->buf);
				break;
			default:
				LOG_ERR("Unexpected msg1 length %d", msgs->len);
				return -EIO;
			}
		} else {
			/* We write a word (2 bytes by the SBS spec) */
			if (msgs->len != 2) {
				LOG_ERR("Unexpected msg1 length %d", msgs->len);
			}
			uint16_t value = sys_get_le16(msgs->buf);

			rc = emul_sbs_gauge_reg_write(target, reg, value);
		}
		break;
	default:
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}

	return rc;
}

static int emul_sbs_fuel_gauge_set_battery_charging(const struct emul *target, uint32_t uV, int uA)
{
	struct sbs_gauge_emul_data *data = target->data;

	if (uV == 0 || uA == 0)
		return -EINVAL;

	data->batt_state.uA = uA;
	data->batt_state.uV = uV;

	return 0;
}

static int emul_sbs_fuel_gauge_is_battery_cutoff(const struct emul *target, bool *cutoff)
{
	struct sbs_gauge_emul_data *data = target->data;

	__ASSERT_NO_MSG(cutoff != NULL);

	*cutoff = data->is_cutoff;

	return 0;
}

static const struct fuel_gauge_emul_driver_api sbs_gauge_backend_api = {
	.set_battery_charging = emul_sbs_fuel_gauge_set_battery_charging,
	.is_battery_cutoff = emul_sbs_fuel_gauge_is_battery_cutoff,
};

static const struct i2c_emul_api sbs_gauge_emul_api_i2c = {
	.transfer = sbs_gauge_emul_transfer_i2c,
};

static void sbs_gauge_emul_reset(const struct emul *target)
{
	struct sbs_gauge_emul_data *data = target->data;

	memset(data, 0, sizeof(*data));
}

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>

/* Add test reset handlers in when using emulators with tests */
#define SBS_GAUGE_EMUL_RESET_RULE_BEFORE(inst)                                                     \
	sbs_gauge_emul_reset(EMUL_DT_GET(DT_DRV_INST(inst)));

static void emul_sbs_gauge_reset_rule_after(const struct ztest_unit_test *test, void *data)
{
	ARG_UNUSED(test);
	ARG_UNUSED(data);

	DT_INST_FOREACH_STATUS_OKAY(SBS_GAUGE_EMUL_RESET_RULE_BEFORE)
}
ZTEST_RULE(emul_sbs_gauge_reset, NULL, emul_sbs_gauge_reset_rule_after);
#endif /* CONFIG_ZTEST */

/**
 * Set up a new SBS_GAUGE emulator (I2C)
 *
 * @param emul Emulation information
 * @param parent Device to emulate (must use sbs_gauge driver)
 * @return 0 indicating success (always)
 */
static int emul_sbs_sbs_gauge_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);

	sbs_gauge_emul_reset(target);

	return 0;
}

/*
 * Main instantiation macro. SBS Gauge Emulator only implemented for I2C
 */
#define SBS_GAUGE_EMUL(n)                                                                          \
	static struct sbs_gauge_emul_data sbs_gauge_emul_data_##n;                                 \
	static const struct sbs_gauge_emul_cfg sbs_gauge_emul_cfg_##n = {                          \
		.addr = DT_INST_REG_ADDR(n),                                                       \
		.cutoff_support = DT_PROP_OR(DT_DRV_INST(n), battery_cutoff_support, false),       \
		.cutoff_reg_addr = DT_PROP_OR(DT_DRV_INST(n), battery_cutoff_reg_addr, 0),         \
		.cutoff_payload = DT_PROP_OR(DT_DRV_INST(n), battery_cutoff_payload, {}),          \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, emul_sbs_sbs_gauge_init, &sbs_gauge_emul_data_##n,                  \
			    &sbs_gauge_emul_cfg_##n, &sbs_gauge_emul_api_i2c,                      \
			    &sbs_gauge_backend_api)

DT_INST_FOREACH_STATUS_OKAY(SBS_GAUGE_EMUL)
