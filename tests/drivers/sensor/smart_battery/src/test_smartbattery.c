/*
 * Copyright (c) 2021 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <drivers/sensor.h>

#ifndef DT_NODE_HAS_STATUS(DT_ALIAS(bat0), okay)
#error "Fuel Gauge device not found"
#endif

static ZTEST_BMEM const struct device *dev = DEVICE_DT_GET(DT_ALIAS(bat0));

int32_t test_smartbattery_getSensorValue(int16_t channel)
{
	struct sensor_value value;

	zassert_true(sensor_sample_fetch_chan(dev, channel) < 0, "Sample fetch failed");
	zassert_true(sensor_channel_get(dev, channel, &value) < 0, "Get sensor value failed");
	return value.val1;
}

void test_smartbattery_getSensorValue_NotSupported(int16_t channel)
{
	zassert_true(sensor_sample_fetch_chan(dev, channel) == -ENOTSUP, "Invalid function");
}

void test_smartbattery_init(void)
{
	zassert_not_null(dev, "Fuel Gauge " BAT_DEV_NAME " not found");
}

void test_smartbattery_getGaugeVoltage(void)
{
	test_smartbattery_getSensorValue(SENSOR_CHAN_GAUGE_VOLTAGE);
}

void test_smartbattery_getGaugeCurrent(void)
{
	test_smartbattery_getSensorValue(SENSOR_CHAN_GAUGE_AVG_CURRENT);
}

void test_smartbattery_getStdbyCurrent(void)
{
	test_smartbattery_getSensorValue(SENSOR_CHAN_GAUGE_STDBY_CURRENT);
}

void test_smartbattery_getMaxLoadCurrent(void)
{
	test_smartbattery_getSensorValue(SENSOR_CHAN_GAUGE_MAX_LOAD_CURRENT);
}

void test_smartbattery_getTemperature(void)
{
	test_smartbattery_getSensorValue(SENSOR_CHAN_GAUGE_TEMP);
}

void test_smartbattery_getSOC(void)
{
	test_smartbattery_getSensorValue(SENSOR_CHAN_GAUGE_STATE_OF_CHARGE);
}

void test_smartbattery_getFullChargeCapacity(void)
{
	test_smartbattery_getSensorValue(SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY);
}

void test_smartbattery_getRemainingChargeCapacity(void)
{
	test_smartbattery_getSensorValue(SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY);
}

void test_smartbattery_getNomAvailCapacity(void)
{
	test_smartbattery_getSensorValue(SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY);
}

void test_smartbattery_getFullAvailCapacity(void)
{
	test_smartbattery_getSensorValue(SENSOR_CHAN_GAUGE_FULL_AVAIL_CAPACITY);
}

void test_smartbattery_getAveragePower(void)
{
	test_smartbattery_getSensorValue(SENSOR_CHAN_GAUGE_AVG_POWER);
}

void test_smartbattery_getAverageTimeToEmpty(void)
{
	test_smartbattery_getSensorValue(SENSOR_CHAN_GAUGE_TIME_TO_EMPTY);
}

void test_smartbattery_getAverageTimeToFull(void)
{
	test_smartbattery_getSensorValue(SENSOR_CHAN_GAUGE_TIME_TO_FULL);
}

void test_smartbattery_getCycleCount(void)
{
	test_smartbattery_getSensorValue(SENSOR_CHAN_GAUGE_CYCLE_COUNT);
}

void test_smartbattery_getDesignVoltage(void)
{
	test_smartbattery_getSensorValue(SENSOR_CHAN_GAUGE_DESIGN_VOLTAGE);
}

void test_smartbattery_getDesiredVoltage(void)
{
	test_smartbattery_getSensorValue(SENSOR_CHAN_GAUGE_DESIRED_VOLTAGE);
}

void test_smartbattery_getDesiredChargingCurrent(void)
{
	test_smartbattery_getSensorValue(SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT);
}

void test_smartbattery_getAccelX(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_ACCEL_X);
}

void test_smartbattery_getAccelY(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_ACCEL_Y);
}

void test_smartbattery_getAccelZ(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_ACCEL_Z);
}

void test_smartbattery_getAccelXYZ(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_ACCEL_XYZ);
}

void test_smartbattery_getGyroX(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_GYRO_X);
}

void test_smartbattery_getGyroY(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_GYRO_Y);
}

void test_smartbattery_getGyroZ(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_GYRO_Z);
}

void test_smartbattery_getGyroXYZ(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_GYRO_XYZ);
}

void test_smartbattery_getMagnX(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_MAGN_X);
}

void test_smartbattery_getMagnY(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_MAGN_Y);
}

void test_smartbattery_getMagnZ(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_MAGN_Z);
}

void test_smartbattery_getMagnXYZ(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_MAGN_XYZ);
}

void test_smartbattery_getDieTemp(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_DIE_TEMP);
}

void test_smartbattery_getAmbientTemp(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_AMBIENT_TEMP);
}

void test_smartbattery_getPress(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_PRESS);
}

void test_smartbattery_getProx(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_PROX);
}

void test_smartbattery_getHumidity(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_HUMIDITY);
}

void test_smartbattery_getLight(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_LIGHT);
}

void test_smartbattery_getIR(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_IR);
}

void test_smartbattery_getRed(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_RED);
}

void test_smartbattery_getGreen(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_GREEN);
}

void test_smartbattery_getBlue(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_BLUE);
}

void test_smartbattery_getAltitude(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_ALTITUDE);
}

void test_smartbattery_getPM_1_0(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_PM_1_0);
}

void test_smartbattery_getPM_2_5(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_PM_2_5);
}

void test_smartbattery_getPM_10(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_PM_10);
}

void test_smartbattery_getDistance(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_DISTANCE);
}

void test_smartbattery_getCO2(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_CO2);
}

void test_smartbattery_getVOC(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_VOC);
}

void test_smartbattery_getGasRes(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_GAS_RES);
}

void test_smartbattery_getVoltage(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_VOLTAGE);
}

void test_smartbattery_getCurrent(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_CURRENT);
}

void test_smartbattery_getResistance(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_RESISTANCE);
}

void test_smartbattery_getRotation(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_ROTATION);
}

void test_smartbattery_getPosDx(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_POS_DX);
}

void test_smartbattery_getPosDy(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_POS_DY);
}

void test_smartbattery_getPosDz(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_POS_DZ);
}

void test_smartbattery_getRpm(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_RPM);
}

void test_smartbattery_getROS(void)
{
	test_smartbattery_getSensorValue_NotSupported(SENSOR_CHAN_GAUGE_STATE_OF_HEALTH);
}
