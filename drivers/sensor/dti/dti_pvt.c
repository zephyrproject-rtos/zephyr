/* DTI Process, Voltage and Thermal Sensor Driver
 *
 * Copyright (c) 2024 Rapid Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT dti_pvt

#include <zephyr/init.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <errno.h>

#include "dti_pvt.h"

LOG_MODULE_REGISTER(DTI_PVT, CONFIG_DTI_PVT_LOG_LEVEL);

/// Contract: Any buffer based operation should return the amount of bytes
/// written as it is possible that not all bytes can be pushed in a single
/// operation, which means that to complete the transaction the process would
/// block
struct result_status {
  // encodes the operation status
  uint16_t status;
  // utility for tracking the progress of one operation
  uint16_t data;
};

static int16_t decode_pvt_voltage_code(int16_t code, uint8_t process_corner, uint8_t voltage_range) 
{
  static const int16_t voltage_codes_lut[10][5] = {
      //{vmrange0_val_10mV, vmrange1_val_10mV, ff_1_98, ff_1_62, tt_1_80
      {10, 20, 45, 50, 50},     {20, 40, 97, 100, 100},
      {30, 60, 148, 150, 150},  {40, 80, 199, 200, 201},
      {50, 100, 250, 250, 251}, {60, 120, 301, 301, 302},
      {70, 140, 352, 352, 352}, {80, 160, 403, 403, 404},
      {90, 180, 454, 454, 455}, {98, 196, 495, 495, 497}};
  int16_t value = 0;
  for (int i = 0; i < 9; i++) {
    if (code >= voltage_codes_lut[i][process_corner] &&
        code <= voltage_codes_lut[i + 1][process_corner]) {
      if (code == voltage_codes_lut[i][process_corner]) {
        value = voltage_codes_lut[i][voltage_range];
      } else if (code == voltage_codes_lut[i + 1][process_corner]) {
        value = voltage_codes_lut[i + 1][voltage_range];
      } else {
        // calculate a local linear function
        int16_t delta_x = voltage_codes_lut[i + 1][process_corner] -
                          voltage_codes_lut[i][process_corner];
        int16_t delta_y = voltage_codes_lut[i + 1][voltage_range] -
                          voltage_codes_lut[i][voltage_range];
        // multiply by 100 to increase step precision
        int16_t m = (delta_y * 100) / delta_x;
        value = voltage_codes_lut[i][voltage_range] +
                (m * (code - voltage_codes_lut[i][process_corner])) / 100;
      }
    }
  }
  return value;
}

static int16_t decode_pvt_temperature_code(int16_t code, uint8_t process_corner) 
{
  static const int16_t temperature_codes_lut[18][7] = {
      // temp, ff/1.62, ff/1.98, ss/1.62, ss/1.98, tt/1.62, tt/1.98
      {-40, 464, 463, 452, 449, 458, 456}, {-30, 440, 439, 428, 426, 434, 432},
      {-20, 416, 415, 405, 402, 410, 408}, {-10, 392, 391, 381, 378, 386, 384},
      {0, 368, 367, 357, 355, 362, 360},   {10, 344, 343, 333, 331, 337, 336},
      {20, 320, 319, 309, 307, 313, 312},  {30, 295, 294, 286, 284, 289, 288},
      {40, 271, 270, 262, 260, 265, 264},  {50, 247, 246, 238, 236, 241, 240},
      {60, 223, 221, 214, 212, 217, 216},  {70, 198, 197, 191, 189, 193, 192},
      {80, 174, 172, 167, 165, 169, 168},  {90, 149, 148, 143, 141, 145, 144},
      {100, 124, 123, 119, 117, 121, 119}, {110, 99, 97, 96, 93, 96, 95},
      {120, 74, 72, 72, 69, 71, 70},       {130, 49, 46, 47, 45, 47, 46}};
  int16_t value = 0;
  for (int i = 17; i > 0; i--) {
    if (code >= temperature_codes_lut[i][process_corner] &&
        code <= temperature_codes_lut[i - 1][process_corner]) {
      if (code == temperature_codes_lut[i][process_corner]) {
        value = temperature_codes_lut[i][0];
      } else if (code == temperature_codes_lut[i - 1][process_corner]) {
        value = temperature_codes_lut[i - 1][0];
      } else {
        // calculate a local linear function
        int16_t delta_x = temperature_codes_lut[i - 1][process_corner] -
                          temperature_codes_lut[i][process_corner];
        int16_t delta_y =
            temperature_codes_lut[i - 1][0] - temperature_codes_lut[i][0];
        // multiply by 100 to increase step precision
        int16_t m = (delta_y * 100) / delta_x;
        value = temperature_codes_lut[i][0] +
                (m * (code - temperature_codes_lut[i][6])) / 100;
      }
    }
  }
  return value;
}

static int dti_pvt_request(struct dti_pvt_reg* regs, size_t requests) {
  if ((regs->stt & DTI_PVT_STT_REQ_READY) != DTI_PVT_STT_REQ_READY) {
    return EWOULDBLOCK;
  } else {
    regs->req |= requests;
    return 0;
  }
}

struct result_status dti_pvt_poll_results(
    struct dti_pvt_reg *regs, struct dti_pvt_results* results,
    size_t pending_requests, uint8_t voltage_range,
    uint8_t vm_process_corner, uint8_t ts_process_corner) 
{
	// For this function, We are going to use the result_status `data` to keep track
	// of the pending requests so we can create a blocking function that
	// only tries to request the values not already read while still allowing
	// for reading the results in disorder
	struct result_status op_result = {.data = pending_requests, .status = 0};

	bool is_done, had_error;

	// Check if we want the process monitor values
	if (pending_requests & DTI_PVT_REQUEST_PROCESS_MONITOR) {
		// Check if the results for the Process monitor are done
		is_done = (regs->result & DTI_PVT_MASK_RESULT_PM_DONE) != 0;
		had_error = (regs->stt & DTI_PVT_STT_PROCESS_MONITOR_ERROR) != 0;

		if (is_done && !had_error) {
			results->process_diff_percentage =
				((regs->result) & DTI_PVT_MASK_RESULT_PM_DIFF) >>
				DTI_PVT_OFFSET_RESULT_PM_DIFF;
			results->process_status =
				((regs->result) & DTI_PVT_MASK_RESULT_PM_FAST) >>
				DTI_PVT_OFFSET_RESULT_PM_FAST;
			// Let the caller know process monitor results are no longer pending
			op_result.data &= ~DTI_PVT_REQUEST_PROCESS_MONITOR;

		} else if (had_error) {
			results->error_flags |= DTI_PVT_REQUEST_PROCESS_MONITOR;
			results->process_diff_percentage = 0xFFFF;
			results->process_status = 0xFFFF;
		} else {
			// If we wanted these values and couldn't get them , tell the caller
			// the operation would block
			op_result.status = EWOULDBLOCK;
		}
	}
	// Same flow for process monitor is repeated for the other monitors!
	if (pending_requests & DTI_PVT_REQUEST_VOLTAGE_MONITOR) {
		is_done = (regs->result & DTI_PVT_MASK_RESULT_VM_DONE) != 0;
		had_error = (regs->stt & DTI_PVT_STT_VOLTAGE_MONITOR_ERROR) != 0;

		if (is_done && !had_error) {

			results->voltage =
				decode_pvt_voltage_code((regs->result & DTI_PVT_MASK_RESULT_VM_C) >>
										DTI_PVT_OFFSET_RESULT_VM_C,
										CONFIG_DTI_PVT_VM_PROCESS_CORNER, 
										CONFIG_DTI_PVT_RST_CONF_VMRANGE);

			op_result.data &= ~DTI_PVT_REQUEST_VOLTAGE_MONITOR;

		} else if (had_error) {
			results->error_flags |= DTI_PVT_REQUEST_VOLTAGE_MONITOR;
			results->voltage = 0xFFFF;
		} else {
			op_result.status = EWOULDBLOCK;
		}
	}
	if (pending_requests & DTI_PVT_REQUEST_THERMAL_SENSOR_MONITOR) {
		is_done = (regs->result & DTI_PVT_MASK_RESULT_TS_DONE) != 0;
		had_error = (regs->stt & DTI_PVT_STT_THERMAL_SENSOR_MONITOR_ERROR) != 0;

		if (is_done && !had_error) {
			results->temperature = decode_pvt_temperature_code(
				(regs->result & DTI_PVT_MASK_RESULT_TS_C) >>
					DTI_PVT_OFFSET_RESULT_TS_C,
				CONFIG_DTI_PVT_TS_PROCESS_CORNER);
			op_result.data &= ~DTI_PVT_REQUEST_THERMAL_SENSOR_MONITOR;

		} else if (had_error) {
			results->error_flags |= DTI_PVT_REQUEST_THERMAL_SENSOR_MONITOR;
			results->temperature = 0xFFFF;
		} else {
			op_result.status = EWOULDBLOCK;
		}
	}

	if (results->error_flags != 0) {
		LOG_ERR("error_flags value:0x%08x\n", results->error_flags);
		op_result.status = EINVAL;
	}

	return op_result;
}

static int dti_pvt_read_results(
								const struct device *dev, 
								uint8_t voltage_range,
								size_t pending_requests
							   ) 
{
	int ret = 0;
	struct result_status res_status = {0};

	struct dti_pvt_data *dti_pvt_data = (struct dti_pvt_data *)dev->data;
	
	while (dti_pvt_request(dti_pvt_data->pvt_regs, pending_requests));
	// Values are not initialized to 0 because 0 is a valid value for the
	// different monitors, so if an error is found and the error flags are
	// discarded, there is no mechanism for the caller to know for sure if there
	// was a failure on the monitors. By using 0x7FFF as the initial value we
	// can discard between the two possible failure modes:
	// - Monitor failure: The error conditions that are read in the stt register.
	//   These are the ones we detect if the result never changes from 0x7FFF
	// - Sensor malfunction: A sensor malfunction doesn't provoke a monitor
	//   error, but in many cases it will provoke a 0 to be read
	dti_pvt_data->results.process_status = 0x7FFF;
	dti_pvt_data->results.process_diff_percentage = 0x7FFF;
	dti_pvt_data->results.voltage = 0x7FFF;
	dti_pvt_data->results.temperature = 0x7FFF;
	dti_pvt_data->results.error_flags = 0;

	// result.data here is equal to the pending requests
	res_status.status = EWOULDBLOCK;
	res_status.data = 0;

	while (res_status.status == EWOULDBLOCK) {
		res_status =	dti_pvt_poll_results(
												dti_pvt_data->pvt_regs, &dti_pvt_data->results, 
												pending_requests, voltage_range,
												CONFIG_DTI_PVT_VM_PROCESS_CORNER, 
												CONFIG_DTI_PVT_TS_PROCESS_CORNER
											);
		pending_requests = res_status.data;
	}
	
	if (res_status.status == EINVAL) {
		LOG_ERR("Error Reading Results\n");
		ret = EINVAL;		
	}

	return ret;
}

static int dti_pvt_get_all_values(const struct device *dev, enum sensor_channel chan) {
	
	int ret = 0;
	
  	ret = dti_pvt_read_results(
								dev, 
								CONFIG_DTI_PVT_RST_CONF_VMRANGE,
								DTI_PVT_REQUEST_PROCESS_MONITOR |
								DTI_PVT_REQUEST_VOLTAGE_MONITOR |
								DTI_PVT_REQUEST_THERMAL_SENSOR_MONITOR
	  						  );

	return ret;
}

static int dti_pvt_get_temperature(const struct device *dev)
{
	int ret = 0;

	ret = dti_pvt_read_results(
								dev,
								0, 
								DTI_PVT_REQUEST_THERMAL_SENSOR_MONITOR
							  );

	return ret;
}

static int dti_pvt_get_voltage(const struct device *dev) 
{
	int ret = 0;

	ret = dti_pvt_read_results(
								dev, 
								CONFIG_DTI_PVT_RST_CONF_VMRANGE, 
								DTI_PVT_REQUEST_VOLTAGE_MONITOR
							  );

	return ret;
}

static int dti_pvt_get_channel(
								const struct device *dev, 
								enum sensor_channel chan, 
								struct sensor_value *val
							  )
{
	int ret = 0;

	switch(chan) {
		case(SENSOR_CHAN_DIE_TEMP): {
			ret = (uint32_t)dti_pvt_get_temperature(dev);
			val->val1 = ((struct dti_pvt_data *)dev->data)->results.temperature;
		} break;
		case(SENSOR_CHAN_VOLTAGE): {
			ret = (uint32_t)dti_pvt_get_voltage(dev);
			val->val1 = ((struct dti_pvt_data *)dev->data)->results.voltage;
		} break;
		case(SENSOR_CHAN_ALL): {
			ret = dti_pvt_get_all_values(dev, SENSOR_CHAN_ALL);
			(val++)->val1 = ((struct dti_pvt_data *)dev->data)->results.temperature;
			(val++)->val1 = ((struct dti_pvt_data *)dev->data)->results.voltage;
			(val++)->val1 = ((struct dti_pvt_data *)dev->data)->results.process_status;
		} break;
		default: {
			ret = EINVAL;
		}
	}

	return ret;
}

static int dti_pvt_init(const struct device *dev) {
	
	int ret = 0;
	
	const struct dti_pvt_config *dti_pvt_config = (struct dti_pvt_config *)dev->config;
	struct dti_pvt_data *dti_pvt_data 	=	(struct dti_pvt_data *)dev->data;

	uint32_t freq_range = (dti_pvt_config->clk_mhz > 75) ? DTI_PVT_CONF_FREQRANGE_200M_75M
										: DTI_PVT_CONF_FREQRANGE_UNDER_75M;

	uint32_t div = (((dti_pvt_config->clk_mhz > 75) ? 1200 : 600) / dti_pvt_config->clk_mhz) - 1;
	
	if (dti_pvt_config->clk_mhz < 38) {
		div = DTI_PVT_CONF_DIV_FREQ_75M;
	}

	uint32_t treg_en = ((100 * dti_pvt_config->clk_mhz) / 1000) + 1;    //(RU(100ns/tCK))
	uint32_t treg_mstep = ((30 * dti_pvt_config->clk_mhz) / 1000) + 1;  //(RU(100ns/tCK))

	// EN = 100e-9 * clk = 14 , MSTEP = 30e-9 * clk
	dti_pvt_data->pvt_regs->treg = CONFIG_DTI_PVT_RST_TREG_TIMEOUT << DTI_PVT_OFFSET_TREG_TIMEOUT |
				treg_en << DTI_PVT_OFFSET_TREG_EN |
				treg_mstep << DTI_PVT_OFFSET_TREG_MSTEP;

	// freq of input clk should be the SCU's
	dti_pvt_data->pvt_regs->conf = CONFIG_DTI_PVT_RST_CONF_TRIM << DTI_PVT_OFFSET_CONF_TRIM |
				freq_range << DTI_PVT_OFFSET_CONF_FREQRANGE |
				div << DTI_PVT_OFFSET_CONF_DIV |
				CONFIG_DTI_PVT_RST_CONF_VMRANGE << DTI_PVT_OFFSET_CONF_VMRANGE;
	// No offset configuration as only one corner is being considered.

	return ret;
}

static int dti_pvt_attr_get(const struct device *dev,
				 enum sensor_channel chan,
				 enum sensor_attribute attr,
				 struct sensor_value *val)
{
	(void)dev;
	(void)chan;
	(void)attr;
	(void)val;

	return ENOSYS;
}

static int dti_pvt_attr_set(const struct device *dev,
				 enum sensor_channel chan,
				 enum sensor_attribute attr,
				 const struct sensor_value *val)
{
	(void)dev;
	(void)chan;
	(void)attr;
	(void)val;

	return ENOSYS;
}

static int dti_pvt_sensor_submit(const struct device *sensor, struct rtio_iodev_sqe *sqe)
{
	(void)sensor;
	(void)sqe;

	return ENOSYS;
}

static int dti_pvt_trigger_set( const struct device *dev,
								const struct sensor_trigger *trig,
								sensor_trigger_handler_t handler
							  )
{
	(void)dev;
	(void)trig;
	(void)handler;

	return ENOSYS;
}

static int dti_pvt_get_decoder(const struct device *dev,
				    const struct sensor_decoder_api **api)
{
	(void)dev;
	(void)api;

	return ENOSYS;
}

static const struct sensor_driver_api s_dti_pvt_api = {
	.sample_fetch = dti_pvt_get_all_values,
	.channel_get = dti_pvt_get_channel,
	.attr_get = dti_pvt_attr_get,
	.attr_set = dti_pvt_attr_set,
	.submit = dti_pvt_sensor_submit,
	.trigger_set = dti_pvt_trigger_set,
	.get_decoder = dti_pvt_get_decoder
};

#define DTI_PVT_INIT(inst) 													\
	static const struct dti_pvt_config dti_pvt_##inst##_config =			\
	{																		\
		.clk_mhz = (DT_PROP(DT_DRV_INST(inst), clock_frequency) / 1000000)  \
	};																		\
	static struct dti_pvt_data dti_pvt_##inst##_data = 						\
	{																		\
		.pvt_regs = (struct dti_pvt_reg*)DT_REG_ADDR(DT_DRV_INST(inst)), 	\
		.results  = {0}  													\
	};																		\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, dti_pvt_init, NULL,  				\
	&dti_pvt_##inst##_data, &dti_pvt_##inst##_config, POST_KERNEL, 			\
	CONFIG_SENSOR_INIT_PRIORITY, &s_dti_pvt_api); 							

DT_INST_FOREACH_STATUS_OKAY(DTI_PVT_INIT)
