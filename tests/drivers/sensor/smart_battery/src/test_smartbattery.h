/*
 * Copyright (c) 2021 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_SMART_BATTERY_H_
#define TEST_SMART_BATTERY_H_

const struct device *get_fuel_gauge(void);

int32_t test_smartbattery_getSensorValue(int16_t channel);
void test_smartbattery_getSensorValue_NotSupported(int16_t channel);

void test_smartbattery_init(void);
void test_smartbattery_getGaugeVoltage(void);
void test_smartbattery_getGaugeCurrent(void);
void test_smartbattery_getStdbyCurrent(void);
void test_smartbattery_getMaxLoadCurrent(void);
void test_smartbattery_getTemperature(void);
void test_smartbattery_getSOC(void);
void test_smartbattery_getFullChargeCapacity(void);
void test_smartbattery_getRemainingChargeCapacity(void);
void test_smartbattery_getNomAvailCapacity(void);
void test_smartbattery_getFullAvailCapacity(void);
void test_smartbattery_getAveragePower(void);
void test_smartbattery_getAverageTimeToEmpty(void);
void test_smartbattery_getAverageTimeToFull(void);
void test_smartbattery_getCycleCount(void);
void test_smartbattery_getDesignVoltage(void);
void test_smartbattery_getDesiredVoltage(void);
void test_smartbattery_getDesiredChargingCurrent(void);

/* Not supported functions */
void test_smartbattery_getAccelX(void);
void test_smartbattery_getAccelY(void);
void test_smartbattery_getAccelZ(void);
void test_smartbattery_getAccelXYZ(void);
void test_smartbattery_getGyroX(void);
void test_smartbattery_getGyroY(void);
void test_smartbattery_getGyroZ(void);
void test_smartbattery_getGyroXYZ(void);
void test_smartbattery_getMagnX(void);
void test_smartbattery_getMagnY(void);
void test_smartbattery_getMagnZ(void);
void test_smartbattery_getMagnXYZ(void);
void test_smartbattery_getDieTemp(void);
void test_smartbattery_getAmbientTemp(void);
void test_smartbattery_getPress(void);
void test_smartbattery_getProx(void);
void test_smartbattery_getHumidity(void);
void test_smartbattery_getLight(void);
void test_smartbattery_getIR(void);
void test_smartbattery_getRed(void);
void test_smartbattery_getGreen(void);
void test_smartbattery_getBlue(void);
void test_smartbattery_getAltitude(void);
void test_smartbattery_getPM_1_0(void);
void test_smartbattery_getPM_2_5(void);
void test_smartbattery_getPM_10(void);
void test_smartbattery_getDistance(void);
void test_smartbattery_getCO2(void);
void test_smartbattery_getVOC(void);
void test_smartbattery_getGasRes(void);
void test_smartbattery_getVoltage(void);
void test_smartbattery_getCurrent(void);
void test_smartbattery_getResistance(void);
void test_smartbattery_getRotation(void);
void test_smartbattery_getPosDx(void);
void test_smartbattery_getPosDy(void);
void test_smartbattery_getPosDz(void);
void test_smartbattery_getRpm(void);
void test_smartbattery_getROS(void);

#endif
