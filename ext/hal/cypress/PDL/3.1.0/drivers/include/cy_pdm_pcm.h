/***************************************************************************//**
* \file cy_pdm_pcm.h
* \version 2.20
*
* The header file of the PDM_PCM driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \defgroup group_pdm_pcm PDM-PCM Converter (PDM_PCM)
* \{
*
* The pulse-density modulation to pulse-code modulation (PDM-PCM) driver provides an
* API to manage PDM-PCM conversion. A PDM-PCM converter is used
* to convert 1-bit digital audio streaming data to PCM data.
*
* Features:
* * Supports FIFO buffer for Incoming Data
* * Supports Software Mute Mode
* * Programmable Gain Settings
* * Programmable Word Length
*
* Pulse-density modulation, or PDM, represents
* an analog signal with a binary signal. In a PDM signal, specific amplitude values
* are not encoded into codewords of pulses of different weight as they would be
* in pulse-code modulation (PCM); rather, the relative density of the pulses corresponds
* to the analog signal's amplitude. The output of a 1-bit DAC is the same
* as the PDM encoding of the signal.
*
* Pulse-code modulation (PCM) is the method used to digitally represent sampled analog signals.
* It is the standard form of digital audio in computers, compact discs, digital telephony,
* and other digital audio applications. In a PCM stream, the amplitude of the analog signal
* is sampled regularly at uniform intervals, and each sample is quantized
* to the nearest value within a range of digital steps.
*
* \section group_pdm_pcm_configuration_considerations Configuration Considerations
*
* To set up a PDM-PCM, provide the configuration parameters in the
* \ref cy_stc_pdm_pcm_config_t structure.
*
* For example, set dataStreamingEnable to true, configure rxFifoTriggerLevel,
* dmaTriggerEnable (depending on whether DMA is going to be used),
* provide clock settings (clkDiv, mclkDiv and ckoDiv), set sincDecRate
* to the appropriate decimation rate, wordLen, and wordBitExtension.
* No other parameters are necessary for this example.
*
* To initialize the PDM-PCM block, call the \ref Cy_PDM_PCM_Init function, providing the
* filled \ref cy_stc_pdm_pcm_config_t structure.
*
* If you use a DMA, the DMA channel should be previously configured. PDM-PCM interrupts
* (if applicable) can be enabled by calling \ref Cy_PDM_PCM_SetInterruptMask.
*
* For example, if the trigger interrupt is used during operation, the ISR
* should call the \ref Cy_PDM_PCM_ReadFifo as many times as required for your
* FIFO payload. Then call \ref Cy_PDM_PCM_ClearInterrupt with appropriate parameters.
*
* If a DMA is used and the DMA channel is properly configured, no CPU activity
* (or application code) is needed for PDM-PCM operation.
*
* \section group_pdm_pcm_more_information More Information
* See: the PDM-PCM chapter of the device technical reference manual (TRM);
*      the PDM_PCM_PDL Component datasheet;
*      CE219431 - PSOC 6 MCU PDM-TO-PCM EXAMPLE.
*
* \section group_pdm_pcm_MISRA MISRA-C Compliance
* The PDM-PCM driver has the following specific deviations:
* <table class="doxtable">
*   <tr>
*     <th>MISRA Rule</th>
*     <th>Rule Class (Required/Advisory)</th>
*     <th>Rule Description</th>
*     <th>Description of Deviation(s)</th>
*   </tr>
*   <tr>
*     <td>10.3</td>
*     <td>R</td>
*     <td>A composite expression of the "essentially unsigned" type is
*         cast to a different type category.</td>
*     <td>The value got from the bitfield physically can't exceed the enumeration
*         that describes this bitfield. So the code is safe by design.</td>
*   </tr>
*   <tr>
*     <td>11.4</td>
*     <td>A</td>
*     <td>A cast should not be performed between a pointer to the object type and
*         a different pointer to the object type.</td>
*     <td>The function \ref Cy_I2S_DeepSleepCallback is a callback of
*         \ref cy_en_syspm_status_t type. The cast operation safety in this
*         function becomes the user responsibility because the pointer is
*         initialized when a callback is registered in SysPm driver.</td>
*   </tr>
* </table>
*
* \section group_pdm_pcm_changelog Changelog
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td>2.20</td>
*     <td>Static Library support
*         Flattened the driver source code organization into the single
*         source directory and the single include directory
*     </td>
*     <td>Simplified the driver library directory structure</td>
*   </tr>
*   <tr>
*     <td>2.10</td>
*     <td>The gain values in range +4.5...+10.5dB (5 items) of /ref cy_en_pdm_pcm_gain_t are corrected. 
*         Added Low Power Callback section.</td>
*     <td>Incorrect setting of gain values in limited range.
*         Documentation update and clarification.</td>
*   </tr>
*   <tr>
*     <td>2.0</td>
*     <td>Enumeration types for gain and soft mute cycles are added.<br>
*         Function parameter checks are added.<br>        
*         The next functions are removed:
*         * Cy_PDM_PCM_EnterLowPowerCallback
*         * Cy_PDM_PCM_ExitLowPowerCallback
*         * Cy_PDM_PCM_EnableDataStream
*         * Cy_PDM_PCM_DisableDataStream
*         * Cy_PDM_PCM_SetFifoLevel
*         * Cy_PDM_PCM_GetFifoLevel
*         * Cy_PDM_PCM_EnableDmaRequest
*         * Cy_PDM_PCM_DisableDmaRequest
*
*         The next functions behaviour are modified:
*         * Cy_PDM_PCM_Enable
*         * Cy_PDM_PCM_Disable
*         * Cy_PDM_PCM_SetInterruptMask
*         * Cy_PDM_PCM_GetInterruptMask
*         * Cy_PDM_PCM_GetInterruptStatusMasked
*         * Cy_PDM_PCM_GetInterruptStatus
*         * Cy_PDM_PCM_ClearInterrupt
*         * Cy_PDM_PCM_SetInterrupt
*
*         The Cy_PDM_PCM_GetFifoNumWords function is renamed to Cy_PDM_PCM_GetNumInFifo.<br>
*         The Cy_PDM_PCM_GetCurrentState function is added.
*     </td>
*     <td>Improvements based on usability feedbacks.<br>
*         API is reworked for consistency within the PDL.
*     </td>
*   </tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*
* \defgroup group_pdm_pcm_macros Macros
* \defgroup group_pdm_pcm_functions Functions
*   \{
*       \defgroup group_pdm_pcm_functions_syspm_callback  Low Power Callback
*   \}
* \defgroup group_pdm_pcm_data_structures Data Structures
* \defgroup group_pdm_pcm_enums Enumerated Types
*
*/

#if !defined(CY_PDM_PCM_H__)
#define CY_PDM_PCM_H__

/******************************************************************************/
/* Include files                                                              */
/******************************************************************************/

#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_syslib.h"
#include "cy_syspm.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef CY_IP_MXAUDIOSS

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************
 * Global definitions
 ******************************************************************************/

/* Macros */
/**
* \addtogroup group_pdm_pcm_macros
* \{
*/

/** The driver major version */
#define CY_PDM_PCM_DRV_VERSION_MAJOR       2

/** The driver minor version */
#define CY_PDM_PCM_DRV_VERSION_MINOR       20

/** The PDM-PCM driver identifier */
#define CY_PDM_PCM_ID                       CY_PDL_DRV_ID(0x26u)

/**
* \defgroup group_pdm_pcm_macros_intrerrupt_masks Interrupt Masks
* \{
*/

/** Bit 16: More entries in the RX FIFO than specified by Trigger Level. */
#define CY_PDM_PCM_INTR_RX_TRIGGER         (PDM_INTR_RX_TRIGGER_Msk)
/** Bit 18: RX FIFO is not empty. */
#define CY_PDM_PCM_INTR_RX_NOT_EMPTY       (PDM_INTR_RX_NOT_EMPTY_Msk)
/** Bit 21: Attempt to write to a full RX FIFO. */
#define CY_PDM_PCM_INTR_RX_OVERFLOW        (PDM_INTR_RX_OVERFLOW_Msk)
/** Bit 22: Attempt to read from an empty RX FIFO. */
#define CY_PDM_PCM_INTR_RX_UNDERFLOW       (PDM_INTR_RX_UNDERFLOW_Msk)

/** \} group_pdm_pcm_macros_intrerrupt_masks */

/** \} group_pdm_pcm_macros */

/**
* \addtogroup group_pdm_pcm_enums
* \{
*/

/** PDM Word Length. */
typedef enum
{
    CY_PDM_PCM_WLEN_16_BIT     = 0U,         /**< Word length: 16 bit. */
    CY_PDM_PCM_WLEN_18_BIT     = 1U,         /**< Word length: 18 bit. */
    CY_PDM_PCM_WLEN_20_BIT     = 2U,         /**< Word length: 20 bit. */
    CY_PDM_PCM_WLEN_24_BIT     = 3U          /**< Word length: 24 bit. */
} cy_en_pdm_pcm_word_len_t;

/** PDM Clock Divider. */
typedef enum
{
    CY_PDM_PCM_CLK_DIV_BYPASS  = 0U,         /**< Clock 1/1. */
    CY_PDM_PCM_CLK_DIV_1_2     = 1U,         /**< Clock 1/2 (no 50% duty cycle). */
    CY_PDM_PCM_CLK_DIV_1_3     = 2U,         /**< Clock 1/3 (no 50% duty cycle). */
    CY_PDM_PCM_CLK_DIV_1_4     = 3U          /**< Clock 1/4 (no 50% duty cycle). */
} cy_en_pdm_pcm_clk_div_t;

/** PDM Output Mode. */
typedef enum
{
    CY_PDM_PCM_OUT_CHAN_LEFT   = 1U,         /**< Channel mono left. */
    CY_PDM_PCM_OUT_CHAN_RIGHT  = 2U,         /**< Channel mono right. */
    CY_PDM_PCM_OUT_STEREO      = 3U          /**< Channel stereo. */
} cy_en_pdm_pcm_out_t;

/** PDM Channel selector. */
typedef enum
{
    CY_PDM_PCM_CHAN_LEFT       = 0U,         /**< Channel left. */
    CY_PDM_PCM_CHAN_RIGHT      = 1U          /**< Channel right. */
} cy_en_pdm_pcm_chan_select_t;

/** PDM Gain. */
typedef enum
{
    CY_PDM_PCM_ATTN_12_DB      = 0U,         /**< -12 dB (attenuation). */
    CY_PDM_PCM_ATTN_10_5_DB    = 1U,         /**< -10.5 dB (attenuation). */
    CY_PDM_PCM_ATTN_9_DB       = 2U,         /**< -9 dB (attenuation). */
    CY_PDM_PCM_ATTN_7_5_DB     = 3U,         /**< -7.5 dB (attenuation). */
    CY_PDM_PCM_ATTN_6_DB       = 4U,         /**< -6 dB (attenuation). */
    CY_PDM_PCM_ATTN_4_5_DB     = 5U,         /**< -4.5 dB (attenuation). */
    CY_PDM_PCM_ATTN_3_DB       = 6U,         /**< -3 dB (attenuation). */
    CY_PDM_PCM_ATTN_1_5_DB     = 7U,         /**< -1.5 dB (attenuation). */
    CY_PDM_PCM_BYPASS          = 8U,         /**< 0 dB (bypass). */
    CY_PDM_PCM_GAIN_1_5_DB     = 9U,         /**< +1.5 dB (amplification). */
    CY_PDM_PCM_GAIN_3_DB       = 10U,        /**< +3 dB (amplification). */
    CY_PDM_PCM_GAIN_4_5_DB     = 11U,        /**< +4.5 dB (amplification). */
    CY_PDM_PCM_GAIN_6_DB       = 12U,        /**< +6 dB (amplification). */
    CY_PDM_PCM_GAIN_7_5_DB     = 13U,        /**< +7.5 dB (amplification). */
    CY_PDM_PCM_GAIN_9_DB       = 14U,        /**< +9 dB (amplification). */
    CY_PDM_PCM_GAIN_10_5_DB    = 15U         /**< +10.5 dB (amplification). */
} cy_en_pdm_pcm_gain_t;


/** The time step for gain change during PGA or soft mute operation in
 *  number of 1/a sampling rate. */
typedef enum
{
    CY_PDM_PCM_SOFT_MUTE_CYCLES_64  = 0U,    /**< 64 steps. */
    CY_PDM_PCM_SOFT_MUTE_CYCLES_96  = 1U,    /**< 96 steps. */
    CY_PDM_PCM_SOFT_MUTE_CYCLES_128 = 2U,    /**< 128 steps. */
    CY_PDM_PCM_SOFT_MUTE_CYCLES_160 = 3U,    /**< 160 steps. */
    CY_PDM_PCM_SOFT_MUTE_CYCLES_192 = 4U,    /**< 192 steps. */
    CY_PDM_PCM_SOFT_MUTE_CYCLES_256 = 5U,    /**< 256 steps. */
    CY_PDM_PCM_SOFT_MUTE_CYCLES_384 = 6U,    /**< 384 steps. */
    CY_PDM_PCM_SOFT_MUTE_CYCLES_512 = 7U     /**< 512 steps. */
} cy_en_pdm_pcm_s_cycles_t;

/** The PDM-PCM status codes. */
typedef enum
{
    CY_PDM_PCM_SUCCESS         = 0x00UL,    /**< Success status code */
    CY_PDM_PCM_BAD_PARAM       = CY_PDM_PCM_ID | CY_PDL_STATUS_ERROR | 0x01UL /**< Bad parameter status code */
} cy_en_pdm_pcm_status_t;

/** \} group_pdm_pcm_enums */


/**
* \addtogroup group_pdm_pcm_data_structures
* \{
*/

/******************************************************************************
 * Global type definitions
 ******************************************************************************/

/** PDM-PCM initialization configuration */
typedef struct
{
    cy_en_pdm_pcm_clk_div_t  clkDiv;              /**< PDM Clock Divider (1st divider), see #cy_en_pdm_pcm_clk_div_t
                                                    This configures a frequency of PDM CLK. The configured frequency
                                                    is used to operate PDM core. I.e. the frequency is input to
                                                    MCLKQ_CLOCK_DIV register. */
    cy_en_pdm_pcm_clk_div_t  mclkDiv;             /**< MCLKQ divider (2nd divider), see #cy_en_pdm_pcm_clk_div_t */
    uint8_t                  ckoDiv;              /**< PDM CKO (FPDM_CKO) clock divider (3rd divider):
                                                    - if CKO_CLOCK_DIV >= 1 - *F(PDM_CKO) = F(PDM_CLK / (mclkDiv + 1))
                                                    - if CKO_CLOCK_DIV  = 0 - *F(PDM_CKO) = MCLKQ / 2 */
    uint8_t                  ckoDelay;            /**< Extra PDM_CKO delay to internal sampler:
                                                    - 0: Three extra PDM_CLK period advance
                                                    - 1: Two extra PDM_CLK period advance
                                                    - 2: One extra PDM_CLK period advance
                                                    - 3: No delay
                                                    - 4: One extra PDM_CLK period delay
                                                    - 5: Two extra PDM_CLK period delay
                                                    - 6: Three extra PDM_CLK period delay
                                                    - 7: Four extra PDM_CLK clock delay */
    uint8_t                  sincDecRate;         /**< F(MCLK_L_R) = Fs * 2 * sincDecRate * mclkDiv,
                                                    Fs is a sampling frequency, 8kHz - 48kHz */
    cy_en_pdm_pcm_out_t      chanSelect;          /**< see #cy_en_pdm_pcm_out_t */
    bool                     chanSwapEnable;      /**< Audio channels swapping */
    uint8_t                  highPassFilterGain;  /**< High pass filter gain:
                                                    H(Z) = (1 - Z^-1) / (1 - (1 - 2^highPassFilterGain) * Z^-1) */
    bool                     highPassDisable;     /**< High pass filter disable */
    cy_en_pdm_pcm_s_cycles_t softMuteCycles;      /**< The time step for gain change during PGA or soft mute operation in
                                                    number of 1/a sampling rate, see #cy_en_pdm_pcm_s_cycles_t. */
    uint32_t                 softMuteFineGain;    /**< Soft mute fine gain: 0 = 0.13dB, 1 = 0.26dB */
    bool                     softMuteEnable;      /**< Soft mute enable */
    cy_en_pdm_pcm_word_len_t wordLen;             /**< see #cy_en_pdm_pcm_word_len_t */
    bool                     signExtension;       /**< Word extension type:
                                                    - 0: extension by zero
                                                    - 1: extension by sign bits */
    cy_en_pdm_pcm_gain_t     gainLeft;            /**< Gain for left channel, see #cy_en_pdm_pcm_gain_t */
    cy_en_pdm_pcm_gain_t     gainRight;           /**< Gain for right channel, see #cy_en_pdm_pcm_gain_t */
    uint8_t                  rxFifoTriggerLevel;  /**< Fifo interrupt trigger level (in words), 
                                                    range: 0 - 253 for stereo and 0 - 254 for mono mode */
    bool                     dmaTriggerEnable;    /**< DMA trigger enable */
    uint32_t                 interruptMask;       /**< Interrupts enable mask */
} cy_stc_pdm_pcm_config_t;

/** \} group_pdm_pcm_data_structures */


/** \cond INTERNAL */
/******************************************************************************
 * Local definitions
*******************************************************************************/
/** Define bit mask for all available interrupt sources */
#define CY_PDM_PCM_INTR_MASK        (CY_PDM_PCM_INTR_RX_TRIGGER   | \
                                     CY_PDM_PCM_INTR_RX_NOT_EMPTY | \
                                     CY_PDM_PCM_INTR_RX_OVERFLOW  | \
                                     CY_PDM_PCM_INTR_RX_UNDERFLOW)

/* Non-zero default values */
#define CY_PDM_PCM_CTL_PGA_R_DEFAULT    (0x8U)
#define CY_PDM_PCM_CTL_PGA_L_DEFAULT    (0x8U)
#define CY_PDM_PCM_CTL_STEP_SEL_DEFAULT (0x1U)

#define CY_PDM_PCM_CTL_DEFAULT (_VAL2FLD(PDM_CTL_PGA_R, CY_PDM_PCM_CTL_PGA_R_DEFAULT) | \
                                _VAL2FLD(PDM_CTL_PGA_L, CY_PDM_PCM_CTL_PGA_L_DEFAULT) | \
                                _VAL2FLD(PDM_CTL_STEP_SEL, CY_PDM_PCM_CTL_STEP_SEL_DEFAULT))

#define CY_PDM_PCM_CLOCK_CTL_MCLKQ_CLOCK_DIV_DEFAULT (0x1U)
#define CY_PDM_PCM_CLOCK_CTL_CKO_CLOCK_DIV_DEFAULT   (0x3U)
#define CY_PDM_PCM_CLOCK_CTL_SINC_RATE_DEFAULT       (0x20U)

#define CY_PDM_PCM_CLOCK_CTL_DEFAULT (_VAL2FLD(PDM_CLOCK_CTL_MCLKQ_CLOCK_DIV, CY_PDM_PCM_CLOCK_CTL_MCLKQ_CLOCK_DIV_DEFAULT) | \
                                      _VAL2FLD(PDM_CLOCK_CTL_CKO_CLOCK_DIV, CY_PDM_PCM_CLOCK_CTL_CKO_CLOCK_DIV_DEFAULT)     | \
                                      _VAL2FLD(PDM_CLOCK_CTL_SINC_RATE, CY_PDM_PCM_CLOCK_CTL_SINC_RATE_DEFAULT))

#define CY_PDM_PCM_MODE_CTL_PCM_CH_SET_DEFAULT (0x3U)
#define CY_PDM_PCM_MODE_CTL_S_CYCLES_DEFAULT   (0x1U)
#define CY_PDM_PCM_MODE_CTL_HPF_GAIN_DEFAULT   (0xBU)
#define CY_PDM_PCM_MODE_CTL_HPF_EN_N_DEFAULT   (0x1U)

#define CY_PDM_PCM_MODE_CTL_DEFAULT (_VAL2FLD(PDM_MODE_CTL_PCM_CH_SET, CY_PDM_PCM_MODE_CTL_PCM_CH_SET_DEFAULT) | \
                                     _VAL2FLD(PDM_MODE_CTL_S_CYCLES, CY_PDM_PCM_MODE_CTL_S_CYCLES_DEFAULT)     | \
                                     _VAL2FLD(PDM_MODE_CTL_HPF_GAIN, CY_PDM_PCM_MODE_CTL_HPF_GAIN_DEFAULT)     | \
                                     _VAL2FLD(PDM_MODE_CTL_HPF_EN_N, CY_PDM_PCM_MODE_CTL_HPF_EN_N_DEFAULT))

/* Macros for conditions used by CY_ASSERT calls */
#define CY_PDM_PCM_IS_CLK_DIV_VALID(clkDiv)    (((clkDiv) == CY_PDM_PCM_CLK_DIV_BYPASS) || \
                                                ((clkDiv) == CY_PDM_PCM_CLK_DIV_1_2)    || \
                                                ((clkDiv) == CY_PDM_PCM_CLK_DIV_1_3)    || \
                                                ((clkDiv) == CY_PDM_PCM_CLK_DIV_1_4))

#define CY_PDM_PCM_IS_CH_SET_VALID(chanSelect) (((chanSelect) == CY_PDM_PCM_OUT_CHAN_LEFT)  || \
                                                ((chanSelect) == CY_PDM_PCM_OUT_CHAN_RIGHT) || \
                                                ((chanSelect) == CY_PDM_PCM_OUT_STEREO))

#define CY_PDM_PCM_IS_GAIN_VALID(gain)         (((gain) == CY_PDM_PCM_ATTN_12_DB)   || \
                                                ((gain) == CY_PDM_PCM_ATTN_10_5_DB) || \
                                                ((gain) == CY_PDM_PCM_ATTN_9_DB)    || \
                                                ((gain) == CY_PDM_PCM_ATTN_7_5_DB)  || \
                                                ((gain) == CY_PDM_PCM_ATTN_6_DB)    || \
                                                ((gain) == CY_PDM_PCM_ATTN_4_5_DB)  || \
                                                ((gain) == CY_PDM_PCM_ATTN_3_DB)    || \
                                                ((gain) == CY_PDM_PCM_ATTN_1_5_DB)  || \
                                                ((gain) == CY_PDM_PCM_BYPASS)       || \
                                                ((gain) == CY_PDM_PCM_GAIN_1_5_DB)  || \
                                                ((gain) == CY_PDM_PCM_GAIN_3_DB)    || \
                                                ((gain) == CY_PDM_PCM_GAIN_4_5_DB)  || \
                                                ((gain) == CY_PDM_PCM_GAIN_6_DB)    || \
                                                ((gain) == CY_PDM_PCM_GAIN_7_5_DB)  || \
                                                ((gain) == CY_PDM_PCM_GAIN_9_DB)    || \
                                                ((gain) == CY_PDM_PCM_GAIN_10_5_DB))

#define CY_PDM_PCM_IS_WORD_LEN_VALID(wordLen)  (((wordLen) == CY_PDM_PCM_WLEN_16_BIT) || \
                                                ((wordLen) == CY_PDM_PCM_WLEN_18_BIT) || \
                                                ((wordLen) == CY_PDM_PCM_WLEN_20_BIT) || \
                                                ((wordLen) == CY_PDM_PCM_WLEN_24_BIT))

#define CY_PDM_PCM_IS_CHAN_VALID(chan)         (((chan) == CY_PDM_PCM_CHAN_LEFT) || \
                                                ((chan) == CY_PDM_PCM_CHAN_RIGHT))
                                                
#define CY_PDM_PCM_IS_S_CYCLES_VALID(sCycles)  (((sCycles) == CY_PDM_PCM_SOFT_MUTE_CYCLES_64)  || \
                                                ((sCycles) == CY_PDM_PCM_SOFT_MUTE_CYCLES_96)  || \
                                                ((sCycles) == CY_PDM_PCM_SOFT_MUTE_CYCLES_128) || \
                                                ((sCycles) == CY_PDM_PCM_SOFT_MUTE_CYCLES_160) || \
                                                ((sCycles) == CY_PDM_PCM_SOFT_MUTE_CYCLES_192) || \
                                                ((sCycles) == CY_PDM_PCM_SOFT_MUTE_CYCLES_256) || \
                                                ((sCycles) == CY_PDM_PCM_SOFT_MUTE_CYCLES_384) || \
                                                ((sCycles) == CY_PDM_PCM_SOFT_MUTE_CYCLES_512))
                                                
#define CY_PDM_PCM_IS_INTR_MASK_VALID(interrupt)  (0UL == ((interrupt) & ((uint32_t) ~CY_PDM_PCM_INTR_MASK)))
#define CY_PDM_PCM_IS_SINC_RATE_VALID(sincRate)   ((sincRate) <= 127U)
#define CY_PDM_PCM_IS_STEP_SEL_VALID(stepSel)     ((stepSel)  <= 1UL)
#define CY_PDM_PCM_IS_CKO_DELAY_VALID(ckoDelay)   ((ckoDelay) <= 7U)
#define CY_PDM_PCM_IS_HPF_GAIN_VALID(hpfGain)     ((hpfGain)  <= 15U)
#define CY_PDM_PCM_IS_CKO_CLOCK_DIV_VALID(ckoDiv) (((ckoDiv)  >= 1U) && ((ckoDiv) <= 15U))
#define CY_PDM_PCM_IS_TRIG_LEVEL(trigLevel, chanSelect) ((trigLevel) <= (((chanSelect) == CY_PDM_PCM_OUT_STEREO)? 253U : 254U))
/** \endcond */

/**
* \addtogroup group_pdm_pcm_functions
* \{
*/

cy_en_pdm_pcm_status_t   Cy_PDM_PCM_Init(PDM_Type * base, cy_stc_pdm_pcm_config_t const * config);
                void     Cy_PDM_PCM_DeInit(PDM_Type * base);
                void     Cy_PDM_PCM_SetGain(PDM_Type * base, cy_en_pdm_pcm_chan_select_t chan, cy_en_pdm_pcm_gain_t gain);
cy_en_pdm_pcm_gain_t     Cy_PDM_PCM_GetGain(PDM_Type const * base, cy_en_pdm_pcm_chan_select_t chan);

/** \addtogroup group_pdm_pcm_functions_syspm_callback
* The driver supports SysPm callback for Deep Sleep transition.
* \{
*/
cy_en_syspm_status_t     Cy_PDM_PCM_DeepSleepCallback(cy_stc_syspm_callback_params_t * callbackParams);
/** \} */

__STATIC_INLINE void     Cy_PDM_PCM_Enable(PDM_Type * base);
__STATIC_INLINE void     Cy_PDM_PCM_Disable(PDM_Type * base);
__STATIC_INLINE void     Cy_PDM_PCM_SetInterruptMask(PDM_Type * base, uint32_t interrupt);
__STATIC_INLINE uint32_t Cy_PDM_PCM_GetInterruptMask(PDM_Type const * base);
__STATIC_INLINE uint32_t Cy_PDM_PCM_GetInterruptStatusMasked(PDM_Type const * base);
__STATIC_INLINE uint32_t Cy_PDM_PCM_GetInterruptStatus(PDM_Type const * base);
__STATIC_INLINE void     Cy_PDM_PCM_ClearInterrupt(PDM_Type * base, uint32_t interrupt);
__STATIC_INLINE void     Cy_PDM_PCM_SetInterrupt(PDM_Type * base, uint32_t interrupt);
__STATIC_INLINE uint8_t  Cy_PDM_PCM_GetNumInFifo(PDM_Type const * base);
__STATIC_INLINE void     Cy_PDM_PCM_ClearFifo(PDM_Type * base);
__STATIC_INLINE uint32_t Cy_PDM_PCM_ReadFifo(PDM_Type const * base);
__STATIC_INLINE void     Cy_PDM_PCM_EnableSoftMute(PDM_Type * base);
__STATIC_INLINE void     Cy_PDM_PCM_DisableSoftMute(PDM_Type * base);
__STATIC_INLINE void     Cy_PDM_PCM_FreezeFifo(PDM_Type * base);
__STATIC_INLINE void     Cy_PDM_PCM_UnfreezeFifo(PDM_Type * base);
__STATIC_INLINE uint32_t Cy_PDM_PCM_ReadFifoSilent(PDM_Type const * base);


/** \} group_pdm_pcm_functions */

/**
* \addtogroup group_pdm_pcm_functions
* \{
*/

/******************************************************************************
* Function Name: Cy_PDM_PCM_Enable
***************************************************************************//**
*
* Enables the PDM-PCM data conversion.
*
* \param base The pointer to the  PDM-PCM instance address.
*
******************************************************************************/
__STATIC_INLINE void Cy_PDM_PCM_Enable(PDM_Type * base)
{
    PDM_PCM_CMD(base) |= PDM_CMD_STREAM_EN_Msk;
}

/******************************************************************************
* Function Name: Cy_PDM_PCM_Disable
***************************************************************************//**
*
* Disables the PDM-PCM data conversion.
*
* \param base The pointer to the PDM-PCM instance address.
*
******************************************************************************/
__STATIC_INLINE void Cy_PDM_PCM_Disable(PDM_Type * base)
{
    PDM_PCM_CMD(base) &= (uint32_t) ~PDM_CMD_STREAM_EN_Msk;
}


/******************************************************************************
* Function Name: Cy_PDM_PCM_GetCurrentState
***************************************************************************//**
*
* Returns the current PDM-PCM state (running/stopped).
*
* \param base The pointer to the PDM-PCM instance address.
* \return The current state (CMD register).
*
******************************************************************************/
__STATIC_INLINE uint32_t Cy_PDM_PCM_GetCurrentState(PDM_Type const * base)
{
    return (PDM_PCM_CMD(base));
}


/******************************************************************************
* Function Name: Cy_PDM_PCM_SetInterruptMask
***************************************************************************//**
*
* Sets one or more PDM-PCM interrupt factor bits (sets the INTR_MASK register).
*
* \param base The pointer to the PDM-PCM instance address
* \param interrupt Interrupt bit mask \ref group_pdm_pcm_macros_intrerrupt_masks.
*
******************************************************************************/
__STATIC_INLINE void Cy_PDM_PCM_SetInterruptMask(PDM_Type * base, uint32_t interrupt)
{
    CY_ASSERT_L2(CY_PDM_PCM_IS_INTR_MASK_VALID(interrupt));
    PDM_PCM_INTR_MASK(base) = interrupt;
}


/******************************************************************************
* Function Name: Cy_PDM_PCM_GetInterruptMask
***************************************************************************//**
*
* Returns the PDM-PCM interrupt mask (a content of the INTR_MASK register).
*
* \param base The pointer to the PDM-PCM instance address.
* \return The interrupt bit mask \ref group_pdm_pcm_macros_intrerrupt_masks.
*
******************************************************************************/
__STATIC_INLINE uint32_t Cy_PDM_PCM_GetInterruptMask(PDM_Type const * base)
{
    return (PDM_PCM_INTR_MASK(base));
}


/******************************************************************************
* Function Name: Cy_PDM_PCM_GetInterruptStatusMasked
***************************************************************************//**
*
* Reports the status of enabled (masked) PDM-PCM interrupt sources.
* (an INTR_MASKED register).
*
* \param base The pointer to the PDM-PCM instance address.
* \return The interrupt bit mask \ref group_pdm_pcm_macros_intrerrupt_masks.
*
*****************************************************************************/
__STATIC_INLINE uint32_t Cy_PDM_PCM_GetInterruptStatusMasked(PDM_Type const * base)
{
    return (PDM_PCM_INTR_MASKED(base));
}


/******************************************************************************
* Function Name: Cy_PDM_PCM_GetInterruptStatus
***************************************************************************//**
*
* Reports the status of PDM-PCM interrupt sources (an INTR register).
*
* \param base The pointer to the PDM-PCM instance address.
* \return The interrupt bit mask \ref group_pdm_pcm_macros_intrerrupt_masks.
*
******************************************************************************/
__STATIC_INLINE uint32_t Cy_PDM_PCM_GetInterruptStatus(PDM_Type const * base)
{
    return (PDM_PCM_INTR(base));
}


/******************************************************************************
* Function Name: Cy_PDM_PCM_ClearInterrupt
***************************************************************************//**
*
* Clears one or more PDM-PCM interrupt statuses (sets an INTR register's bits).
*
* \param base The pointer to the PDM-PCM instance address
* \param interrupt 
*  The interrupt bit mask \ref group_pdm_pcm_macros_intrerrupt_masks.
*
******************************************************************************/
__STATIC_INLINE void Cy_PDM_PCM_ClearInterrupt(PDM_Type * base, uint32_t interrupt)
{
    CY_ASSERT_L2(CY_PDM_PCM_IS_INTR_MASK_VALID(interrupt));
    PDM_PCM_INTR(base) = interrupt;
    (void) PDM_PCM_INTR(base);
}


/******************************************************************************
* Function Name: Cy_PDM_PCM_SetInterrupt
***************************************************************************//**
*
* Sets one or more interrupt source statuses (sets an INTR_SET register).
*
* \param base The pointer to the PDM-PCM instance address.
* \param interrupt
*  The interrupt bit mask \ref group_pdm_pcm_macros_intrerrupt_masks.
*
******************************************************************************/
__STATIC_INLINE void Cy_PDM_PCM_SetInterrupt(PDM_Type * base, uint32_t interrupt)
{
    CY_ASSERT_L2(CY_PDM_PCM_IS_INTR_MASK_VALID(interrupt));
    PDM_PCM_INTR_SET(base) = interrupt;
}


/******************************************************************************
* Function Name: Cy_PDM_PCM_GetNumInFifo
***************************************************************************//**
*
* Reports the current number of used words in the output data FIFO.
*
* \param base The pointer to the PDM-PCM instance address.
* \return The current number of used FIFO words (range is 0 - 254).
*
******************************************************************************/
__STATIC_INLINE uint8_t Cy_PDM_PCM_GetNumInFifo(PDM_Type const * base)
{
    return (uint8_t) (_FLD2VAL(PDM_RX_FIFO_STATUS_USED, PDM_PCM_RX_FIFO_STATUS(base)));
}


/******************************************************************************
* Function Name: Cy_PDM_PCM_ClearFifo
***************************************************************************//**
*
* Resets the output data FIFO, removing all data words from the FIFO.
*
* \param base The pointer to the PDM-PCM instance address.
*
******************************************************************************/
__STATIC_INLINE void Cy_PDM_PCM_ClearFifo(PDM_Type * base)
{
    PDM_PCM_RX_FIFO_CTL(base) |= PDM_RX_FIFO_CTL_CLEAR_Msk; /* clear FIFO and disable it */
    PDM_PCM_RX_FIFO_CTL(base) &= (uint32_t) ~PDM_RX_FIFO_CTL_CLEAR_Msk; /* enable FIFO */
}


/******************************************************************************
* Function Name: Cy_PDM_PCM_ReadFifo
***************************************************************************//**
*
* Reads ("pops") one word from the output data FIFO.
*
* \param base The pointer to the PDM-PCM instance address.
* \return The data word.
*
******************************************************************************/
__STATIC_INLINE uint32_t Cy_PDM_PCM_ReadFifo(PDM_Type const * base)
{
    return (PDM_PCM_RX_FIFO_RD(base));
}


/******************************************************************************
* Function Name: Cy_PDM_PCM_EnableSoftMute
***************************************************************************//**
*
* Enables soft mute.
*
* \param base The pointer to the PDM-PCM instance address.
*
******************************************************************************/
__STATIC_INLINE void Cy_PDM_PCM_EnableSoftMute(PDM_Type * base)
{
    PDM_PCM_CTL(base) |= PDM_CTL_SOFT_MUTE_Msk;
}


/******************************************************************************
* Function Name: Cy_PDM_PCM_DisableSoftMute
***************************************************************************//**
*
* Disables soft mute.
*
* \param base The pointer to the PDM-PCM instance address.
*
******************************************************************************/
__STATIC_INLINE void Cy_PDM_PCM_DisableSoftMute(PDM_Type * base)
{
    PDM_PCM_CTL(base) &= (uint32_t) ~PDM_CTL_SOFT_MUTE_Msk;
}


/******************************************************************************
* Function Name: Cy_PDM_PCM_FreezeFifo
***************************************************************************//**
*
* Freezes the RX FIFO (Debug purpose).
*
* \param base The pointer to the PDM-PCM instance address.
*
******************************************************************************/
__STATIC_INLINE void Cy_PDM_PCM_FreezeFifo(PDM_Type * base)
{
    PDM_PCM_RX_FIFO_CTL(base) |= PDM_RX_FIFO_CTL_FREEZE_Msk;
}


/******************************************************************************
* Function Name: Cy_PDM_PCM_UnfreezeFifo
***************************************************************************//**
*
* Unfreezes the RX FIFO (Debug purpose).
*
* \param base The pointer to the PDM-PCM instance address.
*
******************************************************************************/
__STATIC_INLINE void Cy_PDM_PCM_UnfreezeFifo(PDM_Type * base)
{
    PDM_PCM_RX_FIFO_CTL(base) &= (uint32_t) ~PDM_RX_FIFO_CTL_FREEZE_Msk;
}


/******************************************************************************
* Function Name: Cy_PDM_PCM_ReadFifoSilent
***************************************************************************//**
*
* Reads the RX FIFO silent (without touching the FIFO function).
*
* \param base Pointer to PDM-PCM instance address.
* \return FIFO value.
*
******************************************************************************/
__STATIC_INLINE uint32_t Cy_PDM_PCM_ReadFifoSilent(PDM_Type const * base)
{
    return (PDM_PCM_RX_FIFO_RD_SILENT(base));
}

/** \} group_pdm_pcm_functions */

#ifdef __cplusplus
}
#endif  /* of __cplusplus */

#endif /* CY_IP_MXAUDIOSS */

#endif /* CY_PDM_PCM_H__ */

/** \} group_pdm_pcm */


/* [] END OF FILE */
