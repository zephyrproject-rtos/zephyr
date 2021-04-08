/*
 * Copyright (c) 2021 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#include "test_smartbattery.h"

void test_main(void)
{
	k_object_access_grant(get_fuel_gauge(), k_current_get());

	ztest_test_suite(framework_tests,
		ztest_unit_test(test_smartbattery_init),
		ztest_unit_test(test_smartbattery_getGaugeVoltage),
		ztest_unit_test(test_smartbattery_getGaugeCurrent),
		ztest_unit_test(test_smartbattery_getStdbyCurrent),
		ztest_unit_test(test_smartbattery_getMaxLoadCurrent),
		ztest_unit_test(test_smartbattery_getTemperature),
		ztest_unit_test(test_smartbattery_getSOC),
		ztest_unit_test(test_smartbattery_getFullChargeCapacity),
		ztest_unit_test(test_smartbattery_getRemainingChargeCapacity),
		ztest_unit_test(test_smartbattery_getNomAvailCapacity),
		ztest_unit_test(test_smartbattery_getFullAvailCapacity),
		ztest_unit_test(test_smartbattery_getAveragePower),
		ztest_unit_test(test_smartbattery_getROS),
		ztest_unit_test(test_smartbattery_getAverageTimeToEmpty),
		ztest_unit_test(test_smartbattery_getAverageTimeToFull),
		ztest_unit_test(test_smartbattery_getCycleCount),
		ztest_unit_test(test_smartbattery_getDesignVoltage),
		ztest_unit_test(test_smartbattery_getDesiredVoltage),
		ztest_unit_test(test_smartbattery_getDesiredChargingCurrent),
		ztest_unit_test(test_smartbattery_getAccelX),
		ztest_unit_test(test_smartbattery_getAccelY),
		ztest_unit_test(test_smartbattery_getAccelZ),
		ztest_unit_test(test_smartbattery_getAccelXYZ),
		ztest_unit_test(test_smartbattery_getGyroX),
		ztest_unit_test(test_smartbattery_getGyroY),
		ztest_unit_test(test_smartbattery_getGyroZ),
		ztest_unit_test(test_smartbattery_getGyroXYZ),
		ztest_unit_test(test_smartbattery_getMagnX),
		ztest_unit_test(test_smartbattery_getMagnY),
		ztest_unit_test(test_smartbattery_getMagnZ),
		ztest_unit_test(test_smartbattery_getMagnXYZ),
		ztest_unit_test(test_smartbattery_getDieTemp),
		ztest_unit_test(test_smartbattery_getAmbientTemp),
		ztest_unit_test(test_smartbattery_getPress),
		ztest_unit_test(test_smartbattery_getProx),
		ztest_unit_test(test_smartbattery_getHumidity),
		ztest_unit_test(test_smartbattery_getLight),
		ztest_unit_test(test_smartbattery_getIR),
		ztest_unit_test(test_smartbattery_getRed),
		ztest_unit_test(test_smartbattery_getGreen),
		ztest_unit_test(test_smartbattery_getBlue),
		ztest_unit_test(test_smartbattery_getAltitude),
		ztest_unit_test(test_smartbattery_getPM_1_0),
		ztest_unit_test(test_smartbattery_getPM_2_5),
		ztest_unit_test(test_smartbattery_getPM_10),
		ztest_unit_test(test_smartbattery_getDistance),
		ztest_unit_test(test_smartbattery_getCO2),
		ztest_unit_test(test_smartbattery_getVOC),
		ztest_unit_test(test_smartbattery_getGasRes),
		ztest_unit_test(test_smartbattery_getVoltage),
		ztest_unit_test(test_smartbattery_getCurrent),
		ztest_unit_test(test_smartbattery_getResistance),
		ztest_unit_test(test_smartbattery_getRotation),
		ztest_unit_test(test_smartbattery_getPosDx),
		ztest_unit_test(test_smartbattery_getPosDy),
		ztest_unit_test(test_smartbattery_getPosDz),
		ztest_unit_test(test_smartbattery_getRpm)
	);

	ztest_run_test_suite(framework_tests);
}
