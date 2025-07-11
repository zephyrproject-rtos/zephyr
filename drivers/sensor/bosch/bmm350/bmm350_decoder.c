/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bmm350_decoder.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(BMM350_DECODER, CONFIG_SENSOR_LOG_LEVEL);

void bmm350_decoder_compensate_raw_data(const struct bmm350_raw_mag_data *raw_data,
					const struct mag_compensate *comp,
					struct bmm350_mag_temp_data *out)
{
	int32_t out_data[4];
	int32_t dut_offset_coef[3], dut_sensit_coef[3], dut_tco[3], dut_tcs[3];

	/* Convert mag lsb to uT and temp lsb to degC */
	out_data[0] = ((sign_extend(raw_data->magn_x, BMM350_SIGNED_24_BIT) *
			BMM350_LSB_TO_UT_XY_COEFF) /
		       BMM350_LSB_TO_UT_COEFF_DIV);
	out_data[1] = ((sign_extend(raw_data->magn_y, BMM350_SIGNED_24_BIT) *
			BMM350_LSB_TO_UT_XY_COEFF) /
		       BMM350_LSB_TO_UT_COEFF_DIV);
	out_data[2] = ((sign_extend(raw_data->magn_z, BMM350_SIGNED_24_BIT) *
			BMM350_LSB_TO_UT_Z_COEFF) /
		       BMM350_LSB_TO_UT_COEFF_DIV);
	out_data[3] = ((sign_extend(raw_data->temp, BMM350_SIGNED_24_BIT) *
			BMM350_LSB_TO_UT_TEMP_COEFF) /
		       BMM350_LSB_TO_UT_COEFF_DIV);

	if (out_data[3] > 0) {
		out_data[3] = (out_data[3] - (2549 / 100));
	} else if (out_data[3] < 0) {
		out_data[3] = (out_data[3] + (2549 / 100));
	}

	/* Apply compensation to temperature reading */
	out_data[3] = (((BMM350_MAG_COMP_COEFF_SCALING + comp->dut_sensit_coef.t_sens) *
			out_data[3]) +
		       comp->dut_offset_coef.t_offs) /
		      BMM350_MAG_COMP_COEFF_SCALING;

	/* Store magnetic compensation structure to an array */
	dut_offset_coef[0] = comp->dut_offset_coef.offset_x;
	dut_offset_coef[1] = comp->dut_offset_coef.offset_y;
	dut_offset_coef[2] = comp->dut_offset_coef.offset_z;

	dut_sensit_coef[0] = comp->dut_sensit_coef.sens_x;
	dut_sensit_coef[1] = comp->dut_sensit_coef.sens_y;
	dut_sensit_coef[2] = comp->dut_sensit_coef.sens_z;

	dut_tco[0] = comp->dut_tco.tco_x;
	dut_tco[1] = comp->dut_tco.tco_y;
	dut_tco[2] = comp->dut_tco.tco_z;

	dut_tcs[0] = comp->dut_tcs.tcs_x;
	dut_tcs[1] = comp->dut_tcs.tcs_y;
	dut_tcs[2] = comp->dut_tcs.tcs_z;

	/* Compensate raw magnetic data */
	for (size_t indx = 0; indx < 3; indx++) {
		out_data[indx] = (out_data[indx] *
				  (BMM350_MAG_COMP_COEFF_SCALING + dut_sensit_coef[indx])) /
				 BMM350_MAG_COMP_COEFF_SCALING;
		out_data[indx] = (out_data[indx] + dut_offset_coef[indx]);
		out_data[indx] = ((out_data[indx] * BMM350_MAG_COMP_COEFF_SCALING) +
				  (dut_tco[indx] * (out_data[3] - comp->dut_t0))) /
				 BMM350_MAG_COMP_COEFF_SCALING;
		out_data[indx] = (out_data[indx] * BMM350_MAG_COMP_COEFF_SCALING) /
				 (BMM350_MAG_COMP_COEFF_SCALING +
				  (dut_tcs[indx] * (out_data[3] - comp->dut_t0)));
	}

	out->x = ((((out_data[0] * BMM350_MAG_COMP_COEFF_SCALING) -
		    (comp->cross_axis.cross_x_y * out_data[1])) *
		   BMM350_MAG_COMP_COEFF_SCALING) /
		  ((BMM350_MAG_COMP_COEFF_SCALING * BMM350_MAG_COMP_COEFF_SCALING) -
		   (comp->cross_axis.cross_y_x * comp->cross_axis.cross_x_y)));

	out->y = ((((out_data[1] * BMM350_MAG_COMP_COEFF_SCALING) -
		    (comp->cross_axis.cross_y_x * out_data[0])) *
		   BMM350_MAG_COMP_COEFF_SCALING) /
		  ((BMM350_MAG_COMP_COEFF_SCALING * BMM350_MAG_COMP_COEFF_SCALING) -
		   (comp->cross_axis.cross_y_x * comp->cross_axis.cross_x_y)));

	out->z = (out_data[2] +
		  (((out_data[0] *
		     ((comp->cross_axis.cross_y_x * comp->cross_axis.cross_z_y) -
		      (comp->cross_axis.cross_z_x * BMM350_MAG_COMP_COEFF_SCALING))) -
		    (out_data[1] *
		     ((comp->cross_axis.cross_z_y * BMM350_MAG_COMP_COEFF_SCALING) -
		      (comp->cross_axis.cross_x_y * comp->cross_axis.cross_z_x))))) /
		  (((BMM350_MAG_COMP_COEFF_SCALING * BMM350_MAG_COMP_COEFF_SCALING) -
		    comp->cross_axis.cross_y_x * comp->cross_axis.cross_x_y)));

	LOG_DBG("mag data %d %d %d", out_data[0], out_data[1], out_data[2]);

	out->temperature = out_data[3];
}
