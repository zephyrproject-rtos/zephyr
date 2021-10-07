/*
 * Copyright (c) 2018, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef ZEPHYR_DRIVER_SENSOR_SGP30_SGP30_H_
#define ZEPHYR_DRIVER_SENSOR_SGP30_SGP30_H_

#define SGP30_PRODUCT_TYPE 0
static const uint8_t SGP30_I2C_ADDRESS = 0x58;

/* command and constants for reading the serial ID */
#define SGP30_CMD_GET_SERIAL_ID 0x3682
#define SGP30_CMD_GET_SERIAL_ID_DURATION_US 500
#define SGP30_CMD_GET_SERIAL_ID_WORDS 3

/* command and constants for reading the featureset version */
#define SGP30_CMD_GET_FEATURESET 0x202f
#define SGP30_CMD_GET_FEATURESET_DURATION_US 10000
#define SGP30_CMD_GET_FEATURESET_WORDS 1

/* command and constants for on-chip self-test */
#define SGP30_CMD_MEASURE_TEST 0x2032
#define SGP30_CMD_MEASURE_TEST_DURATION_US 220000
#define SGP30_CMD_MEASURE_TEST_WORDS 1
#define SGP30_CMD_MEASURE_TEST_OK 0xd400

/* command and constants for IAQ init */
#define SGP30_CMD_IAQ_INIT 0x2003
#define SGP30_CMD_IAQ_INIT_DURATION_US 10000

/* command and constants for IAQ measure */
#define SGP30_CMD_IAQ_MEASURE 0x2008
#define SGP30_CMD_IAQ_MEASURE_DURATION_US 12000
#define SGP30_CMD_IAQ_MEASURE_WORDS 2

/* command and constants for getting IAQ baseline */
#define SGP30_CMD_GET_IAQ_BASELINE 0x2015
#define SGP30_CMD_GET_IAQ_BASELINE_DURATION_US 10000
#define SGP30_CMD_GET_IAQ_BASELINE_WORDS 2

/* command and constants for setting IAQ baseline */
#define SGP30_CMD_SET_IAQ_BASELINE 0x201e
#define SGP30_CMD_SET_IAQ_BASELINE_DURATION_US 10000

/* command and constants for raw measure */
#define SGP30_CMD_RAW_MEASURE 0x2050
#define SGP30_CMD_RAW_MEASURE_DURATION_US 25000
#define SGP30_CMD_RAW_MEASURE_WORDS 2

/* command and constants for setting absolute humidity */
#define SGP30_CMD_SET_ABSOLUTE_HUMIDITY 0x2061
#define SGP30_CMD_SET_ABSOLUTE_HUMIDITY_DURATION_US 10000

/* command and constants for getting TVOC inceptive baseline */
#define SGP30_CMD_GET_TVOC_INCEPTIVE_BASELINE 0x20b3
#define SGP30_CMD_GET_TVOC_INCEPTIVE_BASELINE_DURATION_US 10000
#define SGP30_CMD_GET_TVOC_INCEPTIVE_BASELINE_WORDS 1

/* command and constants for setting TVOC baseline */
#define SGP30_CMD_SET_TVOC_BASELINE 0x2077
#define SGP30_CMD_SET_TVOC_BASELINE_DURATION_US 10000

#define SGP30_WORD_SIZE 2
#define SGP30_NUM_WORDS(data) (sizeof(data) / SGP30_WORD_SIZE)

#define SGP30_CRC8_POLYNOMIAL 0x31
#define SGP30_CRC8_INIT 0xFF

#define SGP30_ERR_UNSUPPORTED_FEATURE_SET (-10)
#define SGP30_ERR_INVALID_PRODUCT_TYPE (-12)

#ifdef __cplusplus
extern "C"
{
#endif

  /**
 * sgp30_probe() - check if SGP sensor is available and initialize it
 *
 * This call aleady initializes the IAQ baselines (sgp30_iaq_init())
 *
 * Return:  STATUS_OK on success,
 *          SGP30_ERR_INVALID_PRODUCT_TYPE if the sensor is not an SGP30,
 *          SGP30_ERR_UNSUPPORTED_FEATURE_SET if the sensor's feature set
 *                                            is unknown or outdated,
 *          An error code otherwise
 */
  int16_t sgp30_probe(const struct device *dev);

  /**
 * sgp30_iaq_init() - reset the SGP's internal IAQ baselines
 *
 * Return:  STATUS_OK on success.
 */
  int16_t sgp30_iaq_init(const struct device *dev);

  /**
 * sgp30_get_configured_address() - returns the configured I2C address
 *
 * Return:      uint8_t I2C address
 */
  uint8_t sgp30_get_configured_address(const struct device *dev);

  /**
 * sgp30_get_feature_set_version() - Retrieve the sensor's feature set version
 * and product type
 *
 * @feature_set_version:    The feature set version
 * @product_type:           The product type: 0 for sgp30, 1: sgpc3
 *
 * Return:  STATUS_OK on success
 */
  int16_t sgp30_get_feature_set_version(const struct device *dev,
                                        uint16_t *feature_set_version,
                                        uint8_t *product_type);

  /**
 * sgp30_get_serial_id() - Retrieve the sensor's serial id
 *
 * @serial_id:    Output variable for the serial id
 *
 * Return:  STATUS_OK on success
 */
  int16_t sgp30_get_serial_id(const struct device *dev,
                              uint64_t *serial_id);

  /**
 * sgp30_get_iaq_baseline() - read out the baseline from the chip
 *
 * The IAQ baseline should be retrieved and persisted for a faster sensor
 * startup. See sgp30_set_iaq_baseline() for further documentation.
 *
 * A valid baseline value is only returned approx. 60min after a call to
 * sgp30_iaq_init() when it is not followed by a call to
 * sgp30_set_iaq_baseline() with a valid baseline. This functions returns
 * STATUS_FAIL if the baseline value is not valid.
 *
 * @baseline:   Pointer to raw uint32_t where to store the baseline
 *              If the method returns STATUS_FAIL, the baseline value must be
 *              discarded and must not be passed to sgp30_set_iaq_baseline().
 *
 * Return:      STATUS_OK on success, else STATUS_FAIL
 */
  int16_t sgp30_get_iaq_baseline(const struct device *dev,
                                 uint32_t *baseline);

  /**
 * sgp30_set_iaq_baseline() - set the on-chip baseline
 * @baseline:   A raw uint32_t baseline
 *              This value must be unmodified from what was retrieved by a
 *              successful call to sgp30_get_iaq_baseline() with return value
 *              STATUS_OK. A persisted baseline should not be set if it is
 *              older than one week.
 *
 * Return:      STATUS_OK on success, an error code otherwise
 */
  int16_t sgp30_set_iaq_baseline(const struct device *dev,
                                 uint32_t baseline);

  /**
 * sgp30_get_tvoc_inceptive_baseline() - read the chip's tVOC inceptive baseline
 *
 * The inceptive baseline must only be used on the very first startup of the
 * sensor. It ensures that measured concentrations are consistent with the air
 * quality even before the first clean air event.
 *
 * @tvoc_inceptive_baseline:
 *              Pointer to raw uint16_t where to store the inceptive baseline
 *              If the method returns STATUS_FAIL, the inceptive baseline value
 *              must be discarded and must not be passed to
 *              sgp30_set_tvoc_baseline().
 *
 * Return:      STATUS_OK on success, an error code otherwise
 */
  int16_t sgp30_get_tvoc_inceptive_baseline(const struct device *dev,
                                            uint16_t *tvoc_inceptive_baseline);

  /**
 * sgp30_set_tvoc_baseline() - set the on-chip tVOC baseline
 * @baseline:   A raw uint16_t tVOC baseline
 *              This value must be unmodified from what was retrieved by a
 *              successful call to sgp30_get_tvoc_inceptive_baseline() with
 * return value STATUS_OK.
 *
 * Return:      STATUS_OK on success, an error code otherwise
 */
  int16_t sgp30_set_tvoc_baseline(const struct device *dev,
                                  uint16_t tvoc_baseline);

  /**
 * sgp30_measure_iaq_blocking_read() - Measure IAQ concentrations tVOC, CO2-Eq.
 *
 * @tvoc_ppb:   The tVOC ppb value will be written to this location
 * @co2_eq_ppm: The CO2-Equivalent ppm value will be written to this location
 *
 * The profile is executed synchronously.
 *
 * Return:      STATUS_OK on success, an error code otherwise
 */
  int16_t sgp30_measure_iaq_blocking_read(const struct device *dev,
                                          uint16_t *tvoc_ppb,
                                          uint16_t *co2_eq_ppm);
  /**
 * sgp30_measure_iaq() - Measure IAQ values async
 *
 * The profile is executed asynchronously. Use sgp30_read_iaq to get the values.
 *
 * Return:  STATUS_OK on success, an error code otherwise
 */
  int16_t sgp30_measure_iaq(const struct device *dev);

  /**
 * sgp30_read_iaq() - Read IAQ values async
 *
 * Read the IAQ values. This command can only be exectued after a measurement
 * has started with sgp30_measure_iaq and is finished.
 *
 * @tvoc_ppb:   The tVOC ppb value will be written to this location
 * @co2_eq_ppm: The CO2-Equivalent ppm value will be written to this location
 *
 * Return:      STATUS_OK on success, an error code otherwise
 */
  int16_t sgp30_read_iaq(const struct device *dev,
                         uint16_t *tvoc_ppb,
                         uint16_t *co2_eq_ppm);

  /**
 * sgp30_measure_tvoc_blocking_read() - Measure tVOC concentration
 *
 * The profile is executed synchronously.
 *
 * Return:  tVOC concentration in ppb. Negative if it fails.
 */
  int16_t sgp30_measure_tvoc_blocking_read(const struct device *dev,
                                           uint16_t *tvoc_ppb);

  /**
 * sgp30_measure_tvoc() - Measure tVOC concentration async
 *
 * The profile is executed asynchronously. Use sgp30_read_tvoc to get the
 * ppb value.
 *
 * Return:  STATUS_OK on success, an error code otherwise
 */
  int16_t sgp30_measure_tvoc(const struct device *dev);

  /**
 * sgp30_read_tvoc() - Read tVOC concentration async
 *
 * Read the tVOC value. This command can only be exectued after a measurement
 * has started with sgp30_measure_tvoc and is finished.
 *
 * @tvoc_ppb:   The tVOC ppb value will be written to this location
 *
 * Return:      STATUS_OK on success, an error code otherwise
 */
  int16_t sgp30_read_tvoc(const struct device *dev,
                          uint16_t *tvoc_ppb);

  /**
 * sgp30_measure_co2_eq_blocking_read() - Measure CO2-Equivalent concentration
 *
 * The profile is executed synchronously.
 *
 * Return:  CO2-Equivalent concentration in ppm. Negative if it fails.
 */
  int16_t sgp30_measure_co2_eq_blocking_read(const struct device *dev,
                                             uint16_t *co2_eq_ppm);

  /**
 * sgp30_measure_co2_eq() - Measure tVOC concentration async
 *
 * The profile is executed asynchronously. Use sgp30_read_co2_eq to get the
 * ppm value.
 *
 * Return:  STATUS_OK on success, an error code otherwise
 */
  int16_t sgp30_measure_co2_eq(const struct device *dev);

  /**
 * sgp30_read_co2_eq() - Read CO2-Equivalent concentration async
 *
 * Read the CO2-Equivalent value. This command can only be exectued after a
 * measurement was started with sgp30_measure_co2_eq() and is finished.
 *
 * @co2_eq_ppm: The CO2-Equivalent ppm value will be written to this location
 *
 * Return:      STATUS_OK on success, an error code otherwise
 */
  int16_t sgp30_read_co2_eq(const struct device *dev,
                            uint16_t *co2_eq_ppm);

  /**
 * sgp30_measure_raw_blocking_read() - Measure raw signals
 * The profile is executed synchronously.
 *
 * @ethanol_raw_signal: Output variable for the ethanol raw signal
 * @h2_raw_signal:      Output variable for the h2 raw signal
 *
 * Return:      STATUS_OK on success, an error code otherwise
 */
  int16_t sgp30_measure_raw_blocking_read(const struct device *dev,
                                          uint16_t *ethanol_raw_signal,
                                          uint16_t *h2_raw_signal);

  /**
 * sgp30_measure_raw() - Measure raw signals async
 *
 * The profile is executed asynchronously. Use sgp30_read_raw to get the
 * signal values.
 *
 * Return:  STATUS_OK on success, an error code otherwise
 */
  int16_t sgp30_measure_raw(const struct device *dev);

  /**
 * sgp30_read_raw() - Read raw signals async
 * This command can only be exectued after a measurement started with
 * sgp30_measure_raw and has finished.
 *
 * @ethanol_raw_signal: Output variable for ethanol raw signal.
 * @h2_raw_signal: Output variable for h2 raw signal.
 *
 * Return:      STATUS_OK on success, an error code otherwise
 */
  int16_t sgp30_read_raw(const struct device *dev,
                         uint16_t *ethanol_raw_signal,
                         uint16_t *h2_raw_signal);

  /**
 * sgp30_measure_test() - Run the on-chip self-test
 *
 * This method is executed synchronously and blocks for the duration of the
 * measurement (~220ms)
 *
 * @test_result:    Allocated buffer to store the chip's error code.
 *                  test_result is SGP30_CMD_MEASURE_TEST_OK on success or set
 *                  to zero (0) in the case of a communication error.
 *
 * Return: STATUS_OK on a successful self-test, an error code otherwise
 */
  int16_t sgp30_measure_test(const struct device *dev,
                             uint16_t *test_result);

  /**
 * sgp30_set_absolute_humidity() - set the absolute humidity for compensation
 *
 * The absolute humidity must be provided in mg/m^3 and the value must be
 * between 0 and 256000 mg/m^3.
 * If the absolute humidity is set to zero, humidity compensation is disabled.
 *
 * @absolute_humidity:      absolute humidity in mg/m^3
 *
 * Return:      STATUS_OK on success, an error code otherwise
 */
  int16_t sgp30_set_absolute_humidity(const struct device *dev,
                                      uint32_t absolute_humidity);

  static struct sgp30_config
  {
    /* Label for the I2C bus this device is connected to */
    char *bus_name;

    /* I2C address of the sensor */
    uint8_t base_address;
  };

  struct sgp30_baseline
  {
    uint16_t co2eq;
    uint16_t tvoc;
  };

  /* 
   * Internal data for the SGP30 driver.
   */
  struct sgp30_data
  {
    uint16_t error;

    /* pointer to the device instance. Used for the work queue. */
    const struct device *dev;

    /* pointer to the i2c bus this sensor is connected to. */
    const struct device *bus;

    /* Data read from the sensor every second. */
    uint16_t tvoc_internal;
    uint16_t co2eq_internal;

    /* 
     * This data is updated from the internal buffer on sensor_sample_fetch().
     * this way it is guaranteed that every call of sensor_channel_get()
     * returns the same value if sensor_sample_fetch has not been called in the
     * mean time.
     **/
    struct sensor_value tvoc;
    struct sensor_value co2eq;

    struct sensor_value absolute_humidity;

    /* Used to submit sample fetching to the system work queue */
    struct k_work fetch_work;
    /* Timer used to submit fetching of data to the system work queue */
    struct k_timer fetch_timer;
    struct k_mutex data_mutex;

    /* Sensor serial number */
    uint64_t serial;
  };

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVER_SENSOR_SGP30_SGP30_H_ */
