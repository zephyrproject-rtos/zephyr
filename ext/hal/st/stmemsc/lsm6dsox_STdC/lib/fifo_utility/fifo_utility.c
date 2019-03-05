/*
 ******************************************************************************
 * @file    fifo_utility.c
 * @author  Sensor Solutions Software Team
 * @brief   utility for decoding / decompressing data from FIFO
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2018 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <string.h>
#include "fifo_utility.h"

#define ODR_XL_MASK				(0x0F)
#define ODR_XL_SHIFT			(0x00)
#define BDR_XL_MASK				(0x0F)
#define BDR_XL_SHIFT			(0x00)

#define ODR_GY_MASK				(0xF0)
#define ODR_GY_SHIFT			(0x04)
#define BDR_GY_MASK				(0xF0)
#define BDR_GY_SHIFT			(0x04)

#define BDR_VSENS_MASK			(0x0F)
#define BDR_VSENS_SHIFT			(0x00)

#define TAG_COUNTER_MASK		(0x06)
#define TAG_SENSOR_MASK			(0xF8)
#define TAG_COUNTER_SHIFT		(0x01)
#define TAG_SENSOR_SHIFT		(0x03)

#define TAG_GY					(0x01)
#define TAG_XL					(0x02)
#define TAG_TEMP				(0x03)
#define TAG_TS					(0x04)
#define TAG_ODRCHG				(0x05)
#define TAG_XL_UNCOMPRESSED_T_2	(0x06)
#define TAG_XL_UNCOMPRESSED_T_1	(0x07)
#define TAG_XL_COMPRESSED_2X	(0x08)
#define TAG_XL_COMPRESSED_3X	(0x09)
#define TAG_GY_UNCOMPRESSED_T_2	(0x0A)
#define TAG_GY_UNCOMPRESSED_T_1	(0x0B)
#define TAG_GY_COMPRESSED_2X	(0x0C)
#define TAG_GY_COMPRESSED_3X	(0x0D)
#define TAG_EXT_SENS_0			(0x0E)
#define TAG_EXT_SENS_1			(0x0F)
#define TAG_EXT_SENS_2			(0x10)
#define TAG_EXT_SENS_3			(0x11)
#define TAG_STEP_COUNTER		(0x12)
#define TAG_GAME_RV				(0x13)
#define TAG_GEOM_RV				(0x14)
#define TAG_NORM_RV				(0x15)
#define TAG_GYRO_BIAS			(0x16)
#define TAG_GRAVITIY			(0x17)
#define TAG_MAG_CAL				(0x18)
#define TAG_EXT_SENS_NACK		(0x19)

#define TAG_VALID_LIMIT			(0x19)

#define TIMESTAMP_FREQ			(40000.0f)

#define MAX(a, b)				((a) > (b) ? a : b)
#define MIN(a, b)				((a) < (b) ? a : b)

typedef enum {
        ST_FIFO_COMPRESSION_NC,
        ST_FIFO_COMPRESSION_NC_T_1,
        ST_FIFO_COMPRESSION_NC_T_2,
        ST_FIFO_COMPRESSION_2X,
        ST_FIFO_COMPRESSION_3X
} st_fifo_compression_type;

static uint8_t has_even_parity(uint8_t x);
static st_fifo_sensor_type get_sensor_type(uint8_t tag);
static st_fifo_compression_type get_compression_type(uint8_t tag);
static uint8_t is_tag_valid(uint8_t tag);
static void get_diff_2x(int16_t diff[6], uint8_t input[6]);
static void get_diff_3x(int16_t diff[9], uint8_t input[6]);

static const float bdr_acc_vect_def[] = {
		0, 13, 26, 52, 104, 208, 416,
		833, 1666, 3333, 6666, 1.625,
		0, 0, 0, 0
};

static const float bdr_gyr_vect_def[] = {
		0, 13, 26, 52, 104, 208, 416,
		833, 1666, 3333, 6666, 0, 0,
		0, 0, 0
};

static const float bdr_vsens_vect_def[] = {
		0, 13, 26, 52, 104, 208, 416,
		0, 0, 0, 0, 1.625, 0, 0, 0, 0
};

static uint8_t tag_counter_old = 0x00;
static float bdr_xl = 0.0;
static float bdr_gy = 0.0;
static float bdr_vsens = 0.0;
static float bdr_xl_old = 0.0;
static float bdr_gy_old = 0.0;
static float bdr_max = 0.0;
static uint32_t timestamp = 0;
static uint32_t last_timestamp_xl = 0;
static uint32_t last_timestamp_gy = 0;
static uint8_t bdr_chg_xl_flag = 0;
static uint8_t bdr_chg_gy_flag = 0;
static int16_t last_data_xl[3] = {0};
static int16_t last_data_gy[3] = {0};
static float bdr_acc_vect[] = {
		0, 13, 26, 52, 104, 208, 416,
		833, 1666, 3333, 6666, 1.625,
		0, 0, 0, 0
};

static float bdr_gyr_vect[] = {
		0, 13, 26, 52, 104, 208, 416,
		833, 1666, 3333, 6666, 0, 0,
		0, 0, 0
};

static float bdr_vsens_vect[] = {
		0, 13, 26, 52, 104, 208, 416,
		0, 0, 0, 0, 1.625, 0, 0, 0, 0
};

st_fifo_status st_fifo_init(float bdr_xl_in,
							float bdr_gy_in,
							float bdr_vsens_in)
{
        if (bdr_xl_in < 0.0f || bdr_gy_in < 0.0f || bdr_vsens_in < 0.0f)
                return ST_FIFO_ERR;

        tag_counter_old = 0x00;
        bdr_xl = bdr_xl_in;
        bdr_gy = bdr_gy_in;
        bdr_vsens = bdr_vsens_in;
        bdr_xl_old = bdr_xl_in;
        bdr_gy_old = bdr_gy_in;
        bdr_max = MAX(bdr_xl, bdr_gy);
        bdr_max = MAX(bdr_max, bdr_vsens);
        timestamp = 0;
        bdr_chg_xl_flag = 0;
        bdr_chg_gy_flag = 0;
        last_timestamp_xl = 0;
        last_timestamp_gy = 0;

        memcpy(bdr_acc_vect, bdr_acc_vect_def, sizeof(bdr_acc_vect_def));
        memcpy(bdr_gyr_vect, bdr_gyr_vect_def, sizeof(bdr_gyr_vect_def));
        memcpy(bdr_vsens_vect, bdr_vsens_vect_def, sizeof(bdr_vsens_vect_def));

        for (uint8_t i = 0; i < 3; i++) {
                last_data_xl[i] = 0;
                last_data_gy[i] = 0;
        }

        return ST_FIFO_OK;
}

void st_fifo_rescale_bdr_array(float scale)
{
        for (uint8_t i = 0; i < 16; i++) {
                bdr_acc_vect[i] *= scale;
                bdr_gyr_vect[i] *= scale;
                bdr_vsens_vect[i] *= scale;
        }
}

st_fifo_status st_fifo_decode(st_fifo_out_slot *fifo_out_slot,
							  st_fifo_raw_slot *fifo_raw_slot,
							  uint16_t *out_slot_size,
							  uint16_t stream_size)
{
        uint16_t j = 0;

        for (uint16_t i = 0; i < stream_size; i++) {

                uint8_t tag =
                	(fifo_raw_slot[i].fifo_data_out[0] & TAG_SENSOR_MASK) >> TAG_SENSOR_SHIFT;
                uint8_t tag_counter =
                	(fifo_raw_slot[i].fifo_data_out[0] & TAG_COUNTER_MASK) >> TAG_COUNTER_SHIFT;

                if (!has_even_parity(fifo_raw_slot[i].fifo_data_out[0]) || !is_tag_valid(tag))
                        return ST_FIFO_ERR;

                if ((tag_counter != (tag_counter_old)) && bdr_max != 0) {
                        timestamp += (uint32_t)(TIMESTAMP_FREQ / bdr_max);
                }

                if (tag == TAG_ODRCHG) {

                        uint8_t bdr_acc_cfg =
                        	(fifo_raw_slot[i].fifo_data_out[6] & BDR_XL_MASK) >> BDR_XL_SHIFT;
                        uint8_t bdr_gyr_cfg =
                        	(fifo_raw_slot[i].fifo_data_out[6] & BDR_GY_MASK) >> BDR_GY_SHIFT;
                        uint8_t bdr_vsens_cfg =
                        	(fifo_raw_slot[i].fifo_data_out[3] & BDR_VSENS_MASK) >> BDR_VSENS_SHIFT;

                        bdr_max = MAX(bdr_acc_vect[bdr_acc_cfg], bdr_gyr_vect[bdr_gyr_cfg]);
                        bdr_max = MAX(bdr_max, bdr_vsens_vect[bdr_vsens_cfg]);

                } else if (tag == TAG_TS) {

                        memcpy(&timestamp, &fifo_raw_slot[i].fifo_data_out[1], 4);

                } else {

                        if (tag == TAG_STEP_COUNTER)
                                memcpy(&fifo_out_slot[j].timestamp, &fifo_raw_slot[i].fifo_data_out[3], 4);
                        else
                                fifo_out_slot[j].timestamp = timestamp;

                        fifo_out_slot[j].sensor_tag = get_sensor_type(tag);
                        memcpy(fifo_out_slot[j].sensor_data.raw_data, &fifo_raw_slot[i].fifo_data_out[1], 6);
                        j++;
                        *out_slot_size = j;
                }

                tag_counter_old = tag_counter;
        }

        return ST_FIFO_OK;
}

st_fifo_status st_fifo_decompress(st_fifo_out_slot *fifo_out_slot,
								  st_fifo_raw_slot *fifo_raw_slot,
								  uint16_t *out_slot_size,
								  uint16_t stream_size)
{
        uint16_t j = 0;

        for (uint16_t i = 0; i < stream_size; i++) {

                uint8_t tag =
                	(fifo_raw_slot[i].fifo_data_out[0] & TAG_SENSOR_MASK) >> TAG_SENSOR_SHIFT;
                uint8_t tag_counter =
                	(fifo_raw_slot[i].fifo_data_out[0] & TAG_COUNTER_MASK) >> TAG_COUNTER_SHIFT;

                if (!has_even_parity(fifo_raw_slot[i].fifo_data_out[0]) || !is_tag_valid(tag))
                        continue;

                if ((tag_counter != (tag_counter_old)) && bdr_max != 0) {

                        uint8_t diff_tag_counter = 0;

                        if (tag_counter < tag_counter_old)
                                diff_tag_counter = tag_counter + 4 - tag_counter_old;
                        else
                                diff_tag_counter = tag_counter - tag_counter_old;

                        timestamp += (uint32_t)(TIMESTAMP_FREQ / bdr_max) * diff_tag_counter;
                }

                if (tag == TAG_ODRCHG) {

                        uint8_t bdr_acc_cfg =
                        	(fifo_raw_slot[i].fifo_data_out[6] & BDR_XL_MASK) >> BDR_XL_SHIFT;
                        uint8_t bdr_gyr_cfg =
                        	(fifo_raw_slot[i].fifo_data_out[6] & BDR_GY_MASK) >> BDR_GY_SHIFT;
                        uint8_t bdr_vsens_cfg =
                        	(fifo_raw_slot[i].fifo_data_out[3] & BDR_VSENS_MASK) >> BDR_VSENS_SHIFT;

                        bdr_xl_old = bdr_xl;
                        bdr_gy_old = bdr_gy;

                        bdr_xl = bdr_acc_vect[bdr_acc_cfg];
                        bdr_gy = bdr_gyr_vect[bdr_gyr_cfg];
                        bdr_vsens = bdr_vsens_vect[bdr_vsens_cfg];
                        bdr_max = MAX(bdr_xl, bdr_gy);
                        bdr_max = MAX(bdr_max, bdr_vsens);

                        bdr_chg_xl_flag = 1;
                        bdr_chg_gy_flag = 1;

                } else if (tag == TAG_TS) {

                        memcpy(&timestamp, &fifo_raw_slot[i].fifo_data_out[1], 4);

                } else {

                        st_fifo_compression_type compression_type = get_compression_type(tag);
                        st_fifo_sensor_type sensor_type = get_sensor_type(tag);

                        if (compression_type == ST_FIFO_COMPRESSION_NC) {

                                if (tag == TAG_STEP_COUNTER)
                                        memcpy(&fifo_out_slot[j].timestamp, &fifo_raw_slot[i].fifo_data_out[3], 4);
                                else
                                        fifo_out_slot[j].timestamp = timestamp;

                                fifo_out_slot[j].sensor_tag = sensor_type;
                                memcpy(fifo_out_slot[j].sensor_data.raw_data, &fifo_raw_slot[i].fifo_data_out[1], 6);

                                if (sensor_type == ST_FIFO_ACCELEROMETER) {
                                        memcpy(last_data_xl, fifo_out_slot[j].sensor_data.raw_data, 6);
                                        last_timestamp_xl = timestamp;
                                        bdr_chg_xl_flag = 0;
                                }

                                if (sensor_type == ST_FIFO_GYROSCOPE) {
                                        memcpy(last_data_gy, fifo_out_slot[j].sensor_data.raw_data, 6);
                                        last_timestamp_gy = timestamp;
                                        bdr_chg_gy_flag = 0;
                                }

                                j++;

                        } else if (compression_type == ST_FIFO_COMPRESSION_NC_T_1) {

                                fifo_out_slot[j].sensor_tag = get_sensor_type(tag);
                                memcpy(fifo_out_slot[j].sensor_data.raw_data, &fifo_raw_slot[i].fifo_data_out[1], 6);

                                if (sensor_type == ST_FIFO_ACCELEROMETER) {
                                        uint32_t last_timestamp;

                                        if (bdr_chg_xl_flag)
                                                last_timestamp = (uint32_t)(last_timestamp_xl + TIMESTAMP_FREQ / bdr_xl_old);
                                        else
                                                last_timestamp = (uint32_t)(timestamp - TIMESTAMP_FREQ / bdr_xl);

                                        fifo_out_slot[j].timestamp = last_timestamp;
                                        memcpy(last_data_xl, fifo_out_slot[j].sensor_data.raw_data, 6);
                                        last_timestamp_xl = last_timestamp;
                                }

                                if (sensor_type == ST_FIFO_GYROSCOPE) {
                                        uint32_t last_timestamp;

                                        if (bdr_chg_gy_flag)
                                                last_timestamp = (uint32_t)(last_timestamp_gy + TIMESTAMP_FREQ / bdr_gy_old);
                                        else
                                                last_timestamp = (uint32_t)(timestamp - TIMESTAMP_FREQ / bdr_gy);

                                        fifo_out_slot[j].timestamp = last_timestamp;
                                        memcpy(last_data_gy, fifo_out_slot[j].sensor_data.raw_data, 6);
                                        last_timestamp_gy = last_timestamp;
                                }

                                j++;

                        } else if (compression_type == ST_FIFO_COMPRESSION_NC_T_2) {

                                fifo_out_slot[j].sensor_tag = get_sensor_type(tag);
                                memcpy(fifo_out_slot[j].sensor_data.raw_data, &fifo_raw_slot[i].fifo_data_out[1], 6);

                                if (sensor_type == ST_FIFO_ACCELEROMETER) {
                                        uint32_t last_timestamp;

                                        if (bdr_chg_xl_flag)
                                                last_timestamp = (uint32_t)(last_timestamp_xl + TIMESTAMP_FREQ / bdr_xl_old);
                                        else
                                                last_timestamp = (uint32_t)(timestamp - 2 * TIMESTAMP_FREQ / bdr_xl);

                                        fifo_out_slot[j].timestamp = last_timestamp;
                                        memcpy(last_data_xl, fifo_out_slot[j].sensor_data.raw_data, 6);
                                        last_timestamp_xl = last_timestamp;
                                }
                                if (sensor_type == ST_FIFO_GYROSCOPE) {
                                        uint32_t last_timestamp;

                                        if (bdr_chg_gy_flag)
                                                last_timestamp = (uint32_t)(last_timestamp_gy + TIMESTAMP_FREQ / bdr_gy_old);
                                        else
                                                last_timestamp = (uint32_t)(timestamp - 2 * TIMESTAMP_FREQ / bdr_gy);

                                        fifo_out_slot[j].timestamp = last_timestamp;
                                        memcpy(last_data_gy, fifo_out_slot[j].sensor_data.raw_data, 6);
                                        last_timestamp_gy = last_timestamp;
                                }

                                j++;

                        } else if(compression_type == ST_FIFO_COMPRESSION_2X) {

                                int16_t diff[6];
                                get_diff_2x(diff, &fifo_raw_slot[i].fifo_data_out[1]);

                                fifo_out_slot[j].sensor_tag = sensor_type;

                                if (sensor_type == ST_FIFO_ACCELEROMETER) {
                                        fifo_out_slot[j].sensor_data.data[0] = last_data_xl[0] + diff[0];
                                        fifo_out_slot[j].sensor_data.data[1] = last_data_xl[1] + diff[1];
                                        fifo_out_slot[j].sensor_data.data[2] = last_data_xl[2] + diff[2];
                                        fifo_out_slot[j].timestamp = (uint32_t)(timestamp - 2 * TIMESTAMP_FREQ / bdr_xl);
                                        memcpy(last_data_xl, fifo_out_slot[j].sensor_data.raw_data, 6);
                                }

                                if (sensor_type == ST_FIFO_GYROSCOPE) {
                                        fifo_out_slot[j].sensor_data.data[0] = last_data_gy[0] + diff[0];
                                        fifo_out_slot[j].sensor_data.data[1] = last_data_gy[1] + diff[1];
                                        fifo_out_slot[j].sensor_data.data[2] = last_data_gy[2] + diff[2];
                                        fifo_out_slot[j].timestamp = (uint32_t)(timestamp - 2 * TIMESTAMP_FREQ / bdr_gy);
                                        memcpy(last_data_gy, fifo_out_slot[j].sensor_data.raw_data, 6);
                                }

                                j++;

                                fifo_out_slot[j].sensor_tag = sensor_type;

                                if (sensor_type == ST_FIFO_ACCELEROMETER) {
                                        uint32_t last_timestamp = (uint32_t)(timestamp - TIMESTAMP_FREQ / bdr_xl);
                                        fifo_out_slot[j].sensor_data.data[0] = last_data_xl[0] + diff[3];
                                        fifo_out_slot[j].sensor_data.data[1] = last_data_xl[1] + diff[4];
                                        fifo_out_slot[j].sensor_data.data[2] = last_data_xl[2] + diff[5];
                                        fifo_out_slot[j].timestamp = last_timestamp;
                                        memcpy(last_data_xl, fifo_out_slot[j].sensor_data.raw_data, 6);
                                        last_timestamp_xl = last_timestamp;
                                }

                                if (sensor_type == ST_FIFO_GYROSCOPE) {
                                        uint32_t last_timestamp = (uint32_t)(timestamp - TIMESTAMP_FREQ / bdr_gy);
                                        fifo_out_slot[j].sensor_data.data[0] = last_data_gy[0] + diff[3];
                                        fifo_out_slot[j].sensor_data.data[1] = last_data_gy[1] + diff[4];
                                        fifo_out_slot[j].sensor_data.data[2] = last_data_gy[2] + diff[5];
                                        fifo_out_slot[j].timestamp = last_timestamp;
                                        memcpy(last_data_gy, fifo_out_slot[j].sensor_data.raw_data, 6);
                                        last_timestamp_gy = last_timestamp;
                                }

                                j++;

                        } else if (compression_type == ST_FIFO_COMPRESSION_3X) {

                                int16_t diff[9];
                                get_diff_3x(diff, &fifo_raw_slot[i].fifo_data_out[1]);

                                fifo_out_slot[j].sensor_tag = sensor_type;

                                if (sensor_type == ST_FIFO_ACCELEROMETER) {
                                        fifo_out_slot[j].sensor_data.data[0] = last_data_xl[0] + diff[0];
                                        fifo_out_slot[j].sensor_data.data[1] = last_data_xl[1] + diff[1];
                                        fifo_out_slot[j].sensor_data.data[2] = last_data_xl[2] + diff[2];
                                        fifo_out_slot[j].timestamp = (uint32_t)(timestamp - 2 * TIMESTAMP_FREQ / bdr_xl);
                                        memcpy(last_data_xl, fifo_out_slot[j].sensor_data.raw_data, 6);
                                }

                                if (sensor_type == ST_FIFO_GYROSCOPE) {
                                        fifo_out_slot[j].sensor_data.data[0] = last_data_gy[0] + diff[0];
                                        fifo_out_slot[j].sensor_data.data[1] = last_data_gy[1] + diff[1];
                                        fifo_out_slot[j].sensor_data.data[2] = last_data_gy[2] + diff[2];
                                        fifo_out_slot[j].timestamp = (uint32_t)(timestamp - 2 * TIMESTAMP_FREQ / bdr_gy);
                                        memcpy(last_data_gy, fifo_out_slot[j].sensor_data.raw_data, 6);
                                }

                                j++;

                                fifo_out_slot[j].sensor_tag = sensor_type;

                                if (sensor_type == ST_FIFO_ACCELEROMETER) {
                                        fifo_out_slot[j].sensor_data.data[0] = last_data_xl[0] + diff[3];
                                        fifo_out_slot[j].sensor_data.data[1] = last_data_xl[1] + diff[4];
                                        fifo_out_slot[j].sensor_data.data[2] = last_data_xl[2] + diff[5];
                                        fifo_out_slot[j].timestamp = (uint32_t)(timestamp - TIMESTAMP_FREQ / bdr_xl);
                                        memcpy(last_data_xl, fifo_out_slot[j].sensor_data.raw_data, 6);
                                }

                                if (sensor_type == ST_FIFO_GYROSCOPE) {
                                        fifo_out_slot[j].sensor_data.data[0] = last_data_gy[0] + diff[3];
                                        fifo_out_slot[j].sensor_data.data[1] = last_data_gy[1] + diff[4];
                                        fifo_out_slot[j].sensor_data.data[2] = last_data_gy[2] + diff[5];
                                        fifo_out_slot[j].timestamp = (uint32_t)(timestamp - TIMESTAMP_FREQ / bdr_gy);
                                        memcpy(last_data_gy, fifo_out_slot[j].sensor_data.raw_data, 6);
                                }

                                j++;

                                fifo_out_slot[j].timestamp = timestamp;
                                fifo_out_slot[j].sensor_tag = sensor_type;

                                if (sensor_type == ST_FIFO_ACCELEROMETER) {
                                        fifo_out_slot[j].sensor_data.data[0] = last_data_xl[0] + diff[6];
                                        fifo_out_slot[j].sensor_data.data[1] = last_data_xl[1] + diff[7];
                                        fifo_out_slot[j].sensor_data.data[2] = last_data_xl[2] + diff[8];
                                        memcpy(last_data_xl, fifo_out_slot[j].sensor_data.raw_data, 6);
                                        last_timestamp_xl = timestamp;
                                }

                                if (sensor_type == ST_FIFO_GYROSCOPE) {
                                        fifo_out_slot[j].sensor_data.data[0] = last_data_gy[0] + diff[6];
                                        fifo_out_slot[j].sensor_data.data[1] = last_data_gy[1] + diff[7];
                                        fifo_out_slot[j].sensor_data.data[2] = last_data_gy[2] + diff[8];
                                        memcpy(last_data_gy, fifo_out_slot[j].sensor_data.raw_data, 6);
                                        last_timestamp_gy = timestamp;
                                }

                                j++;
                        }

                        *out_slot_size = j;
                }

                tag_counter_old = tag_counter;
        }

        return ST_FIFO_OK;
}

void st_fifo_sort(st_fifo_out_slot *fifo_out_slot, uint16_t out_slot_size)
{
        uint16_t i;
        int32_t j;
        st_fifo_out_slot temp;

        for (i = 1; i < out_slot_size; i++) {

                memcpy(&temp, &fifo_out_slot[i], sizeof(st_fifo_out_slot));

                j = i - 1;

                while (j >= 0 && fifo_out_slot[j].timestamp > temp.timestamp) {
                        memcpy(&fifo_out_slot[j + 1], &fifo_out_slot[j], sizeof(st_fifo_out_slot));
                        j--;
                }

                memcpy(&fifo_out_slot[j + 1], &temp, sizeof(st_fifo_out_slot));
        }

        return;
}

uint16_t  st_fifo_get_sensor_occurrence(st_fifo_out_slot *fifo_out_slot, uint16_t out_slot_size, st_fifo_sensor_type sensor_type)
{
        uint16_t occurrence = 0;

        for (uint16_t i = 0; i < out_slot_size; i++) {
                if (fifo_out_slot[i].sensor_tag == sensor_type)
                        occurrence++;
        }

        return occurrence;
}

void st_fifo_extract_sensor(st_fifo_out_slot *sensor_out_slot,
							st_fifo_out_slot *fifo_out_slot,
							uint16_t  out_slot_size,
							st_fifo_sensor_type sensor_type)
{
        uint16_t temp_i = 0;

        for (uint16_t i = 0; i < out_slot_size; i++) {
                if (fifo_out_slot[i].sensor_tag == sensor_type) {
                        memcpy(&sensor_out_slot[temp_i], &fifo_out_slot[i], sizeof(st_fifo_out_slot));
                        temp_i++;
                }
        }
}

static uint8_t is_tag_valid(uint8_t tag)
{
        if (tag > TAG_VALID_LIMIT)
                return 0;
        else
                return 1;
}

static st_fifo_sensor_type get_sensor_type(uint8_t tag)
{
        switch (tag) {
                case TAG_GY:
                        return ST_FIFO_GYROSCOPE;
                case TAG_XL:
                        return ST_FIFO_ACCELEROMETER;
                case TAG_TEMP:
                        return ST_FIFO_TEMPERATURE;
                case TAG_EXT_SENS_0:
                        return ST_FIFO_EXT_SENSOR0;
                case TAG_EXT_SENS_1:
                        return ST_FIFO_EXT_SENSOR1;
                case TAG_EXT_SENS_2:
                        return ST_FIFO_EXT_SENSOR2;
                case TAG_EXT_SENS_3:
                        return ST_FIFO_EXT_SENSOR3;
                case TAG_STEP_COUNTER:
                        return ST_FIFO_STEP_COUNTER;
                case TAG_XL_UNCOMPRESSED_T_2:
                        return ST_FIFO_ACCELEROMETER;
                case TAG_XL_UNCOMPRESSED_T_1:
                        return ST_FIFO_ACCELEROMETER;
                case TAG_XL_COMPRESSED_2X:
                        return ST_FIFO_ACCELEROMETER;
                case TAG_XL_COMPRESSED_3X:
                        return ST_FIFO_ACCELEROMETER;
                case TAG_GY_UNCOMPRESSED_T_2:
                        return ST_FIFO_GYROSCOPE;
                case TAG_GY_UNCOMPRESSED_T_1:
                        return ST_FIFO_GYROSCOPE;
                case TAG_GY_COMPRESSED_2X:
                        return ST_FIFO_GYROSCOPE;
                case TAG_GY_COMPRESSED_3X:
                        return ST_FIFO_GYROSCOPE;
                case TAG_GAME_RV:
                        return ST_FIFO_6X_GAME_RV;
                case TAG_GEOM_RV:
                        return ST_FIFO_6X_GEOM_RV;
                case TAG_NORM_RV:
                        return ST_FIFO_9X_RV;
                case TAG_GYRO_BIAS:
                        return ST_FIFO_GYRO_BIAS;
                case TAG_GRAVITIY:
                        return ST_FIFO_GRAVITY;
                case TAG_MAG_CAL:
                        return ST_FIFO_MAGNETOMETER_CALIB;
                case TAG_EXT_SENS_NACK:
                        return ST_FIFO_EXT_SENSOR_NACK;
                default:
                        return ST_FIFO_NONE;
        }
}

static st_fifo_compression_type get_compression_type(uint8_t tag)
{
        switch (tag) {
                case TAG_GY:
                        return ST_FIFO_COMPRESSION_NC;
                case TAG_XL:
                        return ST_FIFO_COMPRESSION_NC;
                case TAG_TEMP:
                        return ST_FIFO_COMPRESSION_NC;
                case TAG_EXT_SENS_0:
                        return ST_FIFO_COMPRESSION_NC;
                case TAG_EXT_SENS_1:
                        return ST_FIFO_COMPRESSION_NC;
                case TAG_EXT_SENS_2:
                        return ST_FIFO_COMPRESSION_NC;
                case TAG_EXT_SENS_3:
                        return ST_FIFO_COMPRESSION_NC;
                case TAG_STEP_COUNTER:
                        return ST_FIFO_COMPRESSION_NC;
                case TAG_XL_UNCOMPRESSED_T_2:
                        return ST_FIFO_COMPRESSION_NC_T_2;
                case TAG_XL_UNCOMPRESSED_T_1:
                        return ST_FIFO_COMPRESSION_NC_T_1;
                case TAG_XL_COMPRESSED_2X:
                        return ST_FIFO_COMPRESSION_2X;
                case TAG_XL_COMPRESSED_3X:
                        return ST_FIFO_COMPRESSION_3X;
                case TAG_GY_UNCOMPRESSED_T_2:
                        return ST_FIFO_COMPRESSION_NC_T_2;
                case TAG_GY_UNCOMPRESSED_T_1:
                        return ST_FIFO_COMPRESSION_NC_T_1;
                case TAG_GY_COMPRESSED_2X:
                        return ST_FIFO_COMPRESSION_2X;
                case TAG_GY_COMPRESSED_3X:
                        return ST_FIFO_COMPRESSION_3X;
                default:
                        return ST_FIFO_COMPRESSION_NC;
        }
}

static uint8_t has_even_parity(uint8_t x)
{
    uint8_t count = 0x00, i, b = 0x01;

    for (i = 0; i < 8; i++) {
        if(x & (b << i))
                        count++;
    }

    if (count & 0x01)
                return 0;

    return 1;
}

static void get_diff_2x(int16_t diff[6], uint8_t input[6])
{
        for (uint8_t i = 0; i < 6; i++)
                diff[i] = input[i] < 128 ? input[i] : (input[i] - 256);
}


static void get_diff_3x(int16_t diff[9], uint8_t input[6])
{
        uint16_t decode_temp;

        for (uint8_t i = 0; i < 3; i++) {

                memcpy(&decode_temp, &input[2 * i], 2);

                for (uint8_t j = 0; j < 3; j++) {
                        int16_t temp = (decode_temp & (0x001F << (5 * j))) >> (5 * j);
                        diff[j + 3 * i] = temp < 16 ? temp : (temp - 32);
                }
        }
}

