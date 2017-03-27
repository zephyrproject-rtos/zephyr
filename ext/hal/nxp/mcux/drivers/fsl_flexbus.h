/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FSL_FLEXBUS_H_
#define _FSL_FLEXBUS_H_

#include "fsl_common.h"

/*!
 * @addtogroup flexbus
 * @{
 */


/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
#define FSL_FLEXBUS_DRIVER_VERSION (MAKE_VERSION(2, 0, 1)) /*!< Version 2.0.1. */
/*@}*/

/*!
 * @brief Defines port size for FlexBus peripheral.
 */
typedef enum _flexbus_port_size
{
    kFLEXBUS_4Bytes = 0x00U, /*!< 32-bit port size */
    kFLEXBUS_1Byte = 0x01U,  /*!< 8-bit port size */
    kFLEXBUS_2Bytes = 0x02U  /*!< 16-bit port size */
} flexbus_port_size_t;

/*!
 * @brief Defines number of cycles to hold address and attributes for FlexBus peripheral.
 */
typedef enum _flexbus_write_address_hold
{
    kFLEXBUS_Hold1Cycle = 0x00U,  /*!< Hold address and attributes one cycles after FB_CSn negates on writes */
    kFLEXBUS_Hold2Cycles = 0x01U, /*!< Hold address and attributes two cycles after FB_CSn negates on writes */
    kFLEXBUS_Hold3Cycles = 0x02U, /*!< Hold address and attributes three cycles after FB_CSn negates on writes */
    kFLEXBUS_Hold4Cycles = 0x03U  /*!< Hold address and attributes four cycles after FB_CSn negates on writes */
} flexbus_write_address_hold_t;

/*!
 * @brief Defines number of cycles to hold address and attributes for FlexBus peripheral.
 */
typedef enum _flexbus_read_address_hold
{
    kFLEXBUS_Hold1Or0Cycles = 0x00U, /*!< Hold address and attributes 1 or 0 cycles on reads */
    kFLEXBUS_Hold2Or1Cycles = 0x01U, /*!< Hold address and attributes 2 or 1 cycles on reads */
    kFLEXBUS_Hold3Or2Cycle = 0x02U,  /*!< Hold address and attributes 3 or 2 cycles on reads */
    kFLEXBUS_Hold4Or3Cycle = 0x03U   /*!< Hold address and attributes 4 or 3 cycles on reads */
} flexbus_read_address_hold_t;

/*!
 * @brief Address setup for FlexBus peripheral.
 */
typedef enum _flexbus_address_setup
{
    kFLEXBUS_FirstRisingEdge = 0x00U,  /*!< Assert FB_CSn on first rising clock edge after address is asserted */
    kFLEXBUS_SecondRisingEdge = 0x01U, /*!< Assert FB_CSn on second rising clock edge after address is asserted */
    kFLEXBUS_ThirdRisingEdge = 0x02U,  /*!< Assert FB_CSn on third rising clock edge after address is asserted */
    kFLEXBUS_FourthRisingEdge = 0x03U, /*!< Assert FB_CSn on fourth rising clock edge after address is asserted */
} flexbus_address_setup_t;

/*!
 * @brief Defines byte-lane shift for FlexBus peripheral.
 */
typedef enum _flexbus_bytelane_shift
{
    kFLEXBUS_NotShifted = 0x00U, /*!< Not shifted. Data is left-justified on FB_AD */
    kFLEXBUS_Shifted = 0x01U,    /*!< Shifted. Data is right justified on FB_AD */
} flexbus_bytelane_shift_t;

/*!
 * @brief Defines multiplex group1 valid signals.
 */
typedef enum _flexbus_multiplex_group1_signal
{
    kFLEXBUS_MultiplexGroup1_FB_ALE = 0x00U, /*!< FB_ALE */
    kFLEXBUS_MultiplexGroup1_FB_CS1 = 0x01U, /*!< FB_CS1 */
    kFLEXBUS_MultiplexGroup1_FB_TS = 0x02U,  /*!< FB_TS */
} flexbus_multiplex_group1_t;

/*!
 * @brief Defines multiplex group2 valid signals.
 */
typedef enum _flexbus_multiplex_group2_signal
{
    kFLEXBUS_MultiplexGroup2_FB_CS4 = 0x00U,      /*!< FB_CS4 */
    kFLEXBUS_MultiplexGroup2_FB_TSIZ0 = 0x01U,    /*!< FB_TSIZ0 */
    kFLEXBUS_MultiplexGroup2_FB_BE_31_24 = 0x02U, /*!< FB_BE_31_24 */
} flexbus_multiplex_group2_t;

/*!
 * @brief Defines multiplex group3 valid signals.
 */
typedef enum _flexbus_multiplex_group3_signal
{
    kFLEXBUS_MultiplexGroup3_FB_CS5 = 0x00U,      /*!< FB_CS5 */
    kFLEXBUS_MultiplexGroup3_FB_TSIZ1 = 0x01U,    /*!< FB_TSIZ1 */
    kFLEXBUS_MultiplexGroup3_FB_BE_23_16 = 0x02U, /*!< FB_BE_23_16 */
} flexbus_multiplex_group3_t;

/*!
 * @brief Defines multiplex group4 valid signals.
 */
typedef enum _flexbus_multiplex_group4_signal
{
    kFLEXBUS_MultiplexGroup4_FB_TBST = 0x00U,    /*!< FB_TBST */
    kFLEXBUS_MultiplexGroup4_FB_CS2 = 0x01U,     /*!< FB_CS2 */
    kFLEXBUS_MultiplexGroup4_FB_BE_15_8 = 0x02U, /*!< FB_BE_15_8 */
} flexbus_multiplex_group4_t;

/*!
 * @brief Defines multiplex group5 valid signals.
 */
typedef enum _flexbus_multiplex_group5_signal
{
    kFLEXBUS_MultiplexGroup5_FB_TA = 0x00U,     /*!< FB_TA */
    kFLEXBUS_MultiplexGroup5_FB_CS3 = 0x01U,    /*!< FB_CS3 */
    kFLEXBUS_MultiplexGroup5_FB_BE_7_0 = 0x02U, /*!< FB_BE_7_0 */
} flexbus_multiplex_group5_t;

/*!
 * @brief Configuration structure that the user needs to set.
 */
typedef struct _flexbus_config
{
    uint8_t chip;                                      /*!< Chip FlexBus for validation */
    uint8_t waitStates;                                /*!< Value of wait states */
    uint32_t chipBaseAddress;                          /*!< Chip base address for using FlexBus */
    uint32_t chipBaseAddressMask;                      /*!< Chip base address mask */
    bool writeProtect;                                 /*!< Write protected */
    bool burstWrite;                                   /*!< Burst-Write enable */
    bool burstRead;                                    /*!< Burst-Read enable */
    bool byteEnableMode;                               /*!< Byte-enable mode support */
    bool autoAcknowledge;                              /*!< Auto acknowledge setting */
    bool extendTransferAddress;                        /*!< Extend transfer start/extend address latch enable */
    bool secondaryWaitStates;                          /*!< Secondary wait states number */
    flexbus_port_size_t portSize;                      /*!< Port size of transfer */
    flexbus_bytelane_shift_t byteLaneShift;            /*!< Byte-lane shift enable */
    flexbus_write_address_hold_t writeAddressHold;     /*!< Write address hold or deselect option */
    flexbus_read_address_hold_t readAddressHold;       /*!< Read address hold or deselect option */
    flexbus_address_setup_t addressSetup;              /*!< Address setup setting */
    flexbus_multiplex_group1_t group1MultiplexControl; /*!< FlexBus Signal Group 1 Multiplex control */
    flexbus_multiplex_group2_t group2MultiplexControl; /*!< FlexBus Signal Group 2 Multiplex control */
    flexbus_multiplex_group3_t group3MultiplexControl; /*!< FlexBus Signal Group 3 Multiplex control */
    flexbus_multiplex_group4_t group4MultiplexControl; /*!< FlexBus Signal Group 4 Multiplex control */
    flexbus_multiplex_group5_t group5MultiplexControl; /*!< FlexBus Signal Group 5 Multiplex control */
} flexbus_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*!
 * @name FlexBus functional operation
 * @{
 */

/*!
 * @brief Initializes and configures the FlexBus module.
 *
 * This function enables the clock gate for FlexBus module.
 * Only chip 0 is validated and set to known values. Other chips are disabled.
 * Note that in this function, certain parameters, depending on external memories,  must
 * be set before using the FLEXBUS_Init() function.
 * This example shows how to set up the uart_state_t and the
 * flexbus_config_t parameters and how to call the FLEXBUS_Init function by passing
 * in these parameters.
   @code
    flexbus_config_t flexbusConfig;
    FLEXBUS_GetDefaultConfig(&flexbusConfig);
    flexbusConfig.waitStates            = 2U;
    flexbusConfig.chipBaseAddress       = 0x60000000U;
    flexbusConfig.chipBaseAddressMask   = 7U;
    FLEXBUS_Init(FB, &flexbusConfig);
   @endcode
 *
 * @param base FlexBus peripheral address.
 * @param config Pointer to the configuration structure
*/
void FLEXBUS_Init(FB_Type *base, const flexbus_config_t *config);

/*!
 * @brief De-initializes a FlexBus instance.
 *
 * This function disables the clock gate of the FlexBus module clock.
 *
 * @param base FlexBus peripheral address.
 */
void FLEXBUS_Deinit(FB_Type *base);

/*!
 * @brief Initializes the FlexBus configuration structure.
 *
 * This function initializes the FlexBus configuration structure to default value. The default
 * values are.
   @code
   fbConfig->chip                   = 0;
   fbConfig->writeProtect           = 0;
   fbConfig->burstWrite             = 0;
   fbConfig->burstRead              = 0;
   fbConfig->byteEnableMode         = 0;
   fbConfig->autoAcknowledge        = true;
   fbConfig->extendTransferAddress  = 0;
   fbConfig->secondaryWaitStates    = 0;
   fbConfig->byteLaneShift          = kFLEXBUS_NotShifted;
   fbConfig->writeAddressHold       = kFLEXBUS_Hold1Cycle;
   fbConfig->readAddressHold        = kFLEXBUS_Hold1Or0Cycles;
   fbConfig->addressSetup           = kFLEXBUS_FirstRisingEdge;
   fbConfig->portSize               = kFLEXBUS_1Byte;
   fbConfig->group1MultiplexControl = kFLEXBUS_MultiplexGroup1_FB_ALE;
   fbConfig->group2MultiplexControl = kFLEXBUS_MultiplexGroup2_FB_CS4 ;
   fbConfig->group3MultiplexControl = kFLEXBUS_MultiplexGroup3_FB_CS5;
   fbConfig->group4MultiplexControl = kFLEXBUS_MultiplexGroup4_FB_TBST;
   fbConfig->group5MultiplexControl = kFLEXBUS_MultiplexGroup5_FB_TA;
   @endcode
 * @param config Pointer to the initialization structure.
 * @see FLEXBUS_Init
 */
void FLEXBUS_GetDefaultConfig(flexbus_config_t *config);

/*! @}*/

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/*! @}*/

#endif /* _FSL_FLEXBUS_H_ */
