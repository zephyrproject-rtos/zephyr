
/**
 * @file
 * @brief Extended public API for LIS3DHH
 *
 * This exposes an API for a configuration of several
 * options of the LIS3DHH sensor during runtime.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_LIS3DHH_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_LIS3DHH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <drivers/sensor.h>


enum lis3dhh_filter {
	filter_FIR,
	filter_IIR
};

enum lis3dhh_bandwidth {
	bandwidth_235,
	bandwidth_440
};

enum lis3dhh_pp_od {
	open_drain,
	push_pull
};

enum lis3dhh_fifo_mode {
	FIFO_BYPASS,
	FIFO_NORMAL,
	FIFO_CONTINUOUS_TO_FIFO,
	FIFO_BYPASS_TO_CONTINUOUS,
	FIFO_CONTINUOUS
};

int lis3dhh_configure_fifo_spi_high_speed(const struct device *dev,
						bool enable);

int lis3dhh_configure_fifo_threshold(const struct device *dev,
						uint8_t threshold);

int lis3dhh_configure_fifo_mode(const struct device *dev,
						enum lis3dhh_fifo_mode fifo_mode);

int lis3dhh_configure_fifo(const struct device *dev, bool enable);

int lis3dhh_configure_pp_od_int2(const struct device *dev,
						enum lis3dhh_pp_od pp_od);

int lis3dhh_configure_pp_od_int1(const struct device *dev,
						enum lis3dhh_pp_od pp_od);

int lis3dhh_configure_bandwidth(const struct device *dev,
						enum lis3dhh_bandwidth bandwidth);

int lis3dhh_configure_filter(const struct device *dev,
						enum lis3dhh_filter filter);

int lis3dhh_configure_int1_as_ext_async_input_trig(const struct device *dev,
						bool enable);

int lis3dhh_configure_bdu(const struct device *dev,
						bool enable);

int lis3dhh_configure_if_add_inc(const struct device *dev,
						bool enable);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_LIS3DHH_H_ */