/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sensor.h"

/* Calculate the compensated temperature */
int32_t calculate_temperature(uint32_t adc_temp, int32_t *t_fine,
			      struct calibration_coeffs *cal_coeffs)
{
	int16_t temperature_compensated = 0;
	int64_t var1, var2, var3;

	var1 = ((int32_t)adc_temp >> 3) - ((int32_t)cal_coeffs->par_t1 << 1);
	var2 = (var1 * (int32_t)cal_coeffs->par_t2) >> 11;
	var3 = ((var1 >> 1) * (var1 >> 1)) >> 12;
	var3 = ((var3) * ((int32_t)cal_coeffs->par_t3 << 4)) >> 14;
	*t_fine = var2 + var3;
	temperature_compensated = (int16_t)(((*t_fine * 5) + 128) >> 8);
	return temperature_compensated;
}

/* Calculate the compensated pressure
 * Note: temperature must be calculated first
 * to obtain the 't_fine'
 */
uint32_t calculate_pressure(uint32_t pres_adc, int32_t t_fine,
			    struct calibration_coeffs *cal_coeffs)
{
	int32_t var1;
	int32_t var2;
	int32_t var3;
	int32_t pressure_comp;
	const int32_t pres_ovf_check = INT32_C(0x40000000);

	var1 = (((int32_t)t_fine) >> 1) - 64000;
	var2 = ((((var1 >> 2) * (var1 >> 2)) >> 11) * (int32_t)cal_coeffs->par_p6) >> 2;
	var2 = var2 + ((var1 * (int32_t)cal_coeffs->par_p5) << 1);
	var2 = (var2 >> 2) + ((int32_t)cal_coeffs->par_p4 << 16);
	var1 = (((((var1 >> 2) * (var1 >> 2)) >> 13) * ((int32_t)cal_coeffs->par_p3 << 5)) >> 3) +
	       (((int32_t)cal_coeffs->par_p2 * var1) >> 1);
	var1 = var1 >> 18;
	var1 = ((32768 + var1) * (int32_t)cal_coeffs->par_p1) >> 15;
	pressure_comp = 1048576 - pres_adc;
	pressure_comp = (int32_t)((pressure_comp - (var2 >> 12)) * ((uint32_t)3125));
	if (pressure_comp >= pres_ovf_check) {
		pressure_comp = ((pressure_comp / var1) << 1);
	} else {
		pressure_comp = ((pressure_comp << 1) / var1);
	}

	var1 = ((int32_t)cal_coeffs->par_p9 *
		(int32_t)(((pressure_comp >> 3) * (pressure_comp >> 3)) >> 13)) >>
	       12;
	var2 = ((int32_t)(pressure_comp >> 2) * (int32_t)cal_coeffs->par_p8) >> 13;
	var3 = ((int32_t)(pressure_comp >> 8) * (int32_t)(pressure_comp >> 8) *
		(int32_t)(pressure_comp >> 8) * (int32_t)cal_coeffs->par_p10) >>
	       17;
	pressure_comp = (int32_t)(pressure_comp) +
			((var1 + var2 + var3 + ((int32_t)cal_coeffs->par_p7 << 7)) >> 4);

	return (uint32_t)pressure_comp;
}

/* Calculate the relative humidity
 * Note: temperature must be calculated first
 * to obtain the 't_fine'
 */
uint32_t calculate_humidity(uint16_t hum_adc, int32_t t_fine, struct calibration_coeffs *cal_coeffs)
{
	int32_t var1;
	int32_t var2;
	int32_t var3;
	int32_t var4;
	int32_t var5;
	int32_t var6;
	int32_t temp_scaled;
	int32_t calc_hum;

	temp_scaled = (((int32_t)t_fine * 5) + 128) >> 8;
	var1 = (int32_t)(hum_adc - ((int32_t)((int32_t)cal_coeffs->par_h1 * 16))) -
	       (((temp_scaled * (int32_t)cal_coeffs->par_h3) / ((int32_t)100)) >> 1);
	var2 = ((int32_t)cal_coeffs->par_h2 *
		(((temp_scaled * (int32_t)cal_coeffs->par_h4) / ((int32_t)100)) +
		 (((temp_scaled * ((temp_scaled * (int32_t)cal_coeffs->par_h5) / ((int32_t)100))) >>
		   6) /
		  ((int32_t)100)) +
		 (int32_t)(1 << 14))) >>
	       10;
	var3 = var1 * var2;
	var4 = (int32_t)cal_coeffs->par_h6 << 7;
	var4 = ((var4) + ((temp_scaled * (int32_t)cal_coeffs->par_h7) / ((int32_t)100))) >> 4;
	var5 = ((var3 >> 14) * (var3 >> 14)) >> 10;
	var6 = (var4 * var5) >> 1;
	calc_hum = (((var3 + var6) >> 10) * ((int32_t)1000)) >> 12;
	if (calc_hum > 100000) {
		calc_hum = 100000;
	} else if (calc_hum < 0) {
		calc_hum = 0;
	}

	return (uint32_t)calc_hum;
}
