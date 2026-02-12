/*
 * Copyright (c) 2025 TDK Invensense
 *
 * SPDX-License-Identifier: BSD 3-Clause
 */

/** @defgroup DriverTAD214x tad214x driver
 *  @brief    Low-level driver for tad214x devices
 *  @ingroup  Drivers
 *  @{
 */

#ifndef _TAD214X_H_
#define _TAD214X_H_

#include "tad214xSerif.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief tad214x driver states definition
 */
typedef struct tad214x {
	struct tad214x_serif serif;
	int    frame_cnt;
} tad214x_t;

/** @brief Debug Data
 *  @details Debug Data Selection
 */
typedef enum {
	TAD214X_OBSERVE_DATA_DISABLED		= 0,
	TAD214X_OBSERVE_DATA_SINCOS_RAW		= 1,
	TAD214X_OBSERVE_DATA_SINCOS_COMP	= 2,
	TAD214X_OBSERVE_DATA_LUT_IN			= 3,
	TAD214X_OBSERVE_DATA_LUT_OUT		= 4,
	TAD214X_OBSERVE_DATA_PRED_OUT		= 5,
	TAD214X_OBSERVE_DATA_OUT_ADJ_OUT	= 6
} tad214x_observe_data_sel_t;

/** @brief Power Mode
 *  @details Power Mode Selection
 */
typedef enum {
	TAD214X_MODE_SBY	= 0,
	TAD214X_MODE_CONT	= 1,
	TAD214X_MODE_LPM	= 2,
	TAD214X_MODE_SINGLE	= 3
} TAD214X_PowerMode_t;

/** @brief ODR
 *  @details ODR Selection
 */
typedef enum {
	TAD214X_ODR_10	= 0,
	TAD214X_ODR_100	= 1,
	TAD214X_ODR_300	= 2
} TAD214X_ODR_t;

/** @brief OTP Command
 */
typedef enum {
	OTP_NOP = 0,
	OTP_READ_INT = 1,
    OTP_READ_EXT = 2,
    OTP_PROG_BITCELL = 3,
    OTP_PROG_WORD = 4,
    OTP_SOAK_BITCELL = 5,
    OTP_READ_EXT_MAX = 6,
    OTP_SOAK_WORD = 7
}TAD214X_OTP_CTRL_CMD_t;

#define CONST_KEY 0x5555

#define OTP_CONF_SETTINGS_NUMBER 2
#define CONF0_KEY 0x5555
#define CONF1_KEY 0xAAAA

#define OTP_CALIB_SETTINGS_NUMBER 5
#define CALIB0_KEY 0x5555
#define CALIB1_KEY 0xAAAA
#define CALIB2_KEY 0x3333
#define CALIB3_KEY 0xCCCC
#define CALIB4_KEY 0xF0F0  

#define UNLOCK_KEY 0xC7CA


/** @brief Sleep function.
 *  @param[in] s   Pointer to device.
 *  @param[in] us  Time to sleep in microseconds.
 */
void TAD214x_sleep_us(struct tad214x *s, uint32_t us);

/** @brief TAD214x Software Reset 
 *  @param[in] s Pointer to driver structure
 *  @return     0 on success, negative value on error
 */
int TAD214x_SoftReset(struct tad214x *s);

/** @brief TAD214x Init
 *  @param[in] s Pointer to driver structure
 *  @return     0 on success, negative value on error
 */
int TAD214x_Init(struct tad214x *s, struct tad214x_serif *serif);

/** @brief TAD214x Write Register
 *  @param[in] s Pointer to driver structure
 *  @param[in] Adr Register Address
 *  @param[in] Data Register Data pointer
 *  @return     0 on success, negative value on error
 */
int TAD214x_WriteReg(struct tad214x_serif *s, uint8_t Adr, uint16_t *Data);

/** @brief TAD214x Read Register
 *  @param[in] s Pointer to driver structure
 *  @param[in] Adr Register Address to Read
 *  @param[in] Data Register Read Data pointer
 *  @return     0 on success, negative value on error
 */
int TAD214x_ReadReg(struct tad214x_serif *s, uint8_t Adr, uint16_t *Data);

/** @brief TAD214x Read Modify Write Register
 *  @param[in] s Pointer to driver structure
 *  @param[in] Adr Register Address to modify
 *  @param[in] Data Register Data to write
 *  @param[in] Mask Register Data Mask
 *  @return     0 on success, negative value on error
 */
int TAD214x_ReadModifyWriteReg(struct tad214x_serif *s, uint8_t Adr, uint16_t Data, uint16_t Mask);

/** @brief TAD214x Set ODR
 *  @param[in] s Pointer to driver structure
 *  @param[in] Odr value see @sa TAD214X_ODR_t
 *  @return     0 on success, negative value on error
 */
int TAD214x_SetODR(struct tad214x *s, TAD214X_ODR_t Odr);

/** @brief TAD214x Get ODR
 *  @param[in] s Pointer to driver structure
 *  @param[in] Odr value pointer see @sa TAD214X_ODR_t
 *  @return     0 on success, negative value on error
 */
int TAD214x_GetODR(struct tad214x *s, TAD214X_ODR_t * Odr);

/** @brief TAD214x Set Power Mode
 *  @param[in] s Pointer to driver structure
 *  @param[in] PowerMode pointer see @sa TAD214X_PowerMode_t
 *  @return     0 on success, negative value on error
 */
int TAD214x_SetMode(struct tad214x *s, TAD214X_PowerMode_t PowerMode);

/** @brief TAD214x Get Power Mode
 *  @param[in] s Pointer to driver structure
 *  @param[in] PowerMode pointer see @sa TAD214X_PowerMode_t
 *  @return     0 on success, negative value on error
 */
int TAD214x_GetMode(struct tad214x *s, TAD214X_PowerMode_t * PowerMode);

/** @brief TAD214x Get Data
 *  @param[in] s Pointer to driver structure
 *  @param[in] TMRData TMR Data pointer
 *  @param[in] TempData Temperature Data pointer
 *  @return     0 on success, negative value on error
 */
int TAD214x_GetData(struct tad214x *s, uint16_t *TMRData, int16_t *TempData);

/** @brief TAD214x Set Angle Offset
 *  @param[in] s Pointer to driver structure
 *  @param[in] AngleOffset Angle Offset (LSB=360*2^-15 deg)
 *  @param[in] cw_ccw_sign Angle Signed 0: No Adjustment 1:360deg-angle
 *  @return     0 on success, negative value on error
 */
int TAD214x_SetAngleOffset(struct tad214x *s, uint16_t AngleOffset, uint8_t cw_ccw_sign);

/** @brief TAD214x Get Angle Offset
 *  @param[in] s Pointer to driver structure
 *  @param[in] AngleOffset Angle Offset pointer (LSB=360*2^-15 deg)
 *  @param[in] cw_ccw_sign Angle Signed pointer 0: No Adjustment 1:360deg-angle
 *  @return     0 on success, negative value on error
 */
int TAD214x_GetAngleOffset(struct tad214x *s, uint16_t * AngleOffset, uint8_t * cw_ccw_sign);

/** @brief TAD214x Write OTP
 *  @param[in] s Pointer to driver structure
 *  @param[in] Adr OTP Address
 *  @param[in] Data OTP Data to write
 *  @return     0 on success, negative value on error
 */
int TAD214x_WriteOTP(struct tad214x_serif *s, uint16_t Adr, uint16_t Data);

/** @brief TAD214x Read OTP
 *  @param[in] s Pointer to driver structure
 *  @param[in] Adr OTP Address
 *  @param[in] Data OTP Data to Read pointer
 *  @return     0 on success, negative value on error
 */
int TAD214x_ReadOTP(struct tad214x_serif *s, uint16_t Adr, uint16_t * Data);

/** @brief TAD214x Unlock RWT Registers
 *  @param[in] s Pointer to driver structure
 *  @return     0 on success, negative value on error
 */
int TAD214x_Unlock(struct tad214x *s);

/** @brief TAD214x Lock RWT Registers
 *  @param[in] s Pointer to driver structure
 *  @return     0 on success, negative value on error
 */
int TAD214x_Lock(struct tad214x *s);

/** @brief TAD214x Enable Program Mode
 *  @param[in] s Pointer to driver structure
 *  @return     0 on success, negative value on error
 */
int TAD214x_ProgramModeEnable(struct tad214x *s);

/** @brief TAD214x Disable Program Mode
 *  @param[in] s Pointer to driver structure
 *  @return     0 on success, negative value on error
 */
int TAD214x_ProgramModeDisable(struct tad214x *s);

/** @brief TAD214x Enable Prediction
 *  @param[in] s Pointer to driver structure
 *  @return     0 on success, negative value on error
 */
int TAD214x_PredictionEnable(struct tad214x *s);

/** @brief TAD214x Disable Prediction
 *  @param[in] s Pointer to driver structure
 *  @return     0 on success, negative value on error
 */
int TAD214x_PredictionDisable(struct tad214x *s);

/** @brief TAD214x Enable Dynamic Compensation
 *  @param[in] s Pointer to driver structure
 *  @return     0 on success, negative value on error
 */
int TAD214x_EnableDynamicCompensation(struct tad214x *s);

/** @brief TAD214x Disable Dynamic Compensation
 *  @param[in] s Pointer to driver structure
 *  @return     0 on success, negative value on error
 */
int TAD214x_DisableDynamicCompensation(struct tad214x *s);

/** @brief TAD214x Enable LUT Compensation
 *  @param[in] s Pointer to driver structure
 *  @return     0 on success, negative value on error
 */
int TAD214x_EnableLUTCompensation(struct tad214x *s);

/** @brief TAD214x Disable LUT Compensation
 *  @param[in] s Pointer to driver structure
 *  @return     0 on success, negative value on error
 */
int TAD214x_DisableLUTCompensation(struct tad214x *s);

/** @brief TAD214x Set SinCos Gain
 *  @param[in] s Pointer to driver structure
 *  @param[in] SinCosGain [0]sin_lin and [1]cos
 *  @return     0 on success, negative value on error
 */
int TAD214x_SetSinCosGain(struct tad214x *s, uint16_t SinCosGain[2]);

/** @brief TAD214x Get SinCos Gain
 *  @param[in] s Pointer to driver structure
 *  @param[in] SinCosGain [0]sin_lin and [1]cos
 *  @return     0 on success, negative value on error
 */
int TAD214x_GetSinCosGain(struct tad214x *s, uint16_t SinCosGain[2]);

/** @brief TAD214x Set SinCos Offset
 *  @param[in] s Pointer to driver structure
 *  @param[in] SinCosOffset [0]sin_lin and [1]cos
 *  @return     0 on success, negative value on error
 */
int TAD214x_SetSinCosOffset(struct tad214x *s, int16_t SinCosOffset[2]);

/** @brief TAD214x Get SinCos Offset
 *  @param[in] s Pointer to driver structure
 *  @param[in] SinCosOffset [0]sin_lin and [1]cos
 *  @return     0 on success, negative value on error
 */
int TAD214x_GetSinCosOffset(struct tad214x *s, int16_t SinCosOffset[2]);

/** @brief TAD214x Get Current SinCos Gain
 *  @param[in] s Pointer to driver structure
 *  @param[in] SinCosGain [0]sin and [1]cos
 *  @return     0 on success, negative value on error
 */
int TAD214x_GetCurrentSinCosGain(struct tad214x *s, uint16_t SinCosGain[2]);

/** @brief TAD214x Get Current SinCos Offset
 *  @param[in] s Pointer to driver structure
 *  @param[in] SinCosOffset [0]sin and [1]cos
 *  @return     0 on success, negative value on error
 */
int TAD214x_GetCurrentSinCosOffset(struct tad214x *s, int16_t SinCosOffset[2]);

/** @brief Select Data to observe for debug
 *  @param[in] s Pointer to driver structure
 *  @param[in] tad214x_observe_data_sel one of @sa tad214x_observe_data_sel_t
 *  @return     0 on success, negative value on error
 */
int TAD214x_SetObserveDebugMode(struct tad214x *s, tad214x_observe_data_sel_t tad214x_observe_data_sel);

/** @brief Get Debug data
 *  @param[in] s Pointer to driver structure
 *  @param[in] ObservData Debug Data selected pointer @sa tad214x_observe_data_sel_t
 *  @return     0 on success, negative value on error
 */
int TAD214x_GetObserveData(struct tad214x *s, int16_t ObservData[2]);

/** @brief Enable SPI Interface
 *  @param[in] s Pointer to driver structure
 *  @details This function can be call only on Encoder part number (TAD214x-GCDA)
 *  @details (Maximum 45mA current is required)
 *  @return     0 on success, negative value on error
 */
int TAD214x_EnableSPIInterface(struct tad214x *s);

/** @brief Get TAD214x OTP Status
 *  @param[in] s Pointer to driver structure
 *  @param[in] CONST_Status pointer -1 No CONST OTP program, 0 CONST OTP Program
 *  @param[in] CONF_Status  pointer -1 No CONF  OTP program, 0 CONF0 OTP Program, 1 CONF1 OTP Program
 *  @param[in] CALIB_Status pointer -1 No CALIB OTP program, n CONFn OTP Program (n from 0 to 4)
 *  @return     0 on success, negative value on error
 */
int TAD214x_GetOTPStatus(struct tad214x *s, int8_t * CONST_Status, int8_t * CONF_Status, int8_t * CALIB_Status);

#ifdef __cplusplus
}
#endif

#endif /* _TAD214X_H_ */

/** @} */
