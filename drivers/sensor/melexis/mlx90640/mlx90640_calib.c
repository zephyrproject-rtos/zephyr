/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Calibration extract and To/Ta reconstruction follow the MLX90640
 * datasheet (Melexis) calculation flow.
 */

#include <errno.h>
#include <math.h>
#include <string.h>

#include <zephyr/sys/util.h>

#include "mlx90640.h"

static float mlx90640_p2(int exp)
{
	return ldexpf(1.0f, exp);
}

static int mlx90640_signed(int value, unsigned int bits)
{
	unsigned int sign = BIT(bits - 1);

	if (value >= (int)sign) {
		value -= (int)BIT(bits);
	}

	return value;
}

static bool mlx90640_pixel_bad(const struct mlx90640_params *p, uint16_t pixel)
{
	for (uint8_t i = 0; i < p->broken_count; i++) {
		if (p->broken[i] == pixel) {
			return true;
		}
	}

	for (uint8_t i = 0; i < p->outlier_count; i++) {
		if (p->outlier[i] == pixel) {
			return true;
		}
	}

	return false;
}

static void extract_vdd(struct mlx90640_params *p, const uint16_t *ee)
{
	p->k_vdd = (int16_t)mlx90640_signed((ee[51] & 0xFF00) >> 8, 8) * 32;
	p->vdd25 = (int16_t)(((ee[51] & 0x00FF) - 256) << 5) - 8192;
}

static void extract_ptat(struct mlx90640_params *p, const uint16_t *ee)
{
	p->kv_ptat = (float)mlx90640_signed((ee[50] & 0xFC00) >> 10, 6) / 4096.0f;
	p->kt_ptat = (float)mlx90640_signed(ee[50] & 0x03FF, 10) / 8.0f;
	p->v_ptat25 = (int16_t)ee[49];
	p->alpha_ptat = (float)(ee[16] & 0xF000) / 16384.0f + 8.0f;
}

static void extract_gain_tgc_res_ksta(struct mlx90640_params *p, const uint16_t *ee)
{
	p->gain_ee = (int16_t)ee[48];
	p->tgc = (float)mlx90640_signed(ee[60] & 0x00FF, 8) / 32.0f;
	p->resolution_ee = (uint8_t)((ee[56] & 0x3000) >> 12);
	p->ks_ta = (float)mlx90640_signed((ee[60] & 0xFF00) >> 8, 8) / 8192.0f;
}

static void extract_ks_to(struct mlx90640_params *p, const uint16_t *ee)
{
	int step = ((ee[63] & 0x3000) >> 12) * 10;
	int scale = (ee[63] & 0x000F) + 8;
	float ks_scale = mlx90640_p2(scale);

	p->ct[0] = -40;
	p->ct[1] = 0;
	p->ct[2] = (int16_t)(((ee[63] & 0x00F0) >> 4) * step);
	p->ct[3] = (int16_t)(p->ct[2] + ((ee[63] & 0x0F00) >> 8) * step);
	p->ct[4] = 400;

	p->ks_to[0] = (float)mlx90640_signed(ee[61] & 0x00FF, 8) / ks_scale;
	p->ks_to[1] = (float)mlx90640_signed((ee[61] & 0xFF00) >> 8, 8) / ks_scale;
	p->ks_to[2] = (float)mlx90640_signed(ee[62] & 0x00FF, 8) / ks_scale;
	p->ks_to[3] = (float)mlx90640_signed((ee[62] & 0xFF00) >> 8, 8) / ks_scale;
	p->ks_to[4] = -0.0002f;
}

static void extract_cp(struct mlx90640_params *p, const uint16_t *ee)
{
	int alpha_scale = ((ee[32] & 0xF000) >> 12) + 27;
	int kta_scale1 = ((ee[56] & 0x00F0) >> 4) + 8;
	int kv_scale = (ee[56] & 0x0F00) >> 8;

	p->cp_offset[0] = (int16_t)mlx90640_signed(ee[58] & 0x03FF, 10);
	p->cp_offset[1] = (int16_t)(p->cp_offset[0] + mlx90640_signed((ee[58] & 0xFC00) >> 10, 6));

	p->cp_alpha[0] = (float)mlx90640_signed(ee[57] & 0x03FF, 10) / mlx90640_p2(alpha_scale);
	p->cp_alpha[1] = (1.0f + (float)mlx90640_signed((ee[57] & 0xFC00) >> 10, 6) / 128.0f) *
			 p->cp_alpha[0];

	p->cp_kta = (float)mlx90640_signed(ee[59] & 0x00FF, 8) / mlx90640_p2(kta_scale1);
	p->cp_kv = (float)mlx90640_signed((ee[59] & 0xFF00) >> 8, 8) / mlx90640_p2(kv_scale);
}

static void extract_alpha(struct mlx90640_params *p, const uint16_t *ee)
{
	int acc_rem_scale = ee[32] & 0x000F;
	int acc_col_scale = (ee[32] & 0x00F0) >> 4;
	int acc_row_scale = (ee[32] & 0x0F00) >> 8;
	int alpha_scale = ((ee[32] & 0xF000) >> 12) + 30;
	int alpha_ref = ee[33];
	int acc_row[24];
	int acc_col[32];
	float temp = 0.0f;
	uint8_t scale = 0;

	for (int i = 0; i < 6; i++) {
		int base = i * 4;

		acc_row[base + 0] = mlx90640_signed(ee[34 + i] & 0x000F, 4);
		acc_row[base + 1] = mlx90640_signed((ee[34 + i] & 0x00F0) >> 4, 4);
		acc_row[base + 2] = mlx90640_signed((ee[34 + i] & 0x0F00) >> 8, 4);
		acc_row[base + 3] = mlx90640_signed((ee[34 + i] & 0xF000) >> 12, 4);
	}

	for (int i = 0; i < 8; i++) {
		int base = i * 4;

		acc_col[base + 0] = mlx90640_signed(ee[40 + i] & 0x000F, 4);
		acc_col[base + 1] = mlx90640_signed((ee[40 + i] & 0x00F0) >> 4, 4);
		acc_col[base + 2] = mlx90640_signed((ee[40 + i] & 0x0F00) >> 8, 4);
		acc_col[base + 3] = mlx90640_signed((ee[40 + i] & 0xF000) >> 12, 4);
	}

	for (int pass = 0; pass < 2; pass++) {
		for (int i = 0; i < 24; i++) {
			for (int j = 0; j < 32; j++) {
				int pix = 32 * i + j;
				float a = (float)mlx90640_signed((ee[64 + pix] & 0x03F0) >> 4, 6);

				a *= mlx90640_p2(acc_rem_scale);
				a += (float)(alpha_ref + (acc_row[i] << acc_row_scale) +
					     (acc_col[j] << acc_col_scale));
				a /= mlx90640_p2(alpha_scale);
				a -= p->tgc * (p->cp_alpha[0] + p->cp_alpha[1]) / 2.0f;
				a = MLX90640_SCALE_ALPHA / a;

				if (pass == 0) {
					if (a > temp) {
						temp = a;
					}
				} else {
					p->alpha[pix] = (uint16_t)(a * mlx90640_p2(scale) + 0.5f);
				}
			}
		}

		if (pass == 0) {
			if (temp <= 0.0f) {
				scale = 0;
			} else {
				while ((temp < 32768.0f) && (scale < 32U)) {
					temp *= 2.0f;
					scale++;
				}
			}
		}
	}
	p->alpha_scale = scale;
}

static void extract_offset(struct mlx90640_params *p, const uint16_t *ee)
{
	int occ_rem_scale = ee[16] & 0x000F;
	int occ_col_scale = (ee[16] & 0x00F0) >> 4;
	int occ_row_scale = (ee[16] & 0x0F00) >> 8;
	int offset_ref = (int16_t)ee[17];
	int occ_row[24];
	int occ_col[32];

	for (int i = 0; i < 6; i++) {
		int base = i * 4;

		occ_row[base + 0] = mlx90640_signed(ee[18 + i] & 0x000F, 4);
		occ_row[base + 1] = mlx90640_signed((ee[18 + i] & 0x00F0) >> 4, 4);
		occ_row[base + 2] = mlx90640_signed((ee[18 + i] & 0x0F00) >> 8, 4);
		occ_row[base + 3] = mlx90640_signed((ee[18 + i] & 0xF000) >> 12, 4);
	}

	for (int i = 0; i < 8; i++) {
		int base = i * 4;

		occ_col[base + 0] = mlx90640_signed(ee[24 + i] & 0x000F, 4);
		occ_col[base + 1] = mlx90640_signed((ee[24 + i] & 0x00F0) >> 4, 4);
		occ_col[base + 2] = mlx90640_signed((ee[24 + i] & 0x0F00) >> 8, 4);
		occ_col[base + 3] = mlx90640_signed((ee[24 + i] & 0xF000) >> 12, 4);
	}

	for (int i = 0; i < 24; i++) {
		for (int j = 0; j < 32; j++) {
			int pix = 32 * i + j;
			int off = mlx90640_signed((ee[64 + pix] & 0xFC00) >> 10, 6);

			off *= (int)BIT(occ_rem_scale);
			off += offset_ref + (occ_row[i] << occ_row_scale) +
			       (occ_col[j] << occ_col_scale);
			p->offset[pix] = (int16_t)off;
		}
	}
}

static void extract_kta(struct mlx90640_params *p, const uint16_t *ee)
{
	int kta_rc[4];
	int kta_scale1 = ((ee[56] & 0x00F0) >> 4) + 8;
	int kta_scale2 = ee[56] & 0x000F;
	float temp = 0.0f;
	uint8_t scale = 0;

	kta_rc[0] = mlx90640_signed((ee[54] & 0xFF00) >> 8, 8);
	kta_rc[2] = mlx90640_signed(ee[54] & 0x00FF, 8);
	kta_rc[1] = mlx90640_signed((ee[55] & 0xFF00) >> 8, 8);
	kta_rc[3] = mlx90640_signed(ee[55] & 0x00FF, 8);

	for (int pass = 0; pass < 2; pass++) {
		for (int i = 0; i < 24; i++) {
			for (int j = 0; j < 32; j++) {
				int pix = 32 * i + j;
				int split = 2 * (pix / 32 - (pix / 64) * 2) + (pix % 2);
				float kta = (float)mlx90640_signed((ee[64 + pix] & 0x000E) >> 1, 3);

				kta *= mlx90640_p2(kta_scale2);
				kta += (float)kta_rc[split];
				kta /= mlx90640_p2(kta_scale1);

				if (pass == 0) {
					float a = fabsf(kta);

					if (a > temp) {
						temp = a;
					}
				} else {
					float v = kta * mlx90640_p2(scale);

					p->kta[pix] = (int8_t)((v < 0.0f) ? (v - 0.5f)
									  : (v + 0.5f));
				}
			}
		}

		if (pass == 0) {
			if (temp <= 0.0f) {
				scale = 0;
			} else {
				while ((temp < 64.0f) && (scale < 32U)) {
					temp *= 2.0f;
					scale++;
				}
			}
		}
	}
	p->kta_scale = scale;
}

static void extract_kv(struct mlx90640_params *p, const uint16_t *ee)
{
	int kv_t[4];
	int kv_scale = (ee[56] & 0x0F00) >> 8;
	float temp = 0.0f;
	uint8_t scale = 0;

	kv_t[0] = mlx90640_signed((ee[52] & 0xF000) >> 12, 4);
	kv_t[2] = mlx90640_signed((ee[52] & 0x0F00) >> 8, 4);
	kv_t[1] = mlx90640_signed((ee[52] & 0x00F0) >> 4, 4);
	kv_t[3] = mlx90640_signed(ee[52] & 0x000F, 4);

	for (int pass = 0; pass < 2; pass++) {
		for (int i = 0; i < 24; i++) {
			for (int j = 0; j < 32; j++) {
				int pix = 32 * i + j;
				int split = 2 * (pix / 32 - (pix / 64) * 2) + (pix % 2);
				float kv = (float)kv_t[split] / mlx90640_p2(kv_scale);

				if (pass == 0) {
					float a = fabsf(kv);

					if (a > temp) {
						temp = a;
					}
				} else {
					float v = kv * mlx90640_p2(scale);

					p->kv[pix] = (int8_t)((v < 0.0f) ? (v - 0.5f)
									  : (v + 0.5f));
				}
			}
		}

		if (pass == 0) {
			if (temp <= 0.0f) {
				scale = 0;
			} else {
				while ((temp < 64.0f) && (scale < 32U)) {
					temp *= 2.0f;
					scale++;
				}
			}
		}
	}
	p->kv_scale = scale;
}

static void extract_cilc(struct mlx90640_params *p, const uint16_t *ee)
{
	p->calibration_mode_ee = (uint8_t)(((ee[10] & 0x0800) >> 4) ^ 0x80);
	p->il_chess_c[0] = (float)mlx90640_signed(ee[53] & 0x003F, 6) / 16.0f;
	p->il_chess_c[1] = (float)mlx90640_signed((ee[53] & 0x07C0) >> 6, 5) / 2.0f;
	p->il_chess_c[2] = (float)mlx90640_signed((ee[53] & 0xF800) >> 11, 5) / 8.0f;
}

static void extract_bad_pixels(struct mlx90640_params *p, const uint16_t *ee)
{
	p->broken_count = 0;
	p->outlier_count = 0;

	for (uint16_t pix = 0; pix < MLX90640_PIXELS; pix++) {
		uint16_t word = ee[pix + 64];

		if (word == 0U) {
			if (p->broken_count < MLX90640_MAX_BAD_PIXELS) {
				p->broken[p->broken_count++] = pix;
			}
		} else if ((word & 0x0001) != 0U) {
			if (p->outlier_count < MLX90640_MAX_BAD_PIXELS) {
				p->outlier[p->outlier_count++] = pix;
			}
		}
	}
}

int mlx90640_extract_params(struct mlx90640_params *p, const uint16_t *ee)
{
	memset(p, 0, sizeof(*p));

	extract_vdd(p, ee);
	extract_ptat(p, ee);
	extract_gain_tgc_res_ksta(p, ee);
	extract_ks_to(p, ee);
	extract_cp(p, ee);
	extract_alpha(p, ee);
	extract_offset(p, ee);
	extract_kta(p, ee);
	extract_kv(p, ee);
	extract_cilc(p, ee);
	extract_bad_pixels(p, ee);

	if ((p->k_vdd == 0) || (p->gain_ee == 0.0f) || (p->kt_ptat == 0.0f)) {
		return -EINVAL;
	}

	return 0;
}

static float mlx90640_calc_vdd(const struct mlx90640_params *p, const uint16_t *frame)
{
	float vdd = (int16_t)frame[810];
	uint8_t resolution_ram = (uint8_t)((frame[832] & 0x0C00) >> 10);
	float corr = mlx90640_p2(p->resolution_ee) / mlx90640_p2(resolution_ram);

	return (corr * vdd - (float)p->vdd25) / (float)p->k_vdd + 3.3f;
}

float mlx90640_calc_ta(const struct mlx90640_params *p, const uint16_t *frame)
{
	float vdd = mlx90640_calc_vdd(p, frame);
	float ptat = (int16_t)frame[800];
	float ptat_art = (int16_t)frame[768];
	float ta;

	ptat_art = (ptat / (ptat * p->alpha_ptat + ptat_art)) * 262144.0f;
	ta = ptat_art / (1.0f + p->kv_ptat * (vdd - 3.3f)) - p->v_ptat25;
	ta = ta / p->kt_ptat + 25.0f;

	return ta;
}

void mlx90640_calc_to(const struct mlx90640_params *p, const uint16_t *frame, float emissivity,
		      float tr, float *to)
{
	float vdd = mlx90640_calc_vdd(p, frame);
	float ta = mlx90640_calc_ta(p, frame);
	float ta4, tr4, ta_tr;
	float kta_scale = mlx90640_p2(p->kta_scale);
	float kv_scale = mlx90640_p2(p->kv_scale);
	float alpha_scale = mlx90640_p2(p->alpha_scale);
	float alpha_corr[4];
	float ir_cp[2];
	float gain;
	uint8_t subpage = (uint8_t)(frame[833] & 0x0001);
	int mode = (int)((frame[832] & 0x1000) >> 5);

	ta4 = ta + 273.15f;
	ta4 *= ta4;
	ta4 *= ta4;
	tr4 = tr + 273.15f;
	tr4 *= tr4;
	tr4 *= tr4;
	ta_tr = tr4 - (tr4 - ta4) / emissivity;

	alpha_corr[0] = 1.0f / (1.0f + p->ks_to[0] * 40.0f);
	alpha_corr[1] = 1.0f;
	alpha_corr[2] = 1.0f + p->ks_to[1] * (float)p->ct[2];
	alpha_corr[3] = alpha_corr[2] * (1.0f + p->ks_to[2] * (float)(p->ct[3] - p->ct[2]));

	gain = (int16_t)frame[778];
	gain = p->gain_ee / gain;

	ir_cp[0] = (int16_t)frame[776] * gain;
	ir_cp[1] = (int16_t)frame[808] * gain;
	ir_cp[0] -= (float)p->cp_offset[0] * (1.0f + p->cp_kta * (ta - 25.0f)) *
		    (1.0f + p->cp_kv * (vdd - 3.3f));
	if (mode == p->calibration_mode_ee) {
		ir_cp[1] -= (float)p->cp_offset[1] * (1.0f + p->cp_kta * (ta - 25.0f)) *
			    (1.0f + p->cp_kv * (vdd - 3.3f));
	} else {
		ir_cp[1] -= ((float)p->cp_offset[1] + p->il_chess_c[0]) *
			    (1.0f + p->cp_kta * (ta - 25.0f)) * (1.0f + p->cp_kv * (vdd - 3.3f));
	}

	for (int pix = 0; pix < MLX90640_PIXELS; pix++) {
		int il_pattern = pix / 32 - (pix / 64) * 2;
		int chess_pattern = il_pattern ^ (pix - (pix / 2) * 2);
		int conversion_pattern = ((pix + 2) / 4 - (pix + 3) / 4 + (pix + 1) / 4 - pix / 4) *
					 (1 - 2 * il_pattern);
		int pattern = (mode == 0) ? il_pattern : chess_pattern;
		float ir_data, kta, kv, alpha_comp, sx, t_obj;
		int range;

		if (pattern != subpage) {
			continue;
		}

		if (mlx90640_pixel_bad(p, (uint16_t)pix)) {
			to[pix] = -273.15f;
			continue;
		}

		ir_data = (int16_t)frame[pix] * gain;
		kta = (float)p->kta[pix] / kta_scale;
		kv = (float)p->kv[pix] / kv_scale;
		ir_data -= (float)p->offset[pix] * (1.0f + kta * (ta - 25.0f)) *
			   (1.0f + kv * (vdd - 3.3f));

		if (mode != p->calibration_mode_ee) {
			ir_data += p->il_chess_c[2] * (float)(2 * il_pattern - 1) -
				   p->il_chess_c[1] * (float)conversion_pattern;
		}

		ir_data -= p->tgc * ir_cp[subpage];
		ir_data /= emissivity;

		alpha_comp = MLX90640_SCALE_ALPHA * alpha_scale / (float)p->alpha[pix];
		alpha_comp *= 1.0f + p->ks_ta * (ta - 25.0f);

		sx = alpha_comp * alpha_comp * alpha_comp * (ir_data + alpha_comp * ta_tr);
		sx = sqrtf(sqrtf(sx)) * p->ks_to[1];

		t_obj = sqrtf(sqrtf(ir_data / (alpha_comp * (1.0f - p->ks_to[1] * 273.15f) + sx) +
				    ta_tr)) -
			273.15f;

		if (t_obj < (float)p->ct[1]) {
			range = 0;
		} else if (t_obj < (float)p->ct[2]) {
			range = 1;
		} else if (t_obj < (float)p->ct[3]) {
			range = 2;
		} else {
			range = 3;
		}

		t_obj = sqrtf(sqrtf(ir_data /
					    (alpha_comp * alpha_corr[range] *
					     (1.0f + p->ks_to[range] *
						      (t_obj - (float)p->ct[range]))) +
				    ta_tr)) -
			273.15f;
		to[pix] = t_obj;
	}
}
