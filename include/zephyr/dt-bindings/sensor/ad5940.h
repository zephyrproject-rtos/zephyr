/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the Analog Devices AD5940/AD5941 impedance AFE.
 * @ingroup ad5940_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADI_AD5940_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADI_AD5940_H_

/**
 * @defgroup ad5940_interface AD5940/AD5941
 * @ingroup sensor_interface_ext_adi
 * @brief Analog Devices AD5940/AD5941 impedance and electrochemical AFE
 * @{
 */

/**
 * @name FIFO mode
 *
 * Values for the `fifo-mode` devicetree property.
 * @{
 */
/** FIFO Mode */
#define AD5940_DT_FIFO_MODE_FIFO      2
/** Stream Mode */
#define AD5940_DT_FIFO_MODE_STREAM    3
/** @} */

/**
 * @name FIFO size
 *
 * Values for the `fifo-size` devicetree property.
 * @{
 */
/** 2kB SRAM */
#define AD5940_DT_FIFO_SIZE_2KB       1
/** 4kB SRAM */
#define AD5940_DT_FIFO_SIZE_4KB       2
/** 6kB SRAM */
#define AD5940_DT_FIFO_SIZE_6KB       3
/** @} */

/**
 * @name FIFO source
 *
 * Values for the `fifo-src` devicetree property.
 * @{
 */
/** ADC Data */
#define AD5940_DT_FIFO_SRC_ADC        1
/** DFT Data */
#define AD5940_DT_FIFO_SRC_DFT        2
/** Sinc2 Filter Output */
#define AD5940_DT_FIFO_SRC_SINC2      3
/** Variance */
#define AD5940_DT_FIFO_SRC_VARIANCE   4
/** Mean Result */
#define AD5940_DT_FIFO_SRC_MEAN       5
/** @} */

/**
 * @name System clock source
 *
 * Values for the `sys-clk-src` devicetree property.
 * @{
 */
/** Internal high frequency oscillator clock. */
#define AD5940_DT_SYS_CLK_SRC_HFOSC   0
/** External high frequency crystal clock. */
#define AD5940_DT_SYS_CLK_SRC_XTAL    1
/** Internal low frequency oscillator clock. (Not Recommended) */
#define AD5940_DT_SYS_CLK_SRC_LFOSC   2
/** External clock. */
#define AD5940_DT_SYS_CLK_SRC_EXTCLK  3
/** @} */

/**
 * @name System clock divider
 *
 * Values for the `sys-clk-div` devicetree property.
 * @{
 */
/** Divide by 1 */
#define AD5940_DT_SYS_CLK_DIV_1       1
/** Divide by 2 */
#define AD5940_DT_SYS_CLK_DIV_2       2
/** @} */

/**
 * @name ADC clock source
 *
 * Values for the `adc-clk-src` devicetree property.
 * @{
 */
/** Internal high frequency oscillator clock. */
#define AD5940_DT_ADC_CLK_SRC_HFOSC   0
/** External high frequency crystal clock. */
#define AD5940_DT_ADC_CLK_SRC_XTAL    1
/** Internal low frequency oscillator clock. (Not Recommended) */
#define AD5940_DT_ADC_CLK_SRC_LFOSC   2
/** External clock. */
#define AD5940_DT_ADC_CLK_SRC_EXTCLK  3
/** @} */

/**
 * @name ADC clock divider
 *
 * Values for the `adc-clk-div` devicetree property.
 * @{
 */
/** Divide by 1 */
#define AD5940_DT_ADC_CLK_DIV_1       1
/** Divide by 2 */
#define AD5940_DT_ADC_CLK_DIV_2       2
/** @} */

/**
 * @name GPIO pin 0 function options
 *
 * Values for the `p0-func` devicetree property.
 * @{
 */
/** Interrupt 0 output. */
#define AD5940_DT_P0_FUNC_INT0       0
/** Sequence 0 trigger signal input from the MCU side. */
#define AD5940_DT_P0_FUNC_TRIG       1
/** Synchronizes External Device 0 output signal. */
#define AD5940_DT_P0_FUNC_SYNC       2
/** General-purpose input/output. */
#define AD5940_DT_P0_FUNC_GPIO       3
/** @} */

/**
 * @name GPIO pin 1 function options
 *
 * Values for the `p1-func` devicetree property.
 * @{
 */
/** General-purpose input/output. */
#define AD5940_DT_P1_FUNC_GPIO       0
/** Sequence 1 trigger signal input from the MCU side. */
#define AD5940_DT_P1_FUNC_TRIG       1
/** Synchronizes External Device 1 output signal. */
#define AD5940_DT_P1_FUNC_SYNC       2
/** Deep sleep. Sleep flag indicating that the AD5940/AD5941 is in hibernate mode. */
#define AD5940_DT_P1_FUNC_SLEEP      3
/** @} */

/**
 * @name GPIO pin 2 function options
 *
 * Values for the `p2-func` devicetree property.
 * @{
 */
/** POR signal output. */
#define AD5940_DT_P2_FUNC_POR        0
/** Sequence 2 trigger signal input from the MCU side. */
#define AD5940_DT_P2_FUNC_TRIG       1
/** Synchronizes External Device 2 output signal. */
#define AD5940_DT_P2_FUNC_SYNC       2
/** External clock input (EXTCLK). */
#define AD5940_DT_P2_FUNC_EXTCLK     3
/** @} */

/**
 * @name GPIO pin 3 function options
 *
 * Values for the `p3-func` devicetree property.
 * @{
 */
/** General-purpose input/output. */
#define AD5940_DT_P3_FUNC_GPIO       0
/** Sequence 3 trigger signal input from the MCU side. */
#define AD5940_DT_P3_FUNC_TRIG       1
/** Synchronizes External Device 3 output signal. */
#define AD5940_DT_P3_FUNC_SYNC       2
/** Interrupt 0 output. */
#define AD5940_DT_P3_FUNC_INT0       3
/** @} */

/**
 * @name GPIO pin 4 function options
 *
 * Values for the `p4-func` devicetree property.
 * @{
 */
/** General-purpose input/output. */
#define AD5940_DT_P4_FUNC_GPIO       0
/** Sequence 0 trigger signal input from the MCU side. */
#define AD5940_DT_P4_FUNC_TRIG       1
/** Synchronizes External Device 4 output signal. */
#define AD5940_DT_P4_FUNC_SYNC       2
/** Interrupt 1 output. */
#define AD5940_DT_P4_FUNC_INT1       3
/** @} */

/**
 * @name GPIO pin 5 function options
 *
 * Values for the `p5-func` devicetree property.
 * @{
 */
/** General-purpose input/output. */
#define AD5940_DT_P5_FUNC_GPIO       0
/** Sequence 1 trigger signal input from the MCU side. */
#define AD5940_DT_P5_FUNC_TRIG       1
/** Synchronizes External Device 5 output signal. */
#define AD5940_DT_P5_FUNC_SYNC       2
/** External clock input (EXTCLK). */
#define AD5940_DT_P5_FUNC_EXTCLK     3
/** @} */

/**
 * @name GPIO pin 6 function options
 *
 * Values for the `p6-func` devicetree property.
 * @{
 */
/** General-purpose input/output. */
#define AD5940_DT_P6_FUNC_GPIO       0
/** Sequence 2 trigger signal input from the MCU side. */
#define AD5940_DT_P6_FUNC_TRIG       1
/** Synchronizes External Device 6 output signal. */
#define AD5940_DT_P6_FUNC_SYNC       2
/** Interrupt 0 output. */
#define AD5940_DT_P6_FUNC_INT0       3
/** @} */

/**
 * @name GPIO pin 7 function options
 *
 * Values for the `p7-func` devicetree property.
 * @{
 */
/** General-purpose input/output. */
#define AD5940_DT_P7_FUNC_GPIO       0
/** Sequence 3 trigger signal input from the MCU side. */
#define AD5940_DT_P7_FUNC_TRIG       1
/** Synchronizes External Device 7 output signal. */
#define AD5940_DT_P7_FUNC_SYNC       2
/** Interrupt 1 output. */
#define AD5940_DT_P7_FUNC_INT1       3
/** @} */

/**
 * @name Interrupt source options
 *
 * Values for the `int-src` devicetree property.
 * @{
 */
/** ADC Result Ready Status */
#define AD5940_DT_INTSRC_ADC_RDY            0
/** DFT Result Ready Status */
#define AD5940_DT_INTSRC_DFT_RDY            1
/** SINC2/Low Pass Filter Result Status */
#define AD5940_DT_INTSRC_SINC2_RDY          2
/** Temp Sensor Result Ready */
#define AD5940_DT_INTSRC_TEMP_RDY           3
/** ADC Minimum Value Fail */
#define AD5940_DT_INTSRC_ADCMIN_FAIL        4
/** ADC Maximum Value Fail */
#define AD5940_DT_INTSRC_ADCMAX_FAIL        5
/** ADC Delta Fail */
#define AD5940_DT_INTSRC_ADCDELTA_FAIL      6
/** Mean Result Ready */
#define AD5940_DT_INTSRC_MEAN_RDY           7
/** Variance Result Ready */
#define AD5940_DT_INTSRC_VARIANCE_RDY       8
/** Custom interrupt source 0 */
#define AD5940_DT_INTSRC_CUSTOMINT0         9
/** Custom interrupt source 1 */
#define AD5940_DT_INTSRC_CUSTOMINT1         10
/** Custom interrupt source 2 */
#define AD5940_DT_INTSRC_CUSTOMINT2         11
/** Custom interrupt source 3 */
#define AD5940_DT_INTSRC_CUSTOMINT3         12
/** Bootloader done */
#define AD5940_DT_INTSRC_BOOTL_DONE         13
/** End of Sequence Interrupt */
#define AD5940_DT_INTSRC_ENDSEQ             15
/** Sequencer Timeout Command Finished */
#define AD5940_DT_INTSRC_SEQTIMEOUT_DONE    16
/** Sequencer Timeout Command Error */
#define AD5940_DT_INTSRC_SEQTIMEOUT_ERR     17
/** Data FIFO Full Interrupt */
#define AD5940_DT_INTSRC_DATA_FIFO_FULL     23
/** Data FIFO Empty Interrupt */
#define AD5940_DT_INTSRC_DATA_FIFO_EMPTY    24
/** Data FIFO Threshold Interrupt */
#define AD5940_DT_INTSRC_DATA_THRESHOLD     25
/** Data FIFO Overflow Interrupt */
#define AD5940_DT_INTSRC_DATA_FIFO_OVF      26
/** Data FIFO Underflow Interrupt */
#define AD5940_DT_INTSRC_DATA_FIFO_UNDF     27
/** Outlier Detected Interrupt */
#define AD5940_DT_INTSRC_OUTLIER            29
/** Attempt to break */
#define AD5940_DT_INTSRC_IRQ_ATTEMPT_BREAK  31
/** Enable All Interrupt */
#define AD5940_DT_INTSRC_ENABLE_ALL         32
/** @} */

/**
 * @name Interrupt polarity
 *
 * Values for the `int-pol` devicetree property.
 * @{
 */
/** Output Negative Edge Interrupt */
#define AD5940_DT_INTPOL_NEGATIVE_EDGE      0
/** Output Positive Edge Interrupt */
#define AD5940_DT_INTPOL_POSITIVE_EDGE      1
/** @} */

/**
 * @name Sequencer SRAM partition
 *
 * Values for the `seq-mem-size` devicetree property.
 * @{
 */
/** The selfbuild in 32Byte for sequencer. All 6kB SRAM can be used for data FIFO */
#define AD5940_DT_SEQ_MEM_SZ_32B            0
/** Sequencer use 2kB. The reset 4kB can be used for data FIFO */
#define AD5940_DT_SEQ_MEM_SZ_2KB            1
/** 4kB for Sequencer. 2kB for data FIFO */
#define AD5940_DT_SEQ_MEM_SZ_4KB            2
/** All 6kB for Sequencer. Build in 3 */
#define AD5940_DT_SEQ_MEM_SZ_6KB            3
/** @} */

/**
 * @name D switch selection
 *
 * Values for the `swd` devicetree property.
 * @{
 */
/** Open all D switch. */
#define AD5940_DT_SWD_OPEN_ALL                  0
/** Close switch DR0 */
#define AD5940_DT_SWD_DR0                       BIT(0)
/** Close switch D2 */
#define AD5940_DT_SWD_D2                        BIT(1)
/** Close switch D3 */
#define AD5940_DT_SWD_D3                        BIT(2)
/** Close switch D4 */
#define AD5940_DT_SWD_D4                        BIT(3)
/** Close switch D5 */
#define AD5940_DT_SWD_D5                        BIT(4)
/** Close switch D7 */
#define AD5940_DT_SWD_D7                        BIT(6)
/** Close switch D8 */
#define AD5940_DT_SWD_D8                        BIT(7)
/** @} */

/**
 * @name N switch selection
 *
 * Values for the `swn` devicetree property.
 * @{
 */
/** Open all N switches */
#define AD5940_DT_SWN_OPEN_ALL                  0
/** Close switch N1 */
#define AD5940_DT_SWN_N1                        BIT(0)
/** Close switch N2 */
#define AD5940_DT_SWN_N2                        BIT(1)
/** Close switch N3 */
#define AD5940_DT_SWN_N3                        BIT(2)
/** Close switch N4 */
#define AD5940_DT_SWN_N4                        BIT(3)
/** Close switch N5 */
#define AD5940_DT_SWN_N5                        BIT(4)
/** Close switch N6 */
#define AD5940_DT_SWN_N6                        BIT(5)
/** Close switch N7 */
#define AD5940_DT_SWN_N7                        BIT(6)
/** Close switch N8 */
#define AD5940_DT_SWN_N9                        BIT(8)
/** Close switch NR1 */
#define AD5940_DT_SWN_NR1                       BIT(9)
/** Close switch NL */
#define AD5940_DT_SWN_NL                        BIT(10)
/** Close switch NL2 */
#define AD5940_DT_SWN_NL2                       BIT(11)
/** @} */

/**
 * @name P switch selection
 *
 * Values for the `swp` devicetree property.
 * @{
 */
/** Open all P switches */
#define AD5940_DT_SWP_OPEN_ALL                  0
/** Close switch PR0 */
#define AD5940_DT_SWP_PR0                       BIT(0)
/** Close switch P2 */
#define AD5940_DT_SWP_P2                        BIT(1)
/** Close switch P3 */
#define AD5940_DT_SWP_P3                        BIT(2)
/** Close switch P4 */
#define AD5940_DT_SWP_P4                        BIT(3)
/** Close switch P5 */
#define AD5940_DT_SWP_P5                        BIT(4)
/** Close switch P6 */
#define AD5940_DT_SWP_P6                        BIT(5)
/** Close switch P7 */
#define AD5940_DT_SWP_P7                        BIT(6)
/** Close switch P8 */
#define AD5940_DT_SWP_P8                        BIT(7)
/** Close switch P9 */
#define AD5940_DT_SWP_P9                        BIT(8)
/** Close switch P11 */
#define AD5940_DT_SWP_P11                       BIT(10)
/** Close PL switch */
#define AD5940_DT_SWP_PL                        BIT(13)
/** Close PL2 switch */
#define AD5940_DT_SWP_PL2                       BIT(14)
/** @} */

/**
 * @name T switch selection
 *
 * Values for the `swt` devicetree property.
 * @{
 */
/** Open all T switches */
#define AD5940_DT_SWT_OPEN_ALL                  0
/** Close switch T1 */
#define AD5940_DT_SWT_T1                        BIT(0)
/** Close switch T2 */
#define AD5940_DT_SWT_T2                        BIT(1)
/** Close switch T3 */
#define AD5940_DT_SWT_T3                        BIT(2)
/** Close switch T4 */
#define AD5940_DT_SWT_T4                        BIT(3)
/** Close switch T5 */
#define AD5940_DT_SWT_T5                        BIT(4)
/** Close switch T6 */
#define AD5940_DT_SWT_T6                        BIT(5)
/** Close switch T7 */
#define AD5940_DT_SWT_T7                        BIT(6)
/** Close switch T9 */
#define AD5940_DT_SWT_T9                        BIT(8)
/** Close switch T10 */
#define AD5940_DT_SWT_T10                       BIT(9)
/** Close switch TR1 */
#define AD5940_DT_SWT_TR1                       BIT(11)
/** @} */

/**
 * @name HSDAC excitation buffer gain selection
 *
 * Values for the `hsdac-excitbuf-gain` devicetree property.
 * @{
 */
/** Gain of 2 */
#define AD5940_DT_HSDAC_EXCITBUF_GAIN_2         0
/** Gain of 0.25 */
#define AD5940_DT_HSDAC_EXCITBUF_GAIN_0P25      1
/** @} */

/**
 * @name HSDAC output gain selection
 *
 * Values for the `hsdac-gain` devicetree property.
 * @{
 */
/** Gain of 1 */
#define AD5940_DT_HSDAC_GAIN_1                  0
/** Gain of 0.2 */
#define AD5940_DT_HSDAC_GAIN_0P2                1
/** @} */

/**
 * @name Waveform generator DAC gain calibration selection
 *
 * Values for the `wg-dac-gain-cal` devicetree property.
 * @{
 */
/** Bypass gain calibration */
#define AD5940_DT_WG_DAC_GAIN_CAL_BYPASS        0
/** Perform gain calibration */
#define AD5940_DT_WG_DAC_GAIN_CAL_PERFORM       1
/** @} */

/**
 * @name Waveform generator DAC offset calibration selection
 *
 * Values for the `wg-dac-offset-cal` devicetree property.
 * @{
 */
/** Bypass offset calibration */
#define AD5940_DT_WG_DAC_OFFSET_CAL_BYPASS      0
/** Perform offset calibration */
#define AD5940_DT_WG_DAC_OFFSET_CAL_PERFORM     1
/** @} */

/**
 * @name Waveform generator type selection
 *
 * Values for the `wg-type` devicetree property.
 * @{
 */
/** MMR-driven waveform */
#define AD5940_DT_WG_TYPE_MMR                   0
/** Sine waveform */
#define AD5940_DT_WG_TYPE_SINE                  2
/** Trapezoidal waveform */
#define AD5940_DT_WG_TYPE_TRAPZ                 3
/** @} */

/**
 * @name HSTIA bias selection
 *
 * Values for the `hstia-bias` devicetree property.
 * @{
 */
/** Internal 1.1V */
#define AD5940_DT_HSTIA_BIAS_1P1                0
/** From LPDAC0 Vzero0 out */
#define AD5940_DT_HSTIA_BIAS_VZERO0             1
/** From LPDAC1 Vzero1 out */
#define AD5940_DT_HSTIA_BIAS_VZERO1             2
/** @} */

/**
 * @name HSTIA internal RTIA selection
 *
 * Values for the `hstia-rtia` devicetree property.
 * @{
 */
/** HSTIA Internal RTIA resistor 200 ohm */
#define AD5940_DT_HSTIA_RTIA_200                0
/** HSTIA Internal RTIA resistor 1K */
#define AD5940_DT_HSTIA_RTIA_1K                 1
/** HSTIA Internal RTIA resistor 5K */
#define AD5940_DT_HSTIA_RTIA_5K                 2
/** HSTIA Internal RTIA resistor 10K */
#define AD5940_DT_HSTIA_RTIA_10K                3
/** HSTIA Internal RTIA resistor 20K */
#define AD5940_DT_HSTIA_RTIA_20K                4
/** HSTIA Internal RTIA resistor 40K */
#define AD5940_DT_HSTIA_RTIA_40K                5
/** HSTIA Internal RTIA resistor 80K */
#define AD5940_DT_HSTIA_RTIA_80K                6
/** HSTIA Internal RTIA resistor 160K */
#define AD5940_DT_HSTIA_RTIA_160K               7
/** Open internal resistor */
#define AD5940_DT_HSTIA_RTIA_OPEN               8
/** @} */

/**
 * @name HSTIA internal CTIA selection
 *
 * Values for the `hstia-ctia` devicetree property.
 * @{
 */
/** HSTIA Internal CTIA capacitor 1pF */
#define AD5940_DT_HSTIA_CTIA_1PF                0
/** HSTIA Internal CTIA capacitor 2pF */
#define AD5940_DT_HSTIA_CTIA_2PF                1
/** HSTIA Internal CTIA capacitor 4pF */
#define AD5940_DT_HSTIA_CTIA_4PF                2
/** HSTIA Internal CTIA capacitor 8pF */
#define AD5940_DT_HSTIA_CTIA_8PF                4
/** HSTIA Internal CTIA capacitor 16pF */
#define AD5940_DT_HSTIA_CTIA_16PF               8
/** HSTIA Internal CTIA capacitor 32pF */
#define AD5940_DT_HSTIA_CTIA_32PF               16
/** @} */

/**
 * @name HSTIA DE0/DE1 external RTIA selection
 *
 * Values for the `hstia-dex-rtia` devicetree property.
 * @{
 */
/** 50 ohm */
#define AD5940_DT_HSTIA_DEX_RTIA_50             0
/** 100 ohm */
#define AD5940_DT_HSTIA_DEX_RTIA_100            1
/** 200 ohm */
#define AD5940_DT_HSTIA_DEX_RTIA_200            2
/** 1 kohm */
#define AD5940_DT_HSTIA_DEX_RTIA_1K             3
/** 5 kohm */
#define AD5940_DT_HSTIA_DEX_RTIA_5K             4
/** 10 kohm */
#define AD5940_DT_HSTIA_DEX_RTIA_10K            5
/** 20 kohm */
#define AD5940_DT_HSTIA_DEX_RTIA_20K            6
/** 40 kohm */
#define AD5940_DT_HSTIA_DEX_RTIA_40K            7
/** 80 kohm */
#define AD5940_DT_HSTIA_DEX_RTIA_80K            8
/** 160 kohm */
#define AD5940_DT_HSTIA_DEX_RTIA_160K           9
/** Use external TIA/DE resistor */
#define AD5940_DT_HSTIA_DEX_RTIA_TODE           10
/** Open internal resistor */
#define AD5940_DT_HSTIA_DEX_RTIA_OPEN           11
/** @} */

/**
 * @name HSTIA DE0/DE1 external RLOAD selection
 *
 * Values for the `hstia-dex-rload` devicetree property.
 * @{
 */
/** 0 ohm */
#define AD5940_DT_HSTIA_DEX_RLOAD_0R            0
/** 10 ohm */
#define AD5940_DT_HSTIA_DEX_RLOAD_10R           1
/** 30 ohm */
#define AD5940_DT_HSTIA_DEX_RLOAD_30R           2
/** 50 ohm */
#define AD5940_DT_HSTIA_DEX_RLOAD_50R           3
/** 100 ohm */
#define AD5940_DT_HSTIA_DEX_RLOAD_100R          4
/** Open load resistor */
#define AD5940_DT_HSTIA_DEX_RLOAD_OPEN          5
/** @} */

/**
 * @name LPDAC select selection
 *
 * Values for the `lpdac-sel` devicetree property.
 * @{
 */
/** LPDAC0 */
#define AD5940_DT_LPDAC_SEL_DAC0                0
/** LPDAC1 */
#define AD5940_DT_LPDAC_SEL_DAC1                1
/** @} */

/**
 * @name LPDAC source selection
 *
 * Values for the `lpdac-src` devicetree property.
 * @{
 */
/** MMR-driven DAC input */
#define AD5940_DT_LPDAC_SRC_MMR                 0
/** Waveform generator DAC input */
#define AD5940_DT_LPDAC_SRC_WG                  1
/** @} */

/**
 * @name LPDAC VZERO mux selection
 *
 * Values for the `lpdac-vzero-mux` devicetree property.
 * @{
 */
/** 6-bit VZERO resolution */
#define AD5940_DT_LPDAC_VZERO_6BIT               0
/** 12-bit VZERO resolution */
#define AD5940_DT_LPDAC_VZERO_12BIT              1
/** @} */

/**
 * @name LPDAC VBIAS mux selection
 *
 * Values for the `lpdac-vbias-mux` devicetree property.
 * @{
 */
/** 6-bit VBIAS resolution */
#define AD5940_DT_LPDAC_VBIAS_6BIT               0
/** 12-bit VBIAS resolution */
#define AD5940_DT_LPDAC_VBIAS_12BIT              1
/** @} */

/**
 * @name LPDAC reference selection
 *
 * Values for the `lpdac-ref` devicetree property.
 * @{
 */
/** Internal 2.5V reference */
#define AD5940_DT_LPDAC_REF_2P5                 0
/** Use AVDD as reference */
#define AD5940_DT_LPDAC_REF_AVDD                1
/** @} */

/**
 * @name LPAMP select selection
 *
 * Values for the `lpamp-sel` devicetree property.
 * @{
 */
/** LPAMP0, AMP include both LPTIA and Potentio-stat amplifiers */
#define AD5940_DT_LPAMP_SEL_LPAMP0              0
/** LPAMP1, ADuCM355 Only */
#define AD5940_DT_LPAMP_SEL_LPAMP1              1
/** @} */

/**
 * @name LPAMP TIA low-pass filter selection
 *
 * Values for the `lpamp-tia-lpf` devicetree property.
 * @{
 */
/** Disconnect Rf resistor */
#define AD5940_DT_LPAMP_TIA_LPF_OPEN            0
/** Bypass Rf resistor */
#define AD5940_DT_LPAMP_TIA_LPF_SHORT           1
/** 20kOhm Rf */
#define AD5940_DT_LPAMP_TIA_LPF_20K             2
/** Rf resistor 100kOhm */
#define AD5940_DT_LPAMP_TIA_LPF_100K            3
/** Rf resistor 200kOhm */
#define AD5940_DT_LPAMP_TIA_LPF_200K            4
/** Rf resistor 400kOhm */
#define AD5940_DT_LPAMP_TIA_LPF_400K            5
/** Rf resistor 600kOhm */
#define AD5940_DT_LPAMP_TIA_LPF_600K            6
/** Rf resistor 1MOhm */
#define AD5940_DT_LPAMP_TIA_LPF_1M              7
/** @} */

/**
 * @name LPAMP RLOAD selection
 *
 * Values for the `lpamp-tia-rload` devicetree property.
 * @{
 */
/** 0Ohm Rload */
#define AD5940_DT_LPAMP_TIA_RLOAD_SHORT         0
/** 10Ohm Rload */
#define AD5940_DT_LPAMP_TIA_RLOAD_10R           1
/** Rload resistor 30Ohm */
#define AD5940_DT_LPAMP_TIA_RLOAD_30R           2
/** Rload resistor 50Ohm */
#define AD5940_DT_LPAMP_TIA_RLOAD_50R           3
/** Rload resistor 100Ohm */
#define AD5940_DT_LPAMP_TIA_RLOAD_100R          4
/** Only available when RTIA setting >= 2KOHM */
#define AD5940_DT_LPAMP_TIA_RLOAD_1K6           5
/** Only available when RTIA setting >= 4KOHM */
#define AD5940_DT_LPAMP_TIA_RLOAD_3K1           6
/** Only available when RTIA setting >= 4KOHM */
#define AD5940_DT_LPAMP_TIA_RLOAD_3K6           7
/** @} */

/**
 * @name LPAMP TIA RTIA selection
 *
 * Values for the `lpamp-tia-rtia` devicetree property.
 * @{
 */
/** Disconnect LPTIA Internal RTIA */
#define AD5940_DT_LPAMP_TIA_RTIA_OPEN           0
/** 200Ohm Internal RTIA */
#define AD5940_DT_LPAMP_TIA_RTIA_200R           1
/** 1KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_1K             2
/** 2KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_2K             3
/** 3KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_3K             4
/** 4KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_4K             5
/** 6KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_6K             6
/** 8KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_8K             7
/** 10KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_10K            8
/** 12KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_12K            9
/** 16KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_16K            10
/** 20KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_20K            11
/** 24KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_24K            12
/** 30KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_30K            13
/** 32KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_32K            14
/** 40KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_40K            15
/** 48KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_48K            16
/** 64KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_64K            17
/** 85KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_85K            18
/** 96KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_96K            19
/** 100KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_100K           20
/** 120KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_120K           21
/** 128KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_128K           22
/** 160KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_160K           23
/** 196KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_196K           24
/** 256KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_256K           25
/** 512KOHM */
#define AD5940_DT_LPAMP_TIA_RTIA_512K           26
/** @} */

/**
 * @name LPAMP power mode selection
 *
 * Values for the `lpamp-pwr` devicetree property.
 * @{
 */
/** Normal Power mode */
#define AD5940_DT_LPAMP_PWR_NORM                0
/** Boost power to level 1 */
#define AD5940_DT_LPAMP_PWR_BOOST1              1
/** Boost power to level 2 */
#define AD5940_DT_LPAMP_PWR_BOOST2              2
/** Boost power to level 3 */
#define AD5940_DT_LPAMP_PWR_BOOST3              3
/** Put PA and TIA in half power mode */
#define AD5940_DT_LPAMP_PWR_HALF                4
/** @} */

/**
 * @name ADC P channel input selection
 *
 * Values for the `adc-muxp` devicetree property.
 * @{
 */
/** float */
#define AD5940_DT_ADC_MUXP_FLOAT                0x0
/** output of HSTIA */
#define AD5940_DT_ADC_MUXP_HSTIA_P              0x1
/** pin AIN0 */
#define AD5940_DT_ADC_MUXP_AIN0                 0x4
/** pin AIN1 */
#define AD5940_DT_ADC_MUXP_AIN1                 0x5
/** pin AIN2 */
#define AD5940_DT_ADC_MUXP_AIN2                 0x6
/** pin AIN3 */
#define AD5940_DT_ADC_MUXP_AIN3                 0x7
/** AVDD/2 */
#define AD5940_DT_ADC_MUXP_AVDD_2               0x8
/** DVDD/2 */
#define AD5940_DT_ADC_MUXP_DVDD_2               0x9
/** AVDD internal regulator output. It's around 1.8V */
#define AD5940_DT_ADC_MUXP_AVDDREG              0xA
/** Internal temperature output postive terminal */
#define AD5940_DT_ADC_MUXP_TEMPP                0xB
/** VBIAS Cap */
#define AD5940_DT_ADC_MUXP_VBIAS_CAP            0xC
/** Voltage of DE0 pin */
#define AD5940_DT_ADC_MUXP_VDE0                 0xD
/** Voltage of SE0 pin */
#define AD5940_DT_ADC_MUXP_VSE0                 0xE
/** Voltage of SE1 pin on ADuCM355 */
#define AD5940_DT_ADC_MUXP_VSE1                 0xF
/** Voltage of AFE3 pin on AD5940. */
#define AD5940_DT_ADC_MUXP_VAFE3                0xF
/** 1.25V. The internal 2.5V reference buffer output divided by 2. */
#define AD5940_DT_ADC_MUXP_VREF2P5              0x10
/** HSDAC 1.8V internal reference. Only available when AFECON.BIT20 and AFECON.BIT6 are set. */
#define AD5940_DT_ADC_MUXP_VREF1P8DAC           0x12
/** Internal temperature output negative terminal */
#define AD5940_DT_ADC_MUXP_TEMPN                0x13
/** Voltage of AIN4/LPF0 pin */
#define AD5940_DT_ADC_MUXP_AIN4                 0x14
/** Voltage of AIN6 pin, not available on AD5941 */
#define AD5940_DT_ADC_MUXP_AIN6                 0x16
/** Voltage of Vzero0 pin */
#define AD5940_DT_ADC_MUXP_VZERO0               0x17
/** Voltage of Vbias0 pin */
#define AD5940_DT_ADC_MUXP_VBIAS0               0x18
/** Pin CE0 */
#define AD5940_DT_ADC_MUXP_VCE0                 0x19
/** Pin RE0 */
#define AD5940_DT_ADC_MUXP_VRE0                 0x1A
/** Voltage of AFE4 pin on AD5940. */
#define AD5940_DT_ADC_MUXP_VAFE4                0x1B
/** Voltage of AFE1 pin on AD5940. */
#define AD5940_DT_ADC_MUXP_VAFE1                0x1D
/** Voltage of AFE2 pin on AD5940. */
#define AD5940_DT_ADC_MUXP_VAFE2                0x1E
/** VCE0 divide by 2 */
#define AD5940_DT_ADC_MUXP_VCE0_2               0x1F
/** Output of LPTIA0 */
#define AD5940_DT_ADC_MUXP_LPTIA0_P             0x21
/** Internal AGND node */
#define AD5940_DT_ADC_MUXP_AGND                 0x23
/** Buffered voltage of excitation buffer P node. */
#define AD5940_DT_ADC_MUXP_P_NODE               0x24
/** @} */

/**
 * @name ADC N channel input selection
 *
 * Values for the `adc-muxn` devicetree property.
 * @{
 */
/** float */
#define AD5940_DT_ADC_MUXN_FLOAT                 0x0
/** HSTIA negative input node. */
#define AD5940_DT_ADC_MUXN_HSTIA_N               0x1
/** LPTIA0 negative input node. */
#define AD5940_DT_ADC_MUXN_LPTIA0_N              0x2
/** LPTIA1 negative input node. */
#define AD5940_DT_ADC_MUXN_LPTIA1_N              0x3
/** Pin AIN0 */
#define AD5940_DT_ADC_MUXN_AIN0                  0x4
/** Pin AIN1 */
#define AD5940_DT_ADC_MUXN_AIN1                  0x5
/** Pin AIN2 */
#define AD5940_DT_ADC_MUXN_AIN2                  0x6
/** Pin AIN3 */
#define AD5940_DT_ADC_MUXN_AIN3                  0x7
/** VBIAS Cap */
#define AD5940_DT_ADC_MUXN_VBIAS_CAP             0x8
/** Temperature sensor output. */
#define AD5940_DT_ADC_MUXN_TEMPN                 0xB
/** AIN4 */
#define AD5940_DT_ADC_MUXN_AIN4                  0xC
/** AIN6 */
#define AD5940_DT_ADC_MUXN_AIN6                  0xE
/** pin Vzero0 */
#define AD5940_DT_ADC_MUXN_VZERO0                0x10
/** pin Vbias0 */
#define AD5940_DT_ADC_MUXN_VBIAS0                0x11
/** Buffered voltage of excitation buffer N node. */
#define AD5940_DT_ADC_MUXN_N_NODE                0x14
/** @} */

/**
 * @name ADC PGA gain selection
 *
 * Values for the `adc-pga-gain` devicetree property.
 * @{
 */
/** ADC PGA Gain of 1 */
#define AD5940_DT_ADC_PGA_GAIN_1                0
/** ADC PGA Gain of 1.5 */
#define AD5940_DT_ADC_PGA_GAIN_1P5              1
/** ADC PGA Gain of 2 */
#define AD5940_DT_ADC_PGA_GAIN_2                2
/** ADC PGA Gain of 4 */
#define AD5940_DT_ADC_PGA_GAIN_4                3
/** ADC PGA Gain of 9 */
#define AD5940_DT_ADC_PGA_GAIN_9                4
/** @} */

/**
 * @name ADC SINC3 filter oversampling ratio selection
 *
 * Values for the `adc-sinc3-osr` devicetree property.
 * @{
 */
/** ADC SINC3 OSR 5 */
#define AD5940_DT_ADC_SINC3_OSR_5               0
/** ADC SINC3 OSR 4 */
#define AD5940_DT_ADC_SINC3_OSR_4               1
/** ADC SINC3 OSR 2 */
#define AD5940_DT_ADC_SINC3_OSR_2               2
/** @} */

/**
 * @name ADC SINC2 filter oversampling ratio selection
 *
 * Values for the `adc-sinc2-osr` devicetree property.
 * @{
 */
/** ADC SINC2 OSR 22 */
#define AD5940_DT_ADC_SINC2_OSR_22              0
/** ADC SINC2 OSR 44 */
#define AD5940_DT_ADC_SINC2_OSR_44              1
/** ADC SINC2 OSR 89 */
#define AD5940_DT_ADC_SINC2_OSR_89              2
/** ADC SINC2 OSR 178 */
#define AD5940_DT_ADC_SINC2_OSR_178             3
/** ADC SINC2 OSR 267 */
#define AD5940_DT_ADC_SINC2_OSR_267             4
/** ADC SINC2 OSR 533 */
#define AD5940_DT_ADC_SINC2_OSR_533             5
/** ADC SINC2 OSR 640 */
#define AD5940_DT_ADC_SINC2_OSR_640             6
/** ADC SINC2 OSR 667 */
#define AD5940_DT_ADC_SINC2_OSR_667             7
/** ADC SINC2 OSR 800 */
#define AD5940_DT_ADC_SINC2_OSR_800             8
/** ADC SINC2 OSR 889 */
#define AD5940_DT_ADC_SINC2_OSR_889             9
/** ADC SINC2 OSR 1067 */
#define AD5940_DT_ADC_SINC2_OSR_1067            10
/** ADC SINC2 OSR 1333 */
#define AD5940_DT_ADC_SINC2_OSR_1333            11
/** @} */

/**
 * @name ADC averaging count selection
 *
 * Values for the `adc-avg-num` devicetree property.
 * @{
 */
/** Take 2 input to do average. */
#define AD5940_DT_ADC_AVG_NUM_2                 0
/** Take 4 input to do average. */
#define AD5940_DT_ADC_AVG_NUM_4                 1
/** Take 8 input to do average. */
#define AD5940_DT_ADC_AVG_NUM_8                 2
/** Take 16 input to do average. */
#define AD5940_DT_ADC_AVG_NUM_16                3
/** @} */

/**
 * @name ADC sample rate selection
 *
 * Values for the `adc-rate` devicetree property.
 * @{
 */
/** ADC input clock is 16MHz, sample rate is 800kHz */
#define AD5940_DT_ADC_RATE_800KHZ               1
/** ADC input clock is 32MHz, sample rate is 1.6MHz */
#define AD5940_DT_ADC_RATE_1P6MHZ               0
/** @} */

/**
 * @name DFT number selection
 *
 * Values for the `dft-num` devicetree property.
 * @{
 */
/** 4 Point */
#define AD5940_DT_DFT_NUM_4                     0
/** 8 Point */
#define AD5940_DT_DFT_NUM_8                     1
/** 16 Point */
#define AD5940_DT_DFT_NUM_16                    2
/** 32 Point */
#define AD5940_DT_DFT_NUM_32                    3
/** 64 Point */
#define AD5940_DT_DFT_NUM_64                    4
/** 128 Point */
#define AD5940_DT_DFT_NUM_128                   5
/** 256 Point */
#define AD5940_DT_DFT_NUM_256                   6
/** 512 Point */
#define AD5940_DT_DFT_NUM_512                   7
/** 1024 Point */
#define AD5940_DT_DFT_NUM_1024                  8
/** 2048 Point */
#define AD5940_DT_DFT_NUM_2048                  9
/** 4096 Point */
#define AD5940_DT_DFT_NUM_4096                  10
/** 8192 Point */
#define AD5940_DT_DFT_NUM_8192                  11
/** 16384 Point */
#define AD5940_DT_DFT_NUM_16384                 12
/** @} */

/**
 * @name DFT source selection
 *
 * Values for the `dft-src` devicetree property.
 * @{
 */
/** SINC2+Notch filter block output. Bypass Notch to use SINC2 data */
#define AD5940_DT_DFTSRC_SINC2NOTCH             0
/** SINC3 filter */
#define AD5940_DT_DFTSRC_SINC3                  1
/** Raw ADC data */
#define AD5940_DT_DFTSRC_ADCRAW                 2
/** Average output of SINC3. */
#define AD5940_DT_DFTSRC_AVG                    3
/** @} */

/**
 * @name Statistical sample size selection
 *
 * Values for the `stat-sample-sz` devicetree property.
 * @{
 */
/** 128 samples */
#define AD5940_DT_STAT_SAMPLE_SZ_128            0
/** 64 samples */
#define AD5940_DT_STAT_SAMPLE_SZ_64             1
/** 32 samples */
#define AD5940_DT_STAT_SAMPLE_SZ_32             2
/** 16 samples */
#define AD5940_DT_STAT_SAMPLE_SZ_16             3
/** 8 samples */
#define AD5940_DT_STAT_SAMPLE_SZ_8              4
/** @} */

/**
 * @name AFE power mode selection
 *
 * Values for the `afe-pwr-mode` devicetree property.
 * @{
 */
/** Set AFE to Low Power mode. For signal <80kHz */
#define AD5940_DT_AFE_PWR_MODE_LP               0
/** Set AFE to High Power mode. For signal >80kHz */
#define AD5940_DT_AFE_PWR_MODE_HP               1
/** @} */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADI_AD5940_H_ */
