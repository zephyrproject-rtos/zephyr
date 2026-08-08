/*
 * Copyright (c) 2025 Franz Parzer <Franz.Parzer@htl-steyr.ac.at>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* AP33772S I2C USB PD3.1 EPR SINK CONTROLLER driver. */

#define DT_DRV_COMPAT diodes_ap33772s

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "ap33772s.h"

LOG_MODULE_REGISTER(AP33772S, CONFIG_SENSOR_LOG_LEVEL);

static int ap33772s_srcpdo_get(ap33772s_pdo_fld *pdo_arr, enum sensor_channel chan,
			       enum sensor_attribute attr, struct sensor_value *val)
{
	if (val->val1 < 1 || val->val1 > 13) {
		return -ENOTSUP;
	}
	if (pdo_arr[val->val1 - 1].fixed.detect == 0 && pdo_arr[val->val1 - 1].pps.detect == 0 &&
	    pdo_arr[val->val1 - 1].avs.detect == 0) {
		return -ENODATA; /* SRCPDO not detected */
	}
	if (chan == SENSOR_CHAN_CURRENT) {
		uint8_t current = pdo_arr[val->val1 - 1].fixed.cur_max & 0x0f;

		if (attr == SENSOR_ATTR_LOWER_THRESH) {
			switch (current) {
			case 0:
				return sensor_value_from_milli(val, 0);
			default:
				return sensor_value_from_milli(val, current * 250 + 1000);
			case 15:
				return sensor_value_from_milli(val, 5000);
			}
		} else {
			switch (current) {
			default:
				return sensor_value_from_milli(val, current * 250 + 1240);
			case 14:
				return sensor_value_from_milli(val, 4990);
			case 15:
				return sensor_value_from_milli(val, 9999);
			}
		}
	}
	bool isEPR = (val->val1 >= 8); /* 1-7 for SPR, 8-12 for EPR */

	if (chan == SENSOR_CHAN_VOLTAGE) {
		if (pdo_arr[val->val1 - 1].fixed.type == 0) { /* Fixed PDO */
			return sensor_value_from_milli(val, pdo_arr[val->val1 - 1].fixed.vol_max *
								    (isEPR ? 200 : 100));
		}
		if ((!isEPR) && (pdo_arr[val->val1 - 1].pps.type == 1)) { /* PPS PDO */
			if (attr == SENSOR_ATTR_LOWER_THRESH) {
				return sensor_value_from_milli(val, 3300);
			} else {
				return sensor_value_from_milli(
					val, pdo_arr[val->val1 - 1].pps.vol_max * 100);
			}
		}
		if (isEPR && (pdo_arr[val->val1 - 1].avs.type == 1)) { /* AVS PDO */
			if (attr == SENSOR_ATTR_LOWER_THRESH) {
				return sensor_value_from_milli(val, 15000);
			} else {
				return sensor_value_from_milli(
					val, pdo_arr[val->val1 - 1].avs.vol_max * 200);
			}
		}
	}
	return -ENOTSUP;
}

static int ap33772s_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	const struct ap33772s_config *config = dev->config;
	struct ap33772s_data *data = dev->data;
	/* val can be set in sensor_attr_get - select PDO with val1 */
	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE && chan != SENSOR_CHAN_CURRENT) {
		return -ENOTSUP;
	}

	if ((chan == SENSOR_CHAN_VOLTAGE && attr == SENSOR_ATTR_LOWER_THRESH) ||
	    (chan == SENSOR_CHAN_VOLTAGE && attr == SENSOR_ATTR_UPPER_THRESH) ||
	    (chan == SENSOR_CHAN_CURRENT && attr == SENSOR_ATTR_LOWER_THRESH) ||
	    (chan == SENSOR_CHAN_CURRENT && attr == SENSOR_ATTR_UPPER_THRESH)) {
		return ap33772s_srcpdo_get(data->pdo_arr, chan, attr, val);
	}

	if (chan == SENSOR_CHAN_ALL) { /* refresh PDOs */
		if (i2c_burst_read_dt(&config->i2c, AP33772S_CMD_SRCPDO, (uint8_t *)data->pdo_arr,
				      sizeof(data->pdo_arr))) {
			LOG_ERR("Error reading PDOs");
			return -EIO;
		}
		return 0;
	}

	if (chan == SENSOR_CHAN_VOLTAGE && attr == SENSOR_ATTR_FULL_SCALE) {
		uint8_t neg_vol[2];

		if (i2c_burst_read_dt(&config->i2c, AP33772S_CMD_VREQ, neg_vol, sizeof(neg_vol))) {
			LOG_ERR("Error reading negotiated Voltage");
			return -EIO;
		}
		return sensor_value_from_milli(val, sys_get_le16(neg_vol) * 50);
		/* LSB 50mV */
	}

	if (chan == SENSOR_CHAN_CURRENT && attr == SENSOR_ATTR_FULL_SCALE) {
		uint8_t neg_cur[2];

		if (i2c_burst_read_dt(&config->i2c, AP33772S_CMD_IREQ, neg_cur, sizeof(neg_cur))) {
			LOG_ERR("Error reading negotiated Current");
			return -EIO;
		}
		return sensor_value_from_milli(val, sys_get_le16(neg_cur) * 10);
		/* LSB 10mA */
	}

	if (chan == SENSOR_CHAN_VOLTAGE && attr == SENSOR_ATTR_OFFSET) {
		uint8_t min_sel_vol;

		if (i2c_reg_read_byte_dt(&config->i2c, AP33772S_CMD_VSELMIN, &min_sel_vol)) {
			LOG_ERR("Error reading Minimum Selection Voltage");
			return -EIO;
		}
		return sensor_value_from_milli(val, min_sel_vol * 200);
		/* LSB 200mV */
	}
	return -ENOTSUP;
}

static int ap33772s_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct ap33772s_config *config = dev->config;
	struct ap33772s_data *data = dev->data;
	int32_t sel_vol = sensor_value_to_milli(&data->sel_vol);
	int32_t sel_cur = sensor_value_to_milli(&data->sel_cur);
	/* find the closest (equal or lower voltage PDO)*/
	/* find the closest (equal or higher current PDO)*/
	uint8_t pdo_buffer[2];

	if ((attr == SENSOR_ATTR_UPPER_THRESH) && (chan == SENSOR_CHAN_VOLTAGE)) {
		if ((sensor_value_to_milli(val) > AP33772S_MAX_SUPPORTED_VOLTAGE * 1000) ||
		    (sensor_value_to_milli(val) < 3300)) {
			return -EINVAL;
		}
		sel_vol = sensor_value_to_milli(val);
	} else if ((attr == SENSOR_ATTR_LOWER_THRESH) && (chan == SENSOR_CHAN_CURRENT)) {
		if ((sensor_value_to_milli(val) > 5000) || (sensor_value_to_milli(val) < 100)) {
			return -EINVAL;
		}
		sel_cur = sensor_value_to_milli(val);
	} else {
		return -ENOTSUP;
	}

	int32_t vol_dist[AP33772S_MAX_PDO];

	for (int pdo_index = 0; pdo_index < AP33772S_MAX_PDO; pdo_index++) {
		vol_dist[pdo_index] = INT32_MAX;
		/* Determine if it's SPR or EPR based on pdo_index */
		bool isEPR = (pdo_index >= 7 && pdo_index <= 12);
		/* 1-6 for SPR, 7-12 for EPR */

		if (data->pdo_arr[pdo_index].byte0 == 0 && data->pdo_arr[pdo_index].byte1 == 0) {
			continue; /* If both bytes are zero, PDO is not valid */
		}
		if (data->pdo_arr[pdo_index].fixed.type == 0) { /* fixed PDO */
			vol_dist[pdo_index] =
				data->pdo_arr[pdo_index].fixed.vol_max * (isEPR ? 200 : 100) -
				sel_vol;
		} else {
			/* PPS or AVS PDO */
			int32_t vol_max =
				data->pdo_arr[pdo_index].avs.vol_max * (isEPR ? 200 : 100);
			int32_t voltage_min = isEPR ? 15000 : 3300;

			if (sel_vol < voltage_min) {
				vol_dist[pdo_index] = sel_vol - voltage_min;
			} else if (sel_vol > vol_max) {
				vol_dist[pdo_index] = vol_max - sel_vol;
			} else {
				/* within PPS range */
				vol_dist[pdo_index] = 0;
			}
		}
	}

	/* ensure, that requested current can be delivered */
	for (int pdo_index = 0; pdo_index < AP33772S_MAX_PDO; pdo_index++) {
		uint8_t current = data->pdo_arr[pdo_index].fixed.cur_max & 0x0f;

		switch (current) {
		default:
			if (sel_cur > current * 250 + 1240) {
				vol_dist[pdo_index] = INT32_MAX;
			}
		case 14:
			if (sel_cur > 4990) {
				vol_dist[pdo_index] = INT32_MAX;
			}
		case 15:
			if (sel_cur > 5000) {
				vol_dist[pdo_index] = INT32_MAX;
			}
		}
	}

	/* find PDO that is closest to sel_vol (vol_dist <= 0) */
	int32_t closest_pdo_index = INT32_MAX;
	int32_t closest_pdo_distance = -INT32_MAX;

	for (int pdo_index = 0; pdo_index < AP33772S_MAX_PDO; pdo_index++) {
		if (vol_dist[pdo_index] <= 0) {
			if (closest_pdo_distance < vol_dist[pdo_index]) {
				closest_pdo_index = pdo_index;
				closest_pdo_distance = vol_dist[pdo_index];
			}
		}
	}

	if (closest_pdo_index == INT32_MAX) {
		LOG_ERR("No PDO meets %dmV, %dmA criteria", sel_vol, sel_cur);
		return -ENOTSUP;
	}

	bool isEPR = (closest_pdo_index >= 7 && closest_pdo_index <= 12);
	/* 1-6 for SPR, 7-12 for EPR */

	if (sel_vol > data->pdo_arr[closest_pdo_index].fixed.vol_max * (isEPR ? 200 : 100)) {
		sel_vol = data->pdo_arr[closest_pdo_index].fixed.vol_max * (isEPR ? 200 : 100);
	}

	if (sensor_value_from_milli(&data->sel_vol, sel_vol)) {
		LOG_ERR("Could not convert sel_vol");
	}

	if (sensor_value_from_milli(&data->sel_cur, sel_cur)) {
		LOG_ERR("Could not convert sel_cur");
	}

	data->selected_pd_reqmsg.req_fld.pdo_index = closest_pdo_index + 1;
	if (data->selected_pd_reqmsg.req_fld.pdo_index < 8) {
		/* 100mV units*/
		data->selected_pd_reqmsg.req_fld.voltage_sel =
			(data->sel_vol.val1 * 10 + data->sel_vol.val2 / 100000) & 0xff;
	} else {
		/* 200mV units */
		data->selected_pd_reqmsg.req_fld.voltage_sel =
			(data->sel_vol.val1 * 5 + data->sel_vol.val2 / 200000) & 0xff;
	}

	uint8_t fraction = (data->sel_cur.val2 / 250000) & 0x03;

	switch (data->sel_cur.val1 % 6) {
	case 0: /* min current is 1.00A */
		data->selected_pd_reqmsg.req_fld.current_sel = 0;
		break;
	case 1:
		data->selected_pd_reqmsg.req_fld.current_sel = 0 + fraction;
		break;
	case 2:
		data->selected_pd_reqmsg.req_fld.current_sel = 4 + fraction;
		break;
	case 3:
		data->selected_pd_reqmsg.req_fld.current_sel = 8 + fraction;
		break;
	case 4:
		data->selected_pd_reqmsg.req_fld.current_sel = 12 + fraction;
		if (fraction == 3) {
			data->selected_pd_reqmsg.req_fld.current_sel = 14;
		}
		break;
	case 5:
		data->selected_pd_reqmsg.req_fld.current_sel = 15;
		if (fraction) {
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	pdo_buffer[0] = data->selected_pd_reqmsg.byte0;
	pdo_buffer[1] = data->selected_pd_reqmsg.byte1;
	LOG_INF("pdo_buffer 0x%02x%02x PDO index %d", pdo_buffer[1], pdo_buffer[0],
		closest_pdo_index + 1);
	int rc = i2c_burst_write_dt(&config->i2c, AP33772S_CMD_PD_REQMSG, pdo_buffer,
				    sizeof(pdo_buffer));
	if (k_sem_take(&data->update_pdo_sem, K_MSEC(AP33772S_UPDATE_PDO_TIMEOUT_MSEC))) {
		LOG_ERR("PD REQMSG could not be sent");
		return -EAGAIN;
	}
	return rc;
}

static int ap33772s_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct ap33772s_config *config = dev->config;
	struct ap33772s_data *data = dev->data;

	int rc = 0;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE &&
	    chan != SENSOR_CHAN_AMBIENT_TEMP && chan != SENSOR_CHAN_CURRENT) {
		return -ENOTSUP;
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_VOLTAGE) {
		uint8_t voltage[2];

		if (i2c_burst_read_dt(&config->i2c, AP33772S_CMD_VOLTAGE, voltage,
				      sizeof(voltage))) {
			LOG_ERR("Error reading Voltage");
			return -EIO;
		}
		sensor_value_from_milli(&data->vout_voltage, sys_get_le16(voltage) * 80);
		/* LSB 80mV */
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_CURRENT) {
		uint8_t current;

		if (i2c_reg_read_byte_dt(&config->i2c, AP33772S_CMD_CURRENT, &current)) {
			LOG_ERR("Error reading Current");
			return -EIO;
		}
		sensor_value_from_milli(&data->vout_current, current * 24); /* LSB 24mA */
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP) {
		uint8_t temp;

		if (i2c_reg_read_byte_dt(&config->i2c, AP33772S_CMD_TEMP, &temp)) {
			LOG_ERR("Error reading Temperature");
			return -EIO;
		}
		data->temperature = temp;
	}

	return rc;
}

static int ap33772s_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	const struct ap33772s_config *config = dev->config;
	uint8_t voltage[2];
	uint8_t current;
	uint8_t temp;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		if (i2c_burst_read_dt(&config->i2c, AP33772S_CMD_VOLTAGE, voltage,
				      sizeof(voltage))) {
			LOG_ERR("Error reading Voltage");
			return -EIO;
		}
		return sensor_value_from_milli(val, sys_get_le16(voltage) * 80);
		/* LSB 80mV */
	case SENSOR_CHAN_CURRENT:
		if (i2c_reg_read_byte_dt(&config->i2c, AP33772S_CMD_CURRENT, &current)) {
			LOG_ERR("Error reading Current");
			return -EIO;
		}
		return sensor_value_from_milli(val, current * 24); /* LSB 24mA */
	case SENSOR_CHAN_AMBIENT_TEMP:
		if (i2c_reg_read_byte_dt(&config->i2c, AP33772S_CMD_TEMP, &temp)) {
			LOG_ERR("Error reading Temperature");
			return -EIO;
		}
		val->val1 = temp;
		val->val2 = 0;
		return 0;
	default:
		LOG_DBG("Channel not supported by device!");
		return -ENOTSUP;
	}

	return 0;
}

int ap33772s_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct ap33772s_data *data = dev->data;

	if (trig->type == SENSOR_TRIG_DATA_READY) {
		data->data_ready_handler = handler;
		if (handler == NULL) {
			LOG_WRN("ap33772s: no handler");
			return 0;
		}
		data->data_ready_trigger = trig;
		return 0;
	}
	return -ENOTSUP;
}

const char *cur_txt[] = {
	"Less than 1.24A", "1.25 - 1.49A", "1.50 - 1.74A", "1.75 - 1.99A",
	"2.00 - 2.24A",    "2.25 - 2.49A", "2.50 - 2.74A", "2.75 - 2.99A",
	"3.00 - 3.24A",    "3.25 - 3.49A", "3.50 - 3.74A", "3.75 - 3.99A",
	"4.00 - 4.24A",    "4.25 - 4.49A", "4.50 - 4.99A", "More than 5.00A",
};

const char *epr_vol_txt[] = {
	"Reserved",
	"15000mV~",
	"15000mV < VOLTAGE_MIN <= 20000mV ",
	"others",
};

const char *spr_vol_txt[] = {
	"Reserved",
	"3300mV~",
	"3300mV < VOLTAGE_MIN <= 5000mV ",
	"others",
};

void display_pdo_info(ap33772s_pdo_fld *pdo_arr, int pdo_index)
{
	char logtext[90];

	/*
	 * Determine if it's SPR or EPR based on pdo_index
	 * 1-6 for SPR, 7-12 for EPR
	 */
	bool isEPR = (pdo_index >= 7 && pdo_index <= 12);

	/* Check if both bytes are zero */
	if (pdo_arr[pdo_index].byte0 == 0 && pdo_arr[pdo_index].byte1 == 0) {
		return; /* If both bytes are zero, exit the function */
	}

	/* Print the PDO type and index */
	snprintf(logtext, sizeof(logtext),
		 "*%s %2d: ", pdo_index <= 6 ? "SRC_SPR_PDO" : "SRC_EPR_PDO", pdo_index + 1);

	/* Now, the individual fields can be accessed through the union */
	if (pdo_arr[pdo_index].fixed.type == 0) {
		/* Fixed PDO - Print parsed values */
		snprintf(logtext + strlen(logtext), sizeof(logtext) - strlen(logtext),
			 "Fixed PDO: %dmV %s",
			 pdo_arr[pdo_index].fixed.vol_max * (isEPR ? 200 : 100),
			 cur_txt[pdo_arr[pdo_index].fixed.cur_max & 0x0f]);
		/* Voltage in 200mV units for EPR, 100mV for SPR */
	} else {
		/* PPS or AVS PDO - Print parsed values */
		snprintf(logtext + strlen(logtext), sizeof(logtext) - strlen(logtext), "%s",
			 isEPR ? "AVS PDO: " : "PPS PDO: ");
		if (isEPR) {
			snprintf(logtext + strlen(logtext), sizeof(logtext) - strlen(logtext), "%s",
				 epr_vol_txt[pdo_arr[pdo_index].avs.voltage_min & 0x03]);
			/* Assuming displayEPRVoltageMin function is available */
		} else {
			snprintf(logtext + strlen(logtext), sizeof(logtext) - strlen(logtext), "%s",
				 spr_vol_txt[pdo_arr[pdo_index].pps.voltage_min & 0x03]);
			/* Assuming displaySPRVoltageMin function is available */
		}
		snprintf(logtext + strlen(logtext), sizeof(logtext) - strlen(logtext), "%dmV %s",
			 pdo_arr[pdo_index].fixed.vol_max * (isEPR ? 200 : 100),
			 cur_txt[pdo_arr[pdo_index].fixed.cur_max & 0x0f]);
		/* Voltage in 200mV units for EPR, 100mV for SPR */
	}
	LOG_INF("%s", logtext);
}

static int ap33772s_process(const struct device *dev)
{
	const struct ap33772s_config *config = dev->config;
	struct ap33772s_data *data = dev->data;
	uint8_t status;

	/* reg read also clears status */
	if (i2c_reg_read_byte_dt(&config->i2c, AP33772S_CMD_STATUS, &status)) {
		LOG_ERR("Failed reading status");
		return -EIO;
	}

	if (status == 0) {
		return 0;
	}
	if (IS_BIT_SET(status, STARTED_BIT)) {
		LOG_INF("Started");
	}
	if (IS_BIT_SET(status, READY_BIT)) {
		uint8_t msgrlt;

		k_sem_give(&data->update_pdo_sem);
		LOG_INF("Ready");

		if (i2c_burst_read_dt(&config->i2c, AP33772S_CMD_PD_MSGRLT, &msgrlt,
				      sizeof(msgrlt))) {
			LOG_INF("Error reading MSGRLT");
			return -EIO;
		}
		switch (msgrlt) {
		case 0:
			LOG_INF("System busy or no response");
			break;
		case 1:
			LOG_INF("Success");
			break;
		case 2:
			LOG_INF("Invalid command or argument");
			break;
		case 4:
			LOG_INF("Command not supported or rejected");
			break;
		case 5:
			LOG_INF("Transaction failed. "
				"No GoodCRC is received after sending");
			break;
		default:
			LOG_INF("MSGRLT Reserved");
			break;
		}
	}
	if (IS_BIT_SET(status, NEWPDO_BIT)) {
		LOG_INF("New PDO");
		if (i2c_burst_read_dt(&config->i2c, AP33772S_CMD_SRCPDO, (uint8_t *)data->pdo_arr,
				      sizeof(data->pdo_arr))) {
			LOG_ERR("Error reading PDOs");
			return -EIO;
		}
		if (data->data_ready_handler != NULL) {
			data->data_ready_handler(dev, data->data_ready_trigger);
		}
		for (int pdo_index = 0; pdo_index < AP33772S_MAX_PDO; pdo_index++) {
			display_pdo_info(data->pdo_arr, pdo_index);
		}
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(flip_gpios)
		if (config->flip_gpio != NULL) {
			LOG_INF("CC is connected to %s",
				gpio_pin_get_dt(config->flip_gpio) ? "CC2" : "CC1");
		}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(flip_gpios) */
		/*
		 * If the PD Source supports EPR Mode, after the first negotiation,
		 * the AP33772S will try to enter EPR Mode when PDCONFIG is set at
		 * EPR_MODE bit. If successfully entering EPR Mode, the AP33772S
		 * stores the EPR Source Capability in SRC_EPR_PDOx registers
		 * and enables EPR request.
		 * No Interrupt or change in status is generated! -> poll for EPR
		 */
		k_timer_start(&data->epr_timer, K_MSEC(config->epr_delay_ms), K_NO_WAIT);
	}
	if (IS_BIT_SET(status, UVP_BIT)) {
		LOG_INF("undervoltage protection");
	}
	if (IS_BIT_SET(status, OVP_BIT)) {
		LOG_INF("overvoltage protection");
	}
	if (IS_BIT_SET(status, OCP_BIT)) {
		LOG_INF("overcurrent protection");
	}
	if (IS_BIT_SET(status, OTP_BIT)) {
		LOG_INF("over temperature protection");
	}

	return 0;
}

static void ap33772s_epr_work_handler(struct k_work *epr_work)
{
	struct ap33772s_data *data = CONTAINER_OF(epr_work, struct ap33772s_data, epr_work);
	const struct ap33772s_config *config = data->dev->config;
	uint8_t pdo_arr[sizeof(data->pdo_arr)];

	if (i2c_burst_read_dt(&config->i2c, AP33772S_CMD_SRCPDO, pdo_arr, sizeof(data->pdo_arr))) {
		LOG_ERR("Error reading PDOs");
		return;
	}
	if (memcmp((uint8_t *)data->pdo_arr, pdo_arr, sizeof(data->pdo_arr)) == 0) {
		return;
	}
	LOG_INF("EPR available");
	memcpy((uint8_t *)data->pdo_arr, pdo_arr, sizeof(data->pdo_arr));
	if (data->data_ready_handler != NULL) {
		data->data_ready_handler(data->dev, data->data_ready_trigger);
	}
	for (int pdo_index = 0; pdo_index < AP33772S_MAX_PDO; pdo_index++) {
		display_pdo_info(data->pdo_arr, pdo_index);
	}
}

static void ap33772s_poll_work_handler(struct k_work *poll_work)
{
	struct ap33772s_data *data = CONTAINER_OF(poll_work, struct ap33772s_data, poll_work);

	ap33772s_process(data->dev);
}

static void ap33772s_poll_timer_handler(struct k_timer *poll_timer)
{
	struct ap33772s_data *data = CONTAINER_OF(poll_timer, struct ap33772s_data, poll_timer);

	k_work_submit(&data->poll_work);
}

static void ap33772s_epr_timer_handler(struct k_timer *epr_timer)
{
	struct ap33772s_data *data = CONTAINER_OF(epr_timer, struct ap33772s_data, epr_timer);

	k_work_submit(&data->epr_work);
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
static void ap33772s_isr_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct ap33772s_data *data = CONTAINER_OF(cb, struct ap33772s_data, int_gpio_cb);

	k_work_submit(&data->poll_work);
}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */

static int ap33772s_init(const struct device *dev)
{
	const struct ap33772s_config *config = dev->config;
	struct ap33772s_data *data = dev->data;
	int ret;

	/* Initialize time 100 ms */
	k_msleep(100);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("Bus device is not ready");
		return -EINVAL;
	}

	data->dev = dev;

	k_work_init(&data->epr_work, ap33772s_epr_work_handler);
	k_work_init(&data->poll_work, ap33772s_poll_work_handler);
	k_timer_init(&data->epr_timer, ap33772s_epr_timer_handler, NULL);

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (config->int_gpio == NULL) {
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */
		LOG_DBG("ap33772s driver in polling mode");
		k_timer_init(&data->poll_timer, ap33772s_poll_timer_handler, NULL);
		k_timer_start(&data->poll_timer, K_MSEC(config->poll_interval_ms),
			      K_MSEC(config->poll_interval_ms));
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	} else {
		LOG_DBG("ap33772s driver in interrupt mode");
		if (!gpio_is_ready_dt(config->int_gpio)) {
			LOG_ERR("Interrupt GPIO controller device not ready "
				"(missing device tree node?)");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(config->int_gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Could not configure interrupt GPIO pin");
			return ret;
		}

		ret = gpio_pin_interrupt_configure_dt(config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure interrupt GPIO interrupt");
			return ret;
		}

		gpio_init_callback(&data->int_gpio_cb, ap33772s_isr_handler,
				   BIT(config->int_gpio->pin));

		ret = gpio_add_callback_dt(config->int_gpio, &data->int_gpio_cb);
		if (ret < 0) {
			LOG_ERR("Could not set gpio callback");
			return ret;
		}

		if (gpio_pin_get_dt(config->int_gpio)) {
			k_work_submit(&data->poll_work);
		}
	}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(flip_gpios)
	if (config->flip_gpio != NULL) {
		if (!gpio_is_ready_dt(config->flip_gpio)) {
			LOG_WRN("Flip GPIO controller device not ready "
				"(missing device tree node?)");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(config->flip_gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Could not configure flip GPIO pin");
			return ret;
		}
	}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(flip_gpios) */

	ret = i2c_reg_write_byte_dt(&config->i2c, AP33772S_CMD_TR25, config->tr25);
	if (ret < 0) {
		LOG_ERR("Could not set TR25");
		return ret;
	}
	ret = i2c_reg_write_byte_dt(&config->i2c, AP33772S_CMD_TR50, config->tr50);
	if (ret < 0) {
		LOG_ERR("Could not set TR50");
		return ret;
	}
	ret = i2c_reg_write_byte_dt(&config->i2c, AP33772S_CMD_TR75, config->tr75);
	if (ret < 0) {
		LOG_ERR("Could not set TR75");
		return ret;
	}
	ret = i2c_reg_write_byte_dt(&config->i2c, AP33772S_CMD_TR100, config->tr100);
	if (ret < 0) {
		LOG_ERR("Could not set TR100");
		return ret;
	}

	data->selected_pd_reqmsg.byte0 = 0xFF; /* no PDO selected, max U,I */
	data->selected_pd_reqmsg.byte1 = 0x0F; /* no PDO selected, max U,I */
	sensor_value_from_milli(&data->sel_vol, 5000);
	sensor_value_from_milli(&data->sel_cur, 1000);

	k_sem_init(&data->update_pdo_sem, 0, 1);
	ap33772s_process(dev); /* read status and clear irq */

	/* sink PPS, AVS capability and EPR mode are enabled by default */
	/* in AP33772S_CMD_PDCONFIG*/

	/* all relevant bits are set in AP33772S_CMD_CONFIG */
	/* we want to be informed about everything.... */
	ret = i2c_reg_write_byte_dt(&config->i2c, AP33772S_CMD_MASK,
				    BIT(STARTED_BIT) | BIT(READY_BIT) | BIT(NEWPDO_BIT) |
					    BIT(UVP_BIT) | BIT(OVP_BIT) | BIT(OCP_BIT) |
					    BIT(OTP_BIT));
	if (ret < 0) {
		LOG_ERR("Could not set Interrupt enable mask");
		return ret;
	}

	return 0;
}

static DEVICE_API(sensor, ap33772s_api) = {
	.attr_set = ap33772s_attr_set,
	.attr_get = ap33772s_attr_get,
	.sample_fetch = ap33772s_sample_fetch,
	.channel_get = ap33772s_channel_get,
	.trigger_set = ap33772s_trigger_set,
};

#define AP33772S_INIT(index)                                                                       \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(index, int_gpios),					   \
		(static struct gpio_dt_spec ap33772s_int_gpio_##index =				   \
			GPIO_DT_SPEC_INST_GET(index, int_gpios);))                               \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(index, flip_gpios),					   \
		(static struct gpio_dt_spec ap33772s_flip_gpio_##index =			   \
			GPIO_DT_SPEC_INST_GET(index, flip_gpios);))                              \
	static const struct ap33772s_config ap33772s_config_##index = {				   \
		.i2c = I2C_DT_SPEC_INST_GET(index),						   \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(index, int_gpios),				   \
			(.int_gpio = &ap33772s_int_gpio_##index,))				   \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(index, flip_gpios),				   \
			(.flip_gpio = &ap33772s_flip_gpio_##index,))				   \
		.epr_delay_ms = DT_INST_PROP(index, epr_delay_ms),				   \
		.poll_interval_ms = DT_INST_PROP(index, poll_interval_ms),			   \
		.tr25 = DT_INST_PROP(index, tr25),						   \
		.tr50 = DT_INST_PROP(index, tr50,						   \
		.tr75 = DT_INST_PROP(index, tr75),						   \
		.tr100 = DT_INST_PROP(index, tr100),						   \
	};                                                                                         \
	static struct ap33772s_data ap33772s_data_##index;                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(index, ap33772s_init, NULL, &ap33772s_data_##index,           \
				     &ap33772s_config_##index, POST_KERNEL,                        \
				     CONFIG_SENSOR_INIT_PRIORITY, &ap33772s_api);

DT_INST_FOREACH_STATUS_OKAY(AP33772S_INIT)
