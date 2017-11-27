/*******************************************************************************
Copyright © 2016, STMicroelectronics International N.V.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of STMicroelectronics nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
NON-INFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS ARE DISCLAIMED.
IN NO EVENT SHALL STMICROELECTRONICS INTERNATIONAL N.V. BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/**
 * Device specific defines. To be adapted by implementer for the targeted
 * device.
 */

#ifndef _VL53L0X_DEVICE_H_
#define _VL53L0X_DEVICE_H_

#include "vl53l0x_types.h"


/** @defgroup VL53L0X_DevSpecDefines_group VL53L0X cut1.1 Device Specific Defines
 *  @brief VL53L0X cut1.1 Device Specific Defines
 *  @{
 */


/** @defgroup VL53L0X_DeviceError_group Device Error
 *  @brief Device Error code
 *
 *  This enum is Device specific it should be updated in the implementation
 *  Use @a VL53L0X_GetStatusErrorString() to get the string.
 *  It is related to Status Register of the Device.
 *  @{
 */
typedef uint8_t VL53L0X_DeviceError;

#define VL53L0X_DEVICEERROR_NONE                        ((VL53L0X_DeviceError) 0)
	/*!< 0  NoError  */
#define VL53L0X_DEVICEERROR_VCSELCONTINUITYTESTFAILURE  ((VL53L0X_DeviceError) 1)
#define VL53L0X_DEVICEERROR_VCSELWATCHDOGTESTFAILURE    ((VL53L0X_DeviceError) 2)
#define VL53L0X_DEVICEERROR_NOVHVVALUEFOUND             ((VL53L0X_DeviceError) 3)
#define VL53L0X_DEVICEERROR_MSRCNOTARGET                ((VL53L0X_DeviceError) 4)
#define VL53L0X_DEVICEERROR_SNRCHECK                    ((VL53L0X_DeviceError) 5)
#define VL53L0X_DEVICEERROR_RANGEPHASECHECK             ((VL53L0X_DeviceError) 6)
#define VL53L0X_DEVICEERROR_SIGMATHRESHOLDCHECK         ((VL53L0X_DeviceError) 7)
#define VL53L0X_DEVICEERROR_TCC                         ((VL53L0X_DeviceError) 8)
#define VL53L0X_DEVICEERROR_PHASECONSISTENCY            ((VL53L0X_DeviceError) 9)
#define VL53L0X_DEVICEERROR_MINCLIP                     ((VL53L0X_DeviceError) 10)
#define VL53L0X_DEVICEERROR_RANGECOMPLETE               ((VL53L0X_DeviceError) 11)
#define VL53L0X_DEVICEERROR_ALGOUNDERFLOW               ((VL53L0X_DeviceError) 12)
#define VL53L0X_DEVICEERROR_ALGOOVERFLOW                ((VL53L0X_DeviceError) 13)
#define VL53L0X_DEVICEERROR_RANGEIGNORETHRESHOLD        ((VL53L0X_DeviceError) 14)

/** @} end of VL53L0X_DeviceError_group */


/** @defgroup VL53L0X_CheckEnable_group Check Enable list
 *  @brief Check Enable code
 *
 *  Define used to specify the LimitCheckId.
 *  Use @a VL53L0X_GetLimitCheckInfo() to get the string.
 *  @{
 */

#define VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE           0
#define VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE     1
#define VL53L0X_CHECKENABLE_SIGNAL_REF_CLIP             2
#define VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD      3
#define VL53L0X_CHECKENABLE_SIGNAL_RATE_MSRC            4
#define VL53L0X_CHECKENABLE_SIGNAL_RATE_PRE_RANGE       5

#define VL53L0X_CHECKENABLE_NUMBER_OF_CHECKS            6

/** @}  end of VL53L0X_CheckEnable_group */


/** @defgroup VL53L0X_GpioFunctionality_group Gpio Functionality
 *  @brief Defines the different functionalities for the device GPIO(s)
 *  @{
 */
typedef uint8_t VL53L0X_GpioFunctionality;

#define VL53L0X_GPIOFUNCTIONALITY_OFF                     \
	((VL53L0X_GpioFunctionality)  0) /*!< NO Interrupt  */
#define VL53L0X_GPIOFUNCTIONALITY_THRESHOLD_CROSSED_LOW   \
	((VL53L0X_GpioFunctionality)  1) /*!< Level Low (value < thresh_low)  */
#define VL53L0X_GPIOFUNCTIONALITY_THRESHOLD_CROSSED_HIGH   \
	((VL53L0X_GpioFunctionality)  2) /*!< Level High (value > thresh_high) */
#define VL53L0X_GPIOFUNCTIONALITY_THRESHOLD_CROSSED_OUT    \
	((VL53L0X_GpioFunctionality)  3)
	/*!< Out Of Window (value < thresh_low OR value > thresh_high)  */
#define VL53L0X_GPIOFUNCTIONALITY_NEW_MEASURE_READY        \
	((VL53L0X_GpioFunctionality)  4) /*!< New Sample Ready  */

/** @} end of VL53L0X_GpioFunctionality_group */


/* Device register map */

/** @defgroup VL53L0X_DefineRegisters_group Define Registers
 *  @brief List of all the defined registers
 *  @{
 */
#define VL53L0X_REG_SYSRANGE_START                        0x000
	/** mask existing bit in #VL53L0X_REG_SYSRANGE_START*/
	#define VL53L0X_REG_SYSRANGE_MODE_MASK          0x0F
	/** bit 0 in #VL53L0X_REG_SYSRANGE_START write 1 toggle state in
	 * continuous mode and arm next shot in single shot mode */
	#define VL53L0X_REG_SYSRANGE_MODE_START_STOP    0x01
	/** bit 1 write 0 in #VL53L0X_REG_SYSRANGE_START set single shot mode */
	#define VL53L0X_REG_SYSRANGE_MODE_SINGLESHOT    0x00
	/** bit 1 write 1 in #VL53L0X_REG_SYSRANGE_START set back-to-back
	 *  operation mode */
	#define VL53L0X_REG_SYSRANGE_MODE_BACKTOBACK    0x02
	/** bit 2 write 1 in #VL53L0X_REG_SYSRANGE_START set timed operation
	 *  mode */
	#define VL53L0X_REG_SYSRANGE_MODE_TIMED         0x04
	/** bit 3 write 1 in #VL53L0X_REG_SYSRANGE_START set histogram operation
	 *  mode */
	#define VL53L0X_REG_SYSRANGE_MODE_HISTOGRAM     0x08


#define VL53L0X_REG_SYSTEM_THRESH_HIGH               0x000C
#define VL53L0X_REG_SYSTEM_THRESH_LOW                0x000E


#define VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG		0x0001
#define VL53L0X_REG_SYSTEM_RANGE_CONFIG			0x0009
#define VL53L0X_REG_SYSTEM_INTERMEASUREMENT_PERIOD	0x0004


#define VL53L0X_REG_SYSTEM_INTERRUPT_CONFIG_GPIO               0x000A
	#define VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_DISABLED	0x00
	#define VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_LEVEL_LOW	0x01
	#define VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_LEVEL_HIGH	0x02
	#define VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_OUT_OF_WINDOW	0x03
	#define VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_NEW_SAMPLE_READY	0x04

#define VL53L0X_REG_GPIO_HV_MUX_ACTIVE_HIGH          0x0084


#define VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR           0x000B

/* Result registers */
#define VL53L0X_REG_RESULT_INTERRUPT_STATUS          0x0013
#define VL53L0X_REG_RESULT_RANGE_STATUS              0x0014

#define VL53L0X_REG_RESULT_CORE_PAGE  1
#define VL53L0X_REG_RESULT_CORE_AMBIENT_WINDOW_EVENTS_RTN   0x00BC
#define VL53L0X_REG_RESULT_CORE_RANGING_TOTAL_EVENTS_RTN    0x00C0
#define VL53L0X_REG_RESULT_CORE_AMBIENT_WINDOW_EVENTS_REF   0x00D0
#define VL53L0X_REG_RESULT_CORE_RANGING_TOTAL_EVENTS_REF    0x00D4
#define VL53L0X_REG_RESULT_PEAK_SIGNAL_RATE_REF             0x00B6

/* Algo register */

#define VL53L0X_REG_ALGO_PART_TO_PART_RANGE_OFFSET_MM       0x0028

#define VL53L0X_REG_I2C_SLAVE_DEVICE_ADDRESS                0x008a

/* Check Limit registers */
#define VL53L0X_REG_MSRC_CONFIG_CONTROL                     0x0060

#define VL53L0X_REG_PRE_RANGE_CONFIG_MIN_SNR                      0X0027
#define VL53L0X_REG_PRE_RANGE_CONFIG_VALID_PHASE_LOW              0x0056
#define VL53L0X_REG_PRE_RANGE_CONFIG_VALID_PHASE_HIGH             0x0057
#define VL53L0X_REG_PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT            0x0064

#define VL53L0X_REG_FINAL_RANGE_CONFIG_MIN_SNR                    0X0067
#define VL53L0X_REG_FINAL_RANGE_CONFIG_VALID_PHASE_LOW            0x0047
#define VL53L0X_REG_FINAL_RANGE_CONFIG_VALID_PHASE_HIGH           0x0048
#define VL53L0X_REG_FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT   0x0044


#define VL53L0X_REG_PRE_RANGE_CONFIG_SIGMA_THRESH_HI              0X0061
#define VL53L0X_REG_PRE_RANGE_CONFIG_SIGMA_THRESH_LO              0X0062

/* PRE RANGE registers */
#define VL53L0X_REG_PRE_RANGE_CONFIG_VCSEL_PERIOD                 0x0050
#define VL53L0X_REG_PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI            0x0051
#define VL53L0X_REG_PRE_RANGE_CONFIG_TIMEOUT_MACROP_LO            0x0052

#define VL53L0X_REG_SYSTEM_HISTOGRAM_BIN                          0x0081
#define VL53L0X_REG_HISTOGRAM_CONFIG_INITIAL_PHASE_SELECT         0x0033
#define VL53L0X_REG_HISTOGRAM_CONFIG_READOUT_CTRL                 0x0055

#define VL53L0X_REG_FINAL_RANGE_CONFIG_VCSEL_PERIOD               0x0070
#define VL53L0X_REG_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI          0x0071
#define VL53L0X_REG_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_LO          0x0072
#define VL53L0X_REG_CROSSTALK_COMPENSATION_PEAK_RATE_MCPS         0x0020

#define VL53L0X_REG_MSRC_CONFIG_TIMEOUT_MACROP                    0x0046


#define VL53L0X_REG_SOFT_RESET_GO2_SOFT_RESET_N	                 0x00bf
#define VL53L0X_REG_IDENTIFICATION_MODEL_ID                       0x00c0
#define VL53L0X_REG_IDENTIFICATION_REVISION_ID                    0x00c2

#define VL53L0X_REG_OSC_CALIBRATE_VAL                             0x00f8


#define VL53L0X_SIGMA_ESTIMATE_MAX_VALUE                          65535
/* equivalent to a range sigma of 655.35mm */

#define VL53L0X_REG_GLOBAL_CONFIG_VCSEL_WIDTH          0x032
#define VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0   0x0B0
#define VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_1   0x0B1
#define VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_2   0x0B2
#define VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_3   0x0B3
#define VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_4   0x0B4
#define VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_5   0x0B5

#define VL53L0X_REG_GLOBAL_CONFIG_REF_EN_START_SELECT   0xB6
#define VL53L0X_REG_DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD 0x4E /* 0x14E */
#define VL53L0X_REG_DYNAMIC_SPAD_REF_EN_START_OFFSET    0x4F /* 0x14F */
#define VL53L0X_REG_POWER_MANAGEMENT_GO1_POWER_FORCE    0x80

/*
 * Speed of light in um per 1E-10 Seconds
 */

#define VL53L0X_SPEED_OF_LIGHT_IN_AIR 2997

#define VL53L0X_REG_VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV     0x0089

#define VL53L0X_REG_ALGO_PHASECAL_LIM                         0x0030 /* 0x130 */
#define VL53L0X_REG_ALGO_PHASECAL_CONFIG_TIMEOUT              0x0030

/** @} VL53L0X_DefineRegisters_group */

/** @} VL53L0X_DevSpecDefines_group */


#endif

/* _VL53L0X_DEVICE_H_ */


