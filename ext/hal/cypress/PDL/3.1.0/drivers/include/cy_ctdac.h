/***************************************************************************//**
* \file cy_ctdac.h
* \version 1.10
*
* Header file for the CTDAC driver
*
********************************************************************************
* \copyright
* Copyright 2017-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \defgroup group_ctdac Continuous Time Digital to Analog Converter (CTDAC)
* \{
* The CTDAC driver provides APIs to configure the 12-bit Continuous-Time DAC.
*
* - 12-bit continuous time output
* - 2 us settling time for a 25 pF load when output buffered through Opamp0 of \ref group_ctb "CTB"
* - Can be enabled in Deep Sleep power mode
* - Selectable voltage reference:
*       - VDDA
*       - Internal analog reference buffered through Opamp1 of \ref group_ctb "CTB"
*       - External reference buffered through Opamp1 of \ref group_ctb "CTB"
* - Selectable output paths:
*       - Direct DAC output to a pin
*       - Buffered DAC output through Opamp0 of \ref group_ctb "CTB"
*       - Sample and hold output path through Opamp0 of \ref group_ctb "CTB"
* - Selectable input modes:
*       - Unsigned 12-bit mode
*       - Virtual signed 12-bit mode
* - Configurable update rate using clock or strobe signal
* - Double buffered DAC voltage control register
* - Interrupt and DMA trigger on DAC buffer empty
* - Configurable as PGA along with Opamp1 of the \ref group_ctb "CTB"
*
* The CTDAC generates a 12-bit DAC output voltage from the reference.
* The DAC reference can come from VDDA or from any signal buffered through Opamp0
* of the CTB. This can be an external signal through a GPIO or from the internal
* AREF. The CTDAC is closely integrated with the CTB block,
* which provides easy buffering of the DAC output voltage,
* buffered input reference voltage, and sample and hold for the DAC output.
* The CTDAC control interface provides control of the DAC output through CPU or DMA.
* This includes a double-buffered DAC voltage control register, clock input for programmable
* update rate, interrupt on DAC buffer empty, and trigger to DMA.
*
* \image html ctdac_block_diagram.png
* \image latex ctdac_block_diagram.png
*
* The CTDAC has two switches, CO6 for configuring the output path and
* CVD for the reference source.
*
* \image html ctdac_switches.png
* \image latex ctdac_switches.png
*
* \section group_ctdac_init Initialization
*
* Configure the CTDAC hardware block by calling \ref Cy_CTDAC_Init.
* The base address of the CTDAC hardware can be found in the device-specific header file.
* If the buffers in the CTB are used for the reference source or the output,
* initialize the CTB hardware block. After both blocks are initialized,
* enable the CTB block before enabling the CTDAC block.
*
* The driver also provides a \ref Cy_CTDAC_FastInit function for fast and easy initialization of the CTDAC.
* The driver has pre-defined configuration structures for the four combinations of the reference and output buffers.
*
* - \ref Cy_CTDAC_Fast_VddaRef_UnbufferedOut
* - \ref Cy_CTDAC_Fast_VddaRef_BufferedOut
* - \ref Cy_CTDAC_Fast_OA1Ref_UnbufferedOut
* - \ref Cy_CTDAC_Fast_OA1Ref_BufferedOut
*
* After initialization, call \ref Cy_CTDAC_Enable to enable the hardware.
*
* \section group_ctdac_updatemode Update Modes
* The CTDAC contains two registers:
*   -# CTDAC_VAL
*
*       For direct firmware writes to update the current DAC value immediately.
*       This register is written with \ref Cy_CTDAC_SetValue.
*   -# CTDAC_VAL_NXT
*
*       For buffered writes to update the DAC value at a
*       periodic rate or with a strobe trigger input.
*       This register is written with \ref Cy_CTDAC_SetValueBuffered.
*
* The update mode is
* selected during initialization with the \ref cy_stc_ctdac_config_t.updateMode.
* Four of these modes require a dedicated clock resource and the driver
* can configure the clock during initialization (see \ref cy_stc_ctdac_config_t).
*
* Three of these modes use a strobe signal through the digital signal interface (DSI).
* This allows control of the buffered update timing from an external source, for example, by another
* chip peripheral or from an off-chip source.
*
* \subsection group_ctdac_updatemode_direct_write Direct write
*
* In this mode, the user writes directly into the CTDAC_VAL register
* using \ref Cy_CTDAC_SetValue. The action of writing to this register
* will update the DAC output. This mode does not generate an interrupt
* or trigger signal.
* In this mode, a clock must not be configured. Additionally, calling \ref
* Cy_CTDAC_SetValueBuffered does not update the DAC output.
*
* \image html ctdac_update_mode_direct_write.png
* \image latex ctdac_update_mode_direct_write.png
*
* \subsection group_ctdac_updatemode_buffered_write Buffered write
*
* In this mode, the user writes to the CTDAC_VAL_NXT register using
* \ref Cy_CTDAC_SetValueBuffered. The rising edge of the clock
* will update the DAC output and generate the interrupt and trigger signals.
*
* Whenever data is transferred from the CTDAC_VAL_NXT register,
* an interrupt is asserted the same time as the trigger. But while
* the trigger is automatically cleared after two PeriClk cycles, the
* user must clear the interrupt with \ref Cy_CTDAC_ClearInterrupt.
*
* \image html ctdac_update_mode_buffered_write.png
* \image latex ctdac_update_mode_buffered_write.png
*
* \subsection group_ctdac_updatemode_strobe_edge_sync Strobe edge sync
*
* In this mode, the user writes to the CTDAC_VAL_NXT register using
* \ref Cy_CTDAC_SetValueBuffered.
* Each rising edge of the DSI strobe input enables
* one subsequent update from the next rising edge of the clock. The DSI
* input must remain high for two PeriClk cycles and go low for
* another two PeriClk cycles to allow for the next update.
* This restricts the DSI strobe input frequency to the PeriClk frequency divided by four.
*
* \image html ctdac_update_mode_strobe_edge_sync.png
* \image latex ctdac_update_mode_strobe_edge_sync.png
*
* \subsection group_ctdac_updatemode_strobe_edge_immediate Strobe edge immediate
*
* In this mode, the user writes to the CTDAC_VAL_NXT register using
* \ref Cy_CTDAC_SetValueBuffered.
* The clock resource is used but set to a logic high.
* Therefore, each rising edge of the DSI strobe input immediately
* updates the DAC output.
*
* \image html ctdac_update_mode_strobe_edge_immediate.png
* \image latex ctdac_update_mode_strobe_edge_immediate.png
*
* \subsection group_ctdac_updatemode_strobe_level Strobe level
*
* In this mode, the user writes to the CTDAC_VAL_NXT register using
* \ref Cy_CTDAC_SetValueBuffered.
* The DSI strobe input acts as a hardware enable signal.
* While the DSI strobe input is high, the mode behaves
* like the Buffered write mode. When the DSI strobe input is low,
* updates are disabled.
*
* \image html ctdac_update_mode_strobe_level.png
* \image latex ctdac_update_mode_strobe_level.png
*
* \section group_ctdac_dacmode DAC Modes
*
* The format of code stored in the CTDAC_VAL register can either be unsigned
* or signed two's complemented.
* Only the first 12 bits of the register are used by the DAC so there is
* no need for sign extension. With the signed format, the DAC decodes
* the code in the register by adding 0x800.
* The DAC can output the register value or the register value plus 1 (see \ref Cy_CTDAC_SetOutputMode).
*
* <table class="doxtable">
*   <tr>
*     <th>12-bit unsigned code</th>
*     <th>12-bit two's complement signed code</th>
*     <th>Vout (for \ref CY_CTDAC_OUTPUT_VALUE )</th>
*     <th>Vout (for \ref CY_CTDAC_OUTPUT_VALUE_PLUS1 )</th>
*   </tr>
*   <tr>
*     <td>0x000</td>
*     <td>0x800</td>
*     <td>0</td>
*     <td>Vref/4096</td>
*   </tr>
*   <tr>
*     <td>0x800</td>
*     <td>0x000</td>
*     <td>0.5 * Vref</td>
*     <td>Vref * 2049 / 4096</td>
*   </tr>
*   <tr>
*     <td>0xFFF</td>
*     <td>0x7FF</td>
*     <td>Vref * 4095 / 4096</td>
*     <td>Vref</td>
*   </tr>
* </table>
*
* The expressions in the above table are based on an unbuffered DAC output.
* When the output is buffered, the input and output range of the buffer will affect the
* output voltage. See \ref group_ctb_opamp_range in the CTB driver for more information.
*
* \section group_ctdac_trigger Interrupts and Trigger
*
* When data from the CTDAC_VAL_NXT is transferred to the CTDAC_VAL register,
* an interrupt and trigger output are generated. The trigger output can be
* used with a DMA block to update the CTDAC value register at high speeds without any CPU intervention.
* Alternatively, the interrupt output can be used when DMA is not available
* to update the CTDAC value register, but at a slower speed.
*
* Recall with the \ref group_ctdac_updatemode, the interrupt and trigger output are available in all modes except
* \ref group_ctdac_updatemode_direct_write.
*
* \subsection group_ctdac_dma_trigger DMA Trigger
*
* The CTDAC trigger output signal can be routed to a DMA block using the \ref group_trigmux
* to trigger an update to the CTDAC_VAL_NXT register.
* When making the required \ref Cy_TrigMux_Connect calls, use the pre-defined enums, TRIG14_IN_PASS_TR_CTDAC_EMPTY
* and TRIGGER_TYPE_PASS_TR_CTDAC_EMPTY.
*
* \subsection group_ctdac_handling_interrupts Handling Interrupts
*
* The following code snippet demonstrates how to implement a routine to handle the interrupt.
* The routine gets called when any CTDAC on the device generates an interrupt.
*
* \snippet ctdac_sut_01.cydsn/main_cm0p.c SNIPPET_CTDAC_ISR
*
* The following code snippet demonstrates how to configure and enable the interrupt.
*
* \snippet ctdac_sut_01.cydsn/main_cm0p.c SNIPPET_CTDAC_INTR_SETUP
*
* \snippet ctdac_sut_01.cydsn/main_cm4.c CTDAC_SNIPPET_DMA_TRIGGER
*
* \section group_ctdac_deglitch Deglitch
*
* The hardware has the ability to deglitch the output value every time it is updated.
* This prevents small glitches in the DAC output during an update to propagate to
* the pin or opamp input. When deglitch is enabled, a switch on the output path
* is forced open for a configurable number of PeriClk cycles. This deglitch time
* is calculated as:
*
*       (DEGLITCH_CNT + 1) / PERI_CLOCK_FREQ
*
* The optimal and recommended deglitch time is 700 ns. Call \ref Cy_CTDAC_SetDeglitchCycles to set DEGLITCH_CNT.
*
* There are two switches used for deglitching.
* - Switch COS in the CTB between the DAC output and the Opamp0 input
* - Switch CO6 in the CTDAC between the DAC output and external pin
*
* Call \ref Cy_CTDAC_SetDeglitchMode to set the deglitch path. Match this with the output buffer selection.
* If the output is buffered through the CTB, select \ref CY_CTDAC_DEGLITCHMODE_BUFFERED.
* If the output is unbuffered to a direct pin, select \ref CY_CTDAC_DEGLITCHMODE_UNBUFFERED.
*
* \note
* If deglitching is enabled, the hardware does not force the deglitch switches into a closed
* state during Deep Sleep mode. Therefore, there is a chance that the device enters
* Deep Sleep mode while the hardware is deglitching and the switches on the output path remain open.
* To ensure the DAC will operate properly in Deep Sleep when enabled, make sure to
* register the \ref Cy_CTDAC_DeepSleepCallback before entering Deep Sleep mode.
*
* \section group_ctdac_sample_hold Sample and Hold
*
* When buffering the DAC output, the CTB has a Sample and Hold (SH) feature that can be used for saving power.
* The DAC output voltage is retained on an internal capacitor for a duration of time while the
* DAC output can be turned off. The DAC hardware needs to be turned on in a periodic fashion
* to recharge the hold capacitor. This feature is firmware controlled using a sequence of function calls.
* See \ref Cy_CTB_DACSampleAndHold in the \ref group_ctb_sample_hold "CTB" driver.
*
* The hold time depends on the supply and reference voltages. The following hold times are based on the
* time it takes for the buffered output to change by 1 LSB.
*
*   - Hold time = 750 us @ Vref = VDDA , VDDA = 1.7 V
*   - Hold time = 525 us @ Vref = VDDA , VDDA = 3.6 V
*   - Hold time = 200 us @ Vref = 1.2 V, VDDA = 3.6 V
*
* \section group_ctdac_low_power Low Power Support
*
* The CTDAC driver provides a callback function to handle power mode transitions.
* If the CTDAC is configured for Deep Sleep operation and \ref group_ctdac_deglitch "deglitching" is enabled,
* the callback \ref Cy_CTDAC_DeepSleepCallback must be registered before calling
* \ref Cy_SysPm_DeepSleep.
* Refer to \ref group_syspm driver for more information about power mode transitions and
* callback registration.
*
* \section group_ctdac_more_information More Information
*
* Refer to the technical reference manual (TRM) and the device datasheet.
*
* \section group_ctdac_MISRA MISRA-C Compliance]
*
* This driver has the following specific deviations:
*
* <table class="doxtable">
*   <tr>
*     <th>MISRA Rule</th>
*     <th>Rule Class (Required/Advisory)</th>
*     <th>Rule Description</th>
*     <th>Description of Deviation(s)</th>
*   </tr>
*   <tr>
*     <td>11.4</td>
*     <td>Advisory</td>
*     <td>A cast should not be performed between a pointer to object type and a different pointer to object type.</td>
*     <td>The cy_syspm driver defines the pointer to void in the \ref cy_stc_syspm_callback_params_t.base field.
*       This CTDAC driver implements a Deep Sleep callback conforming to the cy_syspm driver requirements.
*       When the callback is called, the base is cast to a pointer to CTDAC_Type.
*     </td>
*   </tr>
* </table>
*
* \section group_ctdac_changelog Changelog
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td>1.10</td>
*     <td>Flattened the driver source code organization into the single
*         source directory and the single include directory
*     </td>
*     <td>Simplified the driver library directory structure</td>
*   </tr>
*   <tr>
*     <td>1.0.1</td>
*     <td>Added low power support section. Minor documentation edits.</td>
*     <td>Documentation update and clarification</td>
*   </tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*
* \defgroup group_ctdac_macros Macros
* \defgroup group_ctdac_functions Functions
*   \{
*       \defgroup group_ctdac_functions_init            Initialization Functions
*       \defgroup group_ctdac_functions_basic           Basic Configuration Functions
*       \defgroup group_ctdac_functions_switches        Switch Control Functions
*       \defgroup group_ctdac_functions_interrupts      Interrupt Functions
*       \defgroup group_ctdac_functions_syspm_callback  Low Power Callback
*   \}
* \defgroup group_ctdac_globals Global Variables
* \defgroup group_ctdac_data_structures Data Structures
* \defgroup group_ctdac_enums Enumerated Types
*/

#if !defined(CY_CTDAC_H)
#define CY_CTDAC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_syspm.h"
#include "cy_syslib.h"
#include "cy_sysclk.h"

#ifdef CY_IP_MXS40PASS_CTDAC

#if defined(__cplusplus)
extern "C" {
#endif

/** \addtogroup group_ctdac_macros
* \{
*/

/** Driver major version */
#define CY_CTDAC_DRV_VERSION_MAJOR       1

/** Driver minor version */
#define CY_CTDAC_DRV_VERSION_MINOR       10

/** CTDAC driver identifier */
#define CY_CTDAC_ID                       CY_PDL_DRV_ID(0x19u)

#define CY_CTDAC_DEINIT                         (0uL)                                                                   /**< De-init value for CTDAC registers */
#define CY_CTDAC_UNSIGNED_MID_CODE_VALUE        (0x800uL)                                                               /**< Middle code value for unsigned values */
#define CY_CTDAC_UNSIGNED_MAX_CODE_VALUE        (0xFFFuL)                                                               /**< Maximum code value for unsigned values */
#define CY_CTDAC_FAST_CLKCFG_TYPE               CY_SYSCLK_DIV_8_BIT                                                     /**< Clock divider type for quick clock setup */
#define CY_CTDAC_FAST_CLKCFG_NUM                (0uL)                                                                   /**< Clock divider number for quick clock setup */
#define CY_CTDAC_FAST_CLKCFG_DIV                (99uL)                                                                  /**< Clock divider integer value for quick clock setup. Divides PERI clock by 100. */

/** \cond INTERNAL */
#define CY_CTDAC_DEINT_CTDAC_SW                 (CTDAC_CTDAC_SW_CLEAR_CTDD_CVD_Msk | CTDAC_CTDAC_SW_CLEAR_CTDO_CO6_Msk) /**< Mask for de-initializing the CTDAC switch control register */
#define CY_CTDAC_STROBE_EDGE_IMMEDIATE_DIV      (0uL)                                                                   /**< Clock divider value for the Strobe Edge Immediate update mode */
#define CY_CTDAC_STROBE_EDGE_IMMEDIATE_DIV_FRAC (0uL)                                                                   /**< Clock fractional divider value for the Strobe Edge Immediate update mode */
#define CY_CTDAC_DEGLITCH_CYCLES_MAX            (63uL)

/**< Macros for conditions used by CY_ASSERT calls */
#define CY_CTDAC_REFSOURCE(source)                  (((source) == CY_CTDAC_REFSOURCE_EXTERNAL) || ((source) == CY_CTDAC_REFSOURCE_VDDA))
#define CY_CTDAC_FORMAT(mode)                       (((mode) == CY_CTDAC_FORMAT_UNSIGNED) || ((mode) == CY_CTDAC_FORMAT_SIGNED))
#define CY_CTDAC_UPDATE(mode)                       ((mode) <= CY_CTDAC_UPDATE_STROBE_LEVEL)
#define CY_CTDAC_DEGLITCH(mode)                     (((mode) == CY_CTDAC_DEGLITCHMODE_NONE) \
                                                    || ((mode) == CY_CTDAC_DEGLITCHMODE_UNBUFFERED) \
                                                    || ((mode) == CY_CTDAC_DEGLITCHMODE_BUFFERED) \
                                                    || ((mode) == CY_CTDAC_DEGLITCHMODE_BOTH))
#define CY_CTDAC_OUTPUTMODE(mode)                   (((mode) == CY_CTDAC_OUTPUT_HIGHZ) \
                                                    || ((mode) == CY_CTDAC_OUTPUT_VALUE) \
                                                    || ((mode) == CY_CTDAC_OUTPUT_VALUE_PLUS1) \
                                                    || ((mode) == CY_CTDAC_OUTPUT_VSSA) \
                                                    || ((mode) == CY_CTDAC_OUTPUT_VREF))
#define CY_CTDAC_OUTPUTBUFFER(buffer)               (((buffer) == CY_CTDAC_OUTPUT_UNBUFFERED) || ((buffer) == CY_CTDAC_OUTPUT_BUFFERED))
#define CY_CTDAC_DEEPSLEEP(deepSleep)               (((deepSleep) == CY_CTDAC_DEEPSLEEP_DISABLE) || ((deepSleep) == CY_CTDAC_DEEPSLEEP_ENABLE))
#define CY_CTDAC_DEGLITCHCYCLES(cycles)             ((cycles) <= CY_CTDAC_DEGLITCH_CYCLES_MAX)
#define CY_CTDAC_SWITCHMASK(mask)                   ((mask) <= (uint32_t) (CY_CTDAC_SWITCH_CVD_MASK | CY_CTDAC_SWITCH_CO6_MASK))
#define CY_CTDAC_SWITCHSTATE(state)                 (((state) == CY_CTDAC_SWITCH_OPEN) || ((state) == CY_CTDAC_SWITCH_CLOSE))
#define CY_CTDAC_INTRMASK(mask)                     (((mask) == 0uL) || ((mask) == 1uL))
/** \endcond */

/** \} group_ctdac_macros */

/***************************************
*       Enumerated Types
***************************************/

/**
* \addtogroup group_ctdac_enums
* \{
*/

/**
* Configure the mode for how the DAC value is updated.
* All the modes require a CTDAC clock except for \ref group_ctdac_updatemode_direct_write.
*/
typedef enum {
    CY_CTDAC_UPDATE_DIRECT_WRITE            = 0uL,  /**< DAC value is updated with a direct write by calling to \ref Cy_CTDAC_SetValue */
    CY_CTDAC_UPDATE_BUFFERED_WRITE          = 1uL,  /**< DAC value stored with \ref Cy_CTDAC_SetValueBuffered is updated on the next CTDAC clock edge */
    CY_CTDAC_UPDATE_STROBE_EDGE_SYNC        = 2uL,  /**< DAC value stored with \ref Cy_CTDAC_SetValueBuffered is updated on the next CTDAC clock edge after a rising edge of the strobe */
    CY_CTDAC_UPDATE_STROBE_EDGE_IMMEDIATE   = 3uL,  /**< DAC value stored with \ref Cy_CTDAC_SetValueBuffered is updated on the rising edge of the strobe input */
    CY_CTDAC_UPDATE_STROBE_LEVEL            = 4uL   /**< DAC value stored with \ref Cy_CTDAC_SetValueBuffered is updated on every CTDAC clock edge while the strobe line is high */
}cy_en_ctdac_update_t;

/**
* Configure the format in which the DAC value register is decoded.
*/
typedef enum {
    CY_CTDAC_FORMAT_UNSIGNED    = 0uL,                                         /**< Unsigned 12-bit DAC. No value decoding */
    CY_CTDAC_FORMAT_SIGNED      = 1uL << CTDAC_CTDAC_CTRL_CTDAC_MODE_Pos       /**< Virtual signed. Add 0x800 to the 12-bit DAC value */
}cy_en_ctdac_format_t;

/**
* Enable or disable the CTDAC hardware during Deep Sleep.
*/
typedef enum {
    CY_CTDAC_DEEPSLEEP_DISABLE  = 0uL,                                  /**< DAC is disabled during Deep Sleep power mode */
    CY_CTDAC_DEEPSLEEP_ENABLE   = CTDAC_CTDAC_CTRL_DEEPSLEEP_ON_Msk     /**< DAC remains enabled during Deep Sleep power mode */
}cy_en_ctdac_deep_sleep_t;

/**
* Configure the output state of the CTDAC.
*/
typedef enum {
    CY_CTDAC_OUTPUT_HIGHZ           = 0uL,                                  /**< DAC output is tri-state */
    CY_CTDAC_OUTPUT_VALUE           = CTDAC_CTDAC_CTRL_OUT_EN_Msk,          /**< DAC Output is enabled and drives the programmed value */
    CY_CTDAC_OUTPUT_VALUE_PLUS1     = CTDAC_CTDAC_CTRL_OUT_EN_Msk \
                                      | CTDAC_CTDAC_CTRL_CTDAC_RANGE_Msk,   /**< DAC Output enabled and drives the programmed value plus 1 */
    CY_CTDAC_OUTPUT_VSSA            = CTDAC_CTDAC_CTRL_DISABLED_MODE_Msk,   /**< Output is pulled to Vssa through a 1.1 MOhm (typ) resistor  */
    CY_CTDAC_OUTPUT_VREF            = CTDAC_CTDAC_CTRL_DISABLED_MODE_Msk \
                                      | CTDAC_CTDAC_CTRL_CTDAC_RANGE_Msk    /**< Output is pulled to Vref through a 1.1 MOhm (typ) resistor */
}cy_en_ctdac_output_mode_t;

/**
* Configure the deglitch mode. See the \ref group_ctdac_deglitch section for
* more information on how deglitching works.
*/
typedef enum {
    CY_CTDAC_DEGLITCHMODE_NONE          = 0uL,                                  /**< Disable deglitch */
    CY_CTDAC_DEGLITCHMODE_UNBUFFERED    = CTDAC_CTDAC_CTRL_DEGLITCH_CO6_Msk,    /**< Deglitch through the CO6 switch */
    CY_CTDAC_DEGLITCHMODE_BUFFERED      = CTDAC_CTDAC_CTRL_DEGLITCH_COS_Msk,    /**< Deglitch through the CTB COS switch */
    CY_CTDAC_DEGLITCHMODE_BOTH          = CTDAC_CTDAC_CTRL_DEGLITCH_COS_Msk \
                                          | CTDAC_CTDAC_CTRL_DEGLITCH_CO6_Msk   /**< Deglitch through both CO6 and CTB COS switches */
}cy_en_ctdac_deglitch_t;

/**
* Configure the reference source for the CTDAC
*
* The CVD switch is closed when Vdda is the reference source.
*/
typedef enum {
    CY_CTDAC_REFSOURCE_EXTERNAL     = 0uL,        /**< Use an external source from Opamp1 of the CTB as the reference. CVD switch is open. */
    CY_CTDAC_REFSOURCE_VDDA         = 1uL         /**< Use Vdda as the reference. CVD switch is closed. */
}cy_en_ctdac_ref_source_t;

/** Configure the output to be buffered or unbuffered
*
* The CO6 switch is closed when the output is unbuffered to Pin 6 of the CTDAC port.
* See the device datasheet for the CTDAC port.
*/
typedef enum {
    CY_CTDAC_OUTPUT_BUFFERED        = 0uL,      /**< Buffer the output through the CTB OA0 */
    CY_CTDAC_OUTPUT_UNBUFFERED      = 1uL       /**< Send output to a direct pin */
}cy_en_ctdac_output_buffer_t;

/** Switch state, either open or closed, to be used in \ref Cy_CTDAC_SetAnalogSwitch. */
typedef enum
{
    CY_CTDAC_SWITCH_OPEN      = 0uL,    /**< Open the switch */
    CY_CTDAC_SWITCH_CLOSE     = 1uL     /**< Close the switch */
}cy_en_ctdac_switch_state_t;

/** Switch mask to be used in \ref Cy_CTDAC_SetAnalogSwitch */
typedef enum
{
    CY_CTDAC_SWITCH_CVD_MASK     = CTDAC_CTDAC_SW_CTDD_CVD_Msk,    /**< Switch for the reference source, Vdda or external */
    CY_CTDAC_SWITCH_CO6_MASK     = CTDAC_CTDAC_SW_CTDO_CO6_Msk     /**< Switch for the output, buffered or direct */
}cy_en_ctdac_switches_t;

/** Return states for \ref Cy_CTDAC_Init, \ref Cy_CTDAC_DeInit, and \ref Cy_CTDAC_FastInit */
typedef enum {
    CY_CTDAC_SUCCESS    = 0x00uL,                                         /**< Initialization completed successfully */
    CY_CTDAC_BAD_PARAM  = CY_CTDAC_ID | CY_PDL_STATUS_ERROR | 0x01uL      /**< Input pointers were NULL and Initialization could not be completed */
}cy_en_ctdac_status_t;

/** \} group_ctdac_enums */

/***************************************
*       Configuration Structures
***************************************/

/**
* \addtogroup group_ctdac_data_structures
* \{
*/

/** Configuration structure to set up the entire CTDAC block to be used with \ref Cy_CTDAC_Init
*/
typedef struct
{
    cy_en_ctdac_ref_source_t    refSource;          /**< Reference source: Vdda or externally through Opamp1 of CTB */
    cy_en_ctdac_format_t        formatMode;         /**< Format of DAC value: signed or unsigned */
    cy_en_ctdac_update_t        updateMode;         /**< Update mode: direct or buffered writes or hardware, edge or level */
    cy_en_ctdac_deglitch_t      deglitchMode;       /**< Deglitch mode: disabled, buffered, unbuffered, or both */
    cy_en_ctdac_output_mode_t   outputMode;         /**< Output mode: enabled (value or value + 1), high-z, Vssa, or Vdda */
    cy_en_ctdac_output_buffer_t outputBuffer;       /**< Output path: Buffered through Opamp0 of CTB or connected directly to Pin 6 */
    cy_en_ctdac_deep_sleep_t    deepSleep;          /**< Enable or disable the CTDAC during Deep Sleep */
    uint32_t                    deglitchCycles;     /**< Number of deglitch cycles from 0 to 63 */
    int32_t                     value;              /**< Current DAC value */
    int32_t                     nextValue;          /**< Next DAC value for double buffering */
    bool                        enableInterrupt;    /**< If true, enable interrupt when next value register is transferred to value register */

    /* Configuring the clock */
    bool                        configClock;        /**< Configure or ignore clock information */
    cy_en_divider_types_t       dividerType;        /**< Specifies which type of divider to use. Can be integer or fractional divider. Not used if updateMode is \ref CY_CTDAC_UPDATE_DIRECT_WRITE */
    uint32_t                    dividerNum;         /**< Specifies which divider of the selected type to configure. Not used if updateMode is \ref CY_CTDAC_UPDATE_DIRECT_WRITE  */
    uint32_t                    dividerIntValue;    /**< The integer divider value. The divider value causes integer division of (divider value + 1). Not used if updateMode is \ref CY_CTDAC_UPDATE_DIRECT_WRITE or \ref CY_CTDAC_UPDATE_STROBE_EDGE_IMMEDIATE */
    uint32_t                    dividerFracValue;   /**< The fractional divider value if using a fractional clock. Not used if updateMode is \ref CY_CTDAC_UPDATE_DIRECT_WRITE or \ref CY_CTDAC_UPDATE_STROBE_EDGE_IMMEDIATE */
}cy_stc_ctdac_config_t;

/** Configuration structure to quickly set up the CTDAC to be used with \ref Cy_CTDAC_FastInit
* This structure provides a selection for the CTDAC reference source and output path.
*
* The other configuration options are set to:
*   - .formatMode       = \ref CY_CTDAC_FORMAT_UNSIGNED
*   - .updateMode       = \ref CY_CTDAC_UPDATE_BUFFERED_WRITE
*   - .deglitchMode     = \ref CY_CTDAC_DEGLITCHMODE_NONE
*   - .outputMode       = \ref CY_CTDAC_OUTPUT_VALUE
*   - .deepSleep        = \ref CY_CTDAC_DEEPSLEEP_DISABLE
*   - .deglitchCycles   = \ref CY_CTDAC_DEINIT
*   - .value            = \ref CY_CTDAC_UNSIGNED_MID_CODE_VALUE
*   - .nextValue        = \ref CY_CTDAC_UNSIGNED_MID_CODE_VALUE
*   - .enableInterrupt  = true
*   - .configClock      = true
*   - .dividerType      = \ref CY_CTDAC_FAST_CLKCFG_TYPE
*   - .dividerNum       = \ref CY_CTDAC_FAST_CLKCFG_NUM
*   - .dividerInitValue = \ref CY_CTDAC_FAST_CLKCFG_DIV
*   - .dividerFracValue = \ref CY_CTDAC_DEINIT
*/
typedef struct
{
    cy_en_ctdac_ref_source_t    refSource;          /**< Reference source: Vdda or externally through Opamp1 of CTB */
    cy_en_ctdac_output_buffer_t outputBuffer;       /**< Output path: Buffered through Opamp0 of CTB or connected directly to Pin 6 */
}cy_stc_ctdac_fast_config_t;

/** \} group_ctdac_data_structures */

/** \addtogroup group_ctdac_globals
* \{
*/
/***************************************
*        Global Variables
***************************************/

/** Configure CTDAC to use Vdda reference and output unbuffered. See \ref Cy_CTDAC_FastInit. */
extern const cy_stc_ctdac_fast_config_t Cy_CTDAC_Fast_VddaRef_UnbufferedOut;

/** Configure CTDAC to use Vdda reference and output buffered through Opamp0 of CTB. See \ref Cy_CTDAC_FastInit.
*
* To quickly configure Opamp0, call with \ref Cy_CTB_FastInit
* with \ref Cy_CTB_Fast_Opamp0_Vdac_Out or \ref Cy_CTB_Fast_Opamp0_Vdac_Out_SH.
*/
extern const cy_stc_ctdac_fast_config_t Cy_CTDAC_Fast_VddaRef_BufferedOut;

/** Configure CTDAC to use a buffered reference from Opamp1 of CTB
* and output unbuffered. See \ref Cy_CTDAC_FastInit.
*
* To use the reference from the Analog Reference (AREF),
* call \ref Cy_CTB_FastInit with \ref Cy_CTB_Fast_Opamp1_Vdac_Ref_Aref.
*
* To use an external reference from a GPIO,
* call \ref Cy_CTB_FastInit with \ref Cy_CTB_Fast_Opamp1_Vdac_Ref_Pin5
* for Pin 5 on the CTB port.
*/
extern const cy_stc_ctdac_fast_config_t Cy_CTDAC_Fast_OA1Ref_UnbufferedOut;

/** Configure CTDAC to use a buffered reference from Opamp1 of CTB
* and output buffered through Opamp0 of CTB. See \ref Cy_CTDAC_FastInit.
*
* To quickly configure Opamp0, call with \ref Cy_CTB_FastInit
* with \ref Cy_CTB_Fast_Opamp0_Vdac_Out or \ref Cy_CTB_Fast_Opamp0_Vdac_Out_SH.
*
* To use the reference from the Analog Reference (AREF),
* call \ref Cy_CTB_FastInit with \ref Cy_CTB_Fast_Opamp1_Vdac_Ref_Aref.
*
* To use an external reference from a GPIO,
* call \ref Cy_CTB_FastInit with \ref Cy_CTB_Fast_Opamp1_Vdac_Ref_Pin5
* for Pins 5 on the CTB port.
*/
extern const cy_stc_ctdac_fast_config_t Cy_CTDAC_Fast_OA1Ref_BufferedOut;

/** \} group_ctdac_globals */

/***************************************
*        Function Prototypes
***************************************/

/**
* \addtogroup group_ctdac_functions
* \{
*/

/**
* \addtogroup group_ctdac_functions_init
* This set of functions are for initializing, enabling, and disabling the CTDAC.
* \{
*/
cy_en_ctdac_status_t Cy_CTDAC_Init(CTDAC_Type *base, const cy_stc_ctdac_config_t *config);
cy_en_ctdac_status_t Cy_CTDAC_DeInit(CTDAC_Type *base, bool deInitRouting);
cy_en_ctdac_status_t Cy_CTDAC_FastInit(CTDAC_Type *base, const cy_stc_ctdac_fast_config_t *config);
__STATIC_INLINE void Cy_CTDAC_Enable(CTDAC_Type *base);
__STATIC_INLINE void Cy_CTDAC_Disable(CTDAC_Type *base);
/** \} */

/**
* \addtogroup group_ctdac_functions_basic
* This set of functions are for configuring basic usage of the CTDAC.
* \{
*/
__STATIC_INLINE void Cy_CTDAC_SetValue(CTDAC_Type *base, int32_t value);
__STATIC_INLINE void Cy_CTDAC_SetValueBuffered(CTDAC_Type *base, int32_t value);
void Cy_CTDAC_SetSignMode(CTDAC_Type *base, cy_en_ctdac_format_t formatMode);
void Cy_CTDAC_SetDeepSleepMode(CTDAC_Type *base, cy_en_ctdac_deep_sleep_t deepSleep);
void Cy_CTDAC_SetOutputMode(CTDAC_Type *base, cy_en_ctdac_output_mode_t outputMode);
void Cy_CTDAC_SetDeglitchMode(CTDAC_Type *base, cy_en_ctdac_deglitch_t deglitchMode);
void Cy_CTDAC_SetDeglitchCycles(CTDAC_Type *base, uint32_t deglitchCycles);
void Cy_CTDAC_SetRef(CTDAC_Type *base, cy_en_ctdac_ref_source_t refSource);
/** \} */

/** \addtogroup group_ctdac_functions_switches
*
* This set of functions is for controlling the two CTDAC analog switches, CVD, and CO6.
* These are advanced functions. The switches will be managed by the reference
* source and output mode selections when initializing the hardware.
* \{
*/
void Cy_CTDAC_SetAnalogSwitch(CTDAC_Type *base, uint32_t switchMask, cy_en_ctdac_switch_state_t state);
__STATIC_INLINE uint32_t Cy_CTDAC_GetAnalogSwitch(const CTDAC_Type *base);
__STATIC_INLINE void Cy_CTDAC_SetSwitchCO6(CTDAC_Type *base, cy_en_ctdac_switch_state_t state);
__STATIC_INLINE void Cy_CTDAC_OpenAllSwitches(CTDAC_Type *base);
/** \} */

/** \addtogroup group_ctdac_functions_interrupts
* This set of functions is related to the CTDAC interrupt
* \{
*/
__STATIC_INLINE uint32_t Cy_CTDAC_GetInterruptStatus(const CTDAC_Type *base);
__STATIC_INLINE void Cy_CTDAC_ClearInterrupt(CTDAC_Type *base);
__STATIC_INLINE void Cy_CTDAC_SetInterrupt(CTDAC_Type *base);
__STATIC_INLINE void Cy_CTDAC_SetInterruptMask(CTDAC_Type *base, uint32_t mask);
__STATIC_INLINE uint32_t Cy_CTDAC_GetInterruptMask(const CTDAC_Type *base);
__STATIC_INLINE uint32_t Cy_CTDAC_GetInterruptStatusMasked(const CTDAC_Type *base);
/** \} */

/** \addtogroup group_ctdac_functions_syspm_callback
* This driver supports one SysPm callback for Deep Sleep transition.
* \{
*/
cy_en_syspm_status_t Cy_CTDAC_DeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams);
/** \} */

/**
* \addtogroup group_ctdac_functions_init
* \{
*/
/*******************************************************************************
* Function Name: Cy_CTDAC_Enable
****************************************************************************//**
*
* Power up the CTDAC hardware block.
*
* \param base
* Pointer to structure describing registers
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_CTDAC_Enable(CTDAC_Type *base)
{
    CTDAC_CTDAC_CTRL(base) |= CTDAC_CTDAC_CTRL_ENABLED_Msk;
}

/*******************************************************************************
* Function Name: Cy_CTDAC_Disable
****************************************************************************//**
*
* Turn off the hardware block.
*
* \param base
* Pointer to structure describing registers
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_CTDAC_Disable(CTDAC_Type *base)
{
    CTDAC_CTDAC_CTRL(base) &= ~CTDAC_CTDAC_CTRL_ENABLED_Msk;
}
/** \} */

/**
* \addtogroup group_ctdac_functions_basic
* \{
*/
/*******************************************************************************
* Function Name: Cy_CTDAC_SetValue
****************************************************************************//**
*
* Set the CTDAC_VAL register (DAC hardware is
* updated on the next PeriClk cycle).  Only the least significant 12 bits
* have an effect. Sign extension of negative values is unnecessary and is
* ignored by the hardware. The way in which the CTDAC interprets the 12-bit
* data is controlled by \ref Cy_CTDAC_SetSignMode.
*
* \note
* Call this function only when the update mode is set to \ref group_ctdac_updatemode_direct_write.
* Calling this function for any other update mode will not have the intended effect.
*
* \param base
* Pointer to structure describing registers
*
* \param value
* Value to write into the CTDAC_VAL register
*
* \return None
*
* \funcusage
*
* \snippet ctdac_sut_01.cydsn/main_cm4.c CTDAC_SNIPPET_SET_VALUE
*
*******************************************************************************/
__STATIC_INLINE void Cy_CTDAC_SetValue(CTDAC_Type *base, int32_t value)
{
    CTDAC_CTDAC_VAL(base) = (((uint32_t)value) << CTDAC_CTDAC_VAL_VALUE_Pos) & CTDAC_CTDAC_VAL_VALUE_Msk;
}

/*******************************************************************************
* Function Name: Cy_CTDAC_SetValueBuffered
****************************************************************************//**
*
* Set the CTDAC_VAL_NEXT register. The value is transferred
* to the CTDAC_VAL register on the next edge of the CTDAC clock.
* Only the least significant 12 bits
* have an effect. Sign extension of negative values is unnecessary and is
* ignored by the hardware. The way in which the CTDAC interprets the 12-bit
* data is controlled by \ref Cy_CTDAC_SetSignMode.
*
* \note
* Calling this function in \ref group_ctdac_updatemode_direct_write mode will not update the DAC output.
* Call this function for all modes that use buffered values (i.e. uses a clock).
*
* \param base
* Pointer to structure describing registers
*
* \param value
* Value to write into the CTDAC_VAL_NEXT register
*
* \return None
*
* \funcusage
*
* \snippet ctdac_sut_01.cydsn/main_cm4.c CTDAC_SNIPPET_SET_VALUE_BUFFERED
*
*******************************************************************************/
__STATIC_INLINE void Cy_CTDAC_SetValueBuffered(CTDAC_Type *base, int32_t value)
{
    CTDAC_CTDAC_VAL_NXT(base) = (((uint32_t)value) << CTDAC_CTDAC_VAL_NXT_VALUE_Pos) & CTDAC_CTDAC_VAL_NXT_VALUE_Msk;
}
/** \} */

/**
* \addtogroup group_ctdac_functions_switches
* \{
*/
/*******************************************************************************
* Function Name: Cy_CTDAC_GetAnalogSwitch
****************************************************************************//**
*
* Return the state (open or close) of the CTDAC switches.
*
* \note
* The switches will be managed by the reference
* source and output mode selections when initializing the hardware.
*
* \param base
* Pointer to structure describing registers
*
* \return
* Switch state. Compare this value to the masks found in \ref cy_en_ctdac_switches_t.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_CTDAC_GetAnalogSwitch(const CTDAC_Type *base)
{
    return CTDAC_CTDAC_SW(base);
}

/*******************************************************************************
* Function Name: Cy_CTDAC_SetSwitchCO6
****************************************************************************//**
*
* Open or close switch CO6 that controls whether the output gets routed
* directly to a pin or through Opamp0 of the CTB. This function calls
* \ref Cy_CTDAC_SetAnalogSwitch with the switchMask set to \ref CY_CTDAC_SWITCH_CO6_MASK.
*
* \note
* The switches is configured by the output mode selections during initialization.
*
* \note
* This switch will temporarily
* be opened for deglitching if the degitch mode is \ref CY_CTDAC_DEGLITCHMODE_UNBUFFERED or
* \ref CY_CTDAC_DEGLITCHMODE_BOTH.
*
* \param base
* Pointer to structure describing registers
*
* \param state
* State of the switch, open or close.
*
* \return None
*
* \funcusage
*
* \snippet ctdac_sut_01.cydsn/main_cm4.c CTDAC_SNIPPET_SET_SWITCH_CO6
*
*******************************************************************************/
__STATIC_INLINE void Cy_CTDAC_SetSwitchCO6(CTDAC_Type *base, cy_en_ctdac_switch_state_t state)
{
    Cy_CTDAC_SetAnalogSwitch(base, (uint32_t) CY_CTDAC_SWITCH_CO6_MASK, state);
}

/*******************************************************************************
* Function Name: Cy_CTDAC_OpenAllSwitches
****************************************************************************//**
*
* Open all switches in the CTDAC (CO6 and CVD).
*
* \param base
* Pointer to structure describing registers
*
* \return None
*
* \funcusage
*
* \snippet ctdac_sut_01.cydsn/main_cm4.c CTDAC_SNIPPET_OPEN_ALL_SWITCHES
*
*******************************************************************************/
__STATIC_INLINE void Cy_CTDAC_OpenAllSwitches(CTDAC_Type *base)
{
    CTDAC_CTDAC_SW_CLEAR(base) = CY_CTDAC_DEINT_CTDAC_SW;
}

/** \} */

/**
* \addtogroup group_ctdac_functions_interrupts
* \{
*/
/*******************************************************************************
* Function Name: Cy_CTDAC_GetInterruptStatus
****************************************************************************//**
*
* Return the interrupt status which gets set by the hardware
* when the CTDAC_VAL_NXT register value is transferred to the CTDAC_VAL register.
* Once set, the CTDAC_VAL_NXT register is ready to accept a new value.
*
* \note
* Interrupts are available in all update modes except \ref group_ctdac_updatemode_direct_write.
*
* \param base
* Pointer to structure describing registers
*
* \return
* - 0: Value not moved from CTDAC_VAL_NXT to CTDAC_VAL
* - 1: Value moved from CTDAC_VAL_NXT to CTDAC_VAL
*
* \funcusage
*
* \snippet ctdac_sut_01.cydsn/main_cm0p.c SNIPPET_CTDAC_GET_INTERRUPT_STATUS
*
*******************************************************************************/
__STATIC_INLINE  uint32_t Cy_CTDAC_GetInterruptStatus(const CTDAC_Type *base)
{
    return (CTDAC_INTR(base) & CTDAC_INTR_VDAC_EMPTY_Msk) >> CTDAC_INTR_VDAC_EMPTY_Pos;
}

/*******************************************************************************
* Function Name: Cy_CTDAC_ClearInterrupt
****************************************************************************//**
*
* Clear the interrupt that was set by the hardware when the
* CTDAC_VAL_NXT register value is transferred to the CTDAC_VAL register.
* The interrupt must be cleared with this function so that
* the hardware can set subsequent interrupts and those interrupts
* can be forwarded to the interrupt controller, if enabled.
*
* \note
* Interrupts are available in all update modes except \ref group_ctdac_updatemode_direct_write.
*
* \param base
* Pointer to structure describing registers
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_CTDAC_ClearInterrupt(CTDAC_Type *base)
{
    CTDAC_INTR(base) = CTDAC_INTR_VDAC_EMPTY_Msk;

    /* Dummy read for buffered writes. */
    (void) CTDAC_INTR(base);
}

/*******************************************************************************
* Function Name: Cy_CTDAC_SetInterrupt
****************************************************************************//**
*
* Force the CTDAC interrupt to trigger using software.
*
* \note
* Interrupts are available in all update modes except \ref group_ctdac_updatemode_direct_write.
*
* \param base
* Pointer to structure describing registers
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_CTDAC_SetInterrupt(CTDAC_Type *base)
{
    CTDAC_INTR_SET(base) = CTDAC_INTR_SET_VDAC_EMPTY_SET_Msk;
}

/*******************************************************************************
* Function Name: Cy_CTDAC_SetInterruptMask
****************************************************************************//**
*
* Configure the CTDAC interrupt to be forwarded to the CPU interrupt
* controller.
*
* \note
* Interrupts are available in all update modes except \ref group_ctdac_updatemode_direct_write.
*
* \param base
* Pointer to structure describing registers
*
* \param mask
* The CTDAC only has one interrupt so the mask is one bit.
* - 0: Disable CTDAC interrupt request (will not be forwarded to CPU interrupt controller)
* - 1: Enable CTDAC interrupt request (will be forwarded to CPU interrupt controller)
*
* \return None
*
* \funcusage
*
* \snippet ctdac_sut_01.cydsn/main_cm0p.c SNIPPET_CTDAC_SET_INTERRUPT_MASK
*
*******************************************************************************/
__STATIC_INLINE void Cy_CTDAC_SetInterruptMask(CTDAC_Type *base, uint32_t mask)
{
    CY_ASSERT_L2(CY_CTDAC_INTRMASK(mask));

    CTDAC_INTR_MASK(base) = mask & CTDAC_INTR_MASK_VDAC_EMPTY_MASK_Msk;
}

/*******************************************************************************
* Function Name: Cy_CTDAC_GetInterruptMask
****************************************************************************//**
*
* Return whether the CTDAC interrupt is
* forwarded to the CPU interrupt controller
* as configured by \ref Cy_CTDAC_SetInterruptMask.
*
* \note
* Interrupts are available in all update modes except \ref group_ctdac_updatemode_direct_write.
*
* \param base
* Pointer to structure describing registers
*
* \return
* The CTDAC only has one interrupt so the return value is either 0 or 1.
* - 0: Interrupt output not forwarded to CPU interrupt controller
* - 1: Interrupt output forwarded to CPU interrupt controller
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_CTDAC_GetInterruptMask(const CTDAC_Type *base)
{
    return (CTDAC_INTR_MASK(base) & CTDAC_INTR_MASK_VDAC_EMPTY_MASK_Msk) >> CTDAC_INTR_MASK_VDAC_EMPTY_MASK_Pos;
}

/*******************************************************************************
* Function Name: Cy_CTDAC_GetInterruptStatusMasked
****************************************************************************//**
*
* Return the bitwise AND of \ref Cy_CTDAC_GetInterruptStatus and
* \ref Cy_CTDAC_SetInterruptMask. When high, the DAC interrupt is
* asserted and the interrupt is forwarded to the CPU interrupt
* controller.
*
* \note
* Interrupts are available in all update modes except \ref group_ctdac_updatemode_direct_write.
*
* \param base
* Pointer to structure describing registers
*
* \return
* - 0: Value not moved from CTDAC_VAL_NXT to CTDAC_VAL or not masked
* - 1: Value moved from CTDAC_VAL_NXT to CTDAC_VAL and masked
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_CTDAC_GetInterruptStatusMasked(const CTDAC_Type *base){
    return (CTDAC_INTR_MASKED(base) & CTDAC_INTR_MASKED_VDAC_EMPTY_MASKED_Msk) >> CTDAC_INTR_MASKED_VDAC_EMPTY_MASKED_Pos;
}

/** \} */

/** \} group_ctdac_functions */

#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXS40PASS_CTDAC */

#endif /** !defined(CY_CTDAC_H) */

/** \} group_ctdac */

/* [] END OF FILE */

