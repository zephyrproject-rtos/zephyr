/***************************************************************************//**
* \file cy_sar.h
* \version 1.20
*
* Header file for the SAR driver.
*
********************************************************************************
* \copyright
* Copyright 2017-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \defgroup group_sar SAR ADC Subsystem (SAR)
* \{
* This driver configures and controls the SAR ADC subsystem block.
*
* This SAR ADC subsystem is comprised of:
*   - a 12-bit SAR converter (SARADC)
*   - an embedded reference block (SARREF)
*   - a mux (\ref group_sar_sarmux "SARMUX") at the inputs of the converter
*   - a sequence controller (\ref group_sar_sarmux "SARSEQ") that enables multi-channel acquisition
*       in a round robin fashion, without CPU intervention, to maximize scan rates.
*
* \image html sar_block_diagram.png
*
* The high level features of the subsystem are:
*   - maximum sample rate of 1 Msps
*   - Sixteen individually configurable channels (depends on device routing capabilities)
*   - per channel selectable
*       - single-ended or differential input mode
*       - input from external pin (8 channels in single-ended mode or 4 channels in differential mode)
*         or from internal signals (AMUXBUS, CTB, Die Temperature Sensor)
*       - choose one of four programmable acquisition times
*       - averaging and accumulation
*   - scan can be triggered by firmware or hardware in single shot or continuous mode
*   - hardware averaging from 2 to 256 samples
*   - selectable voltage references
*       - internal VDDA and VDDA/2 references
*       - buffered 1.2 V bandgap reference from \ref group_sysanalog "AREF"
*       - external reference from dedicated pin
*   - Interrupt generation
*
* \section group_sar_usage Usage
*
* The high level steps to use this driver are:
*
*   -# \ref group_sar_initialization
*   -# \ref group_sar_trigger_conversions
*   -# \ref group_sar_handle_interrupts
*   -# \ref group_sar_retrieve_result
*
* \section group_sar_initialization Initialization and Enable
*
* To configure the SAR subsystem, call \ref Cy_SAR_Init. Pass in a pointer to the \ref SAR_Type
* structure for the base hardware register address and pass in the configuration structure,
* \ref cy_stc_sar_config_t.
*
* After initialization, call \ref Cy_SAR_Enable to enable the hardware.
*
* Here is guidance on how to set the data fields of the configuration structure:
*
* \subsection group_sar_init_struct_ctrl uint32_t ctrl
*
* This field specifies configurations that apply to all channels such as the Vref
* source or the negative terminal selection for all single-ended channels.
* Select a value from each of the following enums that begin with "cy_en_sar_ctrl_" and "OR" them together.
*   - \ref cy_en_sar_ctrl_pwr_ctrl_vref_t
*   - \ref cy_en_sar_ctrl_vref_sel_t
*   - \ref cy_en_sar_ctrl_bypass_cap_t
*   - \ref cy_en_sar_ctrl_neg_sel_t
*   - \ref cy_en_sar_ctrl_hw_ctrl_negvref_t
*   - \ref cy_en_sar_ctrl_comp_delay_t
*   - \ref cy_en_sar_ctrl_comp_pwr_t
*   - \ref cy_en_sar_ctrl_sarmux_deep_sleep_t
*   - \ref cy_en_sar_ctrl_sarseq_routing_switches_t
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_SAR_CTRL
*
* \subsection group_sar_init_struct_sampleCtrl uint32_t sampleCtrl
*
* This field configures sampling details that apply to all channels.
* Select a value from each of the following enums that begin with "cy_en_sar_sample_" and "OR" them together.
*   - \ref cy_en_sar_sample_ctrl_result_align_t
*   - \ref cy_en_sar_sample_ctrl_single_ended_format_t
*   - \ref cy_en_sar_sample_ctrl_differential_format_t
*   - \ref cy_en_sar_sample_ctrl_avg_cnt_t
*   - \ref cy_en_sar_sample_ctrl_avg_mode_t
*   - \ref cy_en_sar_sample_ctrl_trigger_mode_t
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_SAMPLE_CTRL
*
* \subsection group_sar_init_struct_sampleTime01 uint32_t sampleTime01
*
* This field configures the value for sample times 0 and 1 in ADC clock cycles.
*
* The SAR has four programmable 10-bit aperture times that are configured using two data fields,
* \ref group_sar_init_struct_sampleTime01 and
* \ref group_sar_init_struct_sampleTime23.
* Ten bits allow for a range of 0 to 1023 cycles, however 0 and 1 are invalid.
* The minimum aperture time is 167 ns. With an 18 MHz ADC clock, this is
* equal to 3 cycles or a value of 4 in this field. The actual aperture time is one cycle less than
* the value stored in this field.
*
* Use the shifts defined in \ref cy_en_sar_sample_time_shift_t.
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_SAMPLE_TIME01
*
* \subsection group_sar_init_struct_sampleTime23 uint32_t sampleTime23
*
* This field configures the value for sample times 2 and 3 in ADC clock cycles.
* Use the shifts defined in \ref cy_en_sar_sample_time_shift_t.
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_SAMPLE_TIME23
*
* \subsection group_sar_init_struct_rangeThres uint32_t rangeThres
*
* This field configures the upper and lower thresholds for the range interrupt.
* These thresholds apply on a global level for all channels with range detection enabled.
*
* The SARSEQ supports range detection to allow for automatic detection of sample values
* compared to two programmable thresholds without CPU involvement.
* Range detection is done after averaging, alignment, and sign extension (if applicable). In other words the
* threshold values need to have the same data format as the result data.
* The values are interpreted as signed or unsigned according to each channel's configuration.
*
* Use the shifts defined in \ref cy_en_sar_range_thres_shift_t.
*
* The \ref Cy_SAR_SetLowLimit and \ref Cy_SAR_SetHighLimit provide run-time configurability of these thresholds.
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_RANGE_THRES
*
* \subsection group_sar_init_struct_rangeCond cy_en_sar_range_detect_condition_t rangeCond
*
* This field configures the condition (below, inside, outside, or above) that will trigger
* the range interrupt. Select a value from the \ref cy_en_sar_range_detect_condition_t enum.
*
* \subsection group_sar_init_struct_chanEn uint32_t chanEn
*
* This field configures which channels will be scanned.
* Each bit corresponds to a channel. Bit 0 enables channel 0, bit 1 enables channel 1,
* bit 2 enables channel 2, and so on.
*
* \subsection group_sar_init_struct_chanConfig uint32_t chanConfig[16]
*
* Each channel has its own channel configuration register.
* The channel configuration specifies which pin/signal is connected to that channel
* and how the channel is sampled.
*
* Select a value from each of the following enums that begin with "cy_en_sar_chan_config_" and "OR" them together.
*
*   - \ref cy_en_sar_chan_config_input_mode_t
*   - \ref cy_en_sar_chan_config_pos_pin_addr_t
*   - \ref cy_en_sar_chan_config_pos_port_addr_t
*   - \ref cy_en_sar_chan_config_avg_en_t
*   - \ref cy_en_sar_chan_config_sample_time_t
*   - \ref cy_en_sar_chan_config_neg_pin_addr_t
*   - \ref cy_en_sar_chan_config_neg_port_addr_t
*
* Some important considerations are:
*   - The POS_PORT_ADDR and POS_PIN_ADDR bit fields are used by the SARSEQ to select
*   the connection to the positive terminal (Vplus) of the ADC for each channel.
*   - When the channel is an unpaired differential input (\ref CY_SAR_CHAN_DIFFERENTIAL_UNPAIRED), the
*   NEG_PORT_ADDR and NEG_PIN_ADDR are used by the SARSEQ to select the connection
*   to the negative terminal (Vminus) of the ADC.
*   - When the channel is a differential input pair (\ref CY_SAR_CHAN_DIFFERENTIAL_PAIRED), the NEG_PORT_ADDR and NEG_PIN_ADDR are ignored.
*   For differential input pairs, only the pin for the positive terminal needs to be
*   specified and this pin must be even. For example, Pin 0 (positive terminal) and Pin 1 (negative terminal)
*   are a pair. Pin 2 (positive terminal) and Pin 3 (negative terminal) are a pair.
*
* If the SARSEQ is disabled (\ref cy_en_sar_ctrl_sarseq_routing_switches_t) or
* it is not controlling any switches (\ref group_sar_init_struct_muxSwitchSqCtrl = 0), the port and pin addresses
* are ignored. This is possible when there is only one channel to scan.
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_CHAN_CONFIG
*
* \subsection group_sar_init_struct_intrMask uint32_t intrMask
*
* This field configures which interrupt events (end of scan, overflow, or firmware collision) will be serviced by the firmware.
*
* Select one or more values from the \ref cy_en_sar_intr_mask_t enum and "OR" them
* together.
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_INTR_MASK
*
* \subsection group_sar_init_struct_satIntrMask uint32_t satIntrMask
*
* This field configures which channels will cause a saturation interrupt.
*
* The SARSEQ has a saturation detect that is always applied to every conversion.
* This feature detects whether a channel's sample value is equal to the minimum or maximum values.
* This allows the firmware to take action, for example, discard the result, when the SARADC saturates.
* The sample value is tested right after conversion, that is, before averaging. This means that it
* can happen that the interrupt is set while the averaged result in the data register is not
* equal to the minimum or maximum.
*
* Each bit corresponds to a channel. A value of 0 disables saturation detection for all channels.
*
* \subsection group_sar_init_struct_rangeIntrMask uint32_t rangeIntrMask
*
* This field configures which channels will cause a range detection interrupt.
* Each bit corresponds to a channel. A value of 0 disables range detection for all channels.
*
* \subsection group_sar_init_struct_muxSwitch uint32_t muxSwitch
*
* This field is the firmware control of the SARMUX switches.
*
* Use one or more values from the \ref cy_en_sar_mux_switch_fw_ctrl_t enum and "OR" them together.
* If the SARSEQ is enabled, the SARMUX switches that will be used must
* also be closed using this firmware control.
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_MUX_SWITCH
*
* Firmware control can be changed at run-time by calling \ref Cy_SAR_SetAnalogSwitch with \ref CY_SAR_MUX_SWITCH0
* and the desired switch states.
*
* \subsection group_sar_init_struct_muxSwitchSqCtrl uint32_t muxSwitchSqCtrl
*
* This field enables or disables SARSEQ control of the SARMUX switches.
* To disable control of all switches, set this field to 0. To disable the SARSEQ all together,
* use \ref CY_SAR_SARSEQ_SWITCH_DISABLE when configuring the \ref group_sar_init_struct_ctrl register.
*
* Use one or more values from the \ref cy_en_sar_mux_switch_sq_ctrl_t enum and "OR" them together.
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_MUX_SQ_CTRL
*
* SARSEQ control can be changed at run-time by calling \ref Cy_SAR_SetSwitchSarSeqCtrl.
*
* \subsection group_sar_init_struct_configRouting bool configRouting
*
* If true, the \ref group_sar_init_struct_muxSwitch and \ref group_sar_init_struct_muxSwitchSqCtrl fields
* will be used. If false, the fields will be ignored.
*
* \subsection group_sar_init_struct_vrefMvValue uint32_t vrefMvValue
*
* This field sets the value of the reference voltage in millivolts used. This value
* is used for converting counts to volts in the \ref Cy_SAR_CountsTo_Volts, \ref Cy_SAR_CountsTo_mVolts, and
* \ref Cy_SAR_CountsTo_uVolts functions.
*
* \section group_sar_trigger_conversions Triggering Conversions
*
* The SAR subsystem has the following modes for triggering a conversion:
* <table class="doxtable">
*   <tr>
*     <th>Mode</th>
*     <th>Description</th>
*     <th>Usage</th>
*   </tr>
*   <tr>
*     <td>Continuous</td>
*     <td>After completing a scan, the
*     SARSEQ will immediately start the next scan. That is, the SARSEQ will always be BUSY.
*     As a result all other triggers, firmware or hardware, are essentially ignored.
*  </td>
*     <td>To enter this mode, call \ref Cy_SAR_StartConvert with \ref CY_SAR_START_CONVERT_CONTINUOUS.
*     To stop continuous conversions, call \ref Cy_SAR_StopConvert.
*     </td>
*   </tr>
*   <tr>
*     <td>Firmware single shot</td>
*     <td>A single conversion of all enabled channels is triggered with a function call to \ref Cy_SAR_StartConvert with
*     \ref CY_SAR_START_CONVERT_SINGLE_SHOT.
*     </td>
*     <td>
*     Firmware triggering is always available by calling \ref Cy_SAR_StartConvert with \ref CY_SAR_START_CONVERT_SINGLE_SHOT.
*     To allow only firmware triggering, or disable
*     hardware triggering, set up the \ref cy_stc_sar_config_t config structure with \ref CY_SAR_TRIGGER_MODE_FW_ONLY.
*     </td>
*   </tr>
*   <tr>
*     <td>Hardware edge sensitive</td>
*     <td>A single conversion of all enabled channels is triggered on the rising edge of the hardware
*     trigger signal.</td>
*     <td>To enable this mode, set up the \ref cy_stc_sar_config_t config structure with
*     \ref CY_SAR_TRIGGER_MODE_FW_AND_HWEDGE.</td>
*    </tr>
*    <tr>
*     <td>Hardware level sensitive</td>
*     <td>Conversions are triggered continuously when the hardware trigger signal is high.</td>
*     <td>To enable this mode, set up the \ref cy_stc_sar_config_t config structure with
*     \ref CY_SAR_TRIGGER_MODE_FW_AND_HWLEVEL.</td>
*    </tr>
* </table>
*
* The trigger mode can be changed during run time with \ref Cy_SAR_SetConvertMode.
*
* For the hardware trigger modes, use the \ref group_trigmux driver to route an internal or external
* signal to the SAR trigger input.
* When making the required \ref Cy_TrigMux_Connect calls, use the pre-defined enum, TRIG6_OUT_PASS_TR_SAR_IN,
* for the SAR trigger input.
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_CONFIG_TRIGGER
*
* \section group_sar_handle_interrupts Handling Interrupts
*
* The SAR can generate interrupts on these events:
*
*   - End of scan (EOS): when scanning of all enabled channels complete.
*   - Overflow: when the result register is updated before the previous result is read.
*   - FW collision: when a new trigger is received while the SAR is still processing the previous trigger.
*   - Saturation detection: when the channel result is equal to the minimum or maximum value.
*   - Range detection: when the channel result meets the programmed upper or lower threshold values.
*
* The SAR interrupt to the NVIC is raised any time the intersection (logic and) of the interrupt
* flags and the corresponding interrupt masks are non-zero.
*
* Implement an interrupt routine and assign it to the SAR interrupt.
* Use the pre-defined enum, pass_interrupt_sar_IRQn, as the interrupt source for the SAR.
*
* The following code snippet demonstrates how to implement a routine to handle the interrupt.
* The routine gets called when any one of the SAR interrupts are triggered.
* When servicing an interrupt, the user must clear the interrupt so that subsequent
* interrupts can be handled.
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_ISR
*
* The following code snippet demonstrates how to configure and enable the interrupt.
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_CONFIG_INTR
*
* Alternately, instead of handling the interrupts, the \ref Cy_SAR_IsEndConversion function
* allows for firmware polling of the end of conversion status.
*
* \section group_sar_retrieve_result Retrieve Channel Results
*
* Retrieve the ADC result by calling \ref Cy_SAR_GetResult16 with the desired channel.
* To convert the result to a voltage, pass the ADC result to \ref Cy_SAR_CountsTo_Volts, \ref Cy_SAR_CountsTo_mVolts, or
* \ref Cy_SAR_CountsTo_uVolts.
*
* \section group_sar_clock SAR Clock Configuration
*
* The SAR requires a clock. Assign a clock to the SAR using the
* pre-defined enum, PCLK_PASS_CLOCK_SAR, to identify the SAR subsystem.
* Set the clock divider value to achieve the desired clock rate. The SAR can support
* a maximum frequency of 18 MHz.
*
* \snippet sar_sut_01.cydsn/main_cm4.c SAR_SNIPPET_CONFIGURE_CLOCK
*
* \section group_sar_scan_time Scan Rate
*
* The scan rate is dependent on the following:
*
*   - ADC clock rate
*   - Number of channels
*   - Averaging
*   - Resolution
*   - Acquisition times
*
* \subsection group_sar_acquisition_time Acquisition Time
*
* The acquisition time of a channel is based on which of the four global aperture times are selected for that
* channel. The selection is done during initialization per channel with \ref group_sar_init_struct_chanConfig.
* The four global aperture times are also set during initialization with \ref group_sar_init_struct_sampleTime01 and
* \ref group_sar_init_struct_sampleTime23. Note that these global aperture times are in SAR clock cycles and the
* acquisition time is 1 less than that value in the register.
*
* \image html sar_acquisition_time_eqn.png
*
* \subsection group_sar_channel_sample_time Channel Sample Time
*
* The sample time for a channel is the time required to acquire the analog signal
* and convert it to a digital code.
*
* \image html sar_channel_sample_time_eqn.png
*
* The SAR ADC is a 12-bit converter so Resolution = 12.
*
* \subsection group_sar_total_scan_time Total Scan Time
*
* Channels using one of the sequential averaging modes (\ref CY_SAR_AVG_MODE_SEQUENTIAL_ACCUM or \ref CY_SAR_AVG_MODE_SEQUENTIAL_FIXED)
* are sampled multiple times per scan. The number of samples averaged are set during initialization
* with \ref group_sar_init_struct_sampleCtrl using one of the values from \ref cy_en_sar_sample_ctrl_avg_cnt_t.
* Channels that are not averaged or use the \ref CY_SAR_AVG_MODE_INTERLEAVED mode are only sampled once per scan.
*
* The total scan time is the sum of each channel's sample time multiplied by the samples per scan.
*
* \image html sar_scan_rate_eqn.png
*
* where N is the total number of channels in the scan.
*
* \section group_sar_sarmux SARMUX and SARSEQ
*
* The SARMUX is an analog programmable multiplexer. Its switches can be controlled by the SARSEQ or firmware.
* and the inputs can come from:
*   - a dedicated port (can support 8 single-ended channels or 4 differential channels)
*   - an internal die temperature (DieTemp) sensor
*   - CTB output via SARBUS0/1 (if CTBs are available on the device)
*   - AMUXBUSA/B
*
* The following figure shows the SARMUX switches. See the device datasheet for the exact location of SARMUX pins.
*
* \image html sar_sarmux_switches.png
*
* When using the SARSEQ, the following configurations must be performed:
*   - enable SARSEQ control of required switches (see \ref group_sar_init_struct_muxSwitchSqCtrl)
*   - close the required switches with firmware (see \ref group_sar_init_struct_muxSwitch)
*   - configure the POS_PORT_ADDR and POS_PIN_ADDR, and if used, the NEG_PORT_ADDR and NEG_PIN_ADDR (see \ref group_sar_init_struct_chanConfig)
*
* While firmware can control every switch in the SARMUX, not every switch can be controlled by the SARSEQ (green switches in the above figure).
* Additionally, switches outside of the SARMUX such as the AMUXBUSA/B switches or
* CTB switches will require separate function calls (see \ref group_gpio "GPIO" and \ref group_ctb "CTB" drivers).
* The SARSEQ can control three switches in the \ref group_ctb "CTB" driver (see \ref Cy_CTB_EnableSarSeqCtrl).
* These switches need to be enabled for SARSEQ control if the CTB outputs are used as the SARMUX inputs.
*
* The following table shows the required POS_PORT_ADDR and POS_PIN_ADDR settings
* for different input connections.
*
* <table class="doxtable">
*   <tr>
*     <th>Input Connection Selection</th>
*     <th>POS_PORT_ADDR</th>
*     <th>POS_PIN_ADDR</th>
*   </tr>
*   <tr>
*     <td>SARMUX dedicated port</td>
*     <td>\ref CY_SAR_POS_PORT_ADDR_SARMUX</td>
*     <td>\ref CY_SAR_CHAN_POS_PIN_ADDR_0 through \ref CY_SAR_CHAN_POS_PIN_ADDR_7</td>
*   </tr>
*   <tr>
*     <td>DieTemp sensor</td>
*     <td>\ref CY_SAR_POS_PORT_ADDR_SARMUX_VIRT</td>
*     <td>\ref CY_SAR_CHAN_POS_PIN_ADDR_0</td>
*   </tr>
*   <tr>
*     <td>AMUXBUSA</td>
*     <td>\ref CY_SAR_POS_PORT_ADDR_SARMUX_VIRT</td>
*     <td>\ref CY_SAR_CHAN_POS_PIN_ADDR_2</td>
*   </tr>
*   <tr>
*     <td>AMUXBUSB</td>
*     <td>\ref CY_SAR_POS_PORT_ADDR_SARMUX_VIRT</td>
*     <td>\ref CY_SAR_CHAN_POS_PIN_ADDR_3</td>
*   </tr>
*   <tr>
*     <td>CTB0 Opamp0 1x output</td>
*     <td>\ref CY_SAR_POS_PORT_ADDR_CTB0</td>
*     <td>\ref CY_SAR_CHAN_POS_PIN_ADDR_2</td>
*   </tr>
*   <tr>
*     <td>CTB0 Opamp1 1x output</td>
*     <td>\ref CY_SAR_POS_PORT_ADDR_CTB0</td>
*     <td>\ref CY_SAR_CHAN_POS_PIN_ADDR_3</td>
*   </tr>
* </table>
*
* \subsection group_sar_sarmux_dietemp Input from DieTemp sensor
*
* When using the DieTemp sensor, always use single-ended mode.
* The temperature sensor can be routed to Vplus using the \ref CY_SAR_MUX_FW_TEMP_VPLUS switch.
* Connecting this switch will also enable the sensor. Set the \ref group_sar_acquisition_time "acquisition time" to
* be at least 1 us to meet the settling time of the temperature sensor.
*
* \image html sar_sarmux_dietemp.png
*
* \snippet sar_sut_02.cydsn/main_cm4.c SNIPPET_SAR_SARMUX_DIETEMP
*
* \subsection group_sar_sarmux_se_diff Input from SARMUX port
*
* The following figure and code snippet show how two GPIOs on the SARMUX dedicated port
* are connected to the SARADC as separate single-ended channels and as a differential-pair channel.
*
* \image html sar_sarmux_dedicated_port.png
*
* \snippet sar_sut_02.cydsn/main_cm4.c SNIPPET_SAR_SARMUX_SE_DIFF
*
* \subsection group_sar_sarmux_ctb Input from CTB output visa SARBUS0/1
*
* The following figure and code snippet show how the two opamp outputs from the CTB
* are connected to the SARADC as separate single-ended channels and as a differential-pair channel.
* Note that separate function calls are needed to configure and enable the opamps, perform required analog routing,
* and enable SARSEQ control of the switches contained in the CTB.
*
* \image html sar_sarmux_ctb.png
*
* \snippet sar_sut_02.cydsn/main_cm4.c SNIPPET_SAR_SARMUX_CTB
*
* \subsection group_sar_sarmux_amuxbus Input from other pins through AMUXBUSA/B
*
* The following figure and code snippet show how two GPIOs on any port through the AMUXBUSA and AMUXBUSB
* are connected to the SARADC as separate single-ended channels and as a differential-pair channel.
* Note that separate function calls are needed to route the device pins to the SARMUX. The AMUXBUSes
* are separated into multiple segments and these segments are connected/disconnected using the AMUX_SPLIT_CTL
* registers in the HSIOM. In the following code snippet, to connect Port 1 to the SARMUX, the left and right
* switches of AMUX_SPLIT_CTL[1] and AMUX_SPLIT_CTL[6] need to be closed.
*
* \image html sar_sarmux_amuxbus.png
*
* \snippet sar_sut_02.cydsn/main_cm4.c SNIPPET_SAR_SARMUX_AMUXBUS
*
* \section group_sar_low_power Low Power Support
* This SAR driver provides a callback function to handle power mode transitions.
* The \ref Cy_SAR_DeepSleepCallback function ensures that SAR conversions are stopped
* before Deep Sleep entry. Upon wakeup, the callback
* enables the hardware and continuous conversions, if previously enabled.
*
* To trigger the callback execution, the callback must be registered before calling \ref Cy_SysPm_DeepSleep. Refer to
* \ref group_syspm driver for more information about power mode transitions and
* callback registration.
*
* Recall that during configuration of the \ref group_sar_init_struct_ctrl "ctrl" field,
* the SARMUX can be configured to remain enabled in Deep Sleep mode.
* All other blocks (SARADC, REFBUF, and SARSEQ) do not support Deep Sleep mode operation.
*
* \section group_sar_more_information More Information
* For more information on the SAR ADC subsystem, refer to the technical reference manual (TRM).
*
* \section group_sar_MISRA MISRA-C Compliance]
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
*       This SAR driver implements a Deep Sleep callback conforming to the cy_syspm driver requirements.
*       When the callback is called, the base should point to the SAR_Type register address.
*     </td>
*   </tr>
* </table>
*
* \section group_sar_changelog Changelog
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td>1.20</td>
*     <td>Flattened the driver source code organization into the single
*         source directory and the single include directory
*     </td>
*     <td>Simplified the driver library directory structure</td>
*   </tr>
*   <tr>
*     <td rowspan="3">1.10</td>
*     <td> Added workaround for parts with out of range CAP_TRIM in Init API.</td>
*     <td> Correct CAP_TRIM is necessary achieving specified SAR ADC linearity</td>
*   </tr>
*   <tr>
*     <td> Turn off the entire hardware block only if the SARMUX is not enabled 
*          for Deep Sleep operation.                
*     </td>
*     <td> Improvement of the \ref Cy_SAR_Sleep flow</td>
*   </tr>
*   <tr>
*     <td>Updated "Low Power Support" section to describe registering the Deep Sleep callback.
*         Added parenthesis around logical AND operation in Sleep API.</td>
*     <td>Documentation update and clarification</td>
*   </tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*
* \defgroup group_sar_macros Macros
* \defgroup group_sar_functions Functions
*   \{
*       \defgroup group_sar_functions_basic         Initialization and Basic Functions
*       \defgroup group_sar_functions_power         Low Power Callback
*       \defgroup group_sar_functions_config        Run-time Configuration Functions
*       \defgroup group_sar_functions_countsto      Counts Conversion Functions
*       \defgroup group_sar_functions_interrupt     Interrupt Functions
*       \defgroup group_sar_functions_switches      SARMUX Switch Control Functions
*       \defgroup group_sar_functions_helper        Useful Configuration Query Functions
*   \}
* \defgroup group_sar_globals Global Variables
* \defgroup group_sar_data_structures Data Structures
* \defgroup group_sar_enums Enumerated Types
*   \{
*       \defgroup group_sar_ctrl_register_enums         Control Register Enums
*       \defgroup group_sar_sample_ctrl_register_enums  Sample Control Register Enums
*       \defgroup group_sar_sample_time_shift_enums     Sample Time Register Enums
*       \defgroup group_sar_range_thres_register_enums  Range Interrupt Register Enums
*       \defgroup group_sar_chan_config_register_enums  Channel Configuration Register Enums
*       \defgroup group_sar_intr_mask_t_register_enums  Interrupt Mask Register Enums
*       \defgroup group_sar_mux_switch_register_enums   SARMUX Switch Control Register Enums
*   \}
*/

#if !defined(CY_SAR_H)
#define CY_SAR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "cyip_pass.h"
#include "cy_device_headers.h"
#include "cy_syslib.h"
#include "cy_syspm.h"

#ifdef CY_IP_MXS40PASS_SAR

#if defined(__cplusplus)
extern "C" {
#endif

/** \addtogroup group_sar_macros
* \{
*/

/** Driver major version */
#define CY_SAR_DRV_VERSION_MAJOR        1

/** Driver minor version */
#define CY_SAR_DRV_VERSION_MINOR        20

/** SAR driver identifier */
#define CY_SAR_ID                       CY_PDL_DRV_ID(0x01u)

/** Maximum number of channels */
#define CY_SAR_MAX_NUM_CHANNELS         (PASS_SAR_SAR_CHANNELS)

/** \cond INTERNAL */
#define CY_SAR_DEINIT                   (0uL)             /**< De-init value for most SAR registers */
#define CY_SAR_SAMPLE_TIME_DEINIT       ((3uL << SAR_SAMPLE_TIME01_SAMPLE_TIME0_Pos) | (3uL << SAR_SAMPLE_TIME01_SAMPLE_TIME1_Pos))  /**< De-init value for the SAMPLE_TIME* registers */
#define CY_SAR_CLEAR_ALL_SWITCHES       (0x3FFFFFFFuL)    /**< Value to clear all SARMUX switches */
#define CY_SAR_DEINIT_SQ_CTRL           (SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_P0_Msk \
                                        | SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_P1_Msk \
                                        | SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_P2_Msk \
                                        | SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_P3_Msk \
                                        | SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_P4_Msk \
                                        | SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_P5_Msk \
                                        | SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_P6_Msk \
                                        | SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_P7_Msk \
                                        | SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_VSSA_Msk \
                                        | SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_TEMP_Msk \
                                        | SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_AMUXBUSA_Msk \
                                        | SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_AMUXBUSB_Msk \
                                        | SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_SARBUS0_Msk \
                                        | SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_SARBUS1_Msk)
#define CY_SAR_REG_CTRL_MASK            (SAR_CTRL_PWR_CTRL_VREF_Msk \
                                        | SAR_CTRL_VREF_SEL_Msk \
                                        | SAR_CTRL_VREF_BYP_CAP_EN_Msk \
                                        | SAR_CTRL_NEG_SEL_Msk \
                                        | SAR_CTRL_SAR_HW_CTRL_NEGVREF_Msk \
                                        | SAR_CTRL_COMP_DLY_Msk \
                                        | SAR_CTRL_REFBUF_EN_Msk \
                                        | SAR_CTRL_COMP_PWR_Msk \
                                        | SAR_CTRL_DEEPSLEEP_ON_Msk \
                                        | SAR_CTRL_DSI_SYNC_CONFIG_Msk \
                                        | SAR_CTRL_DSI_MODE_Msk \
                                        | SAR_CTRL_SWITCH_DISABLE_Msk \
                                        | SAR_CTRL_ENABLED_Msk)
#define CY_SAR_REG_SAMPLE_CTRL_MASK     (SAR_SAMPLE_CTRL_LEFT_ALIGN_Msk \
                                        | SAR_SAMPLE_CTRL_SINGLE_ENDED_SIGNED_Msk \
                                        | SAR_SAMPLE_CTRL_DIFFERENTIAL_SIGNED_Msk \
                                        | SAR_SAMPLE_CTRL_AVG_CNT_Msk \
                                        | SAR_SAMPLE_CTRL_AVG_SHIFT_Msk \
                                        | SAR_SAMPLE_CTRL_AVG_MODE_Msk \
                                        | SAR_SAMPLE_CTRL_CONTINUOUS_Msk \
                                        | SAR_SAMPLE_CTRL_DSI_TRIGGER_EN_Msk \
                                        | SAR_SAMPLE_CTRL_DSI_TRIGGER_LEVEL_Msk \
                                        | SAR_SAMPLE_CTRL_DSI_SYNC_TRIGGER_Msk \
                                        | SAR_SAMPLE_CTRL_UAB_SCAN_MODE_Msk \
                                        | SAR_SAMPLE_CTRL_REPEAT_INVALID_Msk \
                                        | SAR_SAMPLE_CTRL_VALID_SEL_Msk \
                                        | SAR_SAMPLE_CTRL_VALID_SEL_EN_Msk \
                                        | SAR_SAMPLE_CTRL_VALID_IGNORE_Msk \
                                        | SAR_SAMPLE_CTRL_TRIGGER_OUT_EN_Msk \
                                        | SAR_SAMPLE_CTRL_EOS_DSI_OUT_EN_Msk)
#define CY_SAR_REG_CHAN_CONFIG_MASK     (SAR_CHAN_CONFIG_POS_PIN_ADDR_Msk \
                                        | SAR_CHAN_CONFIG_POS_PORT_ADDR_Msk \
                                        | SAR_CHAN_CONFIG_DIFFERENTIAL_EN_Msk \
                                        | SAR_CHAN_CONFIG_AVG_EN_Msk \
                                        | SAR_CHAN_CONFIG_SAMPLE_TIME_SEL_Msk \
                                        | SAR_CHAN_CONFIG_NEG_PIN_ADDR_Msk \
                                        | SAR_CHAN_CONFIG_NEG_PORT_ADDR_Msk \
                                        | SAR_CHAN_CONFIG_NEG_ADDR_EN_Msk \
                                        | SAR_CHAN_CONFIG_DSI_OUT_EN_Msk)
#define CY_SAR_REG_SAMPLE_TIME_MASK     (SAR_SAMPLE_TIME01_SAMPLE_TIME0_Msk | SAR_SAMPLE_TIME01_SAMPLE_TIME1_Msk)

#define CY_SAR_2US_DELAY                (2u)              /**< Delay used in Enable API function to avoid SAR deadlock */
#define CY_SAR_10V_COUNTS               (10.0F)           /**< Value of 10 in volts */
#define CY_SAR_10MV_COUNTS              (10000)           /**< Value of 10 in millivolts */
#define CY_SAR_10UV_COUNTS              (10000000L)       /**< Value of 10 in microvolts */
#define CY_SAR_WRK_MAX_12BIT            (0x00001000uL)    /**< Maximum SAR work register value for a 12-bit resolution */
#define CY_SAR_RANGE_LIMIT_MAX          (0xFFFFuL)        /**< Maximum value for the low and high range interrupt threshold values */
#define CY_SAR_CAP_TRIM_MAX             (0x3FuL)          /**< Maximum value for CAP_TRIM */
#define CY_SAR_CAP_TRIM_MIN             (0x00uL)          /**< Maximum value for CAP_TRIM */
#define CY_SAR_CAP_TRIM                 (0x0BuL)          /**< Correct cap trim value */

/**< Macros for conditions used in CY_ASSERT calls */
#define CY_SAR_CHANNUM(chan)            ((chan) < CY_SAR_MAX_NUM_CHANNELS)
#define CY_SAR_CHANMASK(mask)           ((mask) < (1uL << CY_SAR_MAX_NUM_CHANNELS))
#define CY_SAR_RANGECOND(cond)          ((cond) <= CY_SAR_RANGE_COND_OUTSIDE)
#define CY_SAR_INTRMASK(mask)           ((mask) <= (uint32_t)(CY_SAR_INTR_EOS_MASK | CY_SAR_INTR_OVERFLOW_MASK | CY_SAR_INTR_FW_COLLISION_MASK))
#define CY_SAR_TRIGGER(mode)            (((mode) == CY_SAR_TRIGGER_MODE_FW_ONLY) \
                                        || ((mode) == CY_SAR_TRIGGER_MODE_FW_AND_HWEDGE) \
                                        || ((mode) == CY_SAR_TRIGGER_MODE_FW_AND_HWLEVEL))
#define CY_SAR_RETURN(mode)             (((mode) == CY_SAR_RETURN_STATUS) || ((mode) == CY_SAR_WAIT_FOR_RESULT))
#define CY_SAR_STARTCONVERT(mode)       (((mode) == CY_SAR_START_CONVERT_SINGLE_SHOT) || ((mode) == CY_SAR_START_CONVERT_CONTINUOUS))
#define CY_SAR_RANGE_LIMIT(limit)       ((limit) <= CY_SAR_RANGE_LIMIT_MAX)
#define CY_SAR_SWITCHSELECT(select)     ((select) == CY_SAR_MUX_SWITCH0)
#define CY_SAR_SWITCHMASK(mask)         ((mask) <= CY_SAR_CLEAR_ALL_SWITCHES)
#define CY_SAR_SWITCHSTATE(state)       (((state) == CY_SAR_SWITCH_OPEN) || ((state) == CY_SAR_SWITCH_CLOSE))
#define CY_SAR_SQMASK(mask)             (((mask) & (~CY_SAR_DEINIT_SQ_CTRL)) == 0uL)
#define CY_SAR_SQCTRL(ctrl)             (((ctrl) == CY_SAR_SWITCH_SEQ_CTRL_ENABLE) || ((ctrl) == CY_SAR_SWITCH_SEQ_CTRL_DISABLE))

#define CY_SAR_CTRL(value)              (((value) & (~CY_SAR_REG_CTRL_MASK)) == 0uL)
#define CY_SAR_SAMPLE_CTRL(value)       (((value) & (~CY_SAR_REG_SAMPLE_CTRL_MASK)) == 0uL)
#define CY_SAR_SAMPLE_TIME(value)       (((value) & (~CY_SAR_REG_SAMPLE_TIME_MASK)) == 0uL)
#define CY_SAR_CHAN_CONFIG(value)       (((value) & (~CY_SAR_REG_CHAN_CONFIG_MASK)) == 0uL)
/** \endcond */

/** \} group_sar_macro */

/** \addtogroup group_sar_globals
* \{
*/
/***************************************
*        Global Variables
***************************************/

/** This array is used to calibrate the offset for each channel.
*
* At initialization, channels that are single-ended, signed, and with Vneg = Vref
* have an offset of -(2^12)/2 = -2048. All other channels have an offset of 0.
* The offset can be overridden using \ref Cy_SAR_SetOffset.
*
* The channel offsets are used by the \ref Cy_SAR_CountsTo_Volts, \ref Cy_SAR_CountsTo_mVolts, and
* \ref Cy_SAR_CountsTo_uVolts functions to convert counts to voltage.
*
*/
extern volatile int16_t Cy_SAR_offset[CY_SAR_MAX_NUM_CHANNELS];

/** This array is used to calibrate the gain for each channel.
*
* It is set at initialization and the value depends on the SARADC resolution
* and voltage reference, 10*(2^12)/(2*Vref).
* The gain can be overridden using \ref Cy_SAR_SetGain.
*
* The channel gains are used by the \ref Cy_SAR_CountsTo_Volts, \ref Cy_SAR_CountsTo_mVolts, and
* \ref Cy_SAR_CountsTo_uVolts functions to convert counts to voltage.
*/
extern volatile int32_t Cy_SAR_countsPer10Volt[CY_SAR_MAX_NUM_CHANNELS];

/** \} group_sar_globals */

/** \addtogroup group_sar_enums
* \{
*/

/******************************************************************************
 * Enumerations
 *****************************************************************************/

/** The SAR status/error code definitions */
typedef enum
{
    CY_SAR_SUCCESS    = 0x00uL,                                      /**< Success */
    CY_SAR_BAD_PARAM  = CY_SAR_ID | CY_PDL_STATUS_ERROR | 0x01uL,    /**< Invalid input parameters */
    CY_SAR_TIMEOUT    = CY_SAR_ID | CY_PDL_STATUS_ERROR | 0x02uL,    /**< A timeout occurred */
    CY_SAR_CONVERSION_NOT_COMPLETE = CY_SAR_ID | CY_PDL_STATUS_ERROR | 0x03uL,    /**< SAR conversion is not complete */
}cy_en_sar_status_t;

/** Definitions for starting a conversion used in \ref Cy_SAR_StartConvert */
typedef enum
{
    CY_SAR_START_CONVERT_SINGLE_SHOT = 0uL, /**< Start a single scan (one shot) from firmware */
    CY_SAR_START_CONVERT_CONTINUOUS  = 1uL, /**< Continuously scan enabled channels and ignores all triggers, firmware or hardware */
}cy_en_sar_start_convert_sel_t;

/** Definitions for the return mode used in \ref Cy_SAR_IsEndConversion */
typedef enum
{
    CY_SAR_RETURN_STATUS    = 0uL,      /**< Immediately returns the conversion status. */
    CY_SAR_WAIT_FOR_RESULT  = 1uL,    /**< Does not return a result until the conversion of all sequential channels is complete. This mode is blocking. */
}cy_en_sar_return_mode_t;

/** Switch state definitions */
typedef enum
{
    CY_SAR_SWITCH_OPEN      = 0uL,    /**< Open the switch */
    CY_SAR_SWITCH_CLOSE     = 1uL     /**< Close the switch */
}cy_en_sar_switch_state_t;

/** Definitions for sequencer control of switches */
typedef enum
{
    CY_SAR_SWITCH_SEQ_CTRL_DISABLE = 0uL, /**< Disable sequencer control of switch */
    CY_SAR_SWITCH_SEQ_CTRL_ENABLE  = 1uL  /**< Enable sequencer control of switch */
}cy_en_sar_switch_sar_seq_ctrl_t;

/** Switch register selection for \ref Cy_SAR_SetAnalogSwitch and \ref Cy_SAR_GetAnalogSwitch */
typedef enum
{
    CY_SAR_MUX_SWITCH0  = 0uL,      /**< SARMUX switch control register */
}cy_en_sar_switch_register_sel_t;

/** \addtogroup group_sar_ctrl_register_enums
* This set of enumerations aids in configuring the SAR CTRL register
* \{
*/
/** Reference voltage buffer power mode definitions */
typedef enum
{
    CY_SAR_VREF_PWR_100     = 0uL << SAR_CTRL_PWR_CTRL_VREF_Pos,      /**< Full power (100%) */
    CY_SAR_VREF_PWR_80      = 1uL << SAR_CTRL_PWR_CTRL_VREF_Pos,      /**< 80% power */
    CY_SAR_VREF_PWR_60      = 2uL << SAR_CTRL_PWR_CTRL_VREF_Pos,      /**< 60% power */
    CY_SAR_VREF_PWR_50      = 3uL << SAR_CTRL_PWR_CTRL_VREF_Pos,      /**< 50% power */
    CY_SAR_VREF_PWR_40      = 4uL << SAR_CTRL_PWR_CTRL_VREF_Pos,      /**< 40% power */
    CY_SAR_VREF_PWR_30      = 5uL << SAR_CTRL_PWR_CTRL_VREF_Pos,      /**< 30% power */
    CY_SAR_VREF_PWR_20      = 6uL << SAR_CTRL_PWR_CTRL_VREF_Pos,      /**< 20% power */
    CY_SAR_VREF_PWR_10      = 7uL << SAR_CTRL_PWR_CTRL_VREF_Pos,      /**< 10% power */
}cy_en_sar_ctrl_pwr_ctrl_vref_t;

/** Reference voltage selection definitions */
typedef enum
{
    CY_SAR_VREF_SEL_BGR         = 4uL << SAR_CTRL_VREF_SEL_Pos,     /**< System wide bandgap from \ref group_sysanalog "AREF" (Vref buffer on) */
    CY_SAR_VREF_SEL_EXT         = 5uL << SAR_CTRL_VREF_SEL_Pos,     /**< External Vref direct from a pin */
    CY_SAR_VREF_SEL_VDDA_DIV_2  = 6uL << SAR_CTRL_VREF_SEL_Pos,     /**< Vdda/2 (Vref buffer on) */
    CY_SAR_VREF_SEL_VDDA        = 7uL << SAR_CTRL_VREF_SEL_Pos      /**< Vdda */
}cy_en_sar_ctrl_vref_sel_t;

/** Vref bypass cap enable.
* When enabled, a bypass capacitor should
* be connected to the dedicated Vref pin of the device.
* Refer to the device datasheet for the minimum bypass capacitor value to use.
*/
typedef enum
{
    CY_SAR_BYPASS_CAP_DISABLE = 0uL << SAR_CTRL_VREF_BYP_CAP_EN_Pos,    /**< Disable Vref bypass cap */
    CY_SAR_BYPASS_CAP_ENABLE  = 1uL << SAR_CTRL_VREF_BYP_CAP_EN_Pos     /**< Enable Vref bypass cap */
}cy_en_sar_ctrl_bypass_cap_t;

/** Negative terminal (Vminus) selection definitions for single-ended channels.
*
* The Vminus input for single ended channels can be connected to
* Vref, VSSA, or routed out to an external pin.
* The options for routing to a pin are through Pin 1, Pin 3, Pin 5, or Pin 7
* of the SARMUX dedicated port or an acore wire in AROUTE, if available on the device.
*
* \ref CY_SAR_NEG_SEL_VSSA_KELVIN comes straight from a Vssa pad without any shared branches
* so as to keep quiet and avoid voltage drops.
*/
typedef enum
{
    CY_SAR_NEG_SEL_VSSA_KELVIN  = 0uL << SAR_CTRL_NEG_SEL_Pos,  /**< Connect Vminus to VSSA_KELVIN */
    CY_SAR_NEG_SEL_P1           = 2uL << SAR_CTRL_NEG_SEL_Pos,  /**< Connect Vminus to Pin 1 of SARMUX dedicated port */
    CY_SAR_NEG_SEL_P3           = 3uL << SAR_CTRL_NEG_SEL_Pos,  /**< Connect Vminus to Pin 3 of SARMUX dedicated port */
    CY_SAR_NEG_SEL_P5           = 4uL << SAR_CTRL_NEG_SEL_Pos,  /**< Connect Vminus to Pin 5 of SARMUX dedicated port */
    CY_SAR_NEG_SEL_P7           = 5uL << SAR_CTRL_NEG_SEL_Pos,  /**< Connect Vminus to Pin 6 of SARMUX dedicated port */
    CY_SAR_NEG_SEL_ACORE        = 6uL << SAR_CTRL_NEG_SEL_Pos,  /**< Connect Vminus to an ACORE in AROUTE */
    CY_SAR_NEG_SEL_VREF         = 7uL << SAR_CTRL_NEG_SEL_Pos,  /**< Connect Vminus to VREF input of SARADC */
}cy_en_sar_ctrl_neg_sel_t;

/** Enable hardware control of the switch between Vref and the Vminus input */
typedef enum
{
    CY_SAR_CTRL_NEGVREF_FW_ONLY = 0uL << SAR_CTRL_SAR_HW_CTRL_NEGVREF_Pos,    /**< Only firmware control of the switch */
    CY_SAR_CTRL_NEGVREF_HW      = 1uL << SAR_CTRL_SAR_HW_CTRL_NEGVREF_Pos     /**< Enable hardware control of the switch */
}cy_en_sar_ctrl_hw_ctrl_negvref_t;

/** Configure the comparator latch delay */
typedef enum
{
    CY_SAR_CTRL_COMP_DLY_2P5    = 0uL << SAR_CTRL_COMP_DLY_Pos,    /**< 2.5 ns delay, use for SAR conversion rate up to 2.5 Msps */
    CY_SAR_CTRL_COMP_DLY_4      = 1uL << SAR_CTRL_COMP_DLY_Pos,    /**< 4 ns delay, use for SAR conversion rate up to 2.0 Msps */
    CY_SAR_CTRL_COMP_DLY_10     = 2uL << SAR_CTRL_COMP_DLY_Pos,    /**< 10 ns delay, use for SAR conversion rate up to 1.5 Msps */
    CY_SAR_CTRL_COMP_DLY_12     = 3uL << SAR_CTRL_COMP_DLY_Pos     /**< 12 ns delay, use for SAR conversion rate up to 1 Msps */
}cy_en_sar_ctrl_comp_delay_t;

/** Configure the comparator power mode */
typedef enum
{
    CY_SAR_COMP_PWR_100     = 0uL << SAR_CTRL_COMP_PWR_Pos,      /**< 100% power, use this for > 2 Msps */
    CY_SAR_COMP_PWR_80      = 1uL << SAR_CTRL_COMP_PWR_Pos,      /**< 80% power, use this for 1.5 - 2 Msps */
    CY_SAR_COMP_PWR_60      = 2uL << SAR_CTRL_COMP_PWR_Pos,      /**< 60% power, use this for 1.0 - 1.5 Msps */
    CY_SAR_COMP_PWR_50      = 3uL << SAR_CTRL_COMP_PWR_Pos,      /**< 50% power, use this for 500 ksps - 1 Msps */
    CY_SAR_COMP_PWR_40      = 4uL << SAR_CTRL_COMP_PWR_Pos,      /**< 40% power, use this for 250 - 500 ksps */
    CY_SAR_COMP_PWR_30      = 5uL << SAR_CTRL_COMP_PWR_Pos,      /**< 30% power, use this for 100 - 250 ksps */
    CY_SAR_COMP_PWR_20      = 6uL << SAR_CTRL_COMP_PWR_Pos,      /**< 20% power, use this for TDB sps */
    CY_SAR_COMP_PWR_10      = 7uL << SAR_CTRL_COMP_PWR_Pos,      /**< 10% power, use this for < 100 ksps */
}cy_en_sar_ctrl_comp_pwr_t;

/** Enable or disable the SARMUX during Deep Sleep power mode. */
typedef enum
{
    CY_SAR_DEEPSLEEP_SARMUX_OFF = 0uL << SAR_CTRL_DEEPSLEEP_ON_Pos,    /**< Disable SARMUX operation during Deep Sleep */
    CY_SAR_DEEPSLEEP_SARMUX_ON  = 1uL << SAR_CTRL_DEEPSLEEP_ON_Pos     /**< Enable SARMUX operation during Deep Sleep */
}cy_en_sar_ctrl_sarmux_deep_sleep_t;

/** Enable or disable the SARSEQ control of routing switches */
typedef enum
{
    CY_SAR_SARSEQ_SWITCH_ENABLE    = 0uL << SAR_CTRL_SWITCH_DISABLE_Pos,    /**< Enable the SARSEQ to change the routing switches defined in the channel configurations */
    CY_SAR_SARSEQ_SWITCH_DISABLE   = 1uL << SAR_CTRL_SWITCH_DISABLE_Pos     /**< Disable the SARSEQ. It is up to the firmware to set the routing switches */
}cy_en_sar_ctrl_sarseq_routing_switches_t;

/* \} */

/** \addtogroup group_sar_sample_ctrl_register_enums
* This set of enumerations are used in configuring the SAR SAMPLE_CTRL register
* \{
*/
/** Configure result alignment, either left or right aligned.
*
* \note
* Averaging always uses right alignment. If the \ref CY_SAR_LEFT_ALIGN
* is selected with averaging enabled, it is ignored.
*
* \note
* The voltage conversion functions (\ref Cy_SAR_CountsTo_Volts, \ref Cy_SAR_CountsTo_mVolts,
* \ref Cy_SAR_CountsTo_uVolts) are only valid for right alignment.
* */
typedef enum
{
    CY_SAR_RIGHT_ALIGN  = 0uL << SAR_SAMPLE_CTRL_LEFT_ALIGN_Pos,    /**< Right align result data to bits [11:0] with sign extension to 16 bits if channel is signed */
    CY_SAR_LEFT_ALIGN   = 1uL << SAR_SAMPLE_CTRL_LEFT_ALIGN_Pos     /**< Left align result data to bits [15:4] */
}cy_en_sar_sample_ctrl_result_align_t;

/** Configure format, signed or unsigned, of single-ended channels */
typedef enum
{
    CY_SAR_SINGLE_ENDED_UNSIGNED  = 0uL << SAR_SAMPLE_CTRL_SINGLE_ENDED_SIGNED_Pos,    /**< Result data for single-ended channels is unsigned */
    CY_SAR_SINGLE_ENDED_SIGNED    = 1uL << SAR_SAMPLE_CTRL_SINGLE_ENDED_SIGNED_Pos     /**< Result data for single-ended channels is signed */
}cy_en_sar_sample_ctrl_single_ended_format_t;

/** Configure format, signed or unsigned, of differential channels */
typedef enum
{
    CY_SAR_DIFFERENTIAL_UNSIGNED  = 0uL << SAR_SAMPLE_CTRL_DIFFERENTIAL_SIGNED_Pos,    /**< Result data for differential channels is unsigned */
    CY_SAR_DIFFERENTIAL_SIGNED    = 1uL << SAR_SAMPLE_CTRL_DIFFERENTIAL_SIGNED_Pos     /**< Result data for differential channels is signed */
}cy_en_sar_sample_ctrl_differential_format_t;

/** Configure number of samples for averaging.
* This applies only to channels with averaging enabled.
*/
typedef enum
{
    CY_SAR_AVG_CNT_2          = 0uL << SAR_SAMPLE_CTRL_AVG_CNT_Pos,    /**< Set samples averaged to 2 */
    CY_SAR_AVG_CNT_4          = 1uL << SAR_SAMPLE_CTRL_AVG_CNT_Pos,    /**< Set samples averaged to 4 */
    CY_SAR_AVG_CNT_8          = 2uL << SAR_SAMPLE_CTRL_AVG_CNT_Pos,    /**< Set samples averaged to 8 */
    CY_SAR_AVG_CNT_16         = 3uL << SAR_SAMPLE_CTRL_AVG_CNT_Pos,    /**< Set samples averaged to 16 */
    CY_SAR_AVG_CNT_32         = 4uL << SAR_SAMPLE_CTRL_AVG_CNT_Pos,    /**< Set samples averaged to 32 */
    CY_SAR_AVG_CNT_64         = 5uL << SAR_SAMPLE_CTRL_AVG_CNT_Pos,    /**< Set samples averaged to 64 */
    CY_SAR_AVG_CNT_128        = 6uL << SAR_SAMPLE_CTRL_AVG_CNT_Pos,    /**< Set samples averaged to 128 */
    CY_SAR_AVG_CNT_256        = 7uL << SAR_SAMPLE_CTRL_AVG_CNT_Pos     /**< Set samples averaged to 256 */
}cy_en_sar_sample_ctrl_avg_cnt_t;

/** Configure the averaging mode.
*
* - Sequential accumulate and dump: a channel will be sampled back to back.
*   The result is added to a running sum in a 20-bit register. At the end
*   of the scan, the accumulated value is shifted right to fit into 16 bits
*   and stored into the CHAN_RESULT register.
* - Sequential fixed:  a channel will be sampled back to back.
*   The result is added to a running sum in a 20-bit register. At the end
*   of the scan, the accumulated value is shifted right to fit into 12 bits
*   and stored into the CHAN_RESULT register.
* - Interleaved: a channel will be sampled once per scan.
*   The result is added to a running sum in a 16-bit register.
*   In the scan where the final averaging count is reached,
*   the accumulated value is shifted right to fit into 12 bits
*   and stored into the CHAN_RESULT register.
*   In all other scans, the CHAN_RESULT will have an invalid result.
*   In interleaved mode, make sure that the averaging
*   count is low enough to ensure that the intermediate value does not exceed 16 bits,
*   that is averaging count is 16 or less. Otherwise, the MSBs will be lost.
*   In the special case that averaging is enabled for all enabled channels
*   and interleaved mode is used, the interrupt frequency
*   will be reduced by a factor of the number of samples averaged.
*/
typedef enum
{
    CY_SAR_AVG_MODE_SEQUENTIAL_ACCUM    = 0uL,                               /**< Set mode to sequential accumulate and dump */
    CY_SAR_AVG_MODE_SEQUENTIAL_FIXED    = SAR_SAMPLE_CTRL_AVG_SHIFT_Msk,     /**< Set mode to sequential 12-bit fixed */
    CY_SAR_AVG_MODE_INTERLEAVED         = SAR_SAMPLE_CTRL_AVG_MODE_Msk,      /**< Set mode to interleaved. Number of samples per scan must be 16 or less. */
}cy_en_sar_sample_ctrl_avg_mode_t;

/** Configure the trigger mode.
*
* Firmware triggering is always enabled and can be single shot or continuous.
* Additionally, hardware triggering can be enabled with the option to be
* edge or level sensitive.
*/
typedef enum
{
    CY_SAR_TRIGGER_MODE_FW_ONLY        = 0uL,                                /**< Firmware trigger only, disable hardware trigger*/
    CY_SAR_TRIGGER_MODE_FW_AND_HWEDGE  = SAR_SAMPLE_CTRL_DSI_TRIGGER_EN_Msk, /**< Enable edge sensitive hardware trigger. Each rising edge will trigger a single scan. */
    CY_SAR_TRIGGER_MODE_FW_AND_HWLEVEL = SAR_SAMPLE_CTRL_DSI_TRIGGER_EN_Msk | SAR_SAMPLE_CTRL_DSI_TRIGGER_LEVEL_Msk, /**< Enable level sensitive hardware trigger. The SAR will continuously scan while the trigger signal is high. */
}cy_en_sar_sample_ctrl_trigger_mode_t;

/* \} */

/** \addtogroup group_sar_sample_time_shift_enums
* This set of enumerations aids in configuring the SAR SAMPLE_TIME* registers
* \{
*/
/** Configure the sample time by using these shifts */
typedef enum
{
    CY_SAR_SAMPLE_TIME0_SHIFT       = SAR_SAMPLE_TIME01_SAMPLE_TIME0_Pos,             /**< Shift for sample time 0 */
    CY_SAR_SAMPLE_TIME1_SHIFT       = SAR_SAMPLE_TIME01_SAMPLE_TIME1_Pos,             /**< Shift for sample time 1 */
    CY_SAR_SAMPLE_TIME2_SHIFT       = SAR_SAMPLE_TIME23_SAMPLE_TIME2_Pos,             /**< Shift for sample time 2 */
    CY_SAR_SAMPLE_TIME3_SHIFT       = SAR_SAMPLE_TIME23_SAMPLE_TIME3_Pos,             /**< Shift for sample time 3 */
}cy_en_sar_sample_time_shift_t;
/* \} */

/** \addtogroup group_sar_range_thres_register_enums
* This set of enumerations aids in configuring the SAR RANGE* registers
* \{
*/
/** Configure the lower and upper thresholds for range detection
*
* The SARSEQ supports range detection to allow for automatic detection of sample
* values compared to two programmable thresholds without CPU involvement.
* Range detection is defined by two global thresholds and a condition.
* The RANGE_LOW value defines the lower threshold and RANGE_HIGH defines
* the upper threshold of the range.
*
* Range detect is done after averaging, alignment, and sign extension (if applicable).
* In other words, the thresholds values must have the same data format as the result data.
* Range detection is always done for all channels scanned. By making RANGE_INTR_MASK=0,
* the firmware can choose to ignore the range detect interrupt for any channel.
*/
typedef enum
{
    CY_SAR_RANGE_LOW_SHIFT      = SAR_RANGE_THRES_RANGE_LOW_Pos,        /**< Shift for setting lower limit of range detection */
    CY_SAR_RANGE_HIGH_SHIFT     = SAR_RANGE_THRES_RANGE_HIGH_Pos,       /**< Shift for setting upper limit of range detection */
}cy_en_sar_range_thres_shift_t;

/** Configure the condition (below, inside, above, or outside) of the range detection interrupt */
typedef enum
{
    CY_SAR_RANGE_COND_BELOW     = 0uL,  /**< Range interrupt detected when result < RANGE_LOW */
    CY_SAR_RANGE_COND_INSIDE    = 1uL,  /**< Range interrupt detected when RANGE_LOW <= result < RANGE_HIGH */
    CY_SAR_RANGE_COND_ABOVE     = 2uL,  /**< Range interrupt detected when RANGE_HIGH <= result */
    CY_SAR_RANGE_COND_OUTSIDE   = 3uL,  /**< Range interrupt detected when result < RANGE_LOW || RANGE_HIGH <= result */
}cy_en_sar_range_detect_condition_t;
/* \} */

/** \addtogroup group_sar_chan_config_register_enums
* This set of enumerations aids in configuring the SAR CHAN_CONFIG register
* \{
*/
/** Configure the input mode of the channel
*
* - Single ended channel: the \ref cy_en_sar_ctrl_neg_sel_t selection in the \ref group_sar_init_struct_ctrl register
*   determines what drives the Vminus pin
* - Differential paired: Vplus and Vminus are a pair. Bit 0 of \ref cy_en_sar_chan_config_pos_pin_addr_t "POS_PIN_ADDR"
*   is ignored and considered to be 0.
*   In other words, \ref cy_en_sar_chan_config_pos_pin_addr_t "POS_PIN_ADDR" points to the even pin of a pin pair.
*   The even pin is connected to Vplus and the odd pin is connected to Vminus.
*   \ref cy_en_sar_chan_config_pos_port_addr_t "POS_PORT_ADDR" is used to identify the port that contains the pins.
* - Differential unpaired: The \ref cy_en_sar_chan_config_neg_pin_addr_t "NEG_PIN_ADDR" and
*   \ref cy_en_sar_chan_config_neg_port_addr_t "NEG_PORT_ADDR" determine what drives the Vminus pin.
*   This is a variation of differential mode with no even-odd pair limitation
*/
typedef enum
{
    CY_SAR_CHAN_SINGLE_ENDED            = 0uL,                                     /**< Single ended channel */
    CY_SAR_CHAN_DIFFERENTIAL_PAIRED     = SAR_CHAN_CONFIG_DIFFERENTIAL_EN_Msk,     /**< Differential with even-odd pair limitation */
    CY_SAR_CHAN_DIFFERENTIAL_UNPAIRED   = SAR_CHAN_CONFIG_NEG_ADDR_EN_Msk          /**< Differential with no even-odd pair limitation */
}cy_en_sar_chan_config_input_mode_t;

/** Configure address of the pin connected to the Vplus terminal of the SARADC. */
typedef enum
{
    CY_SAR_CHAN_POS_PIN_ADDR_0     = 0uL,                                            /**< Pin 0 on port specified in \ref cy_en_sar_chan_config_pos_port_addr_t */
    CY_SAR_CHAN_POS_PIN_ADDR_1     = 1uL << SAR_CHAN_CONFIG_POS_PIN_ADDR_Pos,        /**< Pin 1 on port specified in \ref cy_en_sar_chan_config_pos_port_addr_t */
    CY_SAR_CHAN_POS_PIN_ADDR_2     = 2uL << SAR_CHAN_CONFIG_POS_PIN_ADDR_Pos,        /**< Pin 2 on port specified in \ref cy_en_sar_chan_config_pos_port_addr_t */
    CY_SAR_CHAN_POS_PIN_ADDR_3     = 3uL << SAR_CHAN_CONFIG_POS_PIN_ADDR_Pos,        /**< Pin 3 on port specified in \ref cy_en_sar_chan_config_pos_port_addr_t */
    CY_SAR_CHAN_POS_PIN_ADDR_4     = 4uL << SAR_CHAN_CONFIG_POS_PIN_ADDR_Pos,        /**< Pin 4 on port specified in \ref cy_en_sar_chan_config_pos_port_addr_t */
    CY_SAR_CHAN_POS_PIN_ADDR_5     = 5uL << SAR_CHAN_CONFIG_POS_PIN_ADDR_Pos,        /**< Pin 5 on port specified in \ref cy_en_sar_chan_config_pos_port_addr_t */
    CY_SAR_CHAN_POS_PIN_ADDR_6     = 6uL << SAR_CHAN_CONFIG_POS_PIN_ADDR_Pos,        /**< Pin 6 on port specified in \ref cy_en_sar_chan_config_pos_port_addr_t */
    CY_SAR_CHAN_POS_PIN_ADDR_7     = 7uL << SAR_CHAN_CONFIG_POS_PIN_ADDR_Pos,        /**< Pin 7 on port specified in \ref cy_en_sar_chan_config_pos_port_addr_t */
}cy_en_sar_chan_config_pos_pin_addr_t;

/** Configure address of the port that contains the pin connected to the Vplus terminal of the SARADC
*
* - \ref CY_SAR_POS_PORT_ADDR_SARMUX is for the dedicated SARMUX port (8 pins)
* - Port 1 through 4 are respectively the pins of CTB0, CTB1, CTB2, and CTB3 (if present)
* - Port 7, 5, and 6 (VPORT0/1/2) are the groups of internal signals that can be selected
*   in the SARMUX or AROUTE (if present).
*
* See the \ref group_sar_sarmux section for more guidance.
*/
typedef enum
{
    CY_SAR_POS_PORT_ADDR_SARMUX         = 0uL,                                       /**< Dedicated SARMUX port with 8 possible pins */
    CY_SAR_POS_PORT_ADDR_CTB0           = 1uL << SAR_CHAN_CONFIG_POS_PORT_ADDR_Pos,  /**< Outputs from CTB0, if present */
    CY_SAR_POS_PORT_ADDR_CTB1           = 2uL << SAR_CHAN_CONFIG_POS_PORT_ADDR_Pos,  /**< Outputs from CTB1, if present */
    CY_SAR_POS_PORT_ADDR_CTB2           = 3uL << SAR_CHAN_CONFIG_POS_PORT_ADDR_Pos,  /**< Outputs from CTB2, if present */
    CY_SAR_POS_PORT_ADDR_CTB3           = 4uL << SAR_CHAN_CONFIG_POS_PORT_ADDR_Pos,  /**< Outputs from CTB3, if present */
    CY_SAR_POS_PORT_ADDR_AROUTE_VIRT2   = 5uL << SAR_CHAN_CONFIG_POS_PORT_ADDR_Pos,  /**< AROUTE virtual port (VPORT2), if present */
    CY_SAR_POS_PORT_ADDR_AROUTE_VIRT1   = 6uL << SAR_CHAN_CONFIG_POS_PORT_ADDR_Pos,  /**< AROUTE virtual port (VPORT1), if present */
    CY_SAR_POS_PORT_ADDR_SARMUX_VIRT    = 7uL << SAR_CHAN_CONFIG_POS_PORT_ADDR_Pos,  /**< SARMUX virtual port for DieTemp and AMUXBUSA/B */
}cy_en_sar_chan_config_pos_port_addr_t;

/** Enable or disable averaging for the channel */
typedef enum
{
    CY_SAR_CHAN_AVG_DISABLE      = 0uL,                                 /**< Disable averaging for the channel */
    CY_SAR_CHAN_AVG_ENABLE       = 1uL << SAR_CHAN_CONFIG_AVG_EN_Pos    /**< Enable averaging for the channel */
}cy_en_sar_chan_config_avg_en_t;

/** Select which sample time to use for the channel.
* There are four global samples times available set by \ref group_sar_init_struct_sampleTime01 and
* \ref group_sar_init_struct_sampleTime23.
*/
typedef enum
{
    CY_SAR_CHAN_SAMPLE_TIME_0     = 0uL,                                          /**< Use sample time 0 for the channel */
    CY_SAR_CHAN_SAMPLE_TIME_1     = 1uL << SAR_CHAN_CONFIG_SAMPLE_TIME_SEL_Pos,   /**< Use sample time 1 for the channel */
    CY_SAR_CHAN_SAMPLE_TIME_2     = 2uL << SAR_CHAN_CONFIG_SAMPLE_TIME_SEL_Pos,   /**< Use sample time 2 for the channel */
    CY_SAR_CHAN_SAMPLE_TIME_3     = 3uL << SAR_CHAN_CONFIG_SAMPLE_TIME_SEL_Pos,   /**< Use sample time 3 for the channel */
}cy_en_sar_chan_config_sample_time_t;

/** Configure address of the pin connected to the Vminus terminal of the SARADC. */
typedef enum
{
    CY_SAR_CHAN_NEG_PIN_ADDR_0     = 0uL,                                            /**< Pin 0 on port specified in \ref cy_en_sar_chan_config_neg_port_addr_t */
    CY_SAR_CHAN_NEG_PIN_ADDR_1     = 1uL << SAR_CHAN_CONFIG_NEG_PIN_ADDR_Pos,        /**< Pin 1 on port specified in \ref cy_en_sar_chan_config_neg_port_addr_t */
    CY_SAR_CHAN_NEG_PIN_ADDR_2     = 2uL << SAR_CHAN_CONFIG_NEG_PIN_ADDR_Pos,        /**< Pin 2 on port specified in \ref cy_en_sar_chan_config_neg_port_addr_t */
    CY_SAR_CHAN_NEG_PIN_ADDR_3     = 3uL << SAR_CHAN_CONFIG_NEG_PIN_ADDR_Pos,        /**< Pin 3 on port specified in \ref cy_en_sar_chan_config_neg_port_addr_t */
    CY_SAR_CHAN_NEG_PIN_ADDR_4     = 4uL << SAR_CHAN_CONFIG_NEG_PIN_ADDR_Pos,        /**< Pin 4 on port specified in \ref cy_en_sar_chan_config_neg_port_addr_t */
    CY_SAR_CHAN_NEG_PIN_ADDR_5     = 5uL << SAR_CHAN_CONFIG_NEG_PIN_ADDR_Pos,        /**< Pin 5 on port specified in \ref cy_en_sar_chan_config_neg_port_addr_t */
    CY_SAR_CHAN_NEG_PIN_ADDR_6     = 6uL << SAR_CHAN_CONFIG_NEG_PIN_ADDR_Pos,        /**< Pin 6 on port specified in \ref cy_en_sar_chan_config_neg_port_addr_t */
    CY_SAR_CHAN_NEG_PIN_ADDR_7     = 7uL << SAR_CHAN_CONFIG_NEG_PIN_ADDR_Pos,        /**< Pin 7 on port specified in \ref cy_en_sar_chan_config_neg_port_addr_t */
}cy_en_sar_chan_config_neg_pin_addr_t;

/** Configure address of the port that contains the pin connected to the Vminus terminal of the SARADC.
*
* - Port 0 is 8 pins of the SARMUX
* - Port 7, 5, and 6 (VPORT0/1/2) are the groups of internal signals that can be selected
*   in the SARMUX or AROUTE (if present).
*/
typedef enum
{
    CY_SAR_NEG_PORT_ADDR_SARMUX         = 0uL,                                       /**< Dedicated SARMUX port with 8 possible pins */
    CY_SAR_NEG_PORT_ADDR_AROUTE_VIRT2   = 5uL << SAR_CHAN_CONFIG_POS_PORT_ADDR_Pos,  /**< AROUTE virtual port (VPORT2), if present */
    CY_SAR_NEG_PORT_ADDR_AROUTE_VIRT1   = 6uL << SAR_CHAN_CONFIG_POS_PORT_ADDR_Pos,  /**< AROUTE virtual port (VPORT1), if present */
    CY_SAR_NEG_PORT_ADDR_SARMUX_VIRT    = 7uL << SAR_CHAN_CONFIG_POS_PORT_ADDR_Pos,  /**< SARMUX virtual port for AMUXBUSA/B */
}cy_en_sar_chan_config_neg_port_addr_t;

/* \} */

/** \addtogroup group_sar_intr_mask_t_register_enums
* This set of enumerations aids in configuring the SAR INTR_MASK register
* \{
*/
/** Configure which signal will cause an interrupt event.
*
* - End of scan (EOS): occurs after completing a scan of all enabled channels
* - Overflow: occurs when hardware sets a new EOS interrupt while the previous interrupt
*   has not be cleared by the firmware
* - Firmware collision: occurs when firmware attempts to start one-shot
*   conversion while the SAR is busy.
*
* Enable all, one, or none of the interrupt events.
*/
typedef enum
{
    CY_SAR_INTR_MASK_NONE           = 0uL,                                  /**< Disable all interrupt sources */
    CY_SAR_INTR_EOS_MASK            = SAR_INTR_MASK_EOS_MASK_Msk,           /**< Enable end of scan (EOS) interrupt */
    CY_SAR_INTR_OVERFLOW_MASK       = SAR_INTR_MASK_OVERFLOW_MASK_Msk,      /**< Enable overflow interrupt */
    CY_SAR_INTR_FW_COLLISION_MASK   = SAR_INTR_MASK_FW_COLLISION_MASK_Msk,  /**< Enable firmware collision interrupt */
}cy_en_sar_intr_mask_t;

/* \} */

/** \addtogroup group_sar_mux_switch_register_enums
* This set of enumerations aids in configuring the \ref group_sar_init_struct_muxSwitch and \ref group_sar_init_struct_muxSwitchSqCtrl registers
* \{
*/

/** Firmware control for the SARMUX switches to connect analog signals to the SAR ADC
*
* To close multiple switches, "OR" the enum values together.
*
* See the \ref group_sar_sarmux section for more guidance.
*/
typedef enum
{
    /* SARMUX pins to Vplus */
    CY_SAR_MUX_FW_P0_VPLUS         = SAR_MUX_SWITCH0_MUX_FW_P0_VPLUS_Msk,    /**< Switch between Pin 0 of SARMUX and Vplus of SARADC */
    CY_SAR_MUX_FW_P1_VPLUS         = SAR_MUX_SWITCH0_MUX_FW_P1_VPLUS_Msk,    /**< Switch between Pin 1 of SARMUX and Vplus of SARADC */
    CY_SAR_MUX_FW_P2_VPLUS         = SAR_MUX_SWITCH0_MUX_FW_P2_VPLUS_Msk,    /**< Switch between Pin 2 of SARMUX and Vplus of SARADC */
    CY_SAR_MUX_FW_P3_VPLUS         = SAR_MUX_SWITCH0_MUX_FW_P3_VPLUS_Msk,    /**< Switch between Pin 3 of SARMUX and Vplus of SARADC */
    CY_SAR_MUX_FW_P4_VPLUS         = SAR_MUX_SWITCH0_MUX_FW_P4_VPLUS_Msk,    /**< Switch between Pin 4 of SARMUX and Vplus of SARADC */
    CY_SAR_MUX_FW_P5_VPLUS         = SAR_MUX_SWITCH0_MUX_FW_P5_VPLUS_Msk,    /**< Switch between Pin 5 of SARMUX and Vplus of SARADC */
    CY_SAR_MUX_FW_P6_VPLUS         = SAR_MUX_SWITCH0_MUX_FW_P6_VPLUS_Msk,    /**< Switch between Pin 6 of SARMUX and Vplus of SARADC */
    CY_SAR_MUX_FW_P7_VPLUS         = SAR_MUX_SWITCH0_MUX_FW_P7_VPLUS_Msk,    /**< Switch between Pin 7 of SARMUX and Vplus of SARADC */

    /* SARMUX pins to Vminus */
    CY_SAR_MUX_FW_P0_VMINUS        = SAR_MUX_SWITCH0_MUX_FW_P0_VMINUS_Msk,   /**< Switch between Pin 0 of SARMUX and Vminus of SARADC */
    CY_SAR_MUX_FW_P1_VMINUS        = SAR_MUX_SWITCH0_MUX_FW_P1_VMINUS_Msk,   /**< Switch between Pin 1 of SARMUX and Vminus of SARADC */
    CY_SAR_MUX_FW_P2_VMINUS        = SAR_MUX_SWITCH0_MUX_FW_P2_VMINUS_Msk,   /**< Switch between Pin 2 of SARMUX and Vminus of SARADC */
    CY_SAR_MUX_FW_P3_VMINUS        = SAR_MUX_SWITCH0_MUX_FW_P3_VMINUS_Msk,   /**< Switch between Pin 3 of SARMUX and Vminus of SARADC */
    CY_SAR_MUX_FW_P4_VMINUS        = SAR_MUX_SWITCH0_MUX_FW_P4_VMINUS_Msk,   /**< Switch between Pin 4 of SARMUX and Vminus of SARADC */
    CY_SAR_MUX_FW_P5_VMINUS        = SAR_MUX_SWITCH0_MUX_FW_P5_VMINUS_Msk,   /**< Switch between Pin 5 of SARMUX and Vminus of SARADC */
    CY_SAR_MUX_FW_P6_VMINUS        = SAR_MUX_SWITCH0_MUX_FW_P6_VMINUS_Msk,   /**< Switch between Pin 6 of SARMUX and Vminus of SARADC */
    CY_SAR_MUX_FW_P7_VMINUS        = SAR_MUX_SWITCH0_MUX_FW_P7_VMINUS_Msk,   /**< Switch between Pin 7 of SARMUX and Vminus of SARADC */

    /* Vssa to Vminus and temperature sensor to Vplus */
    CY_SAR_MUX_FW_VSSA_VMINUS      = SAR_MUX_SWITCH0_MUX_FW_VSSA_VMINUS_Msk,    /**< Switch between VSSA and Vminus of SARADC */
    CY_SAR_MUX_FW_TEMP_VPLUS       = SAR_MUX_SWITCH0_MUX_FW_TEMP_VPLUS_Msk,     /**< Switch between the DieTemp sensor and vplus of SARADC */

    /* Amuxbus A and B to Vplus and Vminus */
    CY_SAR_MUX_FW_AMUXBUSA_VPLUS   = SAR_MUX_SWITCH0_MUX_FW_AMUXBUSA_VPLUS_Msk,     /**< Switch between AMUXBUSA and vplus of SARADC */
    CY_SAR_MUX_FW_AMUXBUSB_VPLUS   = SAR_MUX_SWITCH0_MUX_FW_AMUXBUSB_VPLUS_Msk,     /**< Switch between AMUXBUSB and vplus of SARADC */
    CY_SAR_MUX_FW_AMUXBUSA_VMINUS  = SAR_MUX_SWITCH0_MUX_FW_AMUXBUSA_VMINUS_Msk,    /**< Switch between AMUXBUSA and vminus of SARADC */
    CY_SAR_MUX_FW_AMUXBUSB_VMINUS  = SAR_MUX_SWITCH0_MUX_FW_AMUXBUSB_VMINUS_Msk,    /**< Switch between AMUXBUSB and vminus of SARADC */

    /* Sarbus 0 and 1 to Vplus and Vminus */
    CY_SAR_MUX_FW_SARBUS0_VPLUS    = SAR_MUX_SWITCH0_MUX_FW_SARBUS0_VPLUS_Msk,      /**< Switch between SARBUS0 and vplus of SARADC */
    CY_SAR_MUX_FW_SARBUS1_VPLUS    = SAR_MUX_SWITCH0_MUX_FW_SARBUS1_VPLUS_Msk,      /**< Switch between SARBUS1 and vplus of SARADC */
    CY_SAR_MUX_FW_SARBUS0_VMINUS   = SAR_MUX_SWITCH0_MUX_FW_SARBUS0_VMINUS_Msk,     /**< Switch between SARBUS0 and vminus of SARADC */
    CY_SAR_MUX_FW_SARBUS1_VMINUS   = SAR_MUX_SWITCH0_MUX_FW_SARBUS1_VMINUS_Msk,     /**< Switch between SARBUS1 and vminus of SARADC */

    /* SARMUX pins to Core IO */
    CY_SAR_MUX_FW_P4_COREIO0       = SAR_MUX_SWITCH0_MUX_FW_P4_COREIO0_Msk,      /**< Switch between Pin 4 of SARMUX and coreio0, if present */
    CY_SAR_MUX_FW_P5_COREIO1       = SAR_MUX_SWITCH0_MUX_FW_P5_COREIO1_Msk,      /**< Switch between Pin 5 of SARMUX and coreio1, if present */
    CY_SAR_MUX_FW_P6_COREIO2       = SAR_MUX_SWITCH0_MUX_FW_P6_COREIO2_Msk,      /**< Switch between Pin 6 of SARMUX and coreio2, if present */
    CY_SAR_MUX_FW_P7_COREIO3       = SAR_MUX_SWITCH0_MUX_FW_P7_COREIO3_Msk,      /**< Switch between Pin 7 of SARMUX and coreio3, if present */
}cy_en_sar_mux_switch_fw_ctrl_t;

/** Mask definitions of SARMUX switches that can be controlled by the SARSEQ.
*
* To enable sequencer control of multiple switches, "OR" the enum values together.
*
* See the \ref group_sar_sarmux section for more guidance.
*/
typedef enum
{
    CY_SAR_MUX_SQ_CTRL_P0           = SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_P0_Msk,        /**< Enable SARSEQ control of Pin 0 switches (for Vplus and Vminus) of SARMUX dedicated port */
    CY_SAR_MUX_SQ_CTRL_P1           = SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_P1_Msk,        /**< Enable SARSEQ control of Pin 1 switches (for Vplus and Vminus) of SARMUX dedicated port */
    CY_SAR_MUX_SQ_CTRL_P2           = SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_P2_Msk,        /**< Enable SARSEQ control of Pin 2 switches (for Vplus and Vminus) of SARMUX dedicated port */
    CY_SAR_MUX_SQ_CTRL_P3           = SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_P3_Msk,        /**< Enable SARSEQ control of Pin 3 switches (for Vplus and Vminus) of SARMUX dedicated port */
    CY_SAR_MUX_SQ_CTRL_P4           = SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_P4_Msk,        /**< Enable SARSEQ control of Pin 4 switches (for Vplus and Vminus) of SARMUX dedicated port */
    CY_SAR_MUX_SQ_CTRL_P5           = SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_P5_Msk,        /**< Enable SARSEQ control of Pin 5 switches (for Vplus and Vminus) of SARMUX dedicated port */
    CY_SAR_MUX_SQ_CTRL_P6           = SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_P6_Msk,        /**< Enable SARSEQ control of Pin 6 switches (for Vplus and Vminus) of SARMUX dedicated port */
    CY_SAR_MUX_SQ_CTRL_P7           = SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_P7_Msk,        /**< Enable SARSEQ control of Pin 7 switches (for Vplus and Vminus) of SARMUX dedicated port */
    CY_SAR_MUX_SQ_CTRL_VSSA         = SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_VSSA_Msk,      /**< Enable SARSEQ control of the switch between VSSA and Vminus */
    CY_SAR_MUX_SQ_CTRL_TEMP         = SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_TEMP_Msk,      /**< Enable SARSEQ control of the switch between DieTemp and Vplus */
    CY_SAR_MUX_SQ_CTRL_AMUXBUSA     = SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_AMUXBUSA_Msk,  /**< Enable SARSEQ control of AMUXBUSA switches (for Vplus and Vminus) */
    CY_SAR_MUX_SQ_CTRL_AMUXBUSB     = SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_AMUXBUSB_Msk,  /**< Enable SARSEQ control of AMUXBUSB switches (for Vplus and Vminus) */
    CY_SAR_MUX_SQ_CTRL_SARBUS0      = SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_SARBUS0_Msk,   /**< Enable SARSEQ control of SARBUS0 switches (for Vplus and Vminus) */
    CY_SAR_MUX_SQ_CTRL_SARBUS1      = SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_SARBUS1_Msk,   /**< Enable SARSEQ control of SARBUS1 switches (for Vplus and Vminus) */
}cy_en_sar_mux_switch_sq_ctrl_t;

/* \} */

/** \} group_sar_enums */

/** \addtogroup group_sar_data_structures
* \{
*/

/***************************************
*       Configuration Structures
***************************************/

/** This structure is used to initialize the SAR ADC subsystem.
*
* The SAR ADC subsystem is highly configurable with many options.
* When calling \ref Cy_SAR_Init, provide a pointer to the structure containing this configuration data.
* A set of enumerations is provided in this
* driver to help with configuring this structure.
*
* See the \ref group_sar_initialization section for guidance.
**/
typedef struct
{
    uint32_t ctrl;                                      /**< Control register settings (applies to all channels) */
    uint32_t sampleCtrl;                                /**< Sample control register settings (applies to all channels) */
    uint32_t sampleTime01;                              /**< Sample time in ADC clocks for Sample Time 0 and 1 */
    uint32_t sampleTime23;                              /**< Sample time in ADC clocks for Sample Time 2 and 3 */
    uint32_t rangeThres;                                /**< Range detect threshold register for all channels */
    cy_en_sar_range_detect_condition_t rangeCond;       /**< Range detect condition (below, inside, output, or above) for all channels */
    uint32_t chanEn;                                    /**< Enable bits for the channels */
    uint32_t chanConfig[CY_SAR_MAX_NUM_CHANNELS];       /**< Channel configuration */
    uint32_t intrMask;                                  /**< Interrupt enable mask */
    uint32_t satIntrMask;                               /**< Saturation detection interrupt enable mask */
    uint32_t rangeIntrMask;                             /**< Range detection interrupt enable mask  */
    uint32_t muxSwitch;                                 /**< SARMUX firmware switches to connect analog signals to SAR */
    uint32_t muxSwitchSqCtrl;                           /**< Enable SARSEQ control of specific SARMUX switches */
    bool configRouting;                                 /**< Configure or ignore routing related registers (muxSwitch, muxSwitchSqCtrl) */
    uint32_t vrefMvValue;                               /**< Reference voltage in millivolts used in converting counts to volts */
} cy_stc_sar_config_t;

/** This structure is used by the driver to backup the state of the SAR
* before entering sleep so that it can be re-enabled after waking up */
typedef struct
{
    uint32_t hwEnabled;         /**< SAR enabled state */
    uint32_t continuous;        /**< State of the continuous bit */
} cy_stc_sar_state_backup_t;

/** \} group_sar_data_structures */

/** \addtogroup group_sar_functions
* \{
*/

/***************************************
*        Function Prototypes
***************************************/

/** \addtogroup group_sar_functions_basic
* This set of functions is for initialization and basic usage
* \{
*/
cy_en_sar_status_t Cy_SAR_Init(SAR_Type *base, const cy_stc_sar_config_t *config);
cy_en_sar_status_t Cy_SAR_DeInit(SAR_Type *base, bool deInitRouting);
void Cy_SAR_Enable(SAR_Type *base);
__STATIC_INLINE void Cy_SAR_Disable(SAR_Type *base);
void Cy_SAR_StartConvert(SAR_Type *base, cy_en_sar_start_convert_sel_t startSelect);
void Cy_SAR_StopConvert(SAR_Type *base);
cy_en_sar_status_t Cy_SAR_IsEndConversion(SAR_Type *base, cy_en_sar_return_mode_t retMode);
int16_t Cy_SAR_GetResult16(const SAR_Type *base, uint32_t chan);
int32_t Cy_SAR_GetResult32(const SAR_Type *base, uint32_t chan);
__STATIC_INLINE uint32_t Cy_SAR_GetChanResultUpdated(const SAR_Type *base);
/** \} */

/** \addtogroup group_sar_functions_power
* This set of functions is for Deep Sleep entry and exit
* \{
*/
cy_en_syspm_status_t Cy_SAR_DeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams);
void Cy_SAR_Sleep(SAR_Type *base);
void Cy_SAR_Wakeup(SAR_Type *base);
/** \} */

/** \addtogroup group_sar_functions_config
* This set of functions allows changes to the SAR configuration
* after initialization.
* \{
*/
void Cy_SAR_SetConvertMode(SAR_Type *base, cy_en_sar_sample_ctrl_trigger_mode_t mode);
__STATIC_INLINE void Cy_SAR_SetChanMask(SAR_Type *base, uint32_t enableMask);
void Cy_SAR_SetLowLimit(SAR_Type *base, uint32_t lowLimit);
void Cy_SAR_SetHighLimit(SAR_Type *base, uint32_t highLimit);
__STATIC_INLINE void Cy_SAR_SetRangeCond(SAR_Type *base, cy_en_sar_range_detect_condition_t cond);
/** \} */

/** \addtogroup group_sar_functions_countsto
* This set of functions performs counts to *volts conversions.
* \{
*/
int16_t Cy_SAR_RawCounts2Counts(const SAR_Type *base, uint32_t chan, int16_t adcCounts);
float32_t Cy_SAR_CountsTo_Volts(const SAR_Type *base, uint32_t chan, int16_t adcCounts);
int16_t Cy_SAR_CountsTo_mVolts(const SAR_Type *base, uint32_t chan, int16_t adcCounts);
int32_t Cy_SAR_CountsTo_uVolts(const SAR_Type *base, uint32_t chan, int16_t adcCounts);
cy_en_sar_status_t Cy_SAR_SetOffset(uint32_t chan, int16_t offset);
cy_en_sar_status_t Cy_SAR_SetGain(uint32_t chan, int32_t adcGain);
/** \} */

/** \addtogroup group_sar_functions_switches
* This set of functions is for controlling/querying the SARMUX switches
* \{
*/
void Cy_SAR_SetAnalogSwitch(SAR_Type *base, cy_en_sar_switch_register_sel_t switchSelect, uint32_t switchMask, cy_en_sar_switch_state_t state);
uint32_t Cy_SAR_GetAnalogSwitch(const SAR_Type *base, cy_en_sar_switch_register_sel_t switchSelect);
__STATIC_INLINE void Cy_SAR_SetVssaVminusSwitch(SAR_Type *base, cy_en_sar_switch_state_t state);
void Cy_SAR_SetSwitchSarSeqCtrl(SAR_Type *base, uint32_t switchMask, cy_en_sar_switch_sar_seq_ctrl_t ctrl);
__STATIC_INLINE void Cy_SAR_SetVssaSarSeqCtrl(SAR_Type *base, cy_en_sar_switch_sar_seq_ctrl_t ctrl);
/** \} */

/** \addtogroup group_sar_functions_interrupt
* This set of functions are related to SAR interrupts.
* \{
*/
__STATIC_INLINE uint32_t Cy_SAR_GetInterruptStatus(const SAR_Type *base);
__STATIC_INLINE void Cy_SAR_ClearInterrupt(SAR_Type *base, uint32_t intrMask);
__STATIC_INLINE void Cy_SAR_SetInterrupt(SAR_Type *base, uint32_t intrMask);
__STATIC_INLINE void Cy_SAR_SetInterruptMask(SAR_Type *base, uint32_t intrMask);
__STATIC_INLINE uint32_t Cy_SAR_GetInterruptMask(const SAR_Type *base);
__STATIC_INLINE uint32_t Cy_SAR_GetInterruptStatusMasked(const SAR_Type *base);

__STATIC_INLINE uint32_t Cy_SAR_GetRangeInterruptStatus(const SAR_Type *base);
__STATIC_INLINE void Cy_SAR_ClearRangeInterrupt(SAR_Type *base, uint32_t chanMask);
__STATIC_INLINE void Cy_SAR_SetRangeInterrupt(SAR_Type *base, uint32_t chanMask);
__STATIC_INLINE void Cy_SAR_SetRangeInterruptMask(SAR_Type *base, uint32_t chanMask);
__STATIC_INLINE uint32_t Cy_SAR_GetRangeInterruptMask(const SAR_Type *base);
__STATIC_INLINE uint32_t Cy_SAR_GetRangeInterruptStatusMasked(const SAR_Type *base);

__STATIC_INLINE uint32_t Cy_SAR_GetSatInterruptStatus(const SAR_Type *base);
__STATIC_INLINE void Cy_SAR_ClearSatInterrupt(SAR_Type *base, uint32_t chanMask);
__STATIC_INLINE void Cy_SAR_SetSatInterrupt(SAR_Type *base, uint32_t chanMask);
__STATIC_INLINE void Cy_SAR_SetSatInterruptMask(SAR_Type *base, uint32_t chanMask);
__STATIC_INLINE uint32_t Cy_SAR_GetSatInterruptMask(const SAR_Type *base);
__STATIC_INLINE uint32_t Cy_SAR_GetSatInterruptStatusMasked(const SAR_Type *base);

__STATIC_INLINE uint32_t Cy_SAR_GetInterruptCause(const SAR_Type *base);
/** \} */


/** \addtogroup group_sar_functions_helper
* This set of functions is for useful configuration query
* \{
*/
bool Cy_SAR_IsChannelSigned(const SAR_Type *base, uint32_t chan);
bool Cy_SAR_IsChannelSingleEnded(const SAR_Type *base, uint32_t chan);
__STATIC_INLINE bool Cy_SAR_IsChannelDifferential(const SAR_Type *base, uint32_t chan);
/** \} */

/** \addtogroup group_sar_functions_basic
* \{
*/

/*******************************************************************************
* Function Name: Cy_SAR_Disable
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
__STATIC_INLINE void Cy_SAR_Disable(SAR_Type *base)
{
    base->CTRL &= ~SAR_CTRL_ENABLED_Msk;
}

/*******************************************************************************
* Function Name: Cy_SAR_GetChanResultUpdated
****************************************************************************//**
*
* Return whether the RESULT register has been updated or not.
* If the bit is high, the corresponding channel RESULT register was updated,
* i.e. was sampled during the previous scan and, in case of Interleaved averaging,
* reached the averaging count.
* If the bit is low, the corresponding channel is not enabled or the averaging count
* is not yet reached for Interleaved averaging.
*
* \param base
* Pointer to structure describing registers
*
* \return
* Each bit of the result corresponds to the channel.
* Bit 0 is for channel 0, etc.
*
* \funcusage
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_GET_CHAN_RESULT_UPDATED
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SAR_GetChanResultUpdated(const SAR_Type *base)
{
    return base->CHAN_RESULT_UPDATED;
}
/** \} */

/** \addtogroup group_sar_functions_config
* \{
*/
/*******************************************************************************
* Function Name: Cy_SAR_SetChanMask
****************************************************************************//**
*
* Set the enable/disable mask for the channels.
*
* \param base
* Pointer to structure describing registers
*
* \param enableMask
* Channel enable/disable mask. Each bit corresponds to a channel.
* - 0: the corresponding channel is disabled.
* - 1: the corresponding channel is enabled; it will be included in the next scan.
*
* \return None
*
* \funcusage
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_SET_CHAN_MASK
*
*******************************************************************************/
__STATIC_INLINE void Cy_SAR_SetChanMask(SAR_Type *base, uint32_t enableMask)
{
    CY_ASSERT_L2(CY_SAR_CHANMASK(enableMask));

    base->CHAN_EN = enableMask;
}

/*******************************************************************************
* Function Name: Cy_SAR_SetRangeCond
****************************************************************************//**
*
* Set the condition in which range detection interrupts are triggered.
*
* \param base
* Pointer to structure describing registers
*
* \param cond
* A value of the enum \ref cy_en_sar_range_detect_condition_t.
*
* \return None
*
* \funcusage
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_SET_RANGE_COND
*
*******************************************************************************/
__STATIC_INLINE void Cy_SAR_SetRangeCond(SAR_Type *base, cy_en_sar_range_detect_condition_t cond)
{
    CY_ASSERT_L3(CY_SAR_RANGECOND(cond));

    base->RANGE_COND = (uint32_t)cond << SAR_RANGE_COND_RANGE_COND_Pos;
}

/** \} */

/** \addtogroup group_sar_functions_interrupt
* \{
*/
/*******************************************************************************
* Function Name: Cy_SAR_GetInterruptStatus
****************************************************************************//**
*
* Return the interrupt register status.
*
* \param base
* Pointer to structure describing registers
*
* \return Interrupt status
*
* \funcusage
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_ISR
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SAR_GetInterruptStatus(const SAR_Type *base)
{
    return base->INTR;
}

/*******************************************************************************
* Function Name: Cy_SAR_ClearInterrupt
****************************************************************************//**
*
* Clear the interrupt.
* The interrupt must be cleared with this function so that the hardware
* can set subsequent interrupts and those interrupts can be forwarded
* to the interrupt controller, if enabled.
*
* \param base
* Pointer to structure describing registers
*
* \param intrMask
* The mask of interrupts to clear. Typically this will be the value returned
* from \ref Cy_SAR_GetInterruptStatus.
* Alternately, select one or more values from \ref cy_en_sar_intr_mask_t and "OR" them together.
* - \ref CY_SAR_INTR_EOS_MASK
* - \ref CY_SAR_INTR_OVERFLOW_MASK
* - \ref CY_SAR_INTR_FW_COLLISION_MASK
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_SAR_ClearInterrupt(SAR_Type *base, uint32_t intrMask)
{
    CY_ASSERT_L2(CY_SAR_INTRMASK(intrMask));

    base->INTR = intrMask;

    /* Dummy read for buffered writes. */
    (void) base->INTR;
}

/*******************************************************************************
* Function Name: Cy_SAR_SetInterrupt
****************************************************************************//**
*
* Trigger an interrupt with software.
*
* \param base
* Pointer to structure describing registers
*
* \param intrMask
* The mask of interrupts to set.
* Select one or more values from \ref cy_en_sar_intr_mask_t and "OR" them together.
* - \ref CY_SAR_INTR_EOS_MASK
* - \ref CY_SAR_INTR_OVERFLOW_MASK
* - \ref CY_SAR_INTR_FW_COLLISION_MASK
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_SAR_SetInterrupt(SAR_Type *base, uint32_t intrMask)
{
    CY_ASSERT_L2(CY_SAR_INTRMASK(intrMask));

    base->INTR_SET = intrMask;
}

/*******************************************************************************
* Function Name: Cy_SAR_SetInterruptMask
****************************************************************************//**
*
* Enable which interrupts can trigger the CPU interrupt controller.
*
* \param base
* Pointer to structure describing registers
*
* \param intrMask
* The mask of interrupts. Select one or more values from \ref cy_en_sar_intr_mask_t
* and "OR" them together.
* - \ref CY_SAR_INTR_MASK_NONE : Disable EOS, overflow, and firmware collision interrupts.
* - \ref CY_SAR_INTR_EOS_MASK
* - \ref CY_SAR_INTR_OVERFLOW_MASK
* - \ref CY_SAR_INTR_FW_COLLISION_MASK
*
* \return None
*
* \funcusage
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_SET_INTERRUPT_MASK
*
*******************************************************************************/
__STATIC_INLINE void Cy_SAR_SetInterruptMask(SAR_Type *base, uint32_t intrMask)
{
    CY_ASSERT_L2(CY_SAR_INTRMASK(intrMask));

    base->INTR_MASK = intrMask;
}

/*******************************************************************************
* Function Name: Cy_SAR_GetInterruptMask
****************************************************************************//**
*
* Return which interrupts can trigger the CPU interrupt controller
* as configured by \ref Cy_SAR_SetInterruptMask.
*
* \param base
* Pointer to structure describing registers
*
* \return
* Interrupt mask. Compare this value with masks in \ref cy_en_sar_intr_mask_t.
*
* \funcusage
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_GET_INTERRUPT_MASK
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SAR_GetInterruptMask(const SAR_Type *base)
{
    return base->INTR_MASK;
}

/*******************************************************************************
* Function Name: Cy_SAR_GetInterruptStatusMasked
****************************************************************************//**
*
* Return the bitwise AND between the interrupt request and mask registers.
* See \ref Cy_SAR_GetInterruptStatus and \ref Cy_SAR_GetInterruptMask.
*
* \param base
* Pointer to structure describing registers
*
* \return
* Bitwise AND of the interrupt request and mask registers
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SAR_GetInterruptStatusMasked(const SAR_Type *base)
{
    return base->INTR_MASKED;
}

/*******************************************************************************
* Function Name: Cy_SAR_GetRangeInterruptStatus
****************************************************************************//**
*
* Return the range interrupt register status.
* If the status bit is low for a channel, the channel may not be enabled
* (\ref Cy_SAR_SetChanMask), range detection is not enabled for the
* channel (\ref Cy_SAR_SetRangeInterruptMask), or range detection was not
* triggered for the channel.
*
* \param base
* Pointer to structure describing registers
*
* \return
* The range interrupt status for all channels. Bit 0 is for channel 0, etc.
*
* \funcusage
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_GET_RANGE_INTERRUPT_STATUS
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SAR_GetRangeInterruptStatus(const SAR_Type *base)
{
    return base->RANGE_INTR;
}

/*******************************************************************************
* Function Name: Cy_SAR_ClearRangeInterrupt
****************************************************************************//**
*
* Clear the range interrupt for the specified channel mask.
* The interrupt must be cleared with this function so that
* the hardware can set subset interrupts and those interrupts can
* be forwarded to the interrupt controller, if enabled.
*
* \param base
* Pointer to structure describing registers
*
* \param chanMask
* The channel mask. Bit 0 is for channel 0, etc.
* Typically, this is the value returned from \ref Cy_SAR_GetRangeInterruptStatus.
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_SAR_ClearRangeInterrupt(SAR_Type *base, uint32_t chanMask)
{
    CY_ASSERT_L2(CY_SAR_CHANMASK(chanMask));

    base->RANGE_INTR = chanMask;

    /* Dummy read for buffered writes. */
    (void) base->RANGE_INTR;
}

/*******************************************************************************
* Function Name: Cy_SAR_SetRangeInterrupt
****************************************************************************//**
*
* Trigger a range interrupt with software for the specific channel mask.
*
* \param base
* Pointer to structure describing registers
*
* \param chanMask
* The channel mask. Bit 0 is for channel 0, etc.
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_SAR_SetRangeInterrupt(SAR_Type *base, uint32_t chanMask)
{
    CY_ASSERT_L2(CY_SAR_CHANMASK(chanMask));

    base->RANGE_INTR_SET = chanMask;
}

/*******************************************************************************
* Function Name: Cy_SAR_SetRangeInterruptMask
****************************************************************************//**
*
* Enable which channels can trigger a range interrupt.
*
* \param base
* Pointer to structure describing registers
*
* \param chanMask
* The channel mask. Bit 0 is for channel 0, etc.
*
* \return None
*
* \funcusage
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_SET_RANGE_INTERRUPT_MASK
*
*******************************************************************************/
__STATIC_INLINE void Cy_SAR_SetRangeInterruptMask(SAR_Type *base, uint32_t chanMask)
{
    CY_ASSERT_L2(CY_SAR_CHANMASK(chanMask));

    base->RANGE_INTR_MASK = chanMask;
}

/*******************************************************************************
* Function Name: Cy_SAR_GetRangeInterruptMask
****************************************************************************//**
*
* Return which interrupts can trigger a range interrupt as configured by
* \ref Cy_SAR_SetRangeInterruptMask.
*
* \param base
* Pointer to structure describing registers
*
* \return
* The range interrupt mask
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SAR_GetRangeInterruptMask(const SAR_Type *base)
{
    return base->RANGE_INTR_MASK;
}

/*******************************************************************************
* Function Name: Cy_SAR_GetRangeInterruptStatusMasked
****************************************************************************//**
*
* Return the bitwise AND between the range interrupt request and mask registers.
* See \ref Cy_SAR_GetRangeInterruptStatus and \ref Cy_SAR_GetRangeInterruptMask.
*
* \param base
* Pointer to structure describing registers
*
* \return
* Bitwise AND between of range interrupt request and mask
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SAR_GetRangeInterruptStatusMasked(const SAR_Type *base)
{
    return base->RANGE_INTR_MASKED;
}

/*******************************************************************************
* Function Name: Cy_SAR_GetSatInterruptStatus
****************************************************************************//**
*
* Return the saturate interrupt register status.
* If the status bit is low for a channel, the channel may not be enabled
* (\ref Cy_SAR_SetChanMask), saturation detection is not enabled for the
* channel (\ref Cy_SAR_SetSatInterruptMask), or saturation detection was not
* triggered for the channel.
*
* \param base
* Pointer to structure describing registers
*
* \return
* The saturate interrupt status for all channels. Bit 0 is for channel 0, etc.
*
* \funcusage
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_GET_SAT_INTERRUPT_STATUS
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SAR_GetSatInterruptStatus(const SAR_Type *base)
{
    return base->SATURATE_INTR;
}

/*******************************************************************************
* Function Name: Cy_SAR_ClearSatInterrupt
****************************************************************************//**
*
* Clear the saturate interrupt for the specified channel mask.
* The interrupt must be cleared with this function so that the hardware
* can set subsequent interrupts and those interrupts can be forwarded
* to the interrupt controller, if enabled.
*
* \param base
* Pointer to structure describing registers
*
* \param chanMask
* The channel mask. Bit 0 is for channel 0, etc.
* Typically, this is the value returned from \ref Cy_SAR_GetSatInterruptStatus.
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_SAR_ClearSatInterrupt(SAR_Type *base, uint32_t chanMask)
{
    CY_ASSERT_L2(CY_SAR_CHANMASK(chanMask));

    base->SATURATE_INTR = chanMask;

    /* Dummy read for buffered writes. */
    (void) base->SATURATE_INTR;
}

/*******************************************************************************
* Function Name: Cy_SAR_SetSatInterrupt
****************************************************************************//**
*
* Trigger a saturate interrupt with software for the specific channel mask.
*
* \param base
* Pointer to structure describing registers
*
* \param chanMask
* The channel mask. Bit 0 is for channel 0, etc.
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_SAR_SetSatInterrupt(SAR_Type *base, uint32_t chanMask)
{
    CY_ASSERT_L2(CY_SAR_CHANMASK(chanMask));

    base->SATURATE_INTR_SET = chanMask;
}

/*******************************************************************************
* Function Name: Cy_SAR_SetSatInterruptMask
****************************************************************************//**
*
* Enable which channels can trigger a saturate interrupt.
*
* \param base
* Pointer to structure describing registers
*
* \param chanMask
* The channel mask. Bit 0 is for channel 0, etc.
*
* \return None
*
* \funcusage
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_GET_SAT_INTERRUPT_MASK
*
*******************************************************************************/
__STATIC_INLINE void Cy_SAR_SetSatInterruptMask(SAR_Type *base, uint32_t chanMask)
{
    CY_ASSERT_L2(CY_SAR_CHANMASK(chanMask));

    base->SATURATE_INTR_MASK = chanMask;
}

/*******************************************************************************
* Function Name: Cy_SAR_GetSatInterruptMask
****************************************************************************//**
*
* Return which interrupts can trigger a saturate interrupt as configured
* by \ref Cy_SAR_SetSatInterruptMask.
*
* \param base
* Pointer to structure describing registers
*
* \return
* The saturate interrupt mask. Bit 0 is for channel 0, etc.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SAR_GetSatInterruptMask(const SAR_Type *base)
{
    return base->SATURATE_INTR_MASK;
}

/*******************************************************************************
* Function Name: Cy_SAR_GetSatInterruptStatusMasked
****************************************************************************//**
*
* Return the bitwise AND between the saturate interrupt request and mask registers.
* See \ref Cy_SAR_GetSatInterruptStatus and \ref Cy_SAR_GetSatInterruptMask.
*
* \param base
* Pointer to structure describing registers
*
* \return
* Bitwise AND of the saturate interrupt request and mask
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SAR_GetSatInterruptStatusMasked(const SAR_Type *base)
{
    return base->SATURATE_INTR_MASKED;
}

/*******************************************************************************
* Function Name: Cy_SAR_GetInterruptCause
****************************************************************************//**
*
* Return the cause of the interrupt.
* The interrupt routine can be called due to one of the following events:
*   - End of scan (EOS)
*   - Overflow
*   - Firmware collision
*   - Saturation detected on one or more channels
*   - Range detected on one or more channels
*
* \param base
* Pointer to structure describing registers
*
* \return
* Mask of what caused the interrupt. Compare this value with one of these masks:
*   - SAR_INTR_CAUSE_EOS_MASKED_MIR_Msk : EOS caused the interrupt
*   - SAR_INTR_CAUSE_OVERFLOW_MASKED_MIR_Msk : Overflow caused the interrupt
*   - SAR_INTR_CAUSE_FW_COLLISION_MASKED_MIR_Msk : Firmware collision cause the interrupt
*   - SAR_INTR_CAUSE_SATURATE_MASKED_RED_Msk : Saturation detection on one or more channels caused the interrupt
*   - SAR_INTR_CAUSE_RANGE_MASKED_RED_Msk : Range detection on one or more channels caused the interrupt
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SAR_GetInterruptCause(const SAR_Type *base)
{
    return base->INTR_CAUSE;
}
/** \} */

/** \addtogroup group_sar_functions_helper
* \{
*/
/*******************************************************************************
* Function Name: Cy_SAR_IsChannelDifferential
****************************************************************************//**
*
* Return true of channel is differential, else false.
*
* \param base
* Pointer to structure describing registers
*
* \param chan
* The channel to check, starting at 0.
*
* \return
* A false is return if chan is invalid.
*
* \funcusage
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_IS_CHANNEL_DIFF
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SAR_IsChannelDifferential(const SAR_Type *base, uint32_t chan)
{
    return !Cy_SAR_IsChannelSingleEnded(base, chan);
}
/** \} */

/** \addtogroup group_sar_functions_switches
* \{
*/
/*******************************************************************************
* Function Name: Cy_SAR_SetVssaVminusSwitch
****************************************************************************//**
*
* Open or close the switch between VSSA and Vminus of the SARADC through firmware.
* This function calls \ref Cy_SAR_SetAnalogSwitch with switchSelect set to
* \ref CY_SAR_MUX_SWITCH0 and switchMask set to SAR_MUX_SWITCH0_MUX_FW_VSSA_VMINUS_Msk.
*
* \param base
* Pointer to structure describing registers
*
* \param state
* Open or close the switch. Select a value from \ref cy_en_sar_switch_state_t.
*
* \return None
*
* \funcusage
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_VSSA_VMINUS_SWITCH
*
*******************************************************************************/
__STATIC_INLINE void Cy_SAR_SetVssaVminusSwitch(SAR_Type *base, cy_en_sar_switch_state_t state)
{
    Cy_SAR_SetAnalogSwitch(base, CY_SAR_MUX_SWITCH0, SAR_MUX_SWITCH0_MUX_FW_VSSA_VMINUS_Msk, state);
}

/*******************************************************************************
* Function Name: Cy_SAR_SetVssaSarSeqCtrl
****************************************************************************//**
*
* Enable or disable SARSEQ control of the switch between VSSA and Vminus of the SARADC.
* This function calls \ref Cy_SAR_SetSwitchSarSeqCtrl
* with switchMask set to SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_VSSA_Msk.
*
* \param base
* Pointer to structure describing registers
*
* \param ctrl
* Enable or disable control. Select a value from \ref cy_en_sar_switch_sar_seq_ctrl_t.
*
* \return None
*
* \funcusage
*
* \snippet sar_sut_01.cydsn/main_cm0p.c SNIPPET_SAR_VSSA_SARSEQ_CTRL
*
*******************************************************************************/
__STATIC_INLINE void Cy_SAR_SetVssaSarSeqCtrl(SAR_Type *base, cy_en_sar_switch_sar_seq_ctrl_t ctrl)
{
    Cy_SAR_SetSwitchSarSeqCtrl(base, SAR_MUX_SWITCH_SQ_CTRL_MUX_SQ_CTRL_VSSA_Msk, ctrl);
}
/** \} */

/** \} group_sar_functions */

#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXS40PASS_SAR */

#endif /** !defined(CY_SAR_H) */

/** \} group_sar */

/* [] END OF FILE */

