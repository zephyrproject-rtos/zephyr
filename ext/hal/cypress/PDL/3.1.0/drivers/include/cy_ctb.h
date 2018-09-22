/***************************************************************************//**
* \file cy_ctb.h
* \version 1.10
*
* Header file for the CTB driver
*
********************************************************************************
* \copyright
* Copyright 2017-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \defgroup group_ctb Continuous Time Block (CTB)
* \{
* This driver provides API functions to configure and use the analog CTB.
* The CTB comprises two identical opamps, a switch routing matrix,
* and a sample and hold (SH) circuit. The high level features are:
*
*   - Two highly configurable opamps
*       - Each opamp has programmable power and output drive strength
*       - Each opamp can be configured as a voltage follower using internal routing
*       - Each opamp can be configured as a comparator with optional 10 mV hysteresis
*   - Flexible input and output routing
*   - Works as a buffer or amplifier for SAR ADC inputs
*   - Works as a buffer, amplifier, or sample and hold (SH) for the CTDAC output
*   - Can operate in Deep Sleep power mode
*
* Each opamp, marked OA0 and OA1, has one input and three output stages,
* all of which share the common input stage.
* Note that only one output stage can be selected at a time.
* The output stage can operate as a low-drive strength opamp for internal connections (1X), a high-drive strength
* opamp for driving a device pin (10X), or a comparator.
*
* Using the switching matrix, the opamp inputs and outputs
* can be connected to dedicated general-purpose I/Os or other internal analog
* blocks. See the device datasheet for the dedicated CTB port.
*
* \image html ctb_block_diagram.png "CTB Switch Diagram" width=1000px
* \image latex ctb_block_diagram.png
*
* \section group_ctb_init Initialization and Enable
*
* Before enabling the CTB, set up any external components (such as resistors)
* that are needed for the design. To configure the entire hardware block, call \ref Cy_CTB_Init.
* The base address of the CTB hardware can be found in the device specific header file.
* Alternatively, to configure only one opamp without any routing, call \ref Cy_CTB_OpampInit.
* The driver also provides a \ref Cy_CTB_FastInit function for fast and easy initialization of the CTB
* based on commonly used configurations. They are pre-defined in the driver as:
*
* <b> Opamp0 </b>
* - \ref Cy_CTB_Fast_Opamp0_Unused
* - \ref Cy_CTB_Fast_Opamp0_Comp
* - \ref Cy_CTB_Fast_Opamp0_Opamp1x
* - \ref Cy_CTB_Fast_Opamp0_Opamp10x
* - \ref Cy_CTB_Fast_Opamp0_Diffamp
* - \ref Cy_CTB_Fast_Opamp0_Vdac_Out
* - \ref Cy_CTB_Fast_Opamp0_Vdac_Out_SH
*
* <b> Opamp1 </b>
* - \ref Cy_CTB_Fast_Opamp1_Unused
* - \ref Cy_CTB_Fast_Opamp1_Comp
* - \ref Cy_CTB_Fast_Opamp1_Opamp1x
* - \ref Cy_CTB_Fast_Opamp1_Opamp10x
* - \ref Cy_CTB_Fast_Opamp1_Diffamp
* - \ref Cy_CTB_Fast_Opamp1_Vdac_Ref_Aref
* - \ref Cy_CTB_Fast_Opamp1_Vdac_Ref_Pin5
*
* After initialization, call \ref Cy_CTB_Enable to enable the hardware.
*
* \section group_ctb_io_connections Input/Output Connections
*
* The CTB has internal switches to support flexible input and output routing. If these switches
* have not been configured during initialization, call \ref Cy_CTB_SetAnalogSwitch to
* make the input and output connections.
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_SET_ANALOG_SWITCH
*
* As shown in the CTB switch diagram, the 10x output of OA0 and OA1 have dedicated
* connections to Pin 2 and Pin 3, respectively, of the CTB port. If different output
* connections are required, the other CTB switches and/or AMUXBUX A/B switches can be used.
*
* \section group_ctb_comparator Comparator Mode
*
* Each opamp can be configured as a comparator. Note that when used as a
* comparator, the hardware shuts down the 1X and 10X output drivers.
* Specific to the comparator mode, there is an optional 10 mV input hysteresis
* and configurable edge detection interrupt handling.
*
* - Negative input terminal: This input is usually connected to the reference voltage.
* - Positive input terminal: This input is usually connected to the voltage that is being compared.
* - Comparator digital output: This output goes high when the positive input voltage
*   is greater than the negative input voltage.
*
* The comparator output can be routed to a pin or other components using HSIOM or trigger muxes.
*
* \snippet ctb_sut_01.cydsn/main_cm0p.c SNIPPET_COMP_OUT_ROUTING
*
* \subsection group_ctb_comparator_handling_interrupts Handling interrupts
*
* The comparator output is connected to an edge detector
* block, which is used to detect the edge (rising, falling, both, or disabled)
* for interrupt generation.
*
* The following code snippet demonstrates how to implement a routine to handle the interrupt.
* The routine gets called when any comparator on the device generates an interrupt.
*
* \snippet ctb_sut_01.cydsn/main_cm0p.c SNIPPET_COMP_ISR
*
* The following code snippet demonstrates how to configure and enable the interrupt.
*
* \snippet ctb_sut_01.cydsn/main_cm0p.c SNIPPET_COMP_INTR_SETUP
*
* \section group_ctb_opamp_range Opamp Input and Output Range
*
* The input range of the opamp can be rail-to-rail if the charge pump is enabled.
* Without the charge pump, the input range is 0 V to VDDA - 1.5 V. The output range
* of the opamp is typically 0.2 V to VDDA - 0.2 V and will depend on the load. See the
* device datasheet for more detail.
*
* <table class="doxtable">
*   <tr>
*     <th>Charge Pump</th>
*     <th>Input Range</th></tr>
*     <th>Output Range</th></tr>
*   <tr>
*     <td>Enabled</td>
*     <td>0 V to VDDA</td>
*     <td>0.2 V to VDDA - 0.2 V</td>
*   </tr>
*   <tr>
*     <td>Disabled</td>
*     <td>0 V to VDDA - 1.5 V</td>
*     <td>0.2 V to VDDA - 0.2 V</td>
*   </tr>
* </table>
*
* \section group_ctb_sample_hold Sample and Hold Mode
*
* The CTB has a sample and hold (SH) circuit at the non-inverting input of Opamp0.
* The circuit includes a hold capacitor, Chold, with a firmware controlled switch, CHD.
* Sampling and holding the source voltage is performed
* by closing and opening appropriate switches in the CTB using firmware.
* If the SH circuit is used for the CTDAC, the \ref Cy_CTB_DACSampleAndHold function
* should be called.
*
* \image html ctb_fast_config_vdac_sh.png
* \image latex ctb_fast_config_vdac_sh.png
*
* \section group_ctb_dependencies Configuration Dependencies
*
* The CTB relies on other blocks to function properly. The dependencies
* are documented here.
*
* \subsection group_ctb_dependencies_charge_pump Charge Pump Configuration
*
* Each opamp of the CTB has a charge pump that when enabled increases the
* input range to the supply rails. When disabled, the opamp input range is 0 - VDDA - 1.5 V.
* When enabled, the pump requires a clock.
* Call the \ref Cy_CTB_SetClkPumpSource function in the \ref group_sysanalog driver to
* set the clock source for all CTBs. This clock can come from one of two sources:
*
*   -# A dedicated clock divider from one of the CLK_PATH in the SRSS
*
*      Call the following functions to configure the pump clock from the SRSS:
*       - \ref Cy_SysClk_ClkPumpSetSource
*       - \ref Cy_SysClk_ClkPumpSetDivider
*       - \ref Cy_SysClk_ClkPumpEnable
*
*      \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_SET_CLK_PUMP_SOURCE_SRSS
*
*   -# One of the Peri Clock dividers
*
*      Call the following functions to configure a Peri Clock divider as the
*      pump clock:
*       - \ref Cy_SysClk_PeriphAssignDivider with the IP block set to PCLK_PASS_CLOCK_PUMP_PERI
*       - \ref Cy_SysClk_PeriphSetDivider
*       - \ref Cy_SysClk_PeriphEnableDivider
*
*      \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_SET_CLK_PUMP_SOURCE_PERI
*
* When the charge pump is enabled, the clock frequency should be set as follows:
*
* <table class="doxtable">
*   <tr><th>Opamp Power Level</th><th>Pump Clock Freq</th></tr>
*   <tr>
*     <td>Low or Medium</td>
*     <td>8 - 24 MHz</td>
*   </tr>
*   <tr>
*     <td>High</td>
*     <td>24 MHz</td>
*   </tr>
* </table>
*
* The High power level of the opamp requires a 24 MHz pump clock.
* In Deep Sleep mode, all high frequency clocks are
* disabled and the charge pump will be disabled.
*
* \note
* The same pump clock is used by all opamps on the device. Be aware of this
* when configuring different opamps to different power levels.
*
* \subsection group_ctb_dependencies_reference_current Reference Current Configurations
*
* The CTB uses two reference current generators, IPTAT and IZTAT, from
* the AREF block (see \ref group_sysanalog driver). The IPTAT current is
* used to trim the slope of the opamp offset across temperature.
* The AREF must be initialized and enabled for the CTB to function properly.
*
* If the CTB is configured to operate in Deep Sleep mode,
* the appropriate reference current generators from the AREF block must be enabled in Deep Sleep.
* When waking up from Deep Sleep,
* the AREF block has a wakeup time that must be
* considered. Note that configurations in the AREF block
* are chip wide and affect all CTBs on the device.
*
* The following reference current configurations are supported:
*
* <table class="doxtable">
*   <tr><th>Reference Current Level</th><th>Supported Mode</th><th>Input Range</th></tr>
*   <tr>
*     <td>1 uA</td>
*     <td>Active/Low Power</td>
*     <td>Rail-to-Rail (charge pump enabled)</td>
*   </tr>
*   <tr>
*     <td>1 uA</td>
*     <td>Active/Low Power/Deep Sleep</td>
*     <td>0 - VDDA-1.5 V (charge pump disabled)</td>
*   </tr>
*   <tr>
*     <td>100 nA</td>
*     <td>Active/Low Power/Deep Sleep</td>
*     <td>0 - VDDA-1.5 V (charge pump disabled)</td>
*   </tr>
* </table>
*
* The first configuration provides low offset and drift with maximum input range
* while consuming the most current.
* For Deep Sleep operation, use the other two configurations with the charge pump disabled.
* For ultra low power, use the 100 nA current level.
* To configure the opamps to operate in one of these options, call \ref Cy_CTB_SetCurrentMode.
*
* \subsection group_ctb_dependencies_sample_hold Sample and Hold Switch Control
*
* If you are using rev-08 of the CY8CKIT-062, the following eight switches
* in the CTB are enabled by the CTDAC IP block:
*
*   - COS, CA0, CHD, CH6, COB, COR, CRS, and CRD
*
* On the rev-08 board, if any of the above switches are used, you must call \ref Cy_CTDAC_Enable
* to enable these switches.
*
* Additionally, on the rev-08 board, if any of the switches are used in Deep Sleep mode,
* the CTDAC must also be configured to operate in Deep Sleep (see \ref Cy_CTDAC_SetDeepSleepMode).
*
* In later revisions of the board, the switches are enabled by the CTB block so
* calls to the CTDAC IP block are not necessary.
*
* \section group_ctb_more_information More Information
*
* Refer to technical reference manual (TRM) and the device datasheet.
*
* \section group_ctb_MISRA MISRA-C Compliance]
*
* This driver does not have any specific deviations.
*
* \section group_ctb_changelog Changelog
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td>1.10</td>
*     <td>Flattened the driver source code organization into the single
*         source directory and the single include directory
*     </td>
*     <td>Simplified the driver library directory structure
*     </td>
*   </tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*
* \defgroup group_ctb_macros Macros
* \defgroup group_ctb_functions Functions
*   \{
*       \defgroup group_ctb_functions_init          Initialization Functions
*       \defgroup group_ctb_functions_basic         Basic Configuration Functions
*       \defgroup group_ctb_functions_comparator    Comparator Functions
*       \defgroup group_ctb_functions_sample_hold   Sample and Hold Functions
*       \defgroup group_ctb_functions_interrupts    Interrupt Functions
*       \defgroup group_ctb_functions_switches      Switch Control Functions
*       \defgroup group_ctb_functions_trim          Offset and Slope Trim Functions
*       \defgroup group_ctb_functions_aref          Reference Current Mode Functions
*   \}
* \defgroup group_ctb_globals Global Variables
* \defgroup group_ctb_data_structures Data Structures
* \defgroup group_ctb_enums Enumerated Types
*/

#if !defined(CY_CTB_H)
#define CY_CTB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "cy_device_headers.h"
#include "cy_device.h"
#include "cy_syslib.h"
#include "cy_sysanalog.h"

#ifdef CY_IP_MXS40PASS_CTB

#if defined(__cplusplus)
extern "C" {
#endif

/** \addtogroup group_ctb_macros
* \{
*/

/** Driver major version */
#define CY_CTB_DRV_VERSION_MAJOR            1

/** Driver minor version */
#define CY_CTB_DRV_VERSION_MINOR            10

/** CTB driver identifier*/
#define CY_CTB_ID                           CY_PDL_DRV_ID(0x0Bu)

/** \cond INTERNAL */

/**< De-init value for most CTB registers */
#define CY_CTB_DEINIT                       (0uL)

/**< De-init value for the opamp0 switch control register */
#define CY_CTB_DEINIT_OA0_SW                (CTBM_OA0_SW_CLEAR_OA0P_A00_Msk \
                                            | CTBM_OA0_SW_CLEAR_OA0P_A20_Msk \
                                            | CTBM_OA0_SW_CLEAR_OA0P_A30_Msk \
                                            | CTBM_OA0_SW_CLEAR_OA0M_A11_Msk \
                                            | CTBM_OA0_SW_CLEAR_OA0M_A81_Msk \
                                            | CTBM_OA0_SW_CLEAR_OA0O_D51_Msk \
                                            | CTBM_OA0_SW_CLEAR_OA0O_D81_Msk)

/**< De-init value for the opamp1 switch control register */
#define CY_CTB_DEINIT_OA1_SW                (CTBM_OA1_SW_CLEAR_OA1P_A03_Msk \
                                            | CTBM_OA1_SW_CLEAR_OA1P_A13_Msk \
                                            | CTBM_OA1_SW_CLEAR_OA1P_A43_Msk \
                                            | CTBM_OA1_SW_CLEAR_OA1P_A73_Msk \
                                            | CTBM_OA1_SW_CLEAR_OA1M_A22_Msk \
                                            | CTBM_OA1_SW_CLEAR_OA1M_A82_Msk \
                                            | CTBM_OA1_SW_CLEAR_OA1O_D52_Msk \
                                            | CTBM_OA1_SW_CLEAR_OA1O_D62_Msk \
                                            | CTBM_OA1_SW_CLEAR_OA1O_D82_Msk)

/**< De-init value for the CTDAC switch control register */
#define CY_CTB_DEINIT_CTD_SW                (CTBM_CTD_SW_CLEAR_CTDD_CRD_Msk \
                                            | CTBM_CTD_SW_CLEAR_CTDS_CRS_Msk \
                                            | CTBM_CTD_SW_CLEAR_CTDS_COR_Msk \
                                            | CTBM_CTD_SW_CLEAR_CTDO_C6H_Msk \
                                            | CTBM_CTD_SW_CLEAR_CTDO_COS_Msk \
                                            | CTBM_CTD_SW_CLEAR_CTDH_COB_Msk \
                                            | CTBM_CTD_SW_CLEAR_CTDH_CHD_Msk \
                                            | CTBM_CTD_SW_CLEAR_CTDH_CA0_Msk \
                                            | CTBM_CTD_SW_CLEAR_CTDH_CIS_Msk \
                                            | CTBM_CTD_SW_CLEAR_CTDH_ILR_Msk)

#define CY_CTB_TRIM_VALUE_MAX               (63uL)

/**< Macros for conditions used by CY_ASSERT calls */

#define CY_CTB_OPAMPNUM(num)                (((num) == CY_CTB_OPAMP_0) || ((num) == CY_CTB_OPAMP_1) || ((num) == CY_CTB_OPAMP_BOTH))
#define CY_CTB_OPAMPNUM_0_1(num)            (((num) == CY_CTB_OPAMP_0) || ((num) == CY_CTB_OPAMP_1))
#define CY_CTB_OPAMPNUM_ALL(num)            (((num) == CY_CTB_OPAMP_NONE) \
                                            || ((num) == CY_CTB_OPAMP_0) \
                                            || ((num) == CY_CTB_OPAMP_1) \
                                            || ((num) == CY_CTB_OPAMP_BOTH))
#define CY_CTB_IPTAT(iptat)                 (((iptat) == CY_CTB_IPTAT_NORMAL) || ((iptat) == CY_CTB_IPTAT_LOW))
#define CY_CTB_CLKPUMP(clkPump)             (((clkPump) == CY_CTB_CLK_PUMP_SRSS) || ((clkPump) == CY_CTB_CLK_PUMP_PERI))
#define CY_CTB_DEEPSLEEP(deepSleep)         (((deepSleep) == CY_CTB_DEEPSLEEP_DISABLE) || ((deepSleep) == CY_CTB_DEEPSLEEP_ENABLE))
#define CY_CTB_OAPOWER(power)               ((power) <= CY_CTB_POWER_HIGH)
#define CY_CTB_OAMODE(mode)                 (((mode) == CY_CTB_MODE_OPAMP1X) \
                                            || ((mode) == CY_CTB_MODE_OPAMP10X) \
                                            || ((mode) == CY_CTB_MODE_COMP))
#define CY_CTB_OAPUMP(pump)                 (((pump) == CY_CTB_PUMP_DISABLE) || ((pump) == CY_CTB_PUMP_ENABLE))
#define CY_CTB_COMPEDGE(edge)               (((edge) == CY_CTB_COMP_EDGE_DISABLE) \
                                            || ((edge) == CY_CTB_COMP_EDGE_RISING) \
                                            || ((edge) == CY_CTB_COMP_EDGE_FALLING) \
                                            || ((edge) == CY_CTB_COMP_EDGE_BOTH))
#define CY_CTB_COMPLEVEL(level)             (((level) == CY_CTB_COMP_DSI_TRIGGER_OUT_PULSE) || ((level) == CY_CTB_COMP_DSI_TRIGGER_OUT_LEVEL))
#define CY_CTB_COMPBYPASS(bypass)           (((bypass) == CY_CTB_COMP_BYPASS_SYNC) || ((bypass) == CY_CTB_COMP_BYPASS_NO_SYNC))
#define CY_CTB_COMPHYST(hyst)               (((hyst) == CY_CTB_COMP_HYST_DISABLE) || ((hyst) == CY_CTB_COMP_HYST_10MV))
#define CY_CTB_CURRENTMODE(mode)            (((mode) == CY_CTB_CURRENT_HIGH_ACTIVE) \
                                            || ((mode) == CY_CTB_CURRENT_HIGH_ACTIVE_DEEPSLEEP) \
                                            || ((mode) == CY_CTB_CURRENT_LOW_ACTIVE_DEEPSLEEP))
#define CY_CTB_SAMPLEHOLD(mode)             ((mode) <= CY_CTB_SH_HOLD)
#define CY_CTB_TRIM(trim)                   ((trim) <= CY_CTB_TRIM_VALUE_MAX)
#define CY_CTB_SWITCHSELECT(select)         (((select) == CY_CTB_SWITCH_OA0_SW) \
                                            || ((select) == CY_CTB_SWITCH_OA1_SW) \
                                            || ((select) == CY_CTB_SWITCH_CTD_SW))
#define CY_CTB_SWITCHSTATE(state)           (((state) == CY_CTB_SWITCH_OPEN) || ((state) == CY_CTB_SWITCH_CLOSE))
#define CY_CTB_OA0SWITCH(mask)              (((mask) & (~CY_CTB_DEINIT_OA0_SW)) == 0uL)
#define CY_CTB_OA1SWITCH(mask)              (((mask) & (~CY_CTB_DEINIT_OA1_SW)) == 0uL)
#define CY_CTB_CTDSWITCH(mask)              (((mask) & (~CY_CTB_DEINIT_CTD_SW)) == 0uL)
#define CY_CTB_SWITCHMASK(select,mask)      (((select) == CY_CTB_SWITCH_OA0_SW) ? (((mask) & (~CY_CTB_DEINIT_OA0_SW)) == 0uL) : \
                                            (((select) == CY_CTB_SWITCH_OA1_SW) ? (((mask) & (~CY_CTB_DEINIT_OA1_SW)) == 0uL) : \
                                            (((mask) & (~CY_CTB_DEINIT_CTD_SW)) == 0uL)))
#define CY_CTB_SARSEQCTRL(mask)             (((mask) == CY_CTB_SW_SEQ_CTRL_D51_MASK) \
                                            || ((mask) == CY_CTB_SW_SEQ_CTRL_D52_D62_MASK) \
                                            || ((mask) == CY_CTB_SW_SEQ_CTRL_D51_D52_D62_MASK))

/** \endcond */

/** \} group_ctb_macros */

/***************************************
*       Enumerated Types
***************************************/

/**
* \addtogroup group_ctb_enums
* \{
*/

/**
* Most functions allow you to configure a single opamp or both opamps at once.
* The \ref Cy_CTB_SetInterruptMask function can be called with \ref CY_CTB_OPAMP_NONE
* and interrupts will be disabled.
*/
typedef enum{
    CY_CTB_OPAMP_NONE    = 0uL,                                          /**< For disabling interrupts for both opamps. Used with \ref Cy_CTB_SetInterruptMask */
    CY_CTB_OPAMP_0       = CTBM_INTR_COMP0_Msk,                        /**< For configuring Opamp0 */
    CY_CTB_OPAMP_1       = CTBM_INTR_COMP1_Msk,                        /**< For configuring Opamp1 */
    CY_CTB_OPAMP_BOTH    = CTBM_INTR_COMP0_Msk | CTBM_INTR_COMP1_Msk,  /**< For configuring both Opamp0 and Opamp1 */
}cy_en_ctb_opamp_sel_t;

/** Enable or disable CTB while in Deep Sleep mode.
*/
typedef enum {
    CY_CTB_DEEPSLEEP_DISABLE   = 0uL,                               /**< CTB is disabled during Deep Sleep power mode */
    CY_CTB_DEEPSLEEP_ENABLE    = CTBM_CTB_CTRL_DEEPSLEEP_ON_Msk,   /**< CTB remains enabled during Deep Sleep power mode */
}cy_en_ctb_deep_sleep_t;

/**
* Configure the power mode of each opamp. Each power setting
* consumes different levels of current and supports a different
* input range and gain bandwidth.
*
* <table class="doxtable">
*   <tr><th>Opamp Power</th><th>IDD</th><th>Gain bandwidth</th></tr>
*   <tr>
*     <td>OFF</td>
*     <td>0</td>
*     <td>NA</td>
*   </tr>
*   <tr>
*     <td>LOW</td>
*     <td>350 uA</td>
*     <td>1 MHz</td>
*   </tr>
*   <tr>
*     <td>MEDIUM</td>
*     <td>600 uA</td>
*     <td>3 MHz for 1X, 2.5 MHz for 10x</td>
*   </tr>
*   <tr>
*     <td>HIGH</td>
*     <td>1.5 mA</td>
*     <td>8 MHz for 1X, 6 MHz for 10x</td>
*   </tr>
* </table>
*
*/
typedef enum {
    CY_CTB_POWER_OFF       = 0uL,     /**< Opamp is off */
    CY_CTB_POWER_LOW       = 1uL,     /**< Low power: IDD = 350 uA, GBW = 1 MHz for both 1x and 10x */
    CY_CTB_POWER_MEDIUM    = 2uL,     /**< Medium power: IDD = 600 uA, GBW = 3 MHz for 1x and 2.5 MHz for 10x */
    CY_CTB_POWER_HIGH      = 3uL,     /**< High power: IDD = 1500 uA, GBW = 8 MHz for 1x and 6 MHz for 10x */
}cy_en_ctb_power_t;

/**
* The output stage of each opamp can be configured for low-drive strength (1X) to drive internal circuits,
* for high-drive strength (10X) to drive external circuits, or as a comparator.
*/
typedef enum {
    CY_CTB_MODE_OPAMP1X    = 0uL,                                               /**< Configure opamp for low drive strength for internal connections (1x) */
    CY_CTB_MODE_OPAMP10X   = 1uL << CTBM_OA_RES0_CTRL_OA0_DRIVE_STR_SEL_Pos,    /**< Configure opamp high drive strength for driving a device pin (10x) */
    CY_CTB_MODE_COMP       = 1uL << CTBM_OA_RES0_CTRL_OA0_COMP_EN_Pos,          /**< Configure opamp as a comparator */
}cy_en_ctb_mode_t;

/**
* Each opamp has a charge pump to increase the input range to the rails.
* When the charge pump is enabled, the input range is 0 to VDDA.
* When disabled, the input range is 0 to VDDA - 1.5 V.
*
** <table class="doxtable">
*   <tr><th>Charge Pump</th><th>Input Range (V)</th></tr>
*   <tr>
*     <td>OFF</td>
*     <td>0 to VDDA-1.5</td>
*   </tr>
*   <tr>
*     <td>ON</td>
*     <td>0 to VDDA</td>
*   </tr>
* </table>
*
* Note that in Deep Sleep mode, the charge pump is disabled so the input
* range is reduced.
*/
typedef enum{
    CY_CTB_PUMP_DISABLE   = 0uL,                                           /**< Charge pump is disabled for an input range of 0 to VDDA - 1.5 V */
    CY_CTB_PUMP_ENABLE    = CTBM_OA_RES0_CTRL_OA0_PUMP_EN_Msk,            /**< Charge pump is enabled for an input range of 0 to VDDA */
}cy_en_ctb_pump_t;

/**
* Configure the type of edge that will trigger a comparator interrupt or
* disable the interrupt entirely.
*/
typedef enum
{
    CY_CTB_COMP_EDGE_DISABLE       = 0uL,                                       /**< Disabled, no interrupts generated */
    CY_CTB_COMP_EDGE_RISING        = 1uL << CTBM_OA_RES0_CTRL_OA0_COMPINT_Pos,  /**< Rising edge generates an interrupt */
    CY_CTB_COMP_EDGE_FALLING       = 2uL << CTBM_OA_RES0_CTRL_OA0_COMPINT_Pos,  /**< Falling edge generates an interrupt */
    CY_CTB_COMP_EDGE_BOTH          = 3uL << CTBM_OA_RES0_CTRL_OA0_COMPINT_Pos,  /**< Both edges generate an interrupt */
}cy_en_ctb_comp_edge_t;

/** Configure the comparator DSI trigger output level when output is synchronized. */
typedef enum
{
    CY_CTB_COMP_DSI_TRIGGER_OUT_PULSE    = 0uL,                                        /**< Send pulse on DSI for each edge of comparator output */
    CY_CTB_COMP_DSI_TRIGGER_OUT_LEVEL    = CTBM_OA_RES0_CTRL_OA0_DSI_LEVEL_Msk,       /**< DSI output is synchronized version of comparator output */
}cy_en_ctb_comp_level_t;

/** Bypass the comparator output synchronization for DSI trigger. */
typedef enum
{
    CY_CTB_COMP_BYPASS_SYNC          = 0uL,                                          /**< Comparator output is synchronized for DSI trigger */
    CY_CTB_COMP_BYPASS_NO_SYNC       = CTBM_OA_RES0_CTRL_OA0_BYPASS_DSI_SYNC_Msk,   /**< Comparator output is not synchronized for DSI trigger */
}cy_en_ctb_comp_bypass_t;

/** Disable or enable the 10 mV hysteresis for the comparator. */
typedef enum
{
    CY_CTB_COMP_HYST_DISABLE      = 0uL,                                    /**< Disable hysteresis */
    CY_CTB_COMP_HYST_10MV         = CTBM_OA_RES0_CTRL_OA0_HYST_EN_Msk,     /**< Enable the 10 mV hysteresis */
}cy_en_ctb_comp_hyst_t;

/** Switch state, either open or closed, to be used in \ref Cy_CTB_SetAnalogSwitch. */
typedef enum
{
    CY_CTB_SWITCH_OPEN      = 0uL,    /**< Open the switch */
    CY_CTB_SWITCH_CLOSE     = 1uL     /**< Close the switch */
}cy_en_ctb_switch_state_t;

/**
* The switch register to be used in \ref Cy_CTB_SetAnalogSwitch.
* The CTB has three registers for configuring the switch routing matrix.
* */
typedef enum
{
    CY_CTB_SWITCH_OA0_SW     = 0uL,     /**< Switch register for Opamp0 */
    CY_CTB_SWITCH_OA1_SW     = 1uL,     /**< Switch register for Opamp1 */
    CY_CTB_SWITCH_CTD_SW     = 2uL,     /**< Switch register for CTDAC routing */
}cy_en_ctb_switch_register_sel_t;

/**
* Switch masks for Opamp0 to be used in \ref Cy_CTB_SetAnalogSwitch.
*/
typedef enum
{
    CY_CTB_SW_OA0_POS_AMUXBUSA_MASK     = CTBM_OA0_SW_OA0P_A00_Msk,       /**< Switch A00: Opamp0 non-inverting input to AMUXBUS A */
    CY_CTB_SW_OA0_POS_PIN0_MASK         = CTBM_OA0_SW_OA0P_A20_Msk,       /**< Switch A20: Opamp0 non-inverting input to Pin 0 of CTB device port */
    CY_CTB_SW_OA0_POS_PIN6_MASK         = CTBM_OA0_SW_OA0P_A30_Msk,       /**< Switch A30: Opamp0 non-inverting input to Pin 6 of CTB device port */
    CY_CTB_SW_OA0_NEG_PIN1_MASK         = CTBM_OA0_SW_OA0M_A11_Msk,       /**< Switch A11: Opamp0 inverting input to Pin 1 of CTB device port */
    CY_CTB_SW_OA0_NEG_OUT_MASK          = CTBM_OA0_SW_OA0M_A81_Msk,       /**< Switch A81: Opamp0 inverting input to Opamp0 output */
    CY_CTB_SW_OA0_OUT_SARBUS0_MASK      = CTBM_OA0_SW_OA0O_D51_Msk,       /**< Switch D51: Opamp0 output to sarbus0 */
    CY_CTB_SW_OA0_OUT_SHORT_1X_10X_MASK = CTBM_OA0_SW_OA0O_D81_Msk,       /**< Switch D81: Short Opamp0 1x with 10x outputs */
}cy_en_ctb_oa0_switches_t;

/**
* Switch masks for Opamp1 to be used in \ref Cy_CTB_SetAnalogSwitch.
*/
typedef enum
{
    CY_CTB_SW_OA1_POS_AMUXBUSB_MASK     = CTBM_OA1_SW_OA1P_A03_Msk,       /**< Switch A03: Opamp1 non-inverting input to AMUXBUS B */
    CY_CTB_SW_OA1_POS_PIN5_MASK         = CTBM_OA1_SW_OA1P_A13_Msk,       /**< Switch A13: Opamp1 non-inverting input to Pin 5 of CTB device port */
    CY_CTB_SW_OA1_POS_PIN7_MASK         = CTBM_OA1_SW_OA1P_A43_Msk,       /**< Switch A43: Opamp1 non-inverting input to Pin 7 of CTB device port */
    CY_CTB_SW_OA1_POS_AREF_MASK         = CTBM_OA1_SW_OA1P_A73_Msk,       /**< Switch A73: Opamp1 non-inverting input to device Analog Reference (AREF) */
    CY_CTB_SW_OA1_NEG_PIN4_MASK         = CTBM_OA1_SW_OA1M_A22_Msk,       /**< Switch A22: Opamp1 inverting input to Pin 4 of CTB device port */
    CY_CTB_SW_OA1_NEG_OUT_MASK          = CTBM_OA1_SW_OA1M_A82_Msk,       /**< switch A82: Opamp1 inverting input to Opamp1 output */
    CY_CTB_SW_OA1_OUT_SARBUS0_MASK      = CTBM_OA1_SW_OA1O_D52_Msk,       /**< Switch D52: Opamp1 output to sarbus0 */
    CY_CTB_SW_OA1_OUT_SARBUS1_MASK      = CTBM_OA1_SW_OA1O_D62_Msk,       /**< Switch D62: Opamp1 output to sarbus1 */
    CY_CTB_SW_OA1_OUT_SHORT_1X_10X_MASK = CTBM_OA1_SW_OA1O_D82_Msk,       /**< Switch D82: Short Opamp1 1x with 10x outputs */
}cy_en_ctb_oa1_switches_t;

/**
* Switch masks for CTDAC to CTB routing to be used in \ref Cy_CTB_SetAnalogSwitch.
*/
typedef enum
{
    CY_CTB_SW_CTD_REF_OA1_OUT_MASK              = CTBM_CTD_SW_CTDD_CRD_Msk,       /**< Switch CRD: Opamp1 output to CTDAC reference. */
    CY_CTB_SW_CTD_REFSENSE_OA1_NEG_MASK         = CTBM_CTD_SW_CTDS_CRS_Msk,       /**< Switch CRS: CTDAC reference sense to Opamp1 inverting input. */
    CY_CTB_SW_CTD_OUT_OA1_NEG_MASK              = CTBM_CTD_SW_CTDS_COR_Msk,       /**< Switch COR: CTDAC output to Opamp1 inverting input. */
    CY_CTB_SW_CTD_OUT_PIN6_MASK                 = CTBM_CTD_SW_CTDO_C6H_Msk,       /**< Switch C6H: CTDAC output to P6 of CTB device port. */
    CY_CTB_SW_CTD_OUT_CHOLD_MASK                = CTBM_CTD_SW_CTDO_COS_Msk,       /**< Switch COS: CTDAC output to hold cap (deglitch capable). */
    CY_CTB_SW_CTD_OUT_OA0_1X_OUT_MASK           = CTBM_CTD_SW_CTDH_COB_Msk,       /**< Switch COB: Drive CTDAC output with opamp0 1x output during hold mode. */
    CY_CTB_SW_CTD_CHOLD_CONNECT_MASK            = CTBM_CTD_SW_CTDH_CHD_Msk,       /**< Switch CHD: Hold cap connection. */
    CY_CTB_SW_CTD_CHOLD_OA0_POS_MASK            = CTBM_CTD_SW_CTDH_CA0_Msk,       /**< Switch CA0: Hold cap to Opamp0 non-inverting input. */
    CY_CTB_SW_CTD_CHOLD_OA0_POS_ISOLATE_MASK    = CTBM_CTD_SW_CTDH_CIS_Msk,       /**< Switch CIS: Opamp0 non-inverting input isolation (for hold cap) */
    CY_CTB_SW_CTD_CHOLD_LEAKAGE_REDUCTION_MASK  = CTBM_CTD_SW_CTDH_ILR_Msk,       /**< Switch ILR: Hold cap leakage reduction (drives far side of isolation switch CIS) */
}cy_en_ctb_ctd_switches_t;


/**
* Masks for CTB switches that can be controlled by the SAR sequencer.
* These masks are used in \ref Cy_CTB_EnableSarSeqCtrl and \ref Cy_CTB_DisableSarSeqCtrl.
*
* The SAR ADC subsystem supports analog routes through three CTB switches on SARBUS0 and SARBUS1.
* This control allows for pins on the CTB dedicated port to route to the SAR ADC input channels:
*
*   - D51: Connects the inverting terminal of OA0 to SARBUS0
*   - D52: Connects the inverting terminal of OA1 to SARBUS0
*   - D62: Connects the inverting terminal of OA1 to SARBUS1
*/
typedef enum
{
    CY_CTB_SW_SEQ_CTRL_D51_MASK            = CTBM_CTB_SW_SQ_CTRL_P2_SQ_CTRL23_Msk,  /**< Enable SAR sequencer control of the D51 switch */
    CY_CTB_SW_SEQ_CTRL_D52_D62_MASK        = CTBM_CTB_SW_SQ_CTRL_P3_SQ_CTRL23_Msk,  /**< Enable SAR sequencer control of the D52 and D62 switches */
    CY_CTB_SW_SEQ_CTRL_D51_D52_D62_MASK    = CTBM_CTB_SW_SQ_CTRL_P2_SQ_CTRL23_Msk | CTBM_CTB_SW_SQ_CTRL_P3_SQ_CTRL23_Msk, /**< Enable SAR sequency control of all three switches */
}cy_en_ctb_switch_sar_seq_t;

/**
* Each opamp also has a programmable compensation capacitor block,
* that optimizes the stability of the opamp performance based on output load.
* The compensation cap will be set by the driver based on the opamp drive strength (1x or 10x) selection.
*/
typedef enum
{
    CY_CTB_OPAMP_COMPENSATION_CAP_OFF      = 0uL,       /**< No compensation */
    CY_CTB_OPAMP_COMPENSATION_CAP_MIN      = 1uL,       /**< Minimum compensation - for 1x drive*/
    CY_CTB_OPAMP_COMPENSATION_CAP_MED      = 2uL,       /**< Medium compensation */
    CY_CTB_OPAMP_COMPENSATION_CAP_MAX      = 3uL,       /**< Maximum compensation - for 10x drive */
}cy_en_ctb_compensation_cap_t;

/** Enable or disable the gain booster.
* The gain booster will be set by the driver based on the opamp drive strength (1x or 10x) selection.
*/
typedef enum
{
    CY_CTB_OPAMP_BOOST_DISABLE = 0uL,                                   /**< Disable gain booster - for 10x drive */
    CY_CTB_OPAMP_BOOST_ENABLE  = CTBM_OA_RES0_CTRL_OA0_BOOST_EN_Msk,   /**< Enable gain booster - for 1x drive */
}cy_en_ctb_boost_en_t;

/** Sample and hold modes for firmware sampling of the CTDAC output.
*
* To perform a sample or a hold, a preparation step must first be executed to
* open the required switches.
*
* -# Call \ref Cy_CTB_DACSampleAndHold with \ref CY_CTB_SH_PREPARE_SAMPLE or \ref CY_CTB_SH_PREPARE_HOLD
* -# Enable or disable CTDAC output
* -# Call \ref Cy_CTB_DACSampleAndHold with \ref CY_CTB_SH_SAMPLE or \ref CY_CTB_SH_HOLD
*/
typedef enum
{
    CY_CTB_SH_DISABLE           = 0uL,   /**< The hold capacitor is not connected - this disables sample and hold */
    CY_CTB_SH_PREPARE_SAMPLE    = 1uL,   /**< Prepares the required switches for a following sample */
    CY_CTB_SH_SAMPLE            = 2uL,   /**< Performs a sample of the voltage */
    CY_CTB_SH_PREPARE_HOLD      = 3uL,   /**< Prepares the required switches for a following hold */
    CY_CTB_SH_HOLD              = 4uL,   /**< Performs a hold of the previously sampled voltage */
}cy_en_ctb_sample_hold_mode_t;

/** AREF IPTAT bias current output for the CTB
*
* The CTB bias current can be 1 uA (normal) or 100 nA (low current).
*/
typedef enum
{
    CY_CTB_IPTAT_NORMAL       = 0uL,                                               /**< 1 uA bias current to the CTB */
    CY_CTB_IPTAT_LOW          = 1uL << PASS_AREF_AREF_CTRL_CTB_IPTAT_SCALE_Pos,    /**< 100 nA bias current to the CTB */
}cy_en_ctb_iptat_t;

/** CTB charge pump clock sources
*
* The CTB pump clock can come from:
*   - a dedicated divider clock in the SRSS
*   - one of the CLK_PERI dividers
*/
typedef enum
{
    CY_CTB_CLK_PUMP_SRSS       = 0uL,                                                   /**< Use the dedicated pump clock from SRSSp */
    CY_CTB_CLK_PUMP_PERI       = 1uL << PASS_AREF_AREF_CTRL_CLOCK_PUMP_PERI_SEL_Pos,    /**< Use one of the CLK_PERI dividers */
}cy_en_ctb_clk_pump_source_t;

/** High level opamp current modes */
typedef enum
{
    CY_CTB_CURRENT_HIGH_ACTIVE             = 0uL,    /**< Uses 1 uA reference current with charge pump enabled. Available in Active and Low Power */
    CY_CTB_CURRENT_HIGH_ACTIVE_DEEPSLEEP   = 1uL,    /**< Uses 1 uA reference current with charge pump disabled. Available in all power modes */
    CY_CTB_CURRENT_LOW_ACTIVE_DEEPSLEEP    = 2uL,    /**< Uses 100 nA reference current with charge pump disabled. Available in all power modes */
}cy_en_ctb_current_mode_t;

/** Return states for \ref Cy_CTB_Init, \ref Cy_CTB_OpampInit, \ref Cy_CTB_DeInit, and \ref Cy_CTB_FastInit */
typedef enum {
    CY_CTB_SUCCESS    = 0x00uL,                                      /**< Initialization completed successfully */
    CY_CTB_BAD_PARAM  = CY_CTB_ID | CY_PDL_STATUS_ERROR | 0x01uL,    /**< Input pointers were NULL and initialization could not be completed */
}cy_en_ctb_status_t;

/** \} group_ctb_enums */

/***************************************
*       Configuration Structures
***************************************/

/**
* \addtogroup group_ctb_data_structures
* \{
*/

/**
* Configuration structure to set up the entire CTB to be used with \ref Cy_CTB_Init.
*/
typedef struct {
    cy_en_ctb_deep_sleep_t      deepSleep;      /**< Enable or disable the CTB during Deep Sleep */

    /* Opamp0 configuration */
    cy_en_ctb_power_t           oa0Power;       /**< Opamp0 power mode: off, low, medium, or high */
    cy_en_ctb_mode_t            oa0Mode;        /**< Opamp0 usage mode: 1x drive, 10x drive, or as a comparator */
    cy_en_ctb_pump_t            oa0Pump;        /**< Opamp0 charge pump: enable to increase input range for rail-to-rail operation */
    cy_en_ctb_comp_edge_t       oa0CompEdge;    /**< Opamp0 comparator edge detection: disable, rising, falling, or both */
    cy_en_ctb_comp_level_t      oa0CompLevel;   /**< Opamp0 comparator DSI (trigger) output: pulse or level */
    cy_en_ctb_comp_bypass_t     oa0CompBypass;  /**< Opamp0 comparator DSI (trigger) output synchronization */
    cy_en_ctb_comp_hyst_t       oa0CompHyst;    /**< Opamp0 comparator hysteresis: enable for 10 mV hysteresis */
    bool                        oa0CompIntrEn;  /**< Opamp0 comparator interrupt enable */

    /* Opamp1 configuration */
    cy_en_ctb_power_t           oa1Power;       /**< Opamp1 power mode: off, low, medium, or high */
    cy_en_ctb_mode_t            oa1Mode;        /**< Opamp1 usage mode: 1x drive, 10x drive, or as a comparator */
    cy_en_ctb_pump_t            oa1Pump;        /**< Opamp1 charge pump: enable to increase input range for rail-to-rail operation */
    cy_en_ctb_comp_edge_t       oa1CompEdge;    /**< Opamp1 comparator edge detection: disable, rising, falling, or both */
    cy_en_ctb_comp_level_t      oa1CompLevel;   /**< Opamp1 comparator DSI (trigger) output: pulse or level */
    cy_en_ctb_comp_bypass_t     oa1CompBypass;  /**< Opamp1 comparator DSI (trigger) output synchronization */
    cy_en_ctb_comp_hyst_t       oa1CompHyst;    /**< Opamp1 comparator hysteresis: enable for 10 mV hysteresis */
    bool                        oa1CompIntrEn;  /**< Opamp1 comparator interrupt enable */

    /* Switch analog routing configuration */
    bool                        configRouting;  /**< Configure or ignore routing related registers */
    uint32_t                    oa0SwitchCtrl;  /**< Opamp0 routing control */
    uint32_t                    oa1SwitchCtrl;  /**< Opamp1 routing control */
    uint32_t                    ctdSwitchCtrl;  /**< Routing control between the CTDAC and CTB blocks */
}cy_stc_ctb_config_t;

/**
* This configuration structure is used to initialize only one opamp of the CTB
* without impacting analog routing. This structure is used with \ref Cy_CTB_OpampInit.
*/
typedef struct {
    cy_en_ctb_deep_sleep_t      deepSleep;      /**< Enable or disable the CTB during Deep Sleep */

    /* Opamp configuration */
    cy_en_ctb_power_t           oaPower;        /**< Opamp power mode: off, low, medium, or high */
    cy_en_ctb_mode_t            oaMode;         /**< Opamp usage mode: 1x drive, 10x drive, or as a comparator */
    cy_en_ctb_pump_t            oaPump;         /**< Opamp charge pump: enable to increase input range for rail-to-rail operation */
    cy_en_ctb_comp_edge_t       oaCompEdge;     /**< Opamp comparator edge detection: disable, rising, falling, or both */
    cy_en_ctb_comp_level_t      oaCompLevel;    /**< Opamp comparator DSI (trigger) output: pulse or level */
    cy_en_ctb_comp_bypass_t     oaCompBypass;   /**< Opamp comparator DSI (trigger) output synchronization */
    cy_en_ctb_comp_hyst_t       oaCompHyst;     /**< Opamp comparator hysteresis: enable for 10 mV hysteresis */
    bool                        oaCompIntrEn;   /**< Opamp comparator interrupt enable */
}cy_stc_ctb_opamp_config_t;

/** This configuration structure is used to quickly initialize Opamp0 for the most commonly used configurations.
*
* Other configuration options are set to:
*   - .oa0Pump       = \ref CY_CTB_PUMP_ENABLE
*   - .oa0CompEdge   = \ref CY_CTB_COMP_EDGE_BOTH
*   - .oa0CompLevel  = \ref CY_CTB_COMP_DSI_TRIGGER_OUT_LEVEL
*   - .oa0CompBypass = \ref CY_CTB_COMP_BYPASS_SYNC
*   - .oa0CompHyst   = \ref CY_CTB_COMP_HYST_10MV
*   - .oa0CompIntrEn = true
*/
typedef struct
{
    cy_en_ctb_power_t   oa0Power;       /**< Opamp0 power mode: off, low, medium, or high */
    cy_en_ctb_mode_t    oa0Mode;        /**< Opamp0 usage mode: 1x drive, 10x drive, or as a comparator */
    uint32_t            oa0SwitchCtrl;  /**< Opamp0 routing control */
    uint32_t            ctdSwitchCtrl;  /**< Routing control between the CTDAC and CTB blocks */
}cy_stc_ctb_fast_config_oa0_t;

/** This configuration structure is used to quickly initialize Opamp1 for the most commonly used configurations.
*
* Other configuration options are set to:
*   - .oa1Pump       = \ref CY_CTB_PUMP_ENABLE
*   - .oa1CompEdge   = \ref CY_CTB_COMP_EDGE_BOTH
*   - .oa1CompLevel  = \ref CY_CTB_COMP_DSI_TRIGGER_OUT_LEVEL
*   - .oa1CompBypass = \ref CY_CTB_COMP_BYPASS_SYNC
*   - .oa1CompHyst   = \ref CY_CTB_COMP_HYST_10MV
*   - .oa1CompIntrEn = true
*/
typedef struct
{
    cy_en_ctb_power_t   oa1Power;       /**< Opamp1 power mode: off, low, medium, or high */
    cy_en_ctb_mode_t    oa1Mode;        /**< Opamp1 usage mode: 1x drive, 10x drive, or as a comparator */
    uint32_t            oa1SwitchCtrl;  /**< Opamp1 routing control */
    uint32_t            ctdSwitchCtrl;  /**< Routing control between the CTDAC and CTB blocks */
}cy_stc_ctb_fast_config_oa1_t;

/** \} group_ctb_data_structures */


/** \addtogroup group_ctb_globals
* \{
*/
/***************************************
*        Global Variables
***************************************/

/** Configure Opamp0 as unused - powered down. See \ref Cy_CTB_FastInit. */
extern const cy_stc_ctb_fast_config_oa0_t Cy_CTB_Fast_Opamp0_Unused;

/** Configure Opamp0 as a comparator. No routing is configured.
*
* \image html ctb_fast_config_comp.png
* \image latex ctb_fast_config_comp.png width=100px
*
* See \ref Cy_CTB_FastInit.
*/
extern const cy_stc_ctb_fast_config_oa0_t Cy_CTB_Fast_Opamp0_Comp;

/** Configure Opamp0 as an opamp with 1x drive. No routing is configured.
*
* \image html ctb_fast_config_opamp1x.png
* \image latex ctb_fast_config_opamp1x.png width=100px
*
* See \ref Cy_CTB_FastInit.
*/
extern const cy_stc_ctb_fast_config_oa0_t Cy_CTB_Fast_Opamp0_Opamp1x;

/** Configure Opamp0 as an opamp with 10x drive. No routing is configured.
*
* \image html ctb_fast_config_opamp10x.png
* \image latex ctb_fast_config_opamp10x.png width=100px
*
* See \ref Cy_CTB_FastInit.
*/
extern const cy_stc_ctb_fast_config_oa0_t Cy_CTB_Fast_Opamp0_Opamp10x;

/** Configure Opamp0 as one stage of a differential amplifier.
* The opamp is in 10x drive and the switches shown are closed.
*
* \image html ctb_fast_config_oa0_diffamp.png
* \image latex ctb_fast_config_oa0_diffamp.png width=100px
*
* See the device datasheet for the dedicated CTB port.
*
* To be used with \ref Cy_CTB_FastInit and \ref Cy_CTB_Fast_Opamp1_Diffamp.
*/
extern const cy_stc_ctb_fast_config_oa0_t Cy_CTB_Fast_Opamp0_Diffamp;

/** Configure Opamp0 as a buffer for the CTDAC output.
* The buffer is in 10x drive and the switches shown are closed.
* Configure the CTDAC for output buffer mode by calling \ref Cy_CTDAC_FastInit
* with \ref Cy_CTDAC_Fast_VddaRef_BufferedOut or \ref Cy_CTDAC_Fast_OA1Ref_BufferedOut.
*
* \image html ctb_fast_config_vdac_output.png
* \image latex ctb_fast_config_vdac_output.png
*
* See the device datasheet for the dedicated CTB port.
*
* See \ref Cy_CTB_FastInit.
*/
extern const cy_stc_ctb_fast_config_oa0_t Cy_CTB_Fast_Opamp0_Vdac_Out;

/** Configure Opamp0 as a buffer for the CTDAC output with the sample and hold capacitor connected.
* The buffer is in 10x drive and the switches shown are closed.
* Configure the CTDAC for output buffer mode by calling \ref Cy_CTDAC_FastInit
* with \ref Cy_CTDAC_Fast_VddaRef_BufferedOut or \ref Cy_CTDAC_Fast_OA1Ref_BufferedOut.

* \image html ctb_fast_config_vdac_sh.png
* \image latex ctb_fast_config_vdac_sh.png
*
* See the device datasheet for the dedicated CTB port.
*
* See \ref Cy_CTB_FastInit.
*/
extern const cy_stc_ctb_fast_config_oa0_t Cy_CTB_Fast_Opamp0_Vdac_Out_SH;

/** Configure Opamp1 as unused - powered down. See \ref Cy_CTB_FastInit.*/
extern const cy_stc_ctb_fast_config_oa1_t Cy_CTB_Fast_Opamp1_Unused;

/** Configure Opamp1 as a comparator. No routing is configured.
*
* \image html ctb_fast_config_comp.png
* \image latex ctb_fast_config_comp.png width=100px
*
* See \ref Cy_CTB_FastInit.
*/
extern const cy_stc_ctb_fast_config_oa1_t Cy_CTB_Fast_Opamp1_Comp;

/** Configure Opamp1 as an opamp with 1x drive. No routing is configured.
*
* \image html ctb_fast_config_opamp1x.png
* \image latex ctb_fast_config_opamp1x.png width=100px
*
* See \ref Cy_CTB_FastInit.
*/
extern const cy_stc_ctb_fast_config_oa1_t Cy_CTB_Fast_Opamp1_Opamp1x;

/** Configure Opamp1 as an opamp with 10x drive. No routing is configured.
*
* \image html ctb_fast_config_opamp10x.png
* \image latex ctb_fast_config_opamp10x.png width=100px
*
* See \ref Cy_CTB_FastInit.
*/
extern const cy_stc_ctb_fast_config_oa1_t Cy_CTB_Fast_Opamp1_Opamp10x;

/** Configure Opamp1 as one stage of a differential amplifier.
* The opamp is in 10x drive and the switches shown are closed.
*
* \image html ctb_fast_config_oa1_diffamp.png
* \image latex ctb_fast_config_oa1_diffamp.png width=100px
*
* See the device datasheet for the dedicated CTB port.
*
* To be used with \ref Cy_CTB_FastInit and \ref Cy_CTB_Fast_Opamp0_Diffamp.
*
*/
extern const cy_stc_ctb_fast_config_oa1_t Cy_CTB_Fast_Opamp1_Diffamp;

/** Configure Opamp1 as a buffer for the CTDAC reference. The reference comes from the
* internal analog reference block (AREF).
* The buffer is in 1x drive and the switches shown are closed.
* Configure the CTDAC to use the buffered reference by calling \ref Cy_CTDAC_FastInit
* with \ref Cy_CTDAC_Fast_OA1Ref_UnbufferedOut or \ref Cy_CTDAC_Fast_OA1Ref_BufferedOut.
*
* \image html ctb_fast_config_vdac_aref.png
* \image latex ctb_fast_config_vdac_aref.png
*
* See \ref Cy_CTB_FastInit.
*
* Note the AREF block needs to be configured using a separate driver.
*/
extern const cy_stc_ctb_fast_config_oa1_t Cy_CTB_Fast_Opamp1_Vdac_Ref_Aref;

/** Configure Opamp1 as a buffer for the CTDAC reference. The reference comes from Pin 5.
* The buffer is in 1x drive and the switches shown are closed.
* Configure the CTDAC to use the buffered reference by calling \ref Cy_CTDAC_FastInit
* with \ref Cy_CTDAC_Fast_OA1Ref_UnbufferedOut or \ref Cy_CTDAC_Fast_OA1Ref_BufferedOut.
*
* \image html ctb_fast_config_vdac_pin5.png
* \image latex ctb_fast_config_vdac_pin5.png
*
* See the device datasheet for the dedicated CTB port.
*
* See \ref Cy_CTB_FastInit.
*/
extern const cy_stc_ctb_fast_config_oa1_t Cy_CTB_Fast_Opamp1_Vdac_Ref_Pin5;

/** \} group_ctb_globals */

/***************************************
*        Function Prototypes
***************************************/

/**
* \addtogroup group_ctb_functions
* \{
*/

/**
* \addtogroup group_ctb_functions_init
* This set of functions are for initializing, enabling, and disabling the CTB.
* \{
*/
cy_en_ctb_status_t Cy_CTB_Init(CTBM_Type *base, const cy_stc_ctb_config_t *config);
cy_en_ctb_status_t Cy_CTB_OpampInit(CTBM_Type *base, cy_en_ctb_opamp_sel_t opampNum, const cy_stc_ctb_opamp_config_t *config);
cy_en_ctb_status_t Cy_CTB_DeInit(CTBM_Type *base, bool deInitRouting);
cy_en_ctb_status_t Cy_CTB_FastInit(CTBM_Type *base, const cy_stc_ctb_fast_config_oa0_t *config0, const cy_stc_ctb_fast_config_oa1_t *config1);
__STATIC_INLINE void Cy_CTB_Enable(CTBM_Type *base);
__STATIC_INLINE void Cy_CTB_Disable(CTBM_Type *base);
/** \} */

/**
* \addtogroup group_ctb_functions_basic
* This set of functions are for configuring basic usage of the CTB.
* \{
*/
void Cy_CTB_SetDeepSleepMode(CTBM_Type *base, cy_en_ctb_deep_sleep_t deepSleep);
void Cy_CTB_SetOutputMode(CTBM_Type *base, cy_en_ctb_opamp_sel_t opampNum, cy_en_ctb_mode_t mode);
void Cy_CTB_SetPower(CTBM_Type *base, cy_en_ctb_opamp_sel_t opampNum, cy_en_ctb_power_t power, cy_en_ctb_pump_t pump);
/** \} */

/**
* \addtogroup group_ctb_functions_sample_hold
* This function enables sample and hold of the CTDAC output.
* \{
*/
void Cy_CTB_DACSampleAndHold(CTBM_Type *base, cy_en_ctb_sample_hold_mode_t mode);
/** \} */

/**
* \addtogroup group_ctb_functions_comparator
* This set of functions are specific to the comparator mode
* \{
*/
void Cy_CTB_CompSetConfig(CTBM_Type *base, cy_en_ctb_opamp_sel_t compNum, cy_en_ctb_comp_level_t level, cy_en_ctb_comp_bypass_t bypass, cy_en_ctb_comp_hyst_t hyst);
uint32_t Cy_CTB_CompGetConfig(const CTBM_Type *base, cy_en_ctb_opamp_sel_t compNum);
void Cy_CTB_CompSetInterruptEdgeType(CTBM_Type *base, cy_en_ctb_opamp_sel_t compNum, cy_en_ctb_comp_edge_t edge);
uint32_t Cy_CTB_CompGetStatus(const CTBM_Type *base, cy_en_ctb_opamp_sel_t compNum);
/** \} */

/**
* \addtogroup group_ctb_functions_trim
* These are advanced functions for trimming the offset and slope of the opamps.
* Most users do not need to call these functions and can use the factory trimmed values.
* \{
*/
void Cy_CTB_OpampSetOffset(CTBM_Type *base, cy_en_ctb_opamp_sel_t opampNum, uint32_t trim);
uint32_t Cy_CTB_OpampGetOffset(const CTBM_Type *base, cy_en_ctb_opamp_sel_t opampNum);
void Cy_CTB_OpampSetSlope(CTBM_Type *base, cy_en_ctb_opamp_sel_t opampNum, uint32_t trim);
uint32_t Cy_CTB_OpampGetSlope(const CTBM_Type *base, cy_en_ctb_opamp_sel_t opampNum);
/** \} */

/**
* \addtogroup group_ctb_functions_switches
* This set of functions is for controlling routing switches.
* \{
*/
void Cy_CTB_SetAnalogSwitch(CTBM_Type *base, cy_en_ctb_switch_register_sel_t switchSelect, uint32_t switchMask, cy_en_ctb_switch_state_t state);
uint32_t Cy_CTB_GetAnalogSwitch(const CTBM_Type *base, cy_en_ctb_switch_register_sel_t switchSelect);
__STATIC_INLINE void Cy_CTB_OpenAllSwitches(CTBM_Type *base);
__STATIC_INLINE void Cy_CTB_EnableSarSeqCtrl(CTBM_Type *base, cy_en_ctb_switch_sar_seq_t switchMask);
__STATIC_INLINE void Cy_CTB_DisableSarSeqCtrl(CTBM_Type *base, cy_en_ctb_switch_sar_seq_t switchMask);
/** \} */

/**
* \addtogroup group_ctb_functions_interrupts
* This set of functions is related to the comparator interrupts.
* \{
*/
__STATIC_INLINE uint32_t Cy_CTB_GetInterruptStatus(const CTBM_Type *base, cy_en_ctb_opamp_sel_t compNum);
__STATIC_INLINE void Cy_CTB_ClearInterrupt(CTBM_Type *base, cy_en_ctb_opamp_sel_t compNum);
__STATIC_INLINE void Cy_CTB_SetInterrupt(CTBM_Type *base, cy_en_ctb_opamp_sel_t compNum);
__STATIC_INLINE void Cy_CTB_SetInterruptMask(CTBM_Type *base, cy_en_ctb_opamp_sel_t compNum);
__STATIC_INLINE uint32_t Cy_CTB_GetInterruptMask(const CTBM_Type *base, cy_en_ctb_opamp_sel_t compNum);
__STATIC_INLINE uint32_t Cy_CTB_GetInterruptStatusMasked(const CTBM_Type *base, cy_en_ctb_opamp_sel_t compNum);
/** \} */

/**
* \addtogroup group_ctb_functions_aref
* This set of functions impacts all opamps on the chip.
* Notice how some of these functions do not take a base address input.
* When calling \ref Cy_CTB_SetCurrentMode for a CTB instance on the device,
* it should be called for all other CTB instances as well. This is because
* there is only one IPTAT level (1 uA or 100 nA) chip wide.
* \{
*/
void Cy_CTB_SetCurrentMode(CTBM_Type *base, cy_en_ctb_current_mode_t currentMode);
__STATIC_INLINE void Cy_CTB_SetIptatLevel(cy_en_ctb_iptat_t iptat);
__STATIC_INLINE void Cy_CTB_SetClkPumpSource(cy_en_ctb_clk_pump_source_t clkPump);
__STATIC_INLINE void Cy_CTB_EnableRedirect(void);
__STATIC_INLINE void Cy_CTB_DisableRedirect(void);
/** \} */

/**
* \addtogroup group_ctb_functions_init
* \{
*/

/*******************************************************************************
* Function Name: Cy_CTB_Enable
****************************************************************************//**
*
* Power up the CTB hardware block.
*
* \param base
* Pointer to structure describing registers
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_CTB_Enable(CTBM_Type *base)
{
    CTBM_CTB_CTRL(base) |= CTBM_CTB_CTRL_ENABLED_Msk;
}

/*******************************************************************************
* Function Name: Cy_CTB_Disable
****************************************************************************//**
*
* Power down the CTB hardware block.
*
* \param base
* Pointer to structure describing registers
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_CTB_Disable(CTBM_Type *base)
{
    CTBM_CTB_CTRL(base) &= (~CTBM_CTB_CTRL_ENABLED_Msk);
}

/** \} */

/**
* \addtogroup group_ctb_functions_switches
* \{
*/

/*******************************************************************************
* Function Name: Cy_CTB_OpenAllSwitches
****************************************************************************//**
*
* Open all the switches and disable all hardware (SAR Sequencer and DSI) control of the switches.
* Primarily used as a quick method of re-configuring all analog connections
* that are sparsely closed.
*
* \param base
* Pointer to structure describing registers
*
* \return None
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_OPEN_ALL_SWITCHES
*
*******************************************************************************/
__STATIC_INLINE void Cy_CTB_OpenAllSwitches(CTBM_Type *base)
{
    CTBM_OA0_SW_CLEAR(base) = CY_CTB_DEINIT_OA0_SW;
    CTBM_OA1_SW_CLEAR(base) = CY_CTB_DEINIT_OA1_SW;
    CTBM_CTD_SW_CLEAR(base) = CY_CTB_DEINIT_CTD_SW;
    CTBM_CTB_SW_DS_CTRL(base) = CY_CTB_DEINIT;
    CTBM_CTB_SW_SQ_CTRL(base) = CY_CTB_DEINIT;
}

/*******************************************************************************
* Function Name: Cy_CTB_EnableSarSeqCtrl
****************************************************************************//**
*
* Enable SAR sequencer control of specified switch(es).
*
* This allows the SAR ADC to use routes through the CTB when configuring its channels.
*
* There are three switches in the CTB that can be enabled by the SAR sequencer.
* - D51: This switch connects the negative input of Opamp0 to the SARBUS0
* - D52: This switch connects the positive input of Opamp1 to the SARBUS0
* - D62: This switch connects the positive input of Opamp1 to the SARBUS1
*
* \param base
* Pointer to structure describing registers
*
* \param switchMask
* The switch or switches in which to enable SAR sequencer control.
* Use an enumerated value from \ref cy_en_ctb_switch_sar_seq_t.
*
* \return None
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_ENABLE_SAR_SEQ_CTRL
*
*******************************************************************************/
__STATIC_INLINE void Cy_CTB_EnableSarSeqCtrl(CTBM_Type *base, cy_en_ctb_switch_sar_seq_t switchMask)
{
    CY_ASSERT_L3(CY_CTB_SARSEQCTRL(switchMask));

    CTBM_CTB_SW_SQ_CTRL(base) |= (uint32_t) switchMask;
}

/*******************************************************************************
* Function Name: Cy_CTB_DisableSarSeqCtrl
****************************************************************************//**
*
* Disable SAR sequencer control of specified switch(es).
*
* \param base
* Pointer to structure describing registers
*
* \param switchMask
* The switch or switches in which to disable SAR sequencer control.
* Use an enumerated value from \ref cy_en_ctb_switch_sar_seq_t.
*
* \return None
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_DISABLE_SAR_SEQ_CTRL
*
*******************************************************************************/
__STATIC_INLINE void Cy_CTB_DisableSarSeqCtrl(CTBM_Type *base, cy_en_ctb_switch_sar_seq_t switchMask)
{
    CY_ASSERT_L3(CY_CTB_SARSEQCTRL(switchMask));

    CTBM_CTB_SW_SQ_CTRL(base) &= ~((uint32_t) switchMask);
}
/** \} */

/**
* \addtogroup group_ctb_functions_interrupts
* \{
*/

/*******************************************************************************
* Function Name: Cy_CTB_GetInterruptStatus
****************************************************************************//**
*
* Return the status of the interrupt when the configured comparator
* edge is detected.
*
* \param base
* Pointer to structure describing registers
*
* \param compNum
* \ref CY_CTB_OPAMP_0, \ref CY_CTB_OPAMP_1, or \ref CY_CTB_OPAMP_BOTH
*
* \return
* The interrupt status.
* If compNum is \ref CY_CTB_OPAMP_BOTH, cast the returned status
* to \ref cy_en_ctb_opamp_sel_t to determine which comparator edge (or both)
* was detected.
* - 0: Edge was not detected
* - Non-zero: Configured edge type was detected
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm0p.c SNIPPET_COMP_GETINTERRUPTSTATUS
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_CTB_GetInterruptStatus(const CTBM_Type *base, cy_en_ctb_opamp_sel_t compNum)
{
    CY_ASSERT_L3(CY_CTB_OPAMPNUM(compNum));

    return CTBM_INTR(base) & (uint32_t) compNum;
}

/*******************************************************************************
* Function Name: Cy_CTB_ClearInterrupt
****************************************************************************//**
*
* Clear the CTB comparator triggered interrupt.
* The interrupt must be cleared with this function so that the hardware
* can set subsequent interrupts and those interrupts can be forwarded
* to the interrupt controller, if enabled.
*
* \param base
* Pointer to structure describing registers
*
* \param compNum
* \ref CY_CTB_OPAMP_0, \ref CY_CTB_OPAMP_1, or \ref CY_CTB_OPAMP_BOTH
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_CTB_ClearInterrupt(CTBM_Type *base, cy_en_ctb_opamp_sel_t compNum)
{
    CY_ASSERT_L3(CY_CTB_OPAMPNUM(compNum));

    CTBM_INTR(base) = (uint32_t) compNum;

    /* Dummy read for buffered writes. */
    (void) CTBM_INTR(base);
}

/*******************************************************************************
* Function Name: Cy_CTB_SetInterrupt
****************************************************************************//**
*
* Force the CTB interrupt to trigger using software.
*
* \param base
* Pointer to structure describing registers
*
* \param compNum
* \ref CY_CTB_OPAMP_0, \ref CY_CTB_OPAMP_1, or \ref CY_CTB_OPAMP_BOTH
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_CTB_SetInterrupt(CTBM_Type *base, cy_en_ctb_opamp_sel_t compNum)
{
    CY_ASSERT_L3(CY_CTB_OPAMPNUM(compNum));

    CTBM_INTR_SET(base) = (uint32_t) compNum;
}

/*******************************************************************************
* Function Name: Cy_CTB_SetInterruptMask
****************************************************************************//**
*
* Configure the CTB comparator edge interrupt to be forwarded to the
* CPU interrupt controller.
*
* \param base
* Pointer to structure describing registers
*
* \param compNum
* \ref CY_CTB_OPAMP_NONE, \ref CY_CTB_OPAMP_0, \ref CY_CTB_OPAMP_1, or \ref CY_CTB_OPAMP_BOTH.
* Calling this function with CY_CTB_OPAMP_NONE will disable all interrupt requests.
*
* \return None
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_SET_INTERRUPT_MASK
*
*******************************************************************************/
__STATIC_INLINE void Cy_CTB_SetInterruptMask(CTBM_Type *base, cy_en_ctb_opamp_sel_t compNum)
{
    CY_ASSERT_L3(CY_CTB_OPAMPNUM_ALL(compNum));

    CTBM_INTR_MASK(base) = (uint32_t) compNum;
}

/*******************************************************************************
* Function Name: Cy_CTB_GetInterruptMask
****************************************************************************//**
*
* Return whether the CTB comparator edge interrupt output is
* forwarded to the CPU interrupt controller as configured by
* \ref Cy_CTB_SetInterruptMask.
*
* \param base
* Pointer to structure describing registers
*
* \param compNum
* \ref CY_CTB_OPAMP_0, \ref CY_CTB_OPAMP_1, or \ref CY_CTB_OPAMP_BOTH
*
* \return
* The interrupt mask.
* If compNum is \ref CY_CTB_OPAMP_BOTH, cast the returned mask
* to \ref cy_en_ctb_opamp_sel_t to determine which comparator interrupt
* output (or both) is forwarded.
* - 0: Interrupt output not forwarded to interrupt controller
* - Non-zero: Interrupt output forwarded to interrupt controller
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_GET_INTERRUPT_MASK
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_CTB_GetInterruptMask(const CTBM_Type *base, cy_en_ctb_opamp_sel_t compNum)
{
    CY_ASSERT_L3(CY_CTB_OPAMPNUM(compNum));

    return CTBM_INTR_MASK(base)  & (uint32_t) compNum;
}

/*******************************************************************************
* Function Name: Cy_CTB_GetInterruptStatusMasked
****************************************************************************//**
*
* Return the CTB comparator edge output interrupt state after being masked.
* This is the bitwise AND of \ref Cy_CTB_GetInterruptStatus and \ref Cy_CTB_GetInterruptMask.
*
* \param base
* Pointer to structure describing registers
*
* \param compNum
* \ref CY_CTB_OPAMP_0, \ref CY_CTB_OPAMP_1, or \ref CY_CTB_OPAMP_BOTH
*
* \return
* If compNum is \ref CY_CTB_OPAMP_BOTH, cast the returned value
* to \ref cy_en_ctb_opamp_sel_t to determine which comparator interrupt
* output (or both) is detected and masked.
* - 0: Configured edge not detected or not masked
* - Non-zero: Configured edge type detected and masked
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_CTB_GetInterruptStatusMasked(const CTBM_Type *base, cy_en_ctb_opamp_sel_t compNum)
{
    CY_ASSERT_L3(CY_CTB_OPAMPNUM(compNum));

    return CTBM_INTR_MASKED(base) & (uint32_t) compNum;
}
/** \} */

/**
* \addtogroup group_ctb_functions_aref
* \{
*/

/*******************************************************************************
* Function Name: Cy_CTB_SetIptatLevel
****************************************************************************//**
*
* Set the IPTAT reference level to 1 uA or 100 nA. The IPTAT generator is used by the CTB
* for slope offset drift.
*
* \param iptat
* Value from enum \ref cy_en_ctb_iptat_t
*
* \return None
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_SET_IPTAT_LEVEL
*
*******************************************************************************/
__STATIC_INLINE void Cy_CTB_SetIptatLevel(cy_en_ctb_iptat_t iptat)
{
    CY_ASSERT_L3(CY_CTB_IPTAT(iptat));

    PASS_AREF_AREF_CTRL = (PASS_AREF_AREF_CTRL & ~PASS_AREF_AREF_CTRL_CTB_IPTAT_SCALE_Msk) | (uint32_t) iptat;
}

/*******************************************************************************
* Function Name: Cy_CTB_SetClkPumpSource
****************************************************************************//**
*
* Set the clock source for both charge pumps in the CTB. Recall that each opamp
* has its own charge pump. The clock can come from:
*
*   - A dedicated divider off of one of the CLK_PATH in the SRSS.
*     Call the following functions to configure the pump clock from the SRSS:
*       - \ref Cy_SysClk_ClkPumpSetSource
*       - \ref Cy_SysClk_ClkPumpSetDivider
*       - \ref Cy_SysClk_ClkPumpEnable
*   - One of the Peri Clock dividers.
*     Call the following functions to configure a Peri Clock divider as the
*     pump clock:
*       - \ref Cy_SysClk_PeriphAssignDivider with the IP block set to PCLK_PASS_CLOCK_PUMP_PERI
*       - \ref Cy_SysClk_PeriphSetDivider
*       - \ref Cy_SysClk_PeriphEnableDivider
*
* \param clkPump
* Clock source selection (SRSS or PeriClk) for the pump. Select a value from
* \ref cy_en_ctb_clk_pump_source_t
*
* \return None
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_SET_CLK_PUMP_SOURCE_SRSS
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_SET_CLK_PUMP_SOURCE_PERI
*
*******************************************************************************/
__STATIC_INLINE void Cy_CTB_SetClkPumpSource(cy_en_ctb_clk_pump_source_t clkPump)
{
    CY_ASSERT_L3(CY_CTB_CLKPUMP(clkPump));

    PASS_AREF_AREF_CTRL = (PASS_AREF_AREF_CTRL & ~PASS_AREF_AREF_CTRL_CLOCK_PUMP_PERI_SEL_Msk) | (uint32_t) clkPump;
}

/*******************************************************************************
* Function Name: Cy_CTB_EnableRedirect
****************************************************************************//**
*
* Normally, the AREF IZTAT is routed to the CTB IZTAT and the AREF IPTAT
* is routed to the CTB IPTAT:
*
*   - CTB.IZTAT = AREF.IZTAT
*   - CTB.IPTAT = AREF.IPTAT
*
* However, the AREF IPTAT can be redirected to the CTB IZTAT and the CTB IPTAT
* is off.
*
*   - CTB.IZTAT = AREF.IPTAT
*   - CTB.IPTAT = HiZ
*
* The redirection applies to all opamps on the device and
* should be used when the IPTAT bias level is set to 100 nA
* (see \ref Cy_CTB_SetIptatLevel).
*
* When the CTB.IPTAT is HiZ, the CTB cannot compensate for the slope of
* the offset across temperature.
*
* \return None
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_ENABLE_REDIRECT
*
*******************************************************************************/
__STATIC_INLINE void Cy_CTB_EnableRedirect(void)
{
    PASS_AREF_AREF_CTRL |= PASS_AREF_AREF_CTRL_CTB_IPTAT_REDIRECT_Msk;
}

/*******************************************************************************
* Function Name: Cy_CTB_DisableRedirect
****************************************************************************//**
*
* Disable the redirection of the AREF IPTAT to the CTB IZTAT for all opamps
* on the device as enabled by \ref Cy_CTB_EnableRedirect.
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_CTB_DisableRedirect(void)
{
    PASS_AREF_AREF_CTRL &= ~(PASS_AREF_AREF_CTRL_CTB_IPTAT_REDIRECT_Msk);
}

/** \} */

/** \} group_ctb_functions */

#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXS40PASS_CTB */

#endif /** !defined(CY_CTB_H) */

/** \} group_ctb */

/* [] END OF FILE */

