/*
 ******************************************************************************
 * @file    fifo_utility.h
 * @author  Sensor Solutions Software Team
 * @brief   This file contains all the functions prototypes for the
 *          fifo_utility.c.
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

#ifndef _ST_FIFO_H_
#define _ST_FIFO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
        ST_FIFO_OK,
        ST_FIFO_ERR
} st_fifo_status;

typedef enum {
        ST_FIFO_GYROSCOPE,
        ST_FIFO_ACCELEROMETER,
        ST_FIFO_TEMPERATURE,
        ST_FIFO_EXT_SENSOR0,
        ST_FIFO_EXT_SENSOR1,
        ST_FIFO_EXT_SENSOR2,
        ST_FIFO_EXT_SENSOR3,
        ST_FIFO_STEP_COUNTER,
        ST_FIFO_6X_GAME_RV,
        ST_FIFO_6X_GEOM_RV,
        ST_FIFO_9X_RV,
        ST_FIFO_GYRO_BIAS,
        ST_FIFO_GRAVITY,
        ST_FIFO_MAGNETOMETER_CALIB,
        ST_FIFO_EXT_SENSOR_NACK,
        ST_FIFO_NONE
} st_fifo_sensor_type;

typedef struct {
        uint8_t fifo_data_out[7]; /* registers from mems (78h -> 7Dh) */
} st_fifo_raw_slot;

typedef struct {
        uint32_t timestamp;
        st_fifo_sensor_type sensor_tag;
        union {
                uint8_t raw_data[6]; 	/* bytes */
                int16_t data[3]; 		/* 3 axes mems */
                int16_t temp; 			/* temperature sensor */
                uint16_t steps; 		/* step counter */
                uint16_t quat[3]; 		/* quaternion 3 axes format [x,y,z] */
                uint8_t nack; 			/* ext sensor nack index */
        } sensor_data;
} st_fifo_out_slot;

st_fifo_status st_fifo_init(float bdr_xl, float bdr_gy, float bdr_vsens);
void st_fifo_rescale_bdr_array(float scale);
st_fifo_status st_fifo_decode(st_fifo_out_slot *fifo_out_slot,
							  st_fifo_raw_slot *fifo_raw_slot,
							  uint16_t *out_slot_size,
							  uint16_t stream_size);
st_fifo_status st_fifo_decompress(st_fifo_out_slot *fifo_out_slot,
								  st_fifo_raw_slot *fifo_raw_slot,
								  uint16_t *out_slot_size,
								  uint16_t stream_size);
void st_fifo_sort(st_fifo_out_slot *fifo_out_slot, uint16_t out_slot_size);
uint16_t st_fifo_get_sensor_occurrence(st_fifo_out_slot *fifo_out_slot,
									   uint16_t out_slot_size,
									   st_fifo_sensor_type sensor_type);
void st_fifo_extract_sensor(st_fifo_out_slot *sensor_out_slot,
							st_fifo_out_slot *fifo_out_slot,
							uint16_t out_slot_size,
							st_fifo_sensor_type sensor_type);

#ifdef __cplusplus
}
#endif

#endif /* _ST_FIFO_H_ */
