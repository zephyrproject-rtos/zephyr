/*
 * Copyright (c) 2022 Wuerth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the WSEN-PADS sensor driver.
 */

#ifndef _WSEN_PADS_H
#define _WSEN_PADS_H

/*         Includes         */

#include <stdint.h>

#include <WeSensorsSDK.h>


/*         PADS 2511020213301 DEVICE_ID         */

#define PADS_DEVICE_ID_VALUE        0xB3     /**< Device ID of PADS 2511020213301 Sensor */


/*         Available PADS I2C slave addresses         */

#define PADS_ADDRESS_I2C_0          0x5C     /**< when SAO of PADS is connected to ground */
#define PADS_ADDRESS_I2C_1          0x5D     /**< when SAO of PADS is connected to positive supply voltage */


/*         Register address definitions         */

#define PADS_INT_CFG_REG            0x0B     /**< Interrupt configuration register */
#define PADS_THR_P_L_REG            0x0C     /**< Pressure threshold LSB register */
#define PADS_THR_P_H_REG            0x0D     /**< Pressure threshold MSB register */
#define PADS_INTERFACE_CTRL_REG     0x0E     /**< Interface control register */
#define PADS_DEVICE_ID_REG          0x0F     /**< Device ID register */
#define PADS_CTRL_1_REG             0x10     /**< Control register 1 */
#define PADS_CTRL_2_REG             0x11     /**< Control register 2 */
#define PADS_CTRL_3_REG             0x12     /**< Control register 3 */
#define PADS_FIFO_CTRL_REG          0x13     /**< FIFO control register */
#define PADS_FIFO_WTM_REG           0x14     /**< FIFO threshold register */
#define PADS_REF_P_L_REG            0x15     /**< Reference pressure LSB value register */
#define PADS_REF_P_H_REG            0x16     /**< Reference pressure MSB value register */
#define PADS_OPC_P_L_REG            0x18     /**< Pressure offset LSB value register */
#define PADS_OPC_P_H_REG            0x19     /**< Pressure offset MSB value register */
#define PADS_INT_SOURCE_REG         0x24     /**< Interrupt source register */
#define PADS_FIFO_STATUS1_REG       0x25     /**< FIFO status register 1 */
#define PADS_FIFO_STATUS2_REG       0x26     /**< FIFO status register 2 */
#define PADS_STATUS_REG             0x27     /**< Status register */
#define PADS_DATA_P_XL_REG          0x28     /**< Pressure output LSB value register */
#define PADS_DATA_P_L_REG           0x29     /**< Pressure output MID value register */
#define PADS_DATA_P_H_REG           0x2A     /**< Pressure output MSB value register */
#define PADS_DATA_T_L_REG           0x2B     /**< Temperature output LSB value register */
#define PADS_DATA_T_H_REG           0x2C     /**< Temperature output MSB value register */
#define PADS_FIFO_DATA_P_XL_REG     0x78     /**< Pressure LSB data in FIFO buffer */
#define PADS_FIFO_DATA_P_L_REG      0x79     /**< Pressure MID data in FIFO buffer */
#define PADS_FIFO_DATA_P_H_REG      0x7A     /**< Pressure MSB data in FIFO buffer */
#define PADS_FIFO_DATA_T_L_REG      0x7B     /**< Temperature LSB data in FIFO buffer */
#define PADS_FIFO_DATA_T_H_REG      0x7C     /**< Temperature MSB data in FIFO buffer */


/*         Misc. defines         */

#define PADS_FIFO_BUFFER_SIZE     128


/*         Register type definitions         */

/**
 * @brief Interrupt_CFG
 *
 * Address 0x0B
 * Type  R/W
 * Default value: 0x00
 */
typedef struct {
	uint8_t highPresInt : 1;          /**< PHE: Enable/disable interrupt on pressure high event (0: disabled; 1: enabled) */
	uint8_t lowPresInt : 1;           /**< PLE: Enable/disable interrupt on pressure low event (0: disabled; 1: enabled) */
	uint8_t latchedInt : 1;           /**< LIR: Enable/disable latched interrupt (0: normal; 1: enabled) */
	uint8_t diffInt : 1;              /**< DIFF_EN: Enable/disable differential interrupt generation (0: disabled; 1: enabled) */
	uint8_t resetAutoZero : 1;        /**< RESET_AZ: Reset AUTOZERO function; also resets reference pressure register (0: normal mode; 1: reset AUTOZERO) */
	uint8_t autoZero : 1;             /**< AUTOZERO: Turn on AUTOZERO mode; stores reference pressure in REF_P; enables differential output pressure (0: normal mode; 1: turn on) */
	uint8_t resetAutoRefp : 1;        /**< RESET_ARP: Reset AUTOREFP mode; also resets reference pressure register (0: normal mode; 1: enabled) */
	uint8_t autoRefp : 1;             /**< AUTOREFP: Turn on AUTOREFP function; stores reference pressure in REF_P; enables differential output pressure (0: normal mode; 1: turn on) */
} PADS_interruptConfiguration_t;


/**
 * @brief Pressure threshold LSB register
 *
 * Address 0x0C
 * Type  R/W
 * Default value: 0x00
 */
typedef struct {
	uint8_t presThresholdLsb : 8;   /**< THR[7:0] This register contains the low part of threshold value for pressure interrupt */
} PADS_thresholdLSB_t;


/**
 * @brief Pressure threshold MSB register
 *
 * Address 0x0D
 * Type  R/W
 * Default value: 0x00
 */
typedef struct {
	uint8_t presThresholdMsb : 7;   /**< THR[14:8] This register contains the high part of threshold value for pressure interrupt */
	uint8_t notUsed01 : 1;          /**< This bit must be set to 0 for proper operation of the device */
} PADS_thresholdMSB_t;


/**
 * @brief Interface control register
 *
 * Address 0x0B
 * Type  R/W
 * Default value: 0x00
 */
typedef struct {
	uint8_t disableI2C : 1;             /**< I2C_DISABLE: Enable/disable I2C digital Interface (0: I2C enabled; 1: I2C disabled) */
	uint8_t notUsed01 : 1;              /**< This bit must be set to 0 for proper operation of the device */
	uint8_t disPullDownOnIntPin : 1;    /**< PD_DIS_INT: Enable/disable pull down on the INT pin (0: PD connected; 1: PD disconnected) */
	uint8_t pullUpOnSAOPin : 1;         /**< SAO_PU_EN: Enable/disable pull-up on the SAO pin (0: disabled; 1: enabled) */
	uint8_t pullUpOnSDAPin : 1;         /**< SDA_PU_EN: Enable/disable pull-up on the SDA pin (0: disabled; 1: enabled) */
	uint8_t notUsed02 : 2;              /**< This bit must be set to 0 for proper operation of the device */
	uint8_t notUsed03 : 1;              /**< This bit must be set to 0 for proper operation of the device */
} PADS_interfaceCtrl_t;


/**
 * @brief Control register 1
 *
 * Address 0x0F
 * Type  R/W
 * Default value: 0x00
 *
 *    ODR2  | ODR1  | ODR0   | Pressure/Temperature output data rate (Hz)
 * ---------------- ----------------------------------------------------
 *     0    |  0    |  0     |           Single conversion
 *     0    |  0    |  1     |                 1
 *     0    |  1    |  0     |                 10
 *     0    |  1    |  1     |                 25
 *     1    |  0    |  0     |                 50
 *     1    |  0    |  1     |                 75
 *     1    |  1    |  0     |                 100
 *     1    |  1    |  1     |                 200
 *
 *  -------------------------------------------------------------------
 *
 *  EN_LPFP  |   LPFP_CFG     |    LPF2 status         | Device Bandwidth | Samples to be discarded
 * --------------------------------------------------------------------------------------------------
 *      0    | x (don't care) |  Disabled/reset filter |      ODR/2       |          0
 *      1    |     0          |  Enabled               |      ODR/9       |          2
 *      1    |     1          |  Enabled               |      ODR/20      |          2
 *
 */
typedef struct {
	uint8_t notUsed01 : 1;            /**< This bit must be set to 0 for proper operation of the device */
	uint8_t blockDataUpdate : 1;      /**< BDU: Block data update. 0 - continuous update; 1 - output registers are not updated until both MSB and LSB have been read */
	uint8_t lowPassFilterConfig : 1;  /**< LPFP_CFG: Configure low pass filter for pressure data */
	uint8_t enLowPassFilter : 1;      /**< EN_LPFP: Enable low pass filter for pressure data */
	uint8_t outputDataRate : 3;       /**< ODR[2:0]: Output data rate. Default '000' */
	uint8_t notUsed02 : 1;            /**< This bit must be set to 0 for proper operation of the device */
} PADS_ctrl1_t;


/**
 * @brief Control register 2
 *
 * Address 0x11
 * Type  R/W
 * Default value: 0x10
 */
typedef struct {
	uint8_t oneShotBit : 1;         /**< ONE_SHOT: 0: Normal operation; 1: Start single conversion measurement */
	uint8_t lowNoiseMode : 1;       /**< LOW_NOISE_EN: Enables low noise mode (used only if ODR is lower than 100 Hz). Default value: 0 (0: low-power mode; 1: low-noise mode) */
	uint8_t softwareReset : 1;      /**< SWRESET: Software reset. 0: normal mode; 1: SW reset;  Self-clearing upon completion */
	uint8_t notUsed01 : 1;          /**< This bit must be set to 0 for proper operation of the device */
	uint8_t autoAddIncr : 1;        /**< IF_ADD_INC: Register address automatically incremented during a multiple byte access with I2C interface. Default value 1 (0: disable; 1: enable) */
	uint8_t openDrainOnINTPin : 1;  /**< PP_OD: Push-pull/open-drain selection on interrupt pad. Default value: 0 (0: push-pull; 1: open-drain) */
	uint8_t intActiveLevel : 1;     /**< INT_H_L: Interrupt active high, active low. Default value: 0 (0: active high; 1: active low) */
	uint8_t boot : 1;               /**< BOOT: Reboot memory content. 0: normal mode; 1: reboot memory content. Self-clearing upon completion */
} PADS_ctrl2_t;


/**
 * @brief Control register 3
 *
 * Address 0x12
 * Type  R/W
 * Default value: 0x00
 *
 *             Interrupt configurations
 *     INT_S1     |   INT_S0    |   INT pin configuration
 *   ------------------------------------------------------
 *        0       |      0      |   Data signal (in order of priority: DRDY or INT_F_WTM or INT_F_OVR or INT_F_FULL)
 *        0       |      1      |   Pressure high event
 *        1       |      0      |   Pressure low event
 *        1       |      1      |   Pressure low or high event
 *
 */
typedef struct {
	uint8_t intEventCtrl : 2;       /**< INT_S: Data signal on INT pad control bits: Default value: 00 */
	uint8_t dataReadyInt : 1;       /**< DRDY: Data-ready signal on INT pin. Default value: 0 (0: disable; 1: enable) */
	uint8_t fifoOverrunInt : 1;     /**< INT_F_OVR: Enable FIFO overrun interrupt. Default value: 0 (0: disable; 1: enable) */
	uint8_t fifoThresholdInt : 1;   /**< INT_F_WTM: Enable FIFO threshold (watermark) interrupt. Default value: 0 (0: disable; 1: enable) */
	uint8_t fifoFullInt : 1;        /**< INT_F_FULL: Enable FIFO full interrupt. Default value: 0 (0: disable; 1: enable) */
	uint8_t notUsed01 : 2;          /**< These 2 bits must be set to 0 for proper operation of the device */
} PADS_ctrl3_t;


/**
 * @brief Control FIFO control register
 *
 * Address 0x13
 * Type  R/W
 * Default value: 0x00
 *
 *            FIFO mode selection
 * TRIG_MODES    | F_MODE[1:0]    | Mode
 * -------------------------------------------------------
 *   x           | 00             | Bypass
 *   0           | 01             | FIFO mode
 *   0           | 1x             | Continuous
 *   1           | 01             | Bypass-to-FIFO
 *   1           | 10             | Bypass-to-Continuous
 *   1           | 11             | Continuous-to-FIFO
 */
typedef struct {
	uint8_t fifoMode : 3;           /**< [TRIG_MODES; FMODE[1:0]]: select FIFO mode */
	uint8_t stopOnThreshold : 1;    /**< STOP_ON_WTM: When set to true, the FIFO is considered to be full when the user-defined threshold is reached. Default value: 0 (0: disabled; 1: enabled) */
	uint8_t notUsed01 : 4;          /**< These 4 bits must be set to 0 for proper operation of the device */
} PADS_fifoCtrl_t;


/**
 * @brief FIFO threshold setting register
 *
 * Address 0x14
 * Type  R/W
 * Default value: 0x00
 */
typedef struct {
	uint8_t fifoThreshold : 7;    /**< WTM[6:0]: FIFO threshold level setting (value between 0 and 127). Default value: 0x00 */
	uint8_t notUsed01 : 1;        /**< This bit must be set to 0 for proper operation of the device */
} PADS_fifoThreshold_t;


/**
 * @brief Interrupt source register
 *
 * Address 0x24
 * Type  R
 * Default value: Output; 0x00
 */
typedef struct {
	uint8_t diffPresHighEvent : 1;  /**< PH: Differential pressure high (0: no interrupt; 1: high differential pressure event has occurred) */
	uint8_t diffPresLowEvent : 1;   /**< PL: Differential pressure low (0: no interrupt; 1: low differential pressure event has occurred) */
	uint8_t intStatus : 1;          /**< IA: Interrupt active (0: no interrupt; 1: one or more interrupt events have been generated) */
	uint8_t notUsed01 : 4;          /**< These 4 bits must be set to 0 for proper operation of the device */
	uint8_t bootOn : 1;             /**< BOOT_ON: Indication of boot phase (0: Boot phase has ended; 1: Boot phase is running) */
} PADS_intSource_t;


/**
 * @brief FIFO Status register 2
 *
 * Address 0x26
 * Type  R
 * Default value: Output; 0x00
 */
typedef struct {
	uint8_t notUsed01 : 5;    /**< These 5 bits must be set to 0 for proper operation of the device */
	uint8_t fifoFull : 1;     /**< FIFO_FULL_IA: FIFO full status (0: FIFO not full; 1: FIFO is full, no samples overwritten) */
	uint8_t fifoOverrun : 1;  /**< FIFO_OVR_IA: FIFO overrun status (0: FIFO is not overrun; 1: FIFO is full and at least one sample has been overwritten) */
	uint8_t fifoWtm : 1;      /**< FIFO_WTM_IA: Threshold (watermark) status (0: FIFO level below threshold; 1: FIFO  level equal or higher than threshold) */
} PADS_fifoStatus2_t;


/**
 * @brief Status register
 *
 * Address 0x27
 * Type  R
 * Default value: Output; 0x00
 */
typedef struct {
	uint8_t presDataAvailable : 1;  /**< P_DA: Pressure data available. (0: Pressure sample not yet available; 1: A new pressure sample is available) */
	uint8_t tempDataAvailable : 1;  /**< T_DA: Temperature data available. (0: Temperature sample not yet available; 1: A new temperature sample is available) */
	uint8_t notUsed01 : 2;          /**< These 2 bits must be set to 0 for proper operation of the device */
	uint8_t presDataOverrun : 1;    /**< P_OR: Pressure data overrun. (0: No overrun; 1: Pressure data overwritten) */
	uint8_t tempDataOverrun : 1;    /**< T_OR: Temperature data overrun. (0: No overrun; 1: Temperature data overwritten) */
	uint8_t notUsed02 : 2;          /**< These 2 bits must be set to 0 for proper operation of the device */
} PADS_status_t;


/*         Functional type definitions         */

typedef enum {
	PADS_disable = 0,
	PADS_enable = 1
} PADS_state_t;


typedef enum {
	PADS_outputDataRatePowerDown = 0,     /**< single conversion / power down */
	PADS_outputDataRate1Hz = 1,           /**< 1Hz */
	PADS_outputDataRate10Hz = 2,          /**< 10Hz */
	PADS_outputDataRate25Hz = 3,          /**< 25Hz */
	PADS_outputDataRate50Hz = 4,          /**< 50Hz */
	PADS_outputDataRate75Hz = 5,          /**< 75Hz */
	PADS_outputDataRate100Hz = 6,         /**< 100Hz */
	PADS_outputDataRate200Hz = 7          /**< 200Hz */
} PADS_outputDataRate_t;


typedef enum {
	PADS_lpFilterBW1 = 0,                 /**< Bandwidth = outputDataRate / 9 */
	PADS_lpFilterBW2 = 1                  /**< Bandwidth = outputDataRate / 20 */
} PADS_filterConf_t;


typedef enum {
	PADS_activeHigh = 0,
	PADS_activeLow = 1
} PADS_interruptActiveLevel_t;


typedef enum {
	PADS_pushPull = 0,
	PADS_openDrain = 1
} PADS_interruptPinConfig_t;


typedef enum {
	PADS_lowPower = 0,      /**< Low power mode */
	PADS_lowNoise = 1,      /**< Low noise mode */
} PADS_powerMode_t;


typedef enum {
	PADS_dataReady = 0,        /**< Data signal (in order of priority: DRDY or INT_F_WTM or INT_F_OVR or INT_F_FULL) */
	PADS_pressureHigh = 1,
	PADS_pressureLow = 2,
	PADS_pressureHighOrLow = 3
} PADS_interruptEventControl_t;


typedef enum {
	PADS_bypassMode = 0,
	PADS_fifoEnabled = 1,
	PADS_continuousMode = 2,
	PADS_bypassToFifo = 5,
	PADS_bypassToContinuous = 6,
	PADS_continuousToFifo = 7
} PADS_fifoMode_t;

#ifdef __cplusplus
extern "C" {
#endif

/*         Function definitions         */

int8_t PADS_getDefaultInterface(WE_sensorInterface_t *sensorInterface);

int8_t PADS_isInterfaceReady(WE_sensorInterface_t *sensorInterface);

int8_t PADS_getDeviceID(WE_sensorInterface_t *sensorInterface, uint8_t *deviceID);

int8_t PADS_enableAutoRefp(WE_sensorInterface_t *sensorInterface, PADS_state_t autoRefp);
int8_t PADS_isEnablingAutoRefp(WE_sensorInterface_t *sensorInterface, PADS_state_t *autoRefp);
int8_t PADS_resetAutoRefp(WE_sensorInterface_t *sensorInterface, PADS_state_t reset);

int8_t PADS_enableAutoZeroMode(WE_sensorInterface_t *sensorInterface, PADS_state_t autoZero);
int8_t PADS_isEnablingAutoZeroMode(WE_sensorInterface_t *sensorInterface, PADS_state_t *autoZero);
int8_t PADS_resetAutoZeroMode(WE_sensorInterface_t *sensorInterface, PADS_state_t reset);

int8_t PADS_enableDiffPressureInterrupt(WE_sensorInterface_t *sensorInterface, PADS_state_t diffEn);
int8_t PADS_isDiffPressureInterruptEnabled(WE_sensorInterface_t *sensorInterface,
					   PADS_state_t *diffIntState);

int8_t PADS_enableLatchedInterrupt(WE_sensorInterface_t *sensorInterface, PADS_state_t state);
int8_t PADS_isLatchedInterruptEnabled(WE_sensorInterface_t *sensorInterface,
				      PADS_state_t *latchInt);

int8_t PADS_enableLowPressureInterrupt(WE_sensorInterface_t *sensorInterface, PADS_state_t state);
int8_t PADS_isLowPressureInterruptEnabled(WE_sensorInterface_t *sensorInterface,
					  PADS_state_t *lpint);
int8_t PADS_enableHighPressureInterrupt(WE_sensorInterface_t *sensorInterface, PADS_state_t state);
int8_t PADS_isHighPressureInterruptEnabled(WE_sensorInterface_t *sensorInterface,
					   PADS_state_t *hpint);

int8_t PADS_getInterruptSource(WE_sensorInterface_t *sensorInterface, PADS_intSource_t *intSource);
int8_t PADS_getInterruptStatus(WE_sensorInterface_t *sensorInterface, PADS_state_t *intState);
int8_t PADS_getLowPressureInterruptStatus(WE_sensorInterface_t *sensorInterface,
					  PADS_state_t *lpState);
int8_t PADS_getHighPressureInterruptStatus(WE_sensorInterface_t *sensorInterface,
					   PADS_state_t *hpState);

int8_t PADS_enableFifoFullInterrupt(WE_sensorInterface_t *sensorInterface, PADS_state_t fullState);
int8_t PADS_enableFifoThresholdInterrupt(WE_sensorInterface_t *sensorInterface,
					 PADS_state_t threshState);
int8_t PADS_enableFifoOverrunInterrupt(WE_sensorInterface_t *sensorInterface,
				       PADS_state_t ovrState);

int8_t PADS_isFifoFull(WE_sensorInterface_t *sensorInterface, PADS_state_t *fifoFull);
int8_t PADS_isFifoThresholdReached(WE_sensorInterface_t *sensorInterface, PADS_state_t *fifoWtm);
int8_t PADS_getFifoOverrunState(WE_sensorInterface_t *sensorInterface, PADS_state_t *fifoOvr);

int8_t PADS_enableDataReadyInterrupt(WE_sensorInterface_t *sensorInterface, PADS_state_t drdy);
int8_t PADS_isDataReadyInterruptEnabled(WE_sensorInterface_t *sensorInterface, PADS_state_t *drdy);

int8_t PADS_setInterruptEventControl(WE_sensorInterface_t *sensorInterface,
				     PADS_interruptEventControl_t ctr);
int8_t PADS_getInterruptEventControl(WE_sensorInterface_t *sensorInterface,
				     PADS_interruptEventControl_t *intEvent);

int8_t PADS_setPressureThreshold(WE_sensorInterface_t *sensorInterface, uint32_t thresholdPa);
int8_t PADS_getPressureThreshold(WE_sensorInterface_t *sensorInterface, uint32_t *thresholdPa);
int8_t PADS_setPressureThresholdLSB(WE_sensorInterface_t *sensorInterface, uint8_t thr);
int8_t PADS_setPressureThresholdMSB(WE_sensorInterface_t *sensorInterface, uint8_t thr);
int8_t PADS_getPressureThresholdLSB(WE_sensorInterface_t *sensorInterface, uint8_t *thrLSB);
int8_t PADS_getPressureThresholdMSB(WE_sensorInterface_t *sensorInterface, uint8_t *thrMSB);

int8_t PADS_disableI2CInterface(WE_sensorInterface_t *sensorInterface, PADS_state_t i2cDisable);
int8_t PADS_isI2CInterfaceDisabled(WE_sensorInterface_t *sensorInterface,
				   PADS_state_t *i2cDisabled);
int8_t PADS_disablePullDownIntPin(WE_sensorInterface_t *sensorInterface, PADS_state_t pullDown);
int8_t PADS_isPullDownIntDisabled(WE_sensorInterface_t *sensorInterface, PADS_state_t *pinState);
int8_t PADS_setSAOPullUp(WE_sensorInterface_t *sensorInterface, PADS_state_t saoStatus);
int8_t PADS_isSAOPullUp(WE_sensorInterface_t *sensorInterface, PADS_state_t *saoPinState);
int8_t PADS_setSDAPullUp(WE_sensorInterface_t *sensorInterface, PADS_state_t sdaStatus);
int8_t PADS_isSDAPullUp(WE_sensorInterface_t *sensorInterface, PADS_state_t *sdaPinState);

int8_t PADS_setOutputDataRate(WE_sensorInterface_t *sensorInterface, PADS_outputDataRate_t odr);
int8_t PADS_getOutputDataRate(WE_sensorInterface_t *sensorInterface, PADS_outputDataRate_t *odr);

int8_t PADS_enableLowPassFilter(WE_sensorInterface_t *sensorInterface, PADS_state_t filterEnabled);
int8_t PADS_isLowPassFilterEnabled(WE_sensorInterface_t *sensorInterface,
				   PADS_state_t *filterEnabled);
int8_t PADS_setLowPassFilterConfig(WE_sensorInterface_t *sensorInterface, PADS_filterConf_t conf);
int8_t PADS_getLowPassFilterConfig(WE_sensorInterface_t *sensorInterface, PADS_filterConf_t *conf);

int8_t PADS_enableBlockDataUpdate(WE_sensorInterface_t *sensorInterface, PADS_state_t bdu);
int8_t PADS_isBlockDataUpdateEnabled(WE_sensorInterface_t *sensorInterface, PADS_state_t *bdu);

int8_t PADS_reboot(WE_sensorInterface_t *sensorInterface, PADS_state_t reboot);
int8_t PADS_isRebooting(WE_sensorInterface_t *sensorInterface, PADS_state_t *reboot);
int8_t PADS_getBootStatus(WE_sensorInterface_t *sensorInterface, PADS_state_t *boot);

int8_t PADS_setInterruptActiveLevel(WE_sensorInterface_t *sensorInterface,
				    PADS_interruptActiveLevel_t level);
int8_t PADS_getInterruptActiveLevel(WE_sensorInterface_t *sensorInterface,
				    PADS_interruptActiveLevel_t *level);

int8_t PADS_setInterruptPinType(WE_sensorInterface_t *sensorInterface,
				PADS_interruptPinConfig_t pinType);
int8_t PADS_getInterruptPinType(WE_sensorInterface_t *sensorInterface,
				PADS_interruptPinConfig_t *pinType);

int8_t PADS_enableAutoIncrement(WE_sensorInterface_t *sensorInterface, PADS_state_t inc);
int8_t PADS_isAutoIncrementEnabled(WE_sensorInterface_t *sensorInterface, PADS_state_t *inc);

int8_t PADS_softReset(WE_sensorInterface_t *sensorInterface, PADS_state_t swReset);
int8_t PADS_getSoftResetState(WE_sensorInterface_t *sensorInterface, PADS_state_t *swReset);

int8_t PADS_setPowerMode(WE_sensorInterface_t *sensorInterface, PADS_powerMode_t mode);
int8_t PADS_getPowerMode(WE_sensorInterface_t *sensorInterface, PADS_powerMode_t *mode);

int8_t PADS_enableOneShot(WE_sensorInterface_t *sensorInterface, PADS_state_t oneShot);
int8_t PADS_isOneShotEnabled(WE_sensorInterface_t *sensorInterface, PADS_state_t *oneShot);

int8_t PADS_setPressureOffsetLSB(WE_sensorInterface_t *sensorInterface, uint8_t offset);
int8_t PADS_getPressureOffsetLSB(WE_sensorInterface_t *sensorInterface, uint8_t *offset);
int8_t PADS_setPressureOffsetMSB(WE_sensorInterface_t *sensorInterface, uint8_t offset);
int8_t PADS_getPressureOffsetMSB(WE_sensorInterface_t *sensorInterface, uint8_t *offset);

int8_t PADS_setFifoMode(WE_sensorInterface_t *sensorInterface, PADS_fifoMode_t fifoMode);
int8_t PADS_getFifoMode(WE_sensorInterface_t *sensorInterface, PADS_fifoMode_t *fifoMode);

int8_t PADS_enableStopOnThreshold(WE_sensorInterface_t *sensorInterface, PADS_state_t state);
int8_t PADS_isStopOnThresholdEnabled(WE_sensorInterface_t *sensorInterface, PADS_state_t *state);

int8_t PADS_setFifoThreshold(WE_sensorInterface_t *sensorInterface, uint8_t fifoThr);
int8_t PADS_getFifoThreshold(WE_sensorInterface_t *sensorInterface, uint8_t *fifoThr);

int8_t PADS_getFifoFillLevel(WE_sensorInterface_t *sensorInterface, uint8_t *fifoLevel);

int8_t PADS_getReferencePressure(WE_sensorInterface_t *sensorInterface,
				 uint32_t *referencePressurePa);
int8_t PADS_getRawReferencePressure(WE_sensorInterface_t *sensorInterface,
				    uint32_t *referencePressure);
int8_t PADS_getReferencePressureLSB(WE_sensorInterface_t *sensorInterface,
				    uint8_t *lowReferenceValue);
int8_t PADS_getReferencePressureMSB(WE_sensorInterface_t *sensorInterface,
				    uint8_t *highReferenceValue);

int8_t PADS_getTemperatureOverrunStatus(WE_sensorInterface_t *sensorInterface, PADS_state_t *state);
int8_t PADS_getPressureOverrunStatus(WE_sensorInterface_t *sensorInterface, PADS_state_t *state);

int8_t PADS_isPressureDataAvailable(WE_sensorInterface_t *sensorInterface, PADS_state_t *state);
int8_t PADS_isTemperatureDataAvailable(WE_sensorInterface_t *sensorInterface, PADS_state_t *state);

int8_t PADS_getRawPressure(WE_sensorInterface_t *sensorInterface, int32_t *rawPres);
int8_t PADS_getRawTemperature(WE_sensorInterface_t *sensorInterface, int16_t *rawTemp);

int8_t PADS_getFifoRawPressure(WE_sensorInterface_t *sensorInterface, uint8_t numSamples,
			       int32_t *rawPres);
int8_t PADS_getFifoRawTemperature(WE_sensorInterface_t *sensorInterface, uint8_t numSamples,
				  int16_t *rawTemp);
int8_t PADS_getFifoRawValues(WE_sensorInterface_t *sensorInterface, uint8_t numSamples,
			     int32_t *rawPres, int16_t *rawTemp);

int8_t PADS_getPressure_int(WE_sensorInterface_t *sensorInterface, int32_t *pressPa);
int8_t PADS_getDifferentialPressure_int(WE_sensorInterface_t *sensorInterface, int32_t *pressPa);
int8_t PADS_getTemperature_int(WE_sensorInterface_t *sensorInterface, int16_t *temperature);

int8_t PADS_getFifoPressure_int(WE_sensorInterface_t *sensorInterface, uint8_t numSamples,
				int32_t *pressPa);
int8_t PADS_getFifoTemperature_int(WE_sensorInterface_t *sensorInterface, uint8_t numSamples,
				   int16_t *temperature);
int8_t PADS_getFifoValues_int(WE_sensorInterface_t *sensorInterface, uint8_t numSamples,
			      int32_t *pressPa, int16_t *temperature);

int32_t PADS_convertPressure_int(int32_t rawPres);
int32_t PADS_convertDifferentialPressure_int(int32_t rawPres);

int8_t PADS_getPressure_float(WE_sensorInterface_t *sensorInterface, float *presskPa);
int8_t PADS_getDifferentialPressure_float(WE_sensorInterface_t *sensorInterface, float *presskPa);
int8_t PADS_getTemperature_float(WE_sensorInterface_t *sensorInterface, float *tempDegC);

int8_t PADS_getFifoPressure_float(WE_sensorInterface_t *sensorInterface, float *presskPa);
int8_t PADS_getFifoTemperature_float(WE_sensorInterface_t *sensorInterface, float *tempDegC);

float PADS_convertPressure_float(int32_t rawPres);
float PADS_convertDifferentialPressure_float(int32_t rawPres);

#ifdef __cplusplus
}
#endif

#endif /* _WSEN_PADS_H */
