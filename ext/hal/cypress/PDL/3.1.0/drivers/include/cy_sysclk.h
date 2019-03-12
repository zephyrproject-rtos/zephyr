/***************************************************************************//**
* \file cy_sysclk.h
* \version 1.20
*
* Provides an API declaration of the sysclk driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \defgroup group_sysclk System Clock (SysClk)
* \{
* The System Clock (SysClk) driver contains the API for configuring system and
* peripheral clocks. Firmware uses the API to configure , enable, or disable
* a clock.
* 
* The clock system includes a variety of resources that can vary per device, including:
* - Internal clock sources such as internal oscillators
* - External clock sources such as crystal oscillators or a signal on an I/O pin
* - Generated clocks such as an FLL, a PLL, and peripheral clocks
*
* Consult the Technical Reference Manual for your device for details of the 
* clock system.
*
* The PDL defines clock system capabilities in:\n
* devices\<family\>/<series\>/include\<series\>z_config.h. (E.g.
* devices/psoc6/psoc63/include/psoc63_config.h). 
* User-configurable clock speeds are defined in the file system_<series>.h.
*
* As an illustration of the clocking system, the following diagram shows the
* PSoC 63 series clock tree. The actual tree may vary depending on the device series.
* ![](sysclk_tree.png)
*
* The sysclk driver supports multiple peripheral clocks, as well as the fast 
* clock, slow clock, backup domain clock, timer clock, and pump clock. The API
* for any given clock contains the functions to manage that clock. Functions
* for clock measurement and trimming are also provided.
*
* \section group_sysclk_configuration Configuration Considerations
* The availability of clock functions depend on the availability of the chip
* resources that support those functions. Consult the device TRM before
* attempting to use these functions.
*
* LPActive and LPSleep power modes limit the maximum clock frequency allowed
* on the device. Refer to the SysPm driver and the TRM for details.
*
* \section group_sysclk_more_information More Information
* Refer to the technical reference manual (TRM) and the device datasheet.
*
* \section group_sysclk_MISRA MISRA-C Compliance
* <table class="doxtable">
*   <tr>
*     <th>MISRA Rule</th>
*     <th>Rule Class (Required/Advisory)</th>
*     <th>Rule Description</th>
*     <th>Description of Deviation(s)</th>
*   </tr>
*   <tr>
*     <td>1.2</td>
*     <td>R</td>
*     <td>No reliance on undefined behavior.</td>
*     <td>Calculation of an absolute value in the FLL and PLL configuration.</td>
*   </tr>
*   <tr>
*     <td>5.6</td>
*     <td>R</td>
*     <td>No identifier in one name space should have the same spelling as an identifier in another name space, with the
*         exception of structure member and union member names.</td>
*     <td>The "mode" and "retval" are used as a structure/union member they are a label, tag or ordinary
*         identifier.</td>
*   </tr>
*   <tr>
*     <td>10.1</td>
*     <td>R</td>
*     <td>The value of an expression of integer type shall not be implicitly converted to a different, underlying type
*         if the expression is complex.</td>
*     <td>Use of a Cypress defined macro to access memory mapped objects.
*         Use of a Cypress defined macro to divide with rounding.</td>
*   </tr>
*   <tr>
*     <td>10.2</td>
*     <td>R</td>
*     <td>The value of an expression of floating type shall not be implicitly converted to a different type if:
*         a) it is not a conversion to a wider floating type, or
*         b) the expression is complex, or
*         c) the expression is a function argument, or
*         d) the expression is a return expression.</td>
*     <td>The operands of this relational operator are expressions of different "essential type" categories
*         (enum and unsigned).</td>
*   </tr>
*   <tr>
*     <td>10.3</td>
*     <td>R</td>
*     <td>A composite integer expression is being cast to a wider type.</td>
*     <td>Use of a Cypress defined macro to access memory-mapped objects. Calculating the clock parameters.</td>
*   </tr>
*   <tr>
*     <td>10.4</td>
*     <td>R</td>
*     <td>A composite floating point expression is being cast to double, or unsigned.</td>
*     <td>Use of the C library sqrt() function. Casting a floating-point calculation result to an integer.</td>
*   </tr>
*   <tr>
*     <td>10.5</td>
*     <td>R</td>
*     <td>The value of an expression of integer type shall not be implicitly converted to a different, underlying type
*         if the expression is complex.</td>
*     <td>Use of a Cypress defined macro to access memory-mapped objects.</td>
*   </tr>
*   <tr>
*     <td>12.1</td>
*     <td>A</td>
*     <td>Extra parentheses recommended.</td>
*     <td>Ternary operator uses constants; extra parentheses not needed.</td>
*   </tr>
*   <tr>
*     <td>12.2</td>
*     <td>A</td>
*     <td>The value of an expression must be the same under any order of evaluation that the standard permits.</td>
*     <td>The "rtnval" is modified more than once between the sequence points - the evaluation order is not specified.</td>
*   </tr>
*   <tr>
*     <td>12.4</td>
*     <td>A</td>
*     <td>The right hand operand of a logical && or || operator shall not contain side effects.</td>
*     <td>No side effect in this case.</td>
*   </tr>
*   <tr>
*     <td>13.4</td>
*     <td>R</td>
*     <td>The controlling expression of a for statement shall not contain any objects of floating type.</td>
*     <td>Scanning through a list of floating point values.</td>
*   </tr>
*   <tr>
*     <td>16.7</td>
*     <td>R</td>
*     <td>The object addressed by the pointer parameter is not modified and so the pointer could be of
*         type 'pointer to const'.</td>
*     <td>The callback function for system power management (SysPm) must be of generic callback type that
*         contains non-const pointer parameter.</td>
*   </tr>
*   <tr>
*     <td>18.4</td>
*     <td>R</td>
*     <td>Unions shall not be used.</td>
*     <td>The clock path in \ref Cy_SysClk_PeriphGetFrequency() uses either FLL or PLL.</td>
*   </tr>
* </table>
*
*
* \section group_sysclk_changelog Changelog
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
*     <td>1.10.1</td>
*     <td>Renamed Power Management section to Low Power Callback section</td>
*     <td>Documentation update and clarification</td>
*   </tr>
*   <tr>
*     <td rowspan="5">1.10</td>
*     <td>Updated FLL parameter calculation</td>
*     <td>Support low frequency sources</td>
*   </tr>
*   <tr>
*     <td>Added Cy_SysClk_PiloSetTrim() and Cy_SysclkPiloGetTrim() functions</td>
*     <td>Support PILO manual trims</td>
*   </tr>
*   <tr>
*     <td>Made Cy_SysClk_FllLostLock() function dependent on SRSS v1</td>
*     <td>Feature is not supported in SRSS v1</td>
*   </tr>
*   <tr>
*     <td>Updated Cy_SysClk_DeepSleepCallback() to save/restore both FLL and PLL settings</td>
*     <td>The function should return when the lock is established or a timeout has occurred</td>
*   </tr>
*   <tr>
*     <td>General documentation updates</td>
*     <td></td>
*   </tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*
* \defgroup group_sysclk_macros           Macros
* \{
* \}
* \defgroup group_sysclk_enums           General Enumerated Types
* \{
*   \defgroup group_sysclk_returns         Function return values
* \}
* \defgroup group_sysclk_eco             External Crystal Oscillator (ECO)
* \{
*   The External Crystal Oscillator (ECO) is a clock source that consists
*   of an oscillator circuit that drives an external crystal through its
*   dedicated ECO pins. The ECO is a source clock that can be used to
*   source one or more clock paths (Refer to \ref group_sysclk_path_src).
*   These clock paths can then source the processors and peripherals in
*   the device.
*
*   The ECO relies on the presence of an external crystal. The pins
*   connected to this crystal must be configured to operate in analog
*   drive mode with HSIOM connection set to GPIO control (HSIOM_SEL_GPIO).
*
*   \defgroup group_sysclk_eco_funcs       Functions
* \}
* \defgroup group_sysclk_path_src           Clock Path Source
* \{
*   Clock paths are a series of multiplexers that allow a source clock
*   to drive multiple clocking resources down the chain. These paths are
*   used for active domain clocks that are not operational during chip 
*   deep-sleep, hibernate and off modes. Illustrated below is a diagram
*   of the clock paths for the PSoC 63 series, showing the first three
*   clock paths. The source clocks for these paths are highlighted in
*   the red box.
*
*   - IMO: 8 MHz Internal Main Oscillator (Default)
*   - EXTCLK: External clock (signal brought in through dedicated pins)
*   - ECO: External Crystal Oscillator (requires external crystal on dedicated pins)
*   - ALTHF: Select on-chip signals (e.g. BLE ECO)
*   - Digital Signal (DSI): Digital signal from a UDB source
*
*   Some clock paths such as path 0 and path 1 have additional resources
*   that can be utilized to provide a higher frequency clock. For example, 
*   path 0 source clock can be used as the reference clock for the FLL and 
*   path 1 source clock can be used as the reference clock for the PLL.
*
*   ![](sysclk_path_source.png)
*
*   \note The PDL driver cannot configure a clock path to use Digital Signal
*   Interconnect (DSI) outputs as sources. This must be done through DSI
*   configuration tool such as PSoC Creator.
*
*   \defgroup group_sysclk_path_src_funcs     Functions
*   \defgroup group_sysclk_path_src_enums     Enumerated Types
* \}
* \defgroup group_sysclk_fll             Frequency Locked Loop (FLL)
* \{
*   The FLL is a clock generation circuit that can be used to produce a
*   higher frequency clock from a reference clock. The output clock exhibits
*   some characteristics of the reference clock such as the accuracy of the
*   source. However other attributes such as the clock phase are not preserved.
*   The FLL is similar in purpose to a (Phase locked loop) PLL but they are
*   not equivalent.
*
*   - They may have different frequency ranges.
*   - The FLL starts up (locks) faster and consumes less current than the PLL.
*   - The FLL accepts a source clock with lower frequency than PLL, such as the WCO (32 KHz). 
*   - The FLL does not lock phase. The hardware consist of a counter with a
*     current-controlled oscillator (CCO). The counter counts the number of output
*     clock edges in a reference clock period and adjusts the CCO until the
*     expected ratio is achieved (locked). After initial lock, the CCO is
*     adjusted dynamically to keep the ratio within tolerance. The lock tolerance
*     is user-adjustable.
*   ![](sysclk_fll.png)
*
*   The SysClk driver supports two models for configuring the FLL. The first
*   model is to call the Cy_SysClk_FllConfigure() function, which calculates the 
*   necessary parameters for the FLL at run-time. This may be necessary for dynamic
*   run-time changes to the FLL. However this method is slow as it needs to perform 
*   the calculation before configuring the FLL. The other model is to call 
*   Cy_SysClk_FllManualConfigure() function with pre-calculated parameter values. 
*   This method is faster but requires prior knowledge of the necessary parameters.
*   Consult the device TRM for the FLL calculation equations.
*
*   \defgroup group_sysclk_fll_funcs       Functions
*   \defgroup group_sysclk_fll_structs     Data Structures
*   \defgroup group_sysclk_fll_enums       Enumerated Types
* \}
* \defgroup group_sysclk_pll             Phase Locked Loop (PLL)
* \{
*   The PLL is a clock generation circuit that can be used to produce a
*   higher frequency clock from a reference clock. The output clock exhibits
*   characteristics of the reference clock such as the accuracy of the source
*   and its phase. The PLL is similar in purpose to a (Frequency locked loop) FLL
*   but they are not equivalent.
*
*   - They may have different frequency ranges.
*   - The PLL starts up more slowly and consumes more current than the FLL.
*   - The PLL requires a higher frequency source clock than PLL. 
*   ![](sysclk_pll.png)
*
*   The SysClk driver supports two models for configuring the PLL. The first
*   model is to call the Cy_SysClk_PllConfigure() function, which calculates the 
*   necessary parameters for the PLL at run-time. This may be necessary for dynamic
*   run-time changes to the PLL. However this method is slow as it needs to perform 
*   the calculation before configuring the PLL. The other model is to call 
*   Cy_SysClk_PllManualConfigure() function with pre-calculated parameter values. 
*   This method is faster but requires prior knowledge of the necessary parameters.
*   Consult the device TRM for the PLL calculation equations.
*
*   \defgroup group_sysclk_pll_funcs       Functions
*   \defgroup group_sysclk_pll_structs     Data Structures
* \}
* \defgroup group_sysclk_ilo             Internal Low-Speed Oscillator (ILO)
* \{
*   The ILO operates with no external components and outputs a stable clock at
*   32.768 kHz nominal. The ILO is relatively low power and low accuracy. It is 
*   available in all power modes and can be used as a source for the Backup domain clock.
*   ![](sysclk_backup.png)
*
*   To ensure the ILO remains active in Hibernate mode, and across power-on-reset
*   (POR) or brown out detect (BOD), firmware must call Cy_SysClk_IloHibernateOn(). 
*
*   Additionally, the ILO clock can be trimmed to +/- 1.5% of nominal frequency using
*   a higher precision clock source. Use the \ref group_sysclk_calclk API to measure 
*   the current ILO frequency before trimming.
*
*   \note The ILO is always the source clock for the \ref group_wdt. Therefore:
*   - The WDT must be unlocked when making an ILO function call in the PDL
*   - It is recommended to always have the ILO enabled
*
*   \defgroup group_sysclk_ilo_funcs       Functions
* \}
* \defgroup group_sysclk_pilo            Precision Internal Low-Speed Oscillator (PILO)
* \{
*   PILO provides a higher accuracy 32.768 kHz clock than the \ref group_sysclk_ilo "ILO".
*   When periodically calibrated using a high-accuracy clock such as the
*   \ref group_sysclk_eco "ECO", the PILO can achieve 250 ppm accuracy of nominal frequency.
*   The PILO is capable of operating in device Active, Sleep and Deep-Sleep power modes. 
*   It is not available in Hibernate mode.
*
*   The PILO can be used as a source for the \ref group_sysclk_clk_lf. However,
*   because PILO is disabled in Hibernate mode, RTC timers cannot operate in this mode
*   when clocked using the PILO. Instead, either the \ref group_sysclk_ilo "ILO" or
*   \ref group_sysclk_wco "WCO" should be used when hibernate operation is required. 
*
*   ![](sysclk_backup.png)
*
*   Periodic calibration to a high-accuracy clock (such as ECO) is required to
*   maintain accuracy. The application should use the functions described in the 
*   \ref group_sysclk_calclk API to measure the current PILO frequency before trimming.
*
*   \defgroup group_sysclk_pilo_funcs      Functions
* \}
* \defgroup group_sysclk_calclk          Clock Measurement
* \{
*   These functions measure the frequency of a specified clock relative to a
*   reference clock. They are typically called in the following order:
*
*   1. Specify the measured clock, the count, and the reference clock
*   2. Start the counters
*   3. Wait for the measurement counter to finish counting
*   4. Retrieve the measured frequency
*
*   \note These functions may also be used as part of a clock trimming
*   process. Refer to the \ref group_sysclk_trim "Clock Trim" API.
*
*   \defgroup group_sysclk_calclk_funcs    Functions
*   \defgroup group_sysclk_calclk_enums    Enumerated Types
* \}
* \defgroup group_sysclk_trim            Clock Trim (ILO, PILO)
* \{
*   These functions perform a single trim operation on the ILO or PILO. Each
*   function's parameter is the actual frequency of the clock. To measure the
*   frequency, use the functions described in the \ref group_sysclk_calclk API.
*
*   To trim the clock as close as possible to the target frequency, multiple
*   calls to the trim function may be needed. A typical usage example is to:
*   1. Call the clock measurement functions to get the actual frequency of the clock
*   2. Call the trim function, passing in the measured frequency
*   3. Repeat the above until the trim function reports that the clock is trimmed to within limits.
*
*   \defgroup group_sysclk_trim_funcs      Functions
* \}
* \defgroup group_sysclk_pm              Low Power Callback
* \{
*   Entering and exiting low power modes require compatible clock configurations
*   to be set before entering low power and restored upon wake-up and exit. The
*   SysClk driver provides a Cy_SysClk_DeepSleepCallback() function to support
*   deep-sleep mode entry. 
*
*   This function can be called either by itself before initiating low-power mode
*   entry or it can be used in conjunction with the SysPm driver as a registered 
*   callback. To do so, register this function as a callback before calling 
*   Cy_SysPm_DeepSleep(). Specify \ref CY_SYSPM_DEEPSLEEP as the callback type, 
*   and call Cy_SysPm_RegisterCallback().
*   
*   \note If the FLL or PLL source is the ECO, this function must be called.
*
*   \defgroup group_sysclk_pm_funcs        Functions
* \}
* \defgroup group_sysclk_wco             Watch Crystal Oscillator (WCO)
* \{
*   The WCO is a highly accurate 32.768 kHz clock source capable of operating
*   in all power modes (excluding the Off mode). It is the primary clock source for
*   the backup domain clock, which is used by the real-time clock (RTC). The 
*   WCO can also be used as a source for the low-frequency clock to support other
*   low power mode peripherals.
*
*   ![](sysclk_backup.png)
*
*   The WCO requires the configuration of the dedicated WCO pins (SRSS_WCO_IN_PIN,
*   SRSS_WCO_OUT_PIN). These must be configured as Analog Hi-Z drive modes and the
*   HSIOM selection set to GPIO. The WCO can also be used in bypass mode, where
*   an external 32.768 kHz square wave is brought in directly through the 
*   SRSS_WCO_OUT_PIN pin.
*
*   Some devices support a built-in clock supervisor (CSV) in the WCO. The clock
*   supervisor detects if the WCO has been lost; that is, the WCO is no longer 
*   producing clock pulses. The CSV does this by checking to ensure there is at 
*   least one WCO clock pulse within a certain time window. The ILO or PILO can be
*   the supervising clock. Firmware can configure the CSV to trigger a fault,
*   a reset, or both after specified cycles of the supervising clock.
*
*   \defgroup group_sysclk_wco_funcs       Functions
*   \defgroup group_sysclk_wco_structs     Data Structures
*   \defgroup group_sysclk_wco_enums       Enumerated Types
* \}
* \defgroup group_sysclk_clk_hf          High-Frequency Clocks
* \{
*   Multiple high frequency clocks (CLK_HF) are available in the device. For example,
*   PSoC 63 series has five high-frequency root clocks. Each CLK_HF has a particular
*   connection and chip-specific destination on the device.
*
*   |Name     |Description                                             |
*   |:--------|:-------------------------------------------------------|
*   |CLK_HF[0]| Root clock for CPUs, PERI, and AHB infrastructure      |
*   |CLK_HF[1]| Root clock for the PDM/PCM and I2S audio subsystem     |
*   |CLK_HF[2]| Root clock for the Serial Memory Interface subsystem   |
*   |CLK_HF[3]| Root clock for USB communications                      |
*   |CLK_HF[4]| Clock output on clk_ext pin (when used as an output)   |
*
*   ![](sysclk_hf.png)
*   
*   High frequency clocks are sourced by path clocks, which should be configured
*   first. An exception to this rule is CLK_HF[0], which cannot be disabled. 
*   This divided clock drives the core processors and the peripherals in the system.
*   In order to update its clock source, CLK_HF[0] source must be selected without
*   disabling the clock.
*
*   ![](sysclk_hf_dist.png)
*
*   Some devices support a clock supervisor (CSV) for each root clock. These
*   can detect frequency loss, or monitor that the clock frequency stays within
*   a specified range. The possible supervising clocks are IMO, ECO, or ALTHF. 
*   Loss detection and frequency monitoring can be enabled or disabled independently.
*   Each has its own programmable action that occurs on detection of an error.
*
*   \defgroup group_sysclk_clk_hf_funcs    Functions
*   \defgroup group_sysclk_clk_hf_structs  Data Structures
*   \defgroup group_sysclk_clk_hf_enums    Enumerated Types
* \}
* \defgroup group_sysclk_clk_fast        Fast Clock
* \{
*   The fast clock drives the "fast" processor (e.g. Cortex-M4 processor in PSoC 6).
*   This clock is sourced by CLK_HF[0] (\ref group_sysclk_clk_hf "HF Clocks").
*   A divider value of 1~256 can be used to further divide the CLK_HF[0] to a
*   desired clock speed for the processor.
*
*   ![](sysclk_fast.png)
*
*   \defgroup group_sysclk_clk_fast_funcs  Functions
* \}
* \defgroup group_sysclk_clk_peri        Peripheral Clock
* \{
*   The peripheral clock is a divided clock of CLK_HF0 (\ref group_sysclk_clk_hf "HF Clocks").
*   It is the source clock for the \ref group_sysclk_clk_slow, and most active domain
*   peripheral clocks (\ref group_sysclk_clk_peripheral). A divider value of 1~256 
*   can be used to further divide the CLK_HF[0] to a desired clock speed for the peripherals.
*
*   ![](sysclk_peri.png)
*
*   \defgroup group_sysclk_clk_peri_funcs  Functions
* \}
* \defgroup group_sysclk_clk_peripheral  Peripherals Clock Dividers
* \{
*   There are multiple peripheral clock dividers that, in effect, create 
*   multiple separate peripheral clocks. The available dividers vary per device 
*   series. As an example, for the PSoC 63 series there are 29 dividers:
*
*   - eight 8-bit dividers
*   - sixteen 16-bit dividers
*   - four fractional 16.5-bit dividers (16 integer bits, 5 fractional bits)
*   - one fractional 24.5-bit divider (24 integer bits, 5 fractional bits)
*
*   The five fractional bits supports further precision in 1/32nd increments. For
*   example, a divider with an integer value of 3 and a fractional value of
*   4 (4/32) results in a divider of 3.125. Fractional dividers are useful when
*   a high-precision clock is required, for example, for a UART/SPI serial
*   interface.
*
*   ![](sysclk_peri_divs.png)
*
*   Each peripheral can connect to any one of the programmable dividers. A
*   particular peripheral clock divider can drive multiple peripherals.
*   
*   The SysClk driver also supports phase aligning two peripheral clock dividers using
*   Cy_SysClk_PeriphEnablePhaseAlignDivider(). Alignment works for both integer
*   and fractional dividers. The divider to which a second divider is aligned
*   must already be enabled.
*
*   \defgroup group_sysclk_clk_peripheral_funcs Functions
*   \defgroup group_sysclk_clk_peripheral_enums Enumerated Types
* \}
* \defgroup group_sysclk_clk_slow        Slow Clock
* \{
*   The slow clock is the source clock for the "slow" processor (e.g. Cortex-M0+ in PSoC 6).
*   This clock is a divided version of the \ref group_sysclk_clk_peri, which in turn is 
*   a divided version of CLK_HF[0] (\ref group_sysclk_clk_hf "HF Clocks"). A divider 
*   value of 1~256 can be used to further divide the Peri clock to a desired clock speed
*   for the processor.
*
*   ![](sysclk_slow.png)
*
*   \defgroup group_sysclk_clk_slow_funcs  Functions
* \}
* \defgroup group_sysclk_clk_lf          Low-Frequency Clock
* \{
*   The low-frequency clock is the source clock for the \ref group_mcwdt
*   and can be the source clock for \ref group_sysclk_clk_bak, which drives the
*   \ref group_rtc.
*   
*   The low-frequency clock has three possible source clocks:
*   \ref group_sysclk_ilo "ILO", \ref group_sysclk_pilo "PILO", and
*   \ref group_sysclk_wco "WCO".
*
*   ![](sysclk_lf.png)
*
*   \defgroup group_sysclk_clk_lf_funcs    Functions
*   \defgroup group_sysclk_clk_lf_enums    Enumerated Types
* \}
* \defgroup group_sysclk_clk_timer       Timer Clock
* \{
*   The timer clock can be a source for the alternative clock driving 
*   the \ref group_arm_system_timer. It can also be used as a reference clock 
*   for a counter in the \ref group_energy_profiler "Energy Profiler".
*   
*   The timer clock is a divided clock of either the IMO or CLK_HF[0]
*   (\ref group_sysclk_clk_hf "HF Clocks").
*
*   \defgroup group_sysclk_clk_timer_funcs Functions
*   \defgroup group_sysclk_clk_timer_enums Enumerated Types
* \}
* \defgroup group_sysclk_clk_pump        Pump Clock
* \{
*   The pump clock is a clock source used to provide analog precision in low voltage
*   applications. Depedning on the usage scenario, it may be required to drive the
*   internal voltage pump for the Continuous Time Block mini (CTBm) in the analog 
*   subsystem. The pump clock is a divided clock of one of the clock paths 
*   (\ref group_sysclk_path_src).
*
*   \defgroup group_sysclk_clk_pump_funcs  Functions
*   \defgroup group_sysclk_clk_pump_enums  Enumerated Types
* \}
* \defgroup group_sysclk_clk_bak         Backup Domain Clock
* \{
*   The backup domain clock drives the \ref group_rtc.
*   This clock has two possible source clocks: \ref group_sysclk_wco "WCO"
*   or the \ref group_sysclk_clk_lf. In turn the low frequency clock is sourced by
*   \ref group_sysclk_ilo "ILO", \ref group_sysclk_pilo "PILO", or
*   \ref group_sysclk_wco "WCO". Typically the ILO is not suitable as an RTC source,
*   because of its low accuracy. However the ILO does operate in hibernate mode and
*   may be used as an alterative to the WCO with a tradeoff in precision.
*
*   \defgroup group_sysclk_clk_bak_funcs   Functions
*   \defgroup group_sysclk_clk_bak_enums   Enumerated Types
* \}
*/

#if !defined(__CY_SYSCLK_H__)
#define __CY_SYSCLK_H__

#include <stdbool.h>
#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_syslib.h"
#include "cy_syspm.h"


#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/**
* \addtogroup group_sysclk_macros
* \{
*/
/** Driver major version */
#define  CY_SYSCLK_DRV_VERSION_MAJOR   1
/** Driver minor version */
#define  CY_SYSCLK_DRV_VERSION_MINOR   20
/** Sysclk driver identifier */
#define CY_SYSCLK_ID   CY_PDL_DRV_ID(0x12U)

/** \} group_sysclk_macros */

/**
* \addtogroup group_sysclk_returns
* \{
*/
/** Defines general-purpose function return values. */
typedef enum
{
    CY_SYSCLK_SUCCESS       = 0u, /**< Command completed with no errors */
    CY_SYSCLK_BAD_PARAM     = (CY_SYSCLK_ID | CY_PDL_STATUS_ERROR | 1U), /**< Invalid function input parameter */
    CY_SYSCLK_TIMEOUT       = (CY_SYSCLK_ID | CY_PDL_STATUS_ERROR | 2U), /**< Timeout occurred */
    CY_SYSCLK_INVALID_STATE = (CY_SYSCLK_ID | CY_PDL_STATUS_ERROR | 3U)  /**< Clock is in an invalid state */
} cy_en_sysclk_status_t;
/** \} group_sysclk_returns */

/** \cond INTERNAL */
/* Calculate a / b with rounding to the nearest integer. a and b must have the same sign. */
#define CY_SYSCLK_DIV_ROUND(a, b) (((a) + ((b) / 2u)) / (b))
/* Calculate a / b with rounding up if remainder != 0. a and b must both be positive. */
#define CY_SYSCLK_DIV_ROUNDUP(a, b) ((((a) - 1U) / (b)) + 1U)
/** \endcond */


/* ========================================================================== */
/* ===========================    ECO SECTION    ============================ */
/* ========================================================================== */

/**
* \addtogroup group_sysclk_macros
* \{
*/

/**
* \defgroup group_sysclk_ecostatus ECO status
* \{
* Constants used for expressing ECO status.
*/
#define CY_SYSCLK_ECOSTAT_AMPLITUDE  0UL /**< \brief ECO does not have sufficient amplitude */
#define CY_SYSCLK_ECOSTAT_INACCURATE 1UL /**< \brief ECO may not be meeting accuracy and duty cycle specs */
#define CY_SYSCLK_ECOSTAT_STABLE     2UL /**< \brief ECO has fully stabilized */
/** \} */

/** \} group_sysclk_macros */

/**
* \addtogroup group_sysclk_eco_funcs
* \{
*/
cy_en_sysclk_status_t Cy_SysClk_EcoConfigure(uint32_t freq, uint32_t cLoad, uint32_t esr, uint32_t driveLevel);
cy_en_sysclk_status_t Cy_SysClk_EcoEnable(uint32_t timeoutus);
__STATIC_INLINE void Cy_SysClk_EcoDisable(void);
__STATIC_INLINE uint32_t Cy_SysClk_EcoGetStatus(void);

/*******************************************************************************
* Function Name: Cy_SysClk_EcoDisable
****************************************************************************//**
*
* Disables the external crystal oscillator (ECO). This function should not be
* called if the ECO is sourcing clkHf[0].
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_EcoDisable
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_EcoDisable(void)
{
    SRSS_CLK_ECO_CONFIG &= (uint32_t)~_VAL2FLD(SRSS_CLK_ECO_CONFIG_ECO_EN, 1U); /* 0 = disable */
}

/*******************************************************************************
* Function Name: Cy_SysClk_EcoGetStatus
****************************************************************************//**
*
* Reports the current status of the external crystal oscillator (ECO).
*
* \return
* CY_SYSCLK_ECOSTAT_AMPLITUDE = ECO does not have sufficient amplitude<br>
* CY_SYSCLK_ECOSTAT_INACCURATE = ECO has sufficient amplitude but may not be meeting accuracy and duty cycle specifications<br>
* CY_SYSCLK_ECOSTAT_STABLE = ECO has fully stabilized
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_EcoGetStatus
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SysClk_EcoGetStatus(void)
{
    uint32_t retval = (SRSS_CLK_ECO_STATUS & 3UL); /* bit 0 = ECO_OK, bit 1 = ECO_READY */
    /* if ECO is not ready, just report the ECO_OK bit. Otherwise report 2 = ECO ready */
    return (((retval & 2UL) == 0) ? retval : 2UL);
}
/** \} group_sysclk_eco_funcs */


/* ========================================================================== */
/* ====================    INPUT MULTIPLEXER SECTION    ===================== */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_path_src_enums
* \{
*/
/**
*   Input multiplexer clock sources
*/
typedef enum
{
    CY_SYSCLK_CLKPATH_IN_IMO    =     0U, /**< Select the IMO as the output of the path mux */
    CY_SYSCLK_CLKPATH_IN_EXT    =     1U, /**< Select the EXT as the output of the path mux */
    CY_SYSCLK_CLKPATH_IN_ECO    =     2U, /**< Select the ECO as the output of the path mux */
    CY_SYSCLK_CLKPATH_IN_ALTHF  =     3U, /**< Select the ALTHF as the output of the path mux */
    CY_SYSCLK_CLKPATH_IN_DSIMUX =     4U, /**< Select the DSI MUX output as the output of the path mux */
    CY_SYSCLK_CLKPATH_IN_DSI    = 0x100U, /**< Select a DSI signal (0 - 15) as the output of the DSI mux and path mux */
    CY_SYSCLK_CLKPATH_IN_ILO    = 0x110U, /**< Select the ILO (16) as the output of the DSI mux and path mux */
    CY_SYSCLK_CLKPATH_IN_WCO    = 0x111U, /**< Select the WCO (17) as the output of the DSI mux and path mux */
    CY_SYSCLK_CLKPATH_IN_ALTLF  = 0x112U, /**< Select the ALTLF (18) as the output of the DSI mux and path mux */
    CY_SYSCLK_CLKPATH_IN_PILO   = 0x113U  /**< Select the PILO (19) as the output of the DSI mux and path mux */
} cy_en_clkpath_in_sources_t;
/** \} group_sysclk_path_src_enums */

/**
* \addtogroup group_sysclk_path_src_funcs
* \{
*/
cy_en_sysclk_status_t Cy_SysClk_ClkPathSetSource(uint32_t clkPath, cy_en_clkpath_in_sources_t source);
cy_en_clkpath_in_sources_t Cy_SysClk_ClkPathGetSource(uint32_t clkPath);
/** \} group_sysclk_path_src_funcs */


/* ========================================================================== */
/* ===========================    FLL SECTION    ============================ */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_fll_enums
* \{
*/
/** FLL and PLL output mode.
* See registers CLK_FLL_CONFIG3 and CLK_PLL_CONFIG0, bits BYPASS_SEL.
*/
typedef enum
{
    CY_SYSCLK_FLLPLL_OUTPUT_AUTO   = 0U, /**< Output FLL/PLL input source when not locked, and FLL/PLL output when locked. */
    CY_SYSCLK_FLLPLL_OUTPUT_AUTO1  = 1U, /**< Same as AUTO */
    CY_SYSCLK_FLLPLL_OUTPUT_INPUT  = 2U, /**< Output FLL/PLL input source regardless of lock status. */
    CY_SYSCLK_FLLPLL_OUTPUT_OUTPUT = 3U  /**< Output FLL/PLL output regardless of lock status. This can be dangerous if used to clock clkHf, because FLL/PLL output may be unstable. */
} cy_en_fll_pll_output_mode_t;

/** FLL current-controlled oscillator (CCO) frequency ranges.
* See register CLK_FLL_CONFIG4, bits CCO_RANGE.
*/
typedef enum
{
    CY_SYSCLK_FLL_CCO_RANGE0, /**< Target frequency is in range  48 -  64 MHz. */
    CY_SYSCLK_FLL_CCO_RANGE1, /**< Target frequency is in range  64 -  85 MHz. */
    CY_SYSCLK_FLL_CCO_RANGE2, /**< Target frequency is in range  85 - 113 MHz. */
    CY_SYSCLK_FLL_CCO_RANGE3, /**< Target frequency is in range 113 - 150 MHz. */
    CY_SYSCLK_FLL_CCO_RANGE4  /**< Target frequency is in range 150 - 200 MHz. */
} cy_en_fll_cco_ranges_t;
/** \} group_sysclk_fll_enums */

/**
* \addtogroup group_sysclk_fll_structs
* \{
*/
/** Structure containing information for manual configuration of FLL.
*/
typedef struct
{
    uint32_t                    fllMult;         /**< CLK_FLL_CONFIG  register, FLL_MULT bits */
    uint16_t                    refDiv;          /**< CLK_FLL_CONFIG2 register, FLL_REF_DIV bits */
    cy_en_fll_cco_ranges_t      ccoRange;        /**< CLK_FLL_CONFIG4 register, CCO_RANGE bits */
    bool                        enableOutputDiv; /**< CLK_FLL_CONFIG  register, FLL_OUTPUT_DIV bit */
    uint16_t                    lockTolerance;   /**< CLK_FLL_CONFIG2 register, LOCK_TOL bits */
    uint8_t                     igain;           /**< CLK_FLL_CONFIG3 register, FLL_LF_IGAIN bits */
    uint8_t                     pgain;           /**< CLK_FLL_CONFIG3 register, FLL_LF_PGAIN bits */
    uint16_t                    settlingCount;   /**< CLK_FLL_CONFIG3 register, SETTLING_COUNT bits */
    cy_en_fll_pll_output_mode_t outputMode;      /**< CLK_FLL_CONFIG3 register, BYPASS_SEL bits */
    uint16_t                    cco_Freq;        /**< CLK_FLL_CONFIG4 register, CCO_FREQ bits */
} cy_stc_fll_manual_config_t;
/** \} group_sysclk_fll_structs */

/**
* \addtogroup group_sysclk_fll_funcs
* \{
*/
cy_en_sysclk_status_t Cy_SysClk_FllConfigure(uint32_t inputFreq, uint32_t outputFreq, cy_en_fll_pll_output_mode_t outputMode);
cy_en_sysclk_status_t Cy_SysClk_FllManualConfigure(const cy_stc_fll_manual_config_t *config);
void Cy_SysClk_FllGetConfiguration(cy_stc_fll_manual_config_t *config);
cy_en_sysclk_status_t Cy_SysClk_FllEnable(uint32_t timeoutus);
__STATIC_INLINE bool Cy_SysClk_FllLocked(void);
#if (CY_IP_MXS40SRSS_VERSION != 1) || defined(CY_DOXYGEN)
__STATIC_INLINE bool Cy_SysClk_FllLostLock(void);
#endif
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_FllDisable(void);

/*******************************************************************************
* Function Name: Cy_SysClk_FllLocked
****************************************************************************//**
*
* Reports whether or not the FLL is locked.
*
* \return 
* false = not locked<br>
* true = locked
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_FllLocked
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysClk_FllLocked(void)
{
    return (bool)(_FLD2VAL(SRSS_CLK_FLL_STATUS_LOCKED, SRSS_CLK_FLL_STATUS));
}

#if (CY_IP_MXS40SRSS_VERSION != 1) || defined(CY_DOXYGEN)
/*******************************************************************************
* Function Name: Cy_SysClk_FllLostLock
****************************************************************************//**
*
* Reports whether or not the FLL lost its lock since the last time this function
* was called. Clears the lost lock indicator.
*
* \return 
* false = did not lose lock<br>
* true = lost lock
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_FllLostLock
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysClk_FllLostLock(void)
{
    uint32_t retval = _FLD2VAL(SRSS_CLK_FLL_STATUS_UNLOCK_OCCURRED, SRSS_CLK_FLL_STATUS);
    /* write a 1 to clear the unlock occurred bit */
    SRSS_CLK_FLL_STATUS = _VAL2FLD(SRSS_CLK_FLL_STATUS_UNLOCK_OCCURRED, 1U);
    return ((bool)retval);
}
#endif

/*******************************************************************************
* Function Name: Cy_SysClk_FllDisable
****************************************************************************//**
*
* Disables the FLL and the CCO.
*
* \return \ref cy_en_sysclk_status_t
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_FllDisable
*
*******************************************************************************/
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_FllDisable(void)
{
    SRSS_CLK_FLL_CONFIG  &= ~_VAL2FLD(SRSS_CLK_FLL_CONFIG_FLL_ENABLE,  1U); /* 0 = disable */
    SRSS_CLK_FLL_CONFIG4 &= ~_VAL2FLD(SRSS_CLK_FLL_CONFIG4_CCO_ENABLE, 1U); /* 0 = disable */
    return CY_SYSCLK_SUCCESS;
}
/** \} group_sysclk_fll_funcs */


/* ========================================================================== */
/* ===========================    PLL SECTION    ============================ */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_pll_structs
* \{
*/
/** Structure containing information for configuration of a PLL.
*/
typedef struct
{
    uint32_t                    inputFreq;  /**< frequency of PLL source, in Hz */
    uint32_t                    outputFreq; /**< frequency of PLL output, in Hz */
    bool                        lfMode;     /**< CLK_PLL_CONFIG register, PLL_LF_MODE bit */
    cy_en_fll_pll_output_mode_t outputMode; /**< CLK_PLL_CONFIG register, BYPASS_SEL bits */
} cy_stc_pll_config_t;

/** Structure containing information for manual configuration of a PLL.
*/
typedef struct
{
    uint8_t                     feedbackDiv;  /**< CLK_PLL_CONFIG register, FEEDBACK_DIV (P) bits */
    uint8_t                     referenceDiv; /**< CLK_PLL_CONFIG register, REFERENCE_DIV (Q) bits */
    uint8_t                     outputDiv;    /**< CLK_PLL_CONFIG register, OUTPUT_DIV bits */
    bool                        lfMode;       /**< CLK_PLL_CONFIG register, PLL_LF_MODE bit */
    cy_en_fll_pll_output_mode_t outputMode;   /**< CLK_PLL_CONFIG register, BYPASS_SEL bits */
} cy_stc_pll_manual_config_t;
/** \} group_sysclk_pll_structs */

/**
* \addtogroup group_sysclk_pll_funcs
* \{
*/
cy_en_sysclk_status_t Cy_SysClk_PllConfigure(uint32_t clkPath, const cy_stc_pll_config_t *config);
cy_en_sysclk_status_t Cy_SysClk_PllManualConfigure(uint32_t clkPath, const cy_stc_pll_manual_config_t *config);
cy_en_sysclk_status_t Cy_SysClk_PllGetConfiguration(uint32_t clkPath, cy_stc_pll_manual_config_t *config);
cy_en_sysclk_status_t Cy_SysClk_PllEnable(uint32_t clkPath, uint32_t timeoutus);
__STATIC_INLINE bool Cy_SysClk_PllLocked(uint32_t clkPath);
__STATIC_INLINE bool Cy_SysClk_PllLostLock(uint32_t clkPath);
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_PllDisable(uint32_t clkPath);

/*******************************************************************************
* Function Name: Cy_SysClk_PllLocked
****************************************************************************//**
*
* Reports whether or not the selected PLL is locked.
*
* \param clkPath Selects which PLL to check. 1 is the first PLL; 0 is invalid.
*
* \return 
* false = not locked<br>
* true = locked
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PllLocked
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysClk_PllLocked(uint32_t clkPath)
{
    CY_ASSERT_L1((clkPath != 0UL) && (clkPath <= CY_SRSS_NUM_PLL));
    return (bool)(_FLD2VAL(SRSS_CLK_PLL_STATUS_LOCKED, SRSS_CLK_PLL_STATUS[clkPath - 1UL]));
}

/*******************************************************************************
* Function Name: Cy_SysClk_PllLostLock
****************************************************************************//**
*
* Reports whether or not the selected PLL lost its lock since the last time this
* function was called. Clears the lost lock indicator.
*
* \param clkPath Selects which PLL to check. 1 is the first PLL; 0 is invalid.
*
* \return 
* false = did not lose lock<br>
* true = lost lock
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PllLostLock
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysClk_PllLostLock(uint32_t clkPath)
{
    uint32_t retval = 0UL;

    CY_ASSERT_L1((clkPath != 0UL) && (clkPath <= CY_SRSS_NUM_PLL));

    retval = _FLD2VAL(SRSS_CLK_PLL_STATUS_UNLOCK_OCCURRED, SRSS_CLK_PLL_STATUS[clkPath - 1UL]);
    /* write a 1 to clear the unlock occurred bit */
    SRSS_CLK_PLL_STATUS[clkPath - 1UL] = _VAL2FLD(SRSS_CLK_PLL_STATUS_UNLOCK_OCCURRED, 1U);
    return ((bool)retval);
}

/*******************************************************************************
* Function Name: Cy_SysClk_PllDisable
****************************************************************************//**
*
* Disables the selected PLL.
*
* \param clkPath Selects which PLL to disable. 1 is the first PLL; 0 is invalid.
*
* \return Error / status code:<br>
* CY_SYSCLK_SUCCESS - PLL successfully disabled<br>
* CY_SYSCLK_BAD_PARAM - invalid clock path number
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PllDisable
*
*******************************************************************************/
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_PllDisable(uint32_t clkPath)
{
    cy_en_sysclk_status_t retval = CY_SYSCLK_BAD_PARAM;
    if ((clkPath != 0UL) && (clkPath <= CY_SRSS_NUM_PLL))
    {
        SRSS_CLK_PLL_CONFIG[clkPath - 1UL] &= ~_VAL2FLD(SRSS_CLK_PLL_CONFIG_ENABLE, 1U); /* 0 = disable */
        retval = CY_SYSCLK_SUCCESS;
    }
    return (retval);
}
/** \} group_sysclk_pll_funcs */


/* ========================================================================== */
/* ===========================    ILO SECTION    ============================ */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_ilo_funcs
* \{
*/
__STATIC_INLINE void Cy_SysClk_IloEnable(void);
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_IloDisable(void);
__STATIC_INLINE void Cy_SysClk_IloHibernateOn(bool on);

/*******************************************************************************
* Function Name: Cy_SysClk_IloEnable
****************************************************************************//**
*
* Enables the ILO.
*
* \note The watchdog timer (WDT) must be unlocked before calling this function.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_IloEnable
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_IloEnable(void)
{
    SRSS_CLK_ILO_CONFIG |= _VAL2FLD(SRSS_CLK_ILO_CONFIG_ENABLE, 1U); /* 1 = enable */
}

/*******************************************************************************
* Function Name: Cy_SysClk_IloDisable
****************************************************************************//**
*
* Disables the ILO. ILO can't be disabled if WDT is enabled.
*
* \return Error / status code:<br>
* CY_SYSCLK_SUCCESS - ILO successfully disabled<br>
* CY_SYSCLK_INVALID_STATE - Cannot disable the ILO if the WDT is enabled.
*
* \note The watchdog timer (WDT) must be unlocked before calling this function.
* Do not call this function if the WDT is enabled, because the WDT is clocked by
* the ILO.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_IloDisable
*
*******************************************************************************/
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_IloDisable(void)
{
    cy_en_sysclk_status_t retval = CY_SYSCLK_INVALID_STATE;
    if (0UL == _FLD2VAL(SRSS_WDT_CTL_WDT_EN, SRSS_WDT_CTL)) /* 0 = disabled */
    {
        SRSS_CLK_ILO_CONFIG &= ~_VAL2FLD(SRSS_CLK_ILO_CONFIG_ENABLE, 1U); /* 0 = disable */
        retval = CY_SYSCLK_SUCCESS;
    }
    return retval;
}

/*******************************************************************************
* Function Name: Cy_SysClk_IloHibernateOn
****************************************************************************//**
*
* Controls whether the ILO stays on during a hibernate, or through an XRES or
* brown-out detect (BOD) event.
*
* \param on
* true = ILO stays on during hibernate or across XRES/BOD.<br> 
* false = ILO turns off for hibernate or XRES/BOD.
*
* \note Writes to the register/bit are ignored if the watchdog (WDT) is locked.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_IloHibernateOn
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_IloHibernateOn(bool on)
{
    CY_REG32_CLR_SET(SRSS_CLK_ILO_CONFIG, SRSS_CLK_ILO_CONFIG_ILO_BACKUP, (uint32_t)on);
}
/** \} group_sysclk_ilo_funcs */


/* ========================================================================== */
/* ===========================    PILO SECTION    =========================== */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_pilo_funcs
* \{
*/
__STATIC_INLINE void Cy_SysClk_PiloEnable(void);
__STATIC_INLINE void Cy_SysClk_PiloDisable(void);
__STATIC_INLINE void Cy_SysClk_PiloSetTrim(uint32_t trimVal);
__STATIC_INLINE uint32_t Cy_SysClk_PiloGetTrim(void);

/*******************************************************************************
* Function Name: Cy_SysClk_PiloEnable
****************************************************************************//**
*
* Enables the PILO. 
*
* \note This function blocks for 1 millisecond between enabling the PILO and 
* releasing the PILO reset.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PiloEnable
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_PiloEnable(void)
{
    SRSS_CLK_PILO_CONFIG |= _VAL2FLD(SRSS_CLK_PILO_CONFIG_PILO_EN, 1U); /* 1 = enable */
    Cy_SysLib_Delay(1/*msec*/);
    /* release the reset and enable clock output */
    SRSS_CLK_PILO_CONFIG |= (_VAL2FLD(SRSS_CLK_PILO_CONFIG_PILO_RESET_N, 1U) |
                             _VAL2FLD(SRSS_CLK_PILO_CONFIG_PILO_CLK_EN, 1U));
}

/*******************************************************************************
* Function Name: Cy_SysClk_PiloDisable
****************************************************************************//**
*
* Disables the PILO.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PiloDisable
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_PiloDisable(void)
{
    /* Clear PILO_EN, PILO_RESET_N, and PILO_CLK_EN bitfields. This disables the
       PILO and holds the PILO in a reset state. */
    SRSS_CLK_PILO_CONFIG &= ~(_VAL2FLD(SRSS_CLK_PILO_CONFIG_PILO_EN, 1U)      |
                               _VAL2FLD(SRSS_CLK_PILO_CONFIG_PILO_RESET_N, 1U) |
                               _VAL2FLD(SRSS_CLK_PILO_CONFIG_PILO_CLK_EN, 1U));
}

/*******************************************************************************
* Function Name: Cy_SysClk_PiloSetTrim
****************************************************************************//**
*
* Sets the PILO trim bits, which adjusts the PILO frequency. This is typically
* done after measuring the PILO frequency; see \ref Cy_SysClk_StartClkMeasurementCounters().
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PiloSetTrim
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_PiloSetTrim(uint32_t trimVal)
{
    CY_REG32_CLR_SET(SRSS_CLK_PILO_CONFIG, SRSS_CLK_PILO_CONFIG_PILO_FFREQ, trimVal);
}

/*******************************************************************************
* Function Name: Cy_SysClk_PiloGetTrim
****************************************************************************//**
*
* Reports the current PILO trim bits value.
*
* \funcusage
* Refer to the Cy_SysClk_PiloSetTrim() function usage.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SysClk_PiloGetTrim(void)
{
    return (_FLD2VAL(SRSS_CLK_PILO_CONFIG_PILO_FFREQ, SRSS_CLK_PILO_CONFIG));
}
/** \} group_sysclk_pilo_funcs */


/* ========================================================================== */
/* ====================    CLOCK MEASUREMENT SECTION    ===================== */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_calclk_enums
* \{
*/
/** Defines all possible clock sources. */
typedef enum
{
    CY_SYSCLK_MEAS_CLK_ILO =           1U,
    CY_SYSCLK_MEAS_CLK_WCO =           2U,
    CY_SYSCLK_MEAS_CLK_BAK =           3U,
    CY_SYSCLK_MEAS_CLK_ALTLF =         4U,
    CY_SYSCLK_MEAS_CLK_LFCLK =         5U,
    CY_SYSCLK_MEAS_CLK_IMO =           6U,
    CY_SYSCLK_MEAS_CLK_PILO =          8U,
    CY_SYSCLK_MEAS_CLK_FAST_CLKS =     0x100U,
    CY_SYSCLK_MEAS_CLK_ECO =           0x101U,
    CY_SYSCLK_MEAS_CLK_EXT =           0x102U,
    CY_SYSCLK_MEAS_CLK_ALTHF =         0x103U,
    CY_SYSCLK_MEAS_CLK_PATH_CLKS =     0x500U,
    CY_SYSCLK_MEAS_CLK_PATH0 =         0x500U,
    CY_SYSCLK_MEAS_CLK_PATH1 =         0x501U,
    CY_SYSCLK_MEAS_CLK_PATH2 =         0x502U,
    CY_SYSCLK_MEAS_CLK_PATH3 =         0x503U,
    CY_SYSCLK_MEAS_CLK_PATH4 =         0x504U,
    CY_SYSCLK_MEAS_CLK_PATH5 =         0x505U,
    CY_SYSCLK_MEAS_CLK_PATH6 =         0x506U,
    CY_SYSCLK_MEAS_CLK_PATH7 =         0x507U,
    CY_SYSCLK_MEAS_CLK_PATH8 =         0x508U,
    CY_SYSCLK_MEAS_CLK_PATH9 =         0x509U,
    CY_SYSCLK_MEAS_CLK_PATH10 =        0x50AU,
    CY_SYSCLK_MEAS_CLK_PATH11 =        0x50BU,
    CY_SYSCLK_MEAS_CLK_PATH12 =        0x50CU,
    CY_SYSCLK_MEAS_CLK_PATH13 =        0x50DU,
    CY_SYSCLK_MEAS_CLK_PATH14 =        0x50EU,
    CY_SYSCLK_MEAS_CLK_PATH15 =        0x50FU,
    CY_SYSCLK_MEAS_CLK_CLKHFS =        0x600U,
    CY_SYSCLK_MEAS_CLK_CLKHF0 =        0x600U,
    CY_SYSCLK_MEAS_CLK_CLKHF1 =        0x601U,
    CY_SYSCLK_MEAS_CLK_CLKHF2 =        0x602U,
    CY_SYSCLK_MEAS_CLK_CLKHF3 =        0x603U,
    CY_SYSCLK_MEAS_CLK_CLKHF4 =        0x604U,
    CY_SYSCLK_MEAS_CLK_CLKHF5 =        0x605U,
    CY_SYSCLK_MEAS_CLK_CLKHF6 =        0x606U,
    CY_SYSCLK_MEAS_CLK_CLKHF7 =        0x607U,
    CY_SYSCLK_MEAS_CLK_CLKHF8 =        0x608U,
    CY_SYSCLK_MEAS_CLK_CLKHF9 =        0x609U,
    CY_SYSCLK_MEAS_CLK_CLKHF10 =       0x60AU,
    CY_SYSCLK_MEAS_CLK_CLKHF11 =       0x60BU,
    CY_SYSCLK_MEAS_CLK_CLKHF12 =       0x60CU,
    CY_SYSCLK_MEAS_CLK_CLKHF13 =       0x60DU,
    CY_SYSCLK_MEAS_CLK_CLKHF14 =       0x60EU,
    CY_SYSCLK_MEAS_CLK_CLKHF15 =       0x60FU,
    CY_SYSCLK_MEAS_CLK_LAST_CLK =      0x610U
} cy_en_meas_clks_t;
/** \} group_sysclk_calclk_enums */

/**
* \addtogroup group_sysclk_calclk_funcs
* \{
*/
cy_en_sysclk_status_t Cy_SysClk_StartClkMeasurementCounters(cy_en_meas_clks_t clock1, uint32_t count1, cy_en_meas_clks_t clock2);
__STATIC_INLINE bool Cy_SysClk_ClkMeasurementCountersDone(void);
uint32_t Cy_SysClk_ClkMeasurementCountersGetFreq(bool measuredClock, uint32_t refClkFreq);

/*******************************************************************************
* Function Name: Cy_SysClk_ClkMeasurementCountersDone
****************************************************************************//**
*
* Checks if clock measurement counting is done, that is, counter1 has counted down
* to zero. Call \ref Cy_SysClk_StartClkMeasurementCounters() before calling this function.
*
* \return Status of calibration counters:<br>
* true = done<br>
* false = not done
*
* \funcusage
* Refer to the Cy_SysClk_StartClkMeasurementCounters() function usage.
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysClk_ClkMeasurementCountersDone(void)
{
    return (_FLD2VAL(SRSS_CLK_CAL_CNT1_CAL_COUNTER_DONE, SRSS_CLK_CAL_CNT1)); /* 1 = done */
}
/** \} group_sysclk_calclk_funcs */


/* ========================================================================== */
/* ==========================    TRIM SECTION    ============================ */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_trim_funcs
* \{
*/
int32_t Cy_SysClk_IloTrim(uint32_t iloFreq);
int32_t Cy_SysClk_PiloTrim(uint32_t piloFreq);
/** \} group_sysclk_trim_funcs */


/* ========================================================================== */
/* ======================    POWER MANAGEMENT SECTION    ==================== */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_pm_funcs
* \{
*/
cy_en_syspm_status_t Cy_SysClk_DeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams);
/** \} group_sysclk_pm_funcs */


/* ========================================================================== */
/* ===========================    WCO SECTION    ============================ */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_wco_enums
* \{
*/
/** WCO bypass modes */
typedef enum
{
    CY_SYSCLK_WCO_NOT_BYPASSED = 0u, /**< WCO is not bypassed crystal is used */
    CY_SYSCLK_WCO_BYPASSED = 1U      /**< WCO is bypassed external clock must be supplied  on XTAL pin */
} cy_en_wco_bypass_modes_t;

/** WCO CSV supervisor clock selections */
typedef enum
{
    CY_SYSCLK_WCO_CSV_SUPERVISOR_ILO,   /**< WCO CSV supervisor clock source is the ILO */
    CY_SYSCLK_WCO_CSV_SUPERVISOR_ALTLF, /**< WCO CSV supervisor clock source is the alternate low-frequency clock (ALTLF) */
    CY_SYSCLK_WCO_CSV_SUPERVISOR_PILO   /**< WCO CSV supervisor clock source is the PILO */
} cy_en_wco_csv_supervisor_clock_t;

/**
* Clock supervisor clock loss window. There must be one clock of the supervised
* clock within this many clocks of the supervising clock.
* See registers CLK_CSV_HF_CTL and CLK_CSV_WCO_CTL, bitfield CSV_LOSS_WINDOW.
*/
typedef enum
{
    CY_SYSCLK_CSV_LOSS_4_CYCLES =   0u, /**< 1 clock must be seen within 4 cycles of the supervising clock. */
    CY_SYSCLK_CSV_LOSS_8_CYCLES =   1U, /**< 1 clock must be seen within 8 cycles of the supervising clock. */
    CY_SYSCLK_CSV_LOSS_16_CYCLES =  2u, /**< 1 clock must be seen within 16 cycles of the supervising clock. */
    CY_SYSCLK_CSV_LOSS_32_CYCLES =  3u, /**< 1 clock must be seen within 32 cycles of the supervising clock. */
    CY_SYSCLK_CSV_LOSS_64_CYCLES =  4u, /**< 1 clock must be seen within 64 cycles of the supervising clock. */
    CY_SYSCLK_CSV_LOSS_128_CYCLES = 5u, /**< 1 clock must be seen within 128 cycles of the supervising clock. */
    CY_SYSCLK_CSV_LOSS_256_CYCLES = 6u, /**< 1 clock must be seen within 256 cycles of the supervising clock. */
    CY_SYSCLK_CSV_LOSS_512_CYCLES = 7u  /**< 1 clock must be seen within 512 cycles of the supervising clock. */
} cy_en_csv_loss_window_t;

/**
* Clock supervisor error actions. See register CLK_CSV_HF_CTL[CSV_FREQ_ACTION and CSV_LOSS_ACTION].
*/
typedef enum
{
    CY_SYSCLK_CSV_ERROR_IGNORE = 0u,     /**< Ignore the error reported by the clock supervisor. */
    CY_SYSCLK_CSV_ERROR_FAULT = 1U,      /**< Trigger a fault when an error is reported by the clock supervisor. */
    CY_SYSCLK_CSV_ERROR_RESET = 2u,      /**< Trigger a reset when an error is reported by the clock supervisor. */
    CY_SYSCLK_CSV_ERROR_FAULT_RESET = 3u /**< Trigger a fault then reset when an error is reported by the supervisor. */
} cy_en_csv_error_actions_t;
/** \} group_sysclk_wco_enums */

/**
* \addtogroup group_sysclk_wco_structs
* \{
*/
/**
* This structure is used to configure the clock supervisor for the WCO.
*/
typedef struct
{
    cy_en_wco_csv_supervisor_clock_t supervisorClock; /**< supervisor clock selection */
    bool enableLossDetection;                         /**< 1= enabled, 0= disabled. Note that if loss detection is enabled, writes to other register bits are ignored. */
    cy_en_csv_loss_window_t lossWindow;               /**< \ref cy_en_csv_loss_window_t */
    cy_en_csv_error_actions_t lossAction;             /**< \ref cy_en_csv_error_actions_t */
} cy_stc_wco_csv_config_t;
/** \} group_sysclk_wco_structs */

/**
* \addtogroup group_sysclk_wco_funcs
* \{
*/
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_WcoEnable(uint32_t timeoutus);
__STATIC_INLINE bool Cy_SysClk_WcoOkay(void);
__STATIC_INLINE void Cy_SysClk_WcoDisable(void);
__STATIC_INLINE void Cy_SysClk_WcoBypass(cy_en_wco_bypass_modes_t bypass);
#if (SRSS_WCOCSV_PRESENT != 0) || defined(CY_DOXYGEN)
    void Cy_SysClk_WcoConfigureCsv(const cy_stc_wco_csv_config_t *config);
#endif /* (SRSS_WCOCSV_PRESENT != 0) || defined(CY_DOXYGEN) */


/*******************************************************************************
* Function Name: Cy_SysClk_WcoEnable
****************************************************************************//**
*
* Enables the WCO.
*
* \param timeoutus amount of time in microseconds to wait for the WCO to be ready.
* If WCO is not ready, WCO is stopped. To avoid waiting for WCO ready set this to 0,
* and manually check if WCO is okay using \ref Cy_SysClk_WcoOkay.
*
* \return Error / status code:<br>
* CY_SYSCLK_SUCCESS - WCO successfully enabled<br>
* CY_SYSCLK_TIMEOUT - Timeout waiting for WCO to stabilize
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_WcoEnable
*
*******************************************************************************/
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_WcoEnable(uint32_t timeoutus)
{
    cy_en_sysclk_status_t rtnval = CY_SYSCLK_TIMEOUT;

    /* first set the WCO enable bit */
    BACKUP_CTL |= _VAL2FLD(BACKUP_CTL_WCO_EN, 1U); /* 1 = enable */

    /* now do the timeout wait for STATUS, bit WCO_OK */
    for (; (Cy_SysClk_WcoOkay() == false) && (timeoutus != 0UL); timeoutus--)
    {
        Cy_SysLib_DelayUs(1U);
    }
    if (timeoutus != 0UL)
    {
        rtnval = CY_SYSCLK_SUCCESS;
    }

    return (rtnval);
}

/*******************************************************************************
* Function Name: Cy_SysClk_WcoOkay
****************************************************************************//**
*
* Reports the status of the WCO_OK bit.
*
* \return 
* true = okay<br>
* false = not okay
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_WcoOkay
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysClk_WcoOkay(void)
{
    return (bool)(_FLD2VAL(BACKUP_STATUS_WCO_OK, BACKUP_STATUS));
}

/*******************************************************************************
* Function Name: Cy_SysClk_WcoDisable
****************************************************************************//**
*
* Disables the WCO.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_WcoDisable
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_WcoDisable(void)
{
    BACKUP_CTL &= ~_VAL2FLD(BACKUP_CTL_WCO_EN, 1U); /* 0 = disable */
}

/*******************************************************************************
* Function Name: Cy_SysClk_WcoBypass
****************************************************************************//**
*
* Sets whether the WCO is bypassed or not. If it is bypassed, then a 32-kHz clock
* must be provided on the wco_out pin.
*
* \param bypass \ref cy_en_wco_bypass_modes_t
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_WcoBypass
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_WcoBypass(cy_en_wco_bypass_modes_t bypass)
{
    CY_REG32_CLR_SET(BACKUP_CTL, BACKUP_CTL_WCO_BYPASS, bypass);
}
/** \} group_sysclk_wco_funcs */


/* ========================================================================== */
/* =========================    clkHf[n] SECTION    ========================= */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_clk_hf_enums
* \{
*/
/**
* Selects which clkHf input, or root mux, to configure.
* See CLK_ROOT_SELECT registers, bits ROOT_MUX.
* Used with functions \ref Cy_SysClk_ClkHfSetSource and \ref Cy_SysClk_ClkHfGetSource.
*/
typedef enum
{
    CY_SYSCLK_CLKHF_IN_CLKPATH0  = 0U,  /**< clkHf input is Clock Path 0 */
    CY_SYSCLK_CLKHF_IN_CLKPATH1  = 1U,  /**< clkHf input is Clock Path 1 */
    CY_SYSCLK_CLKHF_IN_CLKPATH2  = 2U,  /**< clkHf input is Clock Path 2 */
    CY_SYSCLK_CLKHF_IN_CLKPATH3  = 3U,  /**< clkHf input is Clock Path 3 */
    CY_SYSCLK_CLKHF_IN_CLKPATH4  = 4U,  /**< clkHf input is Clock Path 4 */
    CY_SYSCLK_CLKHF_IN_CLKPATH5  = 5U,  /**< clkHf input is Clock Path 5 */
    CY_SYSCLK_CLKHF_IN_CLKPATH6  = 6U,  /**< clkHf input is Clock Path 6 */
    CY_SYSCLK_CLKHF_IN_CLKPATH7  = 7U,  /**< clkHf input is Clock Path 7 */
    CY_SYSCLK_CLKHF_IN_CLKPATH8  = 8U,  /**< clkHf input is Clock Path 8 */
    CY_SYSCLK_CLKHF_IN_CLKPATH9  = 9U,  /**< clkHf input is Clock Path 9 */
    CY_SYSCLK_CLKHF_IN_CLKPATH10 = 10U, /**< clkHf input is Clock Path 10 */
    CY_SYSCLK_CLKHF_IN_CLKPATH11 = 11U, /**< clkHf input is Clock Path 11 */
    CY_SYSCLK_CLKHF_IN_CLKPATH12 = 12U, /**< clkHf input is Clock Path 12 */
    CY_SYSCLK_CLKHF_IN_CLKPATH13 = 13U, /**< clkHf input is Clock Path 13 */
    CY_SYSCLK_CLKHF_IN_CLKPATH14 = 14U, /**< clkHf input is Clock Path 14 */
    CY_SYSCLK_CLKHF_IN_CLKPATH15 = 15U, /**< clkHf input is Clock Path 15 */
} cy_en_clkhf_in_sources_t;


/**
* clkHf divider values. See CLK_ROOT_SELECT registers, bits ROOT_DIV.
* Used with functions \ref Cy_SysClk_ClkHfSetDivider and \ref Cy_SysClk_ClkHfGetDivider.
*/
typedef enum
{
    CY_SYSCLK_CLKHF_NO_DIVIDE   = 0U,    /**< don't divide clkHf. */
    CY_SYSCLK_CLKHF_DIVIDE_BY_2 = 1U,    /**< divide clkHf by 2 */
    CY_SYSCLK_CLKHF_DIVIDE_BY_4 = 2U,    /**< divide clkHf by 4 */
    CY_SYSCLK_CLKHF_DIVIDE_BY_8 = 3U     /**< divide clkHf by 8 */
} cy_en_clkhf_dividers_t;

/**
* clkHf clock supervisor input sources. See register CLK_CSV_HF_CTL[CSV_MUX].
*/
typedef enum
{
    CY_SYSCLK_CLKHF_CSV_SUPERVISOR_IMO   = 0U,  /**< Supervising clock is the IMO. */
    CY_SYSCLK_CLKHF_CSV_SUPERVISOR_EXT   = 1U,  /**< Supervising clock is the external clock */
    CY_SYSCLK_CLKHF_CSV_SUPERVISOR_ALTHF = 2U   /**< Supervising clock is clk_althf */
} cy_en_clkhf_csv_supervisor_clock_t;
/** \} group_sysclk_clk_hf_enums */

/**
* \addtogroup group_sysclk_clk_hf_structs
* \{SupervisingWindow
*/
/**
* This structure is used to configure the clock supervisor for clkHf.
*/
typedef struct
{
    cy_en_clkhf_csv_supervisor_clock_t supervisorClock; /**< \ref cy_en_clkhf_csv_supervisor_clock_t */
    uint16_t supervisingWindow;                         /**< Number of supervising clock cycles */
    bool enableFrequencyFaultDetection;                 /**< 1= enabled, 0= disabled */
    uint16_t frequencyLowerLimit;                       /**< Lowest frequency in kHz that supervised clock can go */
    uint16_t frequencyUpperLimit;                       /**< Highest frequency in kHz that supervised clock can go */
    cy_en_csv_error_actions_t frequencyAction;          /**< \ref cy_en_csv_error_actions_t */
    bool enableLossDetection;                           /**< 1= enabled, 0= disabled */
    cy_en_csv_loss_window_t lossWindow;                 /**< \ref cy_en_csv_loss_window_t */
    cy_en_csv_error_actions_t lossAction;               /**< \ref cy_en_csv_error_actions_t */
} cy_stc_clkhf_csv_config_t;
/** \} group_sysclk_clk_hf_structs */

/**
* \addtogroup group_sysclk_clk_hf_funcs
* \{
*/
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_ClkHfEnable(uint32_t clkHf);
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_ClkHfDisable(uint32_t clkHf);
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_ClkHfSetSource(uint32_t clkHf, cy_en_clkhf_in_sources_t source);
__STATIC_INLINE cy_en_clkhf_in_sources_t Cy_SysClk_ClkHfGetSource(uint32_t clkHf);
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_ClkHfSetDivider(uint32_t clkHf, cy_en_clkhf_dividers_t divider);
__STATIC_INLINE cy_en_clkhf_dividers_t Cy_SysClk_ClkHfGetDivider(uint32_t clkHf);
#if (SRSS_MASK_HFCSV != 0)  || defined(CY_DOXYGEN)
    cy_en_sysclk_status_t Cy_SysClk_ClkHfConfigureCsv(uint32_t clkHf, const cy_stc_clkhf_csv_config_t *config);
#endif /* (SRSS_MASK_HFCSV != 0)  || defined(CY_DOXYGEN) */


/*******************************************************************************
* Function Name: Cy_SysClk_ClkHfEnable
****************************************************************************//**
*
* Enables the selected clkHf.
*
* \param clkHf Selects which clkHf to enable.
*
* \return \ref cy_en_sysclk_status_t
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkHfEnable
*
*******************************************************************************/
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_ClkHfEnable(uint32_t clkHf)
{
    cy_en_sysclk_status_t retval = CY_SYSCLK_BAD_PARAM;
    if (clkHf < CY_SRSS_NUM_HFROOT)
    {
        SRSS_CLK_ROOT_SELECT[clkHf] |= _VAL2FLD(SRSS_CLK_ROOT_SELECT_ENABLE, 1UL); /* 1 = enable */
        retval = CY_SYSCLK_SUCCESS;
    }
    return (retval);
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkHfDisable
****************************************************************************//**
*
* Disables the selected clkHf.
*
* \param clkHf Selects which clkHf to enable.
*
* \return \ref cy_en_sysclk_status_t
*
* \note clkHf[0] cannot be disabled.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkHfDisable
*
*******************************************************************************/
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_ClkHfDisable(uint32_t clkHf)
{
    cy_en_sysclk_status_t retval = CY_SYSCLK_BAD_PARAM;
    if ((0UL < clkHf) && (clkHf < CY_SRSS_NUM_HFROOT))
    {
        SRSS_CLK_ROOT_SELECT[clkHf] &= ~_VAL2FLD(SRSS_CLK_ROOT_SELECT_ENABLE, 1UL); /* 0 = disable */
        retval = CY_SYSCLK_SUCCESS;
    }
    return (retval);
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkHfSetSource
****************************************************************************//**
*
* Selects the source of the selected clkHf.
*
* \param clkHf selects which clkHf mux to configure.
*
* \param source \ref cy_en_clkhf_in_sources_t
*
* \return \ref cy_en_sysclk_status_t
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkHfSetSource
*
*******************************************************************************/
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_ClkHfSetSource(uint32_t clkHf, cy_en_clkhf_in_sources_t source)
{
    cy_en_sysclk_status_t retval = CY_SYSCLK_BAD_PARAM;
    if ((clkHf < CY_SRSS_NUM_HFROOT) && (source <= CY_SYSCLK_CLKHF_IN_CLKPATH15))
    {
        CY_REG32_CLR_SET(SRSS_CLK_ROOT_SELECT[clkHf], SRSS_CLK_ROOT_SELECT_ROOT_MUX, source);
        retval = CY_SYSCLK_SUCCESS;
    }
    return (retval);
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkHfGetSource
****************************************************************************//**
*
* Reports the source of the selected clkHf.
*
* \param clkHf selects which clkHf to get the source of.
*
* \return \ref cy_en_clkhf_in_sources_t
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkHfSetSource
*
*******************************************************************************/
__STATIC_INLINE cy_en_clkhf_in_sources_t Cy_SysClk_ClkHfGetSource(uint32_t clkHf)
{
    CY_ASSERT_L1(clkHf < CY_SRSS_NUM_HFROOT);
    return (cy_en_clkhf_in_sources_t)(_FLD2VAL(SRSS_CLK_ROOT_SELECT_ROOT_MUX, SRSS_CLK_ROOT_SELECT[clkHf]));
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkHfSetDivider
****************************************************************************//**
*
* Sets the pre-divider for a clkHf.
*
* \param clkHf selects which clkHf divider to configure.
*
* \param divider \ref cy_en_clkhf_dividers_t
*
* \return \ref cy_en_sysclk_status_t
*
* \note Also call \ref Cy_SysClk_ClkHfSetSource to set the clkHf source.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkHfSetDivider
*
*******************************************************************************/
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_ClkHfSetDivider(uint32_t clkHf, cy_en_clkhf_dividers_t divider)
{
    cy_en_sysclk_status_t retval = CY_SYSCLK_BAD_PARAM;
    if ((clkHf < CY_SRSS_NUM_HFROOT) && (divider <= CY_SYSCLK_CLKHF_DIVIDE_BY_8))
    {
        CY_REG32_CLR_SET(SRSS_CLK_ROOT_SELECT[clkHf], SRSS_CLK_ROOT_SELECT_ROOT_DIV, divider);
        retval = CY_SYSCLK_SUCCESS;
    }
    return (retval);
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkHfGetDivider
****************************************************************************//**
*
* Reports the pre-divider value for a clkHf.
*
* \param clkHf selects which clkHf to check divider of.
*
* \return \ref cy_en_clkhf_dividers_t
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkHfSetDivider
*
*******************************************************************************/
__STATIC_INLINE cy_en_clkhf_dividers_t Cy_SysClk_ClkHfGetDivider(uint32_t clkHf)
{
    CY_ASSERT_L1(clkHf < CY_SRSS_NUM_HFROOT);
    return (cy_en_clkhf_dividers_t)(_FLD2VAL(SRSS_CLK_ROOT_SELECT_ROOT_DIV, SRSS_CLK_ROOT_SELECT[clkHf]));
}
/** \} group_sysclk_clk_hf_funcs */


/* ========================================================================== */
/* =========================    clk_fast SECTION    ========================= */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_clk_fast_funcs
* \{
*/
__STATIC_INLINE void Cy_SysClk_ClkFastSetDivider(uint8_t divider);
__STATIC_INLINE uint8_t Cy_SysClk_ClkFastGetDivider(void);

/*******************************************************************************
* Function Name: Cy_SysClk_ClkFastSetDivider
****************************************************************************//**
*
* Sets the clock divider for the fast clock, which sources the main processor.
* The source of this divider is clkHf[0].
*
* \param divider divider value between 0 and 255.
* Causes integer division of (divider value + 1), or division by 1 to 256.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkFastSetDivider
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_ClkFastSetDivider(uint8_t divider)
{
    CY_REG32_CLR_SET(CPUSS_CM4_CLOCK_CTL, CPUSS_CM4_CLOCK_CTL_FAST_INT_DIV, (uint32_t)divider);
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkFastGetDivider
****************************************************************************//**
*
* Returns the clock divider for the fast clock.
*
* \return The divider value for the fast clock.
* The integer division done is by (divider value + 1), or division by 1 to 256.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkFastSetDivider
*
*******************************************************************************/
__STATIC_INLINE uint8_t Cy_SysClk_ClkFastGetDivider(void)
{
    return ((uint8_t)_FLD2VAL(CPUSS_CM4_CLOCK_CTL_FAST_INT_DIV, CPUSS_CM4_CLOCK_CTL));
}
/** \} group_sysclk_clk_fast_funcs */


/* ========================================================================== */
/* ========================    clk_peri SECTION    ========================== */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_clk_peri_funcs
* \{
*/
__STATIC_INLINE void Cy_SysClk_ClkPeriSetDivider(uint8_t divider);
__STATIC_INLINE uint8_t Cy_SysClk_ClkPeriGetDivider(void);

/*******************************************************************************
* Function Name: Cy_SysClk_ClkPeriSetDivider
****************************************************************************//**
*
* Sets the clock divider for the peripheral clock tree. All peripheral clock
* dividers are sourced from this clock. Also the Cortex M0+ clock divider is
* sourced from this clock. The source of this divider is clkHf[0]
*
* \param divider divider value between 0 and 255
* Causes integer division of (divider value + 1), or division by 1 to 256.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkPeriSetDivider
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_ClkPeriSetDivider(uint8_t divider)
{
    CY_REG32_CLR_SET(CPUSS_CM0_CLOCK_CTL, CPUSS_CM0_CLOCK_CTL_PERI_INT_DIV, (uint32_t)divider);
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkPeriGetDivider
****************************************************************************//**
*
* Returns the clock divider of the peripheral (peri) clock.
*
* \return The divider value.
* The integer division done is by (divider value + 1), or division by 1 to 256.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkPeriSetDivider
*
*******************************************************************************/
__STATIC_INLINE uint8_t Cy_SysClk_ClkPeriGetDivider(void)
{
    return ((uint8_t)_FLD2VAL(CPUSS_CM0_CLOCK_CTL_PERI_INT_DIV, CPUSS_CM0_CLOCK_CTL));
}
/** \} group_sysclk_clk_peri_funcs */


/* ========================================================================== */
/* =====================    clk_peripherals SECTION    ====================== */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_clk_peripheral_enums
* \{
*/
/** Programmable clock divider types */
typedef enum
{
    CY_SYSCLK_DIV_8_BIT    = 0U, /**< Divider Type is an 8 bit divider */
    CY_SYSCLK_DIV_16_BIT   = 1U, /**< Divider Type is a 16 bit divider */
    CY_SYSCLK_DIV_16_5_BIT = 2U, /**< Divider Type is a 16.5 bit fractional divider */
    CY_SYSCLK_DIV_24_5_BIT = 3U  /**< Divider Type is a 24.5 bit fractional divider */
} cy_en_divider_types_t;
/** \} group_sysclk_clk_peripheral_enums */

/**
* \addtogroup group_sysclk_clk_peripheral_funcs
* \{
*/
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_PeriphSetDivider(cy_en_divider_types_t dividerType, uint32_t dividerNum, uint32_t dividerValue);
__STATIC_INLINE uint32_t Cy_SysClk_PeriphGetDivider(cy_en_divider_types_t dividerType, uint32_t dividerNum);
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_PeriphSetFracDivider(cy_en_divider_types_t dividerType, uint32_t dividerNum, uint32_t dividerIntValue, uint32_t dividerFracValue);
__STATIC_INLINE void Cy_SysClk_PeriphGetFracDivider(cy_en_divider_types_t dividerType, uint32_t dividerNum, uint32_t *dividerIntValue, uint32_t *dividerFracValue);
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_PeriphAssignDivider(en_clk_dst_t ipBlock, cy_en_divider_types_t dividerType, uint32_t dividerNum);
__STATIC_INLINE uint32_t Cy_SysClk_PeriphGetAssignedDivider(en_clk_dst_t ipBlock);
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_PeriphEnableDivider(cy_en_divider_types_t dividerType, uint32_t dividerNum);
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_PeriphDisableDivider(cy_en_divider_types_t dividerType, uint32_t dividerNum);
__STATIC_INLINE cy_en_sysclk_status_t Cy_SysClk_PeriphEnablePhaseAlignDivider(cy_en_divider_types_t dividerType, uint32_t dividerNum, cy_en_divider_types_t dividerTypePA, uint32_t dividerNumPA);
__STATIC_INLINE bool Cy_SysClk_PeriphGetDividerEnabled(cy_en_divider_types_t dividerType, uint32_t dividerNum);
uint32_t Cy_SysClk_PeriphGetFrequency(cy_en_divider_types_t dividerType, uint32_t dividerNum);

/*******************************************************************************
* Function Name: Cy_SysClk_PeriphSetDivider
****************************************************************************//**
*
* Sets one of the programmable clock dividers. This is only used for integer
* dividers. Use \ref Cy_SysClk_PeriphSetFracDivider for setting factional dividers.
*
* \param dividerType specifies which type of divider to use; \ref cy_en_divider_types_t
*
* \param dividerNum the divider number.
*
* \param dividerValue divider value
* Causes integer division of (divider value + 1), or division by 1 to 256
* (8-bit divider) or 1 to 65536 (16-bit divider).
*
* \return \ref cy_en_sysclk_status_t
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PeriphSetDivider
*
*******************************************************************************/
__STATIC_INLINE cy_en_sysclk_status_t
                Cy_SysClk_PeriphSetDivider(cy_en_divider_types_t dividerType,
                                           uint32_t dividerNum, uint32_t dividerValue)
{
    cy_en_sysclk_status_t retval = CY_SYSCLK_BAD_PARAM;
    if (dividerType == CY_SYSCLK_DIV_8_BIT)
    {
        if ((dividerNum < PERI_DIV_8_NR) &&
            (dividerValue <= (PERI_DIV_8_CTL_INT8_DIV_Msk >> PERI_DIV_8_CTL_INT8_DIV_Pos)))
        {
            CY_REG32_CLR_SET(PERI_DIV_8_CTL[dividerNum], PERI_DIV_8_CTL_INT8_DIV, dividerValue);
            retval = CY_SYSCLK_SUCCESS;
        }
    }
    else if (dividerType == CY_SYSCLK_DIV_16_BIT)
    {
        if ((dividerNum < PERI_DIV_16_NR) &&
            (dividerValue <= (PERI_DIV_16_CTL_INT16_DIV_Msk >> PERI_DIV_16_CTL_INT16_DIV_Pos)))
        {
            CY_REG32_CLR_SET(PERI_DIV_16_CTL[dividerNum], PERI_DIV_16_CTL_INT16_DIV, dividerValue);
            retval = CY_SYSCLK_SUCCESS;
        }
    }
    else
    { /* return bad parameter */
    }
    return (retval);
}

/*******************************************************************************
* Function Name: Cy_SysClk_PeriphGetDivider
****************************************************************************//**
*
* Returns the integer divider value for the specified divider. One works for
* integer dividers. Use \ref Cy_SysClk_PeriphGetFracDivider to get the fractional
* divider value
*
* \param dividerType specifies which type of divider to use; \ref cy_en_divider_types_t
*
* \param dividerNum specifies which divider of the selected type to configure
*
* \return The divider value.
* The integer division done is by (divider value + 1), or division by 1 to 256
* (8-bit divider) or 1 to 65536 (16-bit divider).
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PeriphSetDivider
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SysClk_PeriphGetDivider(cy_en_divider_types_t dividerType, uint32_t dividerNum)
{
    uint32_t retval;

    CY_ASSERT_L1(dividerType <= CY_SYSCLK_DIV_16_BIT);

    if (dividerType == CY_SYSCLK_DIV_8_BIT)
    {
        CY_ASSERT_L1(dividerNum < PERI_DIV_8_NR);
        retval = _FLD2VAL(PERI_DIV_8_CTL_INT8_DIV, PERI_DIV_8_CTL[dividerNum]);
    }
    else
    { /* 16-bit divider */
        CY_ASSERT_L1(dividerNum < PERI_DIV_16_NR);
        retval = _FLD2VAL(PERI_DIV_16_CTL_INT16_DIV, PERI_DIV_16_CTL[dividerNum]);
    }
    return (retval);
}

/*******************************************************************************
* Function Name: Cy_SysClk_PeriphSetFracDivider
****************************************************************************//**
*
* Sets one of the programmable clock dividers. This function should only be used
* for fractional clock dividers.
*
* \param dividerType specifies which type of divider to use; \ref cy_en_divider_types_t
*
* \param dividerNum specifies which divider of the selected type to configure
*
* \param dividerIntValue the integer divider value
* The source of the divider is peri_clk, which is a divided version of hf_clk[0].
* The divider value causes integer division of (divider value + 1), or division
* by 1 to 65536 (16-bit divider) or 1 to 16777216 (24-bit divider).
*
* \param dividerFracValue the fraction part of the divider
* The fractional divider can be 1-32, thus it divides the clock by 1/32 for each
* count. To divide the clock by 11/32nds set this value to 11.
*
* \return \ref cy_en_sysclk_status_t
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PeriphSetFracDivider
*
*******************************************************************************/
__STATIC_INLINE cy_en_sysclk_status_t
                Cy_SysClk_PeriphSetFracDivider(cy_en_divider_types_t dividerType, uint32_t dividerNum,
                                               uint32_t dividerIntValue, uint32_t dividerFracValue)
{
    cy_en_sysclk_status_t retval = CY_SYSCLK_BAD_PARAM;
    if (dividerType == CY_SYSCLK_DIV_16_5_BIT)
    {
        if ((dividerNum < PERI_DIV_16_5_NR) &&
            (dividerIntValue <= (PERI_DIV_16_5_CTL_INT16_DIV_Msk >> PERI_DIV_16_5_CTL_INT16_DIV_Pos)) &&
            (dividerFracValue <= (PERI_DIV_16_5_CTL_FRAC5_DIV_Msk >> PERI_DIV_16_5_CTL_FRAC5_DIV_Pos)))
        {
            CY_REG32_CLR_SET(PERI_DIV_16_5_CTL[dividerNum], PERI_DIV_16_5_CTL_INT16_DIV, dividerIntValue);
            CY_REG32_CLR_SET(PERI_DIV_16_5_CTL[dividerNum], PERI_DIV_16_5_CTL_FRAC5_DIV, dividerFracValue);
            retval = CY_SYSCLK_SUCCESS;
        }
    }
    else if (dividerType == CY_SYSCLK_DIV_24_5_BIT)
    {
        if ((dividerNum < PERI_DIV_24_5_NR) &&
            (dividerIntValue <= (PERI_DIV_24_5_CTL_INT24_DIV_Msk >> PERI_DIV_24_5_CTL_INT24_DIV_Pos)) &&
            (dividerFracValue <= (PERI_DIV_24_5_CTL_FRAC5_DIV_Msk >> PERI_DIV_24_5_CTL_FRAC5_DIV_Pos)))
        {
            CY_REG32_CLR_SET(PERI_DIV_24_5_CTL[dividerNum], PERI_DIV_24_5_CTL_INT24_DIV, dividerIntValue);
            CY_REG32_CLR_SET(PERI_DIV_24_5_CTL[dividerNum], PERI_DIV_24_5_CTL_FRAC5_DIV, dividerFracValue);
            retval = CY_SYSCLK_SUCCESS;
        }
    }
    else
    { /* return bad parameter */
    }
    return (retval);
}

/*******************************************************************************
* Function Name: Cy_SysClk_PeriphGetFracDivider
****************************************************************************//**
*
* Reports the integer and fractional parts of the divider
*
* \param dividerType specifies which type of divider to use; \ref cy_en_divider_types_t
*
* \param dividerNum specifies which divider of the selected type to configure
*
* \param *dividerIntValue pointer to return integer divider value
*
* \param *dividerFracValue pointer to return fractional divider value
*
* \return None. Loads pointed-to variables.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PeriphSetFracDivider
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_PeriphGetFracDivider(cy_en_divider_types_t dividerType, uint32_t dividerNum,
                                                    uint32_t *dividerIntValue, uint32_t *dividerFracValue)
{
    CY_ASSERT_L1(((dividerType == CY_SYSCLK_DIV_16_5_BIT) || (dividerType == CY_SYSCLK_DIV_24_5_BIT)) &&
                 (dividerIntValue != NULL) && (dividerFracValue != NULL));

    if (dividerType == CY_SYSCLK_DIV_16_5_BIT)
    {
        CY_ASSERT_L1(dividerNum < PERI_DIV_16_5_NR);
        *dividerIntValue  = _FLD2VAL(PERI_DIV_16_5_CTL_INT16_DIV, PERI_DIV_16_5_CTL[dividerNum]);
        *dividerFracValue = _FLD2VAL(PERI_DIV_16_5_CTL_FRAC5_DIV, PERI_DIV_16_5_CTL[dividerNum]);
    }
    else
    { /* 24.5-bit divider */
        CY_ASSERT_L1(dividerNum < PERI_DIV_24_5_NR);
        *dividerIntValue  = _FLD2VAL(PERI_DIV_24_5_CTL_INT24_DIV, PERI_DIV_24_5_CTL[dividerNum]);
        *dividerFracValue = _FLD2VAL(PERI_DIV_24_5_CTL_FRAC5_DIV, PERI_DIV_24_5_CTL[dividerNum]);
    }
}

/*******************************************************************************
* Function Name: Cy_SysClk_PeriphAssignDivider
****************************************************************************//**
*
* Assigns a programmable divider to a selected IP block, such as a TCPWM or SCB.
*
* \param ipBlock specifies ip block to connect the clock divider to.
*
* \param dividerType specifies which type of divider to use; \ref cy_en_divider_types_t
*
* \param dividerNum specifies which divider of the selected type to configure
*
* \return \ref cy_en_sysclk_status_t
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PeriphAssignDivider
*
*******************************************************************************/
__STATIC_INLINE cy_en_sysclk_status_t
                Cy_SysClk_PeriphAssignDivider(en_clk_dst_t ipBlock,
                                              cy_en_divider_types_t dividerType, uint32_t dividerNum)
{
    cy_en_sysclk_status_t retval = CY_SYSCLK_BAD_PARAM;
    if ((CY_PERI_CLOCK_NR > ipBlock) && (CY_SYSCLK_DIV_24_5_BIT >= dividerType))
    {
        if (((dividerType == CY_SYSCLK_DIV_8_BIT)    && (dividerNum < PERI_DIV_8_NR))    ||
            ((dividerType == CY_SYSCLK_DIV_16_BIT)   && (dividerNum < PERI_DIV_16_NR))   ||
            ((dividerType == CY_SYSCLK_DIV_16_5_BIT) && (dividerNum < PERI_DIV_16_5_NR)) ||
            ((dividerType == CY_SYSCLK_DIV_24_5_BIT) && (dividerNum < PERI_DIV_24_5_NR)))
        {
            PERI_CLOCK_CTL[ipBlock] = _VAL2FLD(CY_PERI_CLOCK_CTL_TYPE_SEL, dividerType) |
                                      _VAL2FLD(CY_PERI_CLOCK_CTL_DIV_SEL, dividerNum);
            retval = CY_SYSCLK_SUCCESS;
        }
    }
    return (retval);
}

/*******************************************************************************
* Function Name: Cy_SysClk_PeriphGetAssignedDivider
****************************************************************************//**
*
* Reports which clock divider is assigned to a selected IP block.
*
* \param ipBlock specifies ip block to connect the clock divider to.
*
* \return The divider type and number, where bits [7:6] = type, bits[5:0] = divider
* number within that type
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PeriphAssignDivider
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SysClk_PeriphGetAssignedDivider(en_clk_dst_t ipBlock)
{
    CY_ASSERT_L1(CY_PERI_CLOCK_NR > ipBlock);
    return (PERI_CLOCK_CTL[ipBlock] & (CY_PERI_CLOCK_CTL_DIV_SEL_Msk | CY_PERI_CLOCK_CTL_TYPE_SEL_Msk));
}

/*******************************************************************************
* Function Name: Cy_SysClk_PeriphEnableDivider
****************************************************************************//**
*
* Enables the selected divider.
*
* \param dividerType specifies which type of divider to use; \ref cy_en_divider_types_t
*
* \param dividerNum specifies which divider of the selected type to configure
*
* \note This function also sets the phase alignment bits such that the enabled
* divider is aligned to clk_peri. See \ref Cy_SysClk_PeriphDisableDivider()
* for information on how to phase-align a divider after it is enabled.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PeriphEnableDivider
*
*******************************************************************************/
__STATIC_INLINE cy_en_sysclk_status_t
                Cy_SysClk_PeriphEnableDivider(cy_en_divider_types_t dividerType, uint32_t dividerNum)
{
    cy_en_sysclk_status_t retval = CY_SYSCLK_BAD_PARAM;
    if (dividerType <= CY_SYSCLK_DIV_24_5_BIT)
    {
        if (((dividerType == CY_SYSCLK_DIV_8_BIT)    && (dividerNum < PERI_DIV_8_NR))    ||
            ((dividerType == CY_SYSCLK_DIV_16_BIT)   && (dividerNum < PERI_DIV_16_NR))   ||
            ((dividerType == CY_SYSCLK_DIV_16_5_BIT) && (dividerNum < PERI_DIV_16_5_NR)) ||
            ((dividerType == CY_SYSCLK_DIV_24_5_BIT) && (dividerNum < PERI_DIV_24_5_NR)))
        {
            /* specify the divider, make the reference = clk_peri, and enable the divider */
            PERI_DIV_CMD = PERI_DIV_CMD_ENABLE_Msk                         |
                           CY_PERI_DIV_CMD_PA_TYPE_SEL_Msk                 |
                           CY_PERI_DIV_CMD_PA_DIV_SEL_Msk                  |
                           _VAL2FLD(CY_PERI_DIV_CMD_TYPE_SEL, dividerType) |
                           _VAL2FLD(CY_PERI_DIV_CMD_DIV_SEL,  dividerNum);
            (void)PERI_DIV_CMD; /* dummy read to handle buffered writes */
            retval = CY_SYSCLK_SUCCESS;
        }
    }
    return (retval);
}

/*******************************************************************************
* Function Name: Cy_SysClk_PeriphDisableDivider
****************************************************************************//**
*
* Disables a selected divider.
*
* \param dividerType specifies which type of divider to use; \ref cy_en_divider_types_t.
*
* \param dividerNum specifies which divider of the selected type to configure.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PeriphDisableDivider
*
*******************************************************************************/
__STATIC_INLINE cy_en_sysclk_status_t
                Cy_SysClk_PeriphDisableDivider(cy_en_divider_types_t dividerType, uint32_t dividerNum)
{
    cy_en_sysclk_status_t retval = CY_SYSCLK_BAD_PARAM;
    if (dividerType <= CY_SYSCLK_DIV_24_5_BIT)
    {
        if (((dividerType == CY_SYSCLK_DIV_8_BIT)    && (dividerNum < PERI_DIV_8_NR))    ||
            ((dividerType == CY_SYSCLK_DIV_16_BIT)   && (dividerNum < PERI_DIV_16_NR))   ||
            ((dividerType == CY_SYSCLK_DIV_16_5_BIT) && (dividerNum < PERI_DIV_16_5_NR)) ||
            ((dividerType == CY_SYSCLK_DIV_24_5_BIT) && (dividerNum < PERI_DIV_24_5_NR)))
        {
            /* specify the divider and disable it */
            PERI_DIV_CMD = _VAL2FLD(PERI_DIV_CMD_DISABLE, 1U) /* 1 = disable */ |
                            _VAL2FLD(CY_PERI_DIV_CMD_TYPE_SEL, dividerType)        |
                            _VAL2FLD(CY_PERI_DIV_CMD_DIV_SEL,  dividerNum);
            retval = CY_SYSCLK_SUCCESS;
        }
    }
    return (retval);
}

/*******************************************************************************
* Function Name: Cy_SysClk_PeriphEnablePhaseAlignDivider
****************************************************************************//**
*
* First disables a selected divider (\ref Cy_SysClk_PeriphDisableDivider),
* then aligns that divider to another programmable divider, and enables the
* selected divider. The divider to align to must already be enabled in order
* to align a divider to it.
*
* \param dividerType specifies which type of divider to use; \ref cy_en_divider_types_t.
*
* \param dividerNum specifies which divider of the selected type to configure.
*
* \param dividerTypePA type of divider to phase-align to; \ref cy_en_divider_types_t.
*
* \param dividerNumPA divider number of type specified to phase align to.
*
* \note
* To phase-align a divider to clk_peri, set dividerTypePA to 3 and dividerNumPA
* to 63.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PeriphEnablePhaseAlignDivider
*
*******************************************************************************/
__STATIC_INLINE cy_en_sysclk_status_t
                Cy_SysClk_PeriphEnablePhaseAlignDivider(cy_en_divider_types_t dividerType, uint32_t dividerNum,
                                                        cy_en_divider_types_t dividerTypePA, uint32_t dividerNumPA)
{
    cy_en_sysclk_status_t retval = CY_SYSCLK_BAD_PARAM;
    if (dividerTypePA <= CY_SYSCLK_DIV_24_5_BIT)
    {
        if (((dividerTypePA == CY_SYSCLK_DIV_8_BIT)    && (dividerNumPA < PERI_DIV_8_NR))    ||
            ((dividerTypePA == CY_SYSCLK_DIV_16_BIT)   && (dividerNumPA < PERI_DIV_16_NR))   ||
            ((dividerTypePA == CY_SYSCLK_DIV_16_5_BIT) && (dividerNumPA < PERI_DIV_16_5_NR)) ||
            ((dividerTypePA == CY_SYSCLK_DIV_24_5_BIT) && ((dividerNumPA < PERI_DIV_24_5_NR) || (dividerNumPA == 63u))))
        {
            /* First, disable the divider that is to be phase-aligned.
               The other two parameters are checked in that function;
               if they're not valid, the divider is not disabled. */
            retval = Cy_SysClk_PeriphDisableDivider(dividerType, dividerNum);
            if (retval == CY_SYSCLK_SUCCESS)
            {
                /* Then, specify the reference divider, and the divider, and enable the divider. */
                PERI_DIV_CMD = _VAL2FLD(PERI_DIV_CMD_ENABLE, 1U) /* 1 = enable */ |
                                _VAL2FLD(PERI_DIV_CMD_PA_TYPE_SEL, dividerTypePA)  |
                                _VAL2FLD(PERI_DIV_CMD_PA_DIV_SEL,  dividerNumPA)   |
                                _VAL2FLD(CY_PERI_DIV_CMD_TYPE_SEL,    dividerType)    |
                                _VAL2FLD(CY_PERI_DIV_CMD_DIV_SEL,     dividerNum);
            }
        }
    }
    return (retval);
}

/*******************************************************************************
* Function Name: Cy_SysClk_PeriphGetDividerEnabled
****************************************************************************//**
*
* Reports the enabled/disabled atate of the selected divider.
*
* \param dividerType specifies which type of divider to use; \ref cy_en_divider_types_t.
*
* \param dividerNum specifies which divider of the selected type to configure.
*
* \return The enabled/disabled state;<br>
* false = disabled<br>
* true = enabled
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PeriphGetDividerEnabled
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysClk_PeriphGetDividerEnabled(cy_en_divider_types_t dividerType, uint32_t dividerNum)
{
    uint32_t retval = 0UL;

    CY_ASSERT_L1(((dividerType == CY_SYSCLK_DIV_8_BIT)    && (dividerNum < PERI_DIV_8_NR))    ||
                 ((dividerType == CY_SYSCLK_DIV_16_BIT)   && (dividerNum < PERI_DIV_16_NR))   ||
                 ((dividerType == CY_SYSCLK_DIV_16_5_BIT) && (dividerNum < PERI_DIV_16_5_NR)) ||
                 ((dividerType == CY_SYSCLK_DIV_24_5_BIT) && (dividerNum < PERI_DIV_24_5_NR)));

    switch(dividerType)
    {
        case CY_SYSCLK_DIV_8_BIT:
            retval = _FLD2VAL(PERI_DIV_8_CTL_EN, PERI_DIV_8_CTL[dividerNum]);
            break;
        case CY_SYSCLK_DIV_16_BIT:
            retval = _FLD2VAL(PERI_DIV_16_CTL_EN, PERI_DIV_16_CTL[dividerNum]);
            break;
        case CY_SYSCLK_DIV_16_5_BIT:
            retval = _FLD2VAL(PERI_DIV_16_5_CTL_EN, PERI_DIV_16_5_CTL[dividerNum]);
            break;
        case CY_SYSCLK_DIV_24_5_BIT:
            retval = _FLD2VAL(PERI_DIV_24_5_CTL_EN, PERI_DIV_24_5_CTL[dividerNum]);
            break;
        default:
            break;
    }
    return ((bool)retval);
}
/** \} group_sysclk_clk_peripheral_funcs */


/* ========================================================================== */
/* =========================    clk_slow SECTION    ========================= */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_clk_slow_funcs
* \{
*/
__STATIC_INLINE void Cy_SysClk_ClkSlowSetDivider(uint8_t divider);
__STATIC_INLINE uint8_t Cy_SysClk_ClkSlowGetDivider(void);

/*******************************************************************************
* Function Name: Cy_SysClk_ClkSlowSetDivider
****************************************************************************//**
*
* Sets the clock divider for the slow clock. The source of this clock is the
* peripheral clock (clkPeri), which is sourced from clkHf[0].
*
* \param divider Divider value between 0 and 255.
* Causes integer division of (divider value + 1), or division by 1 to 256.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkSlowSetDivider
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_ClkSlowSetDivider(uint8_t divider)
{
    CY_REG32_CLR_SET(CPUSS_CM0_CLOCK_CTL, CPUSS_CM0_CLOCK_CTL_SLOW_INT_DIV, (uint32_t)divider);
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkSlowGetDivider
****************************************************************************//**
*
* Reports the divider value for the slow clock.
*
* \return The divider value.
* The integer division done is by (divider value + 1), or division by 1 to 256.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkSlowSetDivider
*
*******************************************************************************/
__STATIC_INLINE uint8_t Cy_SysClk_ClkSlowGetDivider(void)
{
    return ((uint8_t)_FLD2VAL(CPUSS_CM0_CLOCK_CTL_SLOW_INT_DIV, CPUSS_CM0_CLOCK_CTL));
}
/** \} group_sysclk_clk_slow_funcs */


/* ========================================================================== */
/* ===========================    clkLf SECTION    ========================== */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_clk_lf_enums
* \{
*/
/**
* Low frequency (clkLf) input sources. See CLK_SELECT register, LFCLK_SEL bits.
* Used with functions \ref Cy_SysClk_ClkLfSetSource, and \ref Cy_SysClk_ClkLfGetSource.
*/
typedef enum
{
    CY_SYSCLK_CLKLF_IN_ILO   = 0u, /**< clkLf is sourced by the internal low speed oscillator (ILO) */
    CY_SYSCLK_CLKLF_IN_WCO   = 1U, /**< clkLf is sourced by the watch crystal oscillator (WCO) */
    CY_SYSCLK_CLKLF_IN_ALTLF = 2u, /**< clkLf is sourced by the Alternate Low Frequency Clock (ALTLF) */
    CY_SYSCLK_CLKLF_IN_PILO  = 3u  /**< clkLf is sourced by the precision low speed oscillator (PILO) */
} cy_en_clklf_in_sources_t;
/** \} group_sysclk_clk_lf_enums */

/**
* \addtogroup group_sysclk_clk_lf_funcs
* \{
*/
__STATIC_INLINE void Cy_SysClk_ClkLfSetSource(cy_en_clklf_in_sources_t source);
__STATIC_INLINE cy_en_clklf_in_sources_t Cy_SysClk_ClkLfGetSource(void);

/*******************************************************************************
* Function Name: Cy_SysClk_ClkLfSetSource
****************************************************************************//**
*
* Sets the source for the low frequency clock(clkLf).
*
* \param source \ref cy_en_clklf_in_sources_t
*
* \note The watchdog timer (WDT) must be unlocked before calling this function.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkLfSetSource
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_ClkLfSetSource(cy_en_clklf_in_sources_t source)
{
    CY_ASSERT_L3(source <= CY_SYSCLK_CLKLF_IN_PILO);
    CY_REG32_CLR_SET(SRSS_CLK_SELECT, SRSS_CLK_SELECT_LFCLK_SEL, source);
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkLfGetSource
****************************************************************************//**
*
* Reports the source for the low frequency clock (clkLf).
*
* \return \ref cy_en_clklf_in_sources_t
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkLfSetSource
*
*******************************************************************************/
__STATIC_INLINE cy_en_clklf_in_sources_t Cy_SysClk_ClkLfGetSource(void)
{
    return (cy_en_clklf_in_sources_t)(_FLD2VAL(SRSS_CLK_SELECT_LFCLK_SEL, SRSS_CLK_SELECT));
}
/** \} group_sysclk_clk_lf_funcs */


/* ========================================================================== */
/* ========================    clk_timer SECTION    ========================= */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_clk_timer_enums
* \{
*/
/**
* Timer clock (clk_timer) input sources. See CLK_TIMER_CTL register, TIMER_SEL
* and TIMER_HF0_DIV bits. Used with functions \ref Cy_SysClk_ClkTimerSetSource, and
* \ref Cy_SysClk_ClkTimerGetSource.
*/
typedef enum
{
    CY_SYSCLK_CLKTIMER_IN_IMO       =     0u, /**< clk_timer is sourced by the internal main oscillator (IMO) */
    CY_SYSCLK_CLKTIMER_IN_HF0_NODIV =     1U, /**< clk_timer is sourced by clkHf[0] undivided */
    CY_SYSCLK_CLKTIMER_IN_HF0_DIV2  = 0x101U, /**< clk_timer is sourced by clkHf[0] divided by 2 */
    CY_SYSCLK_CLKTIMER_IN_HF0_DIV4  = 0x201U, /**< clk_timer is sourced by clkHf[0] divided by 4 */
    CY_SYSCLK_CLKTIMER_IN_HF0_DIV8  = 0x301U  /**< clk_timer is sourced by clkHf[0] divided by 8 */
} cy_en_clktimer_in_sources_t;
/** \} group_sysclk_clk_timer_enums */

/**
* \addtogroup group_sysclk_clk_timer_funcs
* \{
*/
__STATIC_INLINE void Cy_SysClk_ClkTimerSetSource(cy_en_clktimer_in_sources_t source);
__STATIC_INLINE cy_en_clktimer_in_sources_t Cy_SysClk_ClkTimerGetSource(void);
__STATIC_INLINE void Cy_SysClk_ClkTimerSetDivider(uint8_t divider);
__STATIC_INLINE uint8_t Cy_SysClk_ClkTimerGetDivider(void);
__STATIC_INLINE void Cy_SysClk_ClkTimerEnable(void);
__STATIC_INLINE void Cy_SysClk_ClkTimerDisable(void);

/*******************************************************************************
* Function Name: Cy_SysClk_ClkTimerSetSource
****************************************************************************//**
*
* Sets the source for the timer clock (clk_timer). The timer clock can be used
* as a source for SYSTICK as an alternate clock and one or more of the energy 
* profiler counters.
*
* \param source \ref cy_en_clktimer_in_sources_t
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkTimerSetSource
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_ClkTimerSetSource(cy_en_clktimer_in_sources_t source)
{
    CY_ASSERT_L3(source <= CY_SYSCLK_CLKTIMER_IN_HF0_DIV8);
    /* set both fields TIMER_SEL and TIMER_HF0_DIV with the same input value */
    SRSS_CLK_TIMER_CTL =
        (SRSS_CLK_TIMER_CTL & ~(SRSS_CLK_TIMER_CTL_TIMER_SEL_Msk | SRSS_CLK_TIMER_CTL_TIMER_HF0_DIV_Msk)) |
        (uint32_t)source;
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkTimerGetSource
****************************************************************************//**
*
* Reports the source for the timer clock (clk_timer).
*
* \return \ref cy_en_clktimer_in_sources_t
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkTimerSetSource
*
*******************************************************************************/
__STATIC_INLINE cy_en_clktimer_in_sources_t Cy_SysClk_ClkTimerGetSource(void)
{
    /* return both fields TIMER_SEL and TIMER_HF0_DIV as a single combined value */
    return (cy_en_clktimer_in_sources_t)
           (SRSS_CLK_TIMER_CTL & (SRSS_CLK_TIMER_CTL_TIMER_SEL_Msk | SRSS_CLK_TIMER_CTL_TIMER_HF0_DIV_Msk));
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkTimerSetDivider
****************************************************************************//**
*
* Sets the divider for the timer clock (clk_timer).
*
* \param divider Divider value; valid range is 0 to 255. Divides the selected
* source (\ref Cy_SysClk_ClkTimerSetSource) by the (value + 1).
*
* \note
* Do not change the divider value while the timer clock is enabled.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkTimerSetDivider
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_ClkTimerSetDivider(uint8_t divider)
{
    CY_REG32_CLR_SET(SRSS_CLK_TIMER_CTL, SRSS_CLK_TIMER_CTL_TIMER_DIV, (uint32_t)divider);
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkTimerGetDivider
****************************************************************************//**
*
* Reports the divider value for the timer clock (clk_timer).
*
* \return The divider value
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkTimerSetDivider
*
*******************************************************************************/
__STATIC_INLINE uint8_t Cy_SysClk_ClkTimerGetDivider(void)
{
    return ((uint8_t)_FLD2VAL(SRSS_CLK_TIMER_CTL_TIMER_DIV, SRSS_CLK_TIMER_CTL));
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkTimerEnable
****************************************************************************//**
*
* Enables the timer clock (clk_timer). The timer clock can be used as a source
* for SYSTICK and one or more of the energy profiler counters.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkTimerEnable
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_ClkTimerEnable(void)
{
    SRSS_CLK_TIMER_CTL |= _VAL2FLD(SRSS_CLK_TIMER_CTL_ENABLE, 1UL); /* 1 = enable */
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkTimerDisable
****************************************************************************//**
*
* Disables the timer clock (clk_timer).
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkTimerDisable
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_ClkTimerDisable(void)
{
    SRSS_CLK_TIMER_CTL &= ~_VAL2FLD(SRSS_CLK_TIMER_CTL_ENABLE, 1UL); /* 0 = disable */
}
/** \} group_sysclk_clk_timer_funcs */


/* ========================================================================== */
/* =========================    clk_pump SECTION    ========================= */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_clk_pump_enums
* \{
*/
/**
* Pump clock (clk_pump) input sources. See CLK_SELECT register, PUMP_SEL bits.
* Used with functions \ref Cy_SysClk_ClkPumpSetSource, and
* \ref Cy_SysClk_ClkPumpGetSource.
*/
typedef enum
{
    CY_SYSCLK_PUMP_IN_CLKPATH0,  /**< Pump clock input is clock path 0 */
    CY_SYSCLK_PUMP_IN_CLKPATH1,  /**< Pump clock input is clock path 1 */
    CY_SYSCLK_PUMP_IN_CLKPATH2,  /**< Pump clock input is clock path 2 */
    CY_SYSCLK_PUMP_IN_CLKPATH3,  /**< Pump clock input is clock path 3 */
    CY_SYSCLK_PUMP_IN_CLKPATH4,  /**< Pump clock input is clock path 4 */
    CY_SYSCLK_PUMP_IN_CLKPATH5,  /**< Pump clock input is clock path 5 */
    CY_SYSCLK_PUMP_IN_CLKPATH6,  /**< Pump clock input is clock path 6 */
    CY_SYSCLK_PUMP_IN_CLKPATH7,  /**< Pump clock input is clock path 7 */
    CY_SYSCLK_PUMP_IN_CLKPATH8,  /**< Pump clock input is clock path 8 */
    CY_SYSCLK_PUMP_IN_CLKPATH9,  /**< Pump clock input is clock path 9 */
    CY_SYSCLK_PUMP_IN_CLKPATH10, /**< Pump clock input is clock path 10 */
    CY_SYSCLK_PUMP_IN_CLKPATH11, /**< Pump clock input is clock path 11 */
    CY_SYSCLK_PUMP_IN_CLKPATH12, /**< Pump clock input is clock path 12 */
    CY_SYSCLK_PUMP_IN_CLKPATH13, /**< Pump clock input is clock path 13 */
    CY_SYSCLK_PUMP_IN_CLKPATH14, /**< Pump clock input is clock path 14 */
    CY_SYSCLK_PUMP_IN_CLKPATH15  /**< Pump clock input is clock path 15 */
} cy_en_clkpump_in_sources_t;

/**
* Pump clock (clk_pump) divide options. See CLK_SELECT register, PUMP_DIV bits.
* Used with functions \ref Cy_SysClk_ClkPumpSetDivider, and
* \ref Cy_SysClk_ClkPumpGetDivider.
*/
typedef enum
{
    CY_SYSCLK_PUMP_NO_DIV, /**< No division on pump clock */
    CY_SYSCLK_PUMP_DIV_2,  /**< Pump clock divided by 2 */
    CY_SYSCLK_PUMP_DIV_4,  /**< Pump clock divided by 4 */
    CY_SYSCLK_PUMP_DIV_8,  /**< Pump clock divided by 8 */
    CY_SYSCLK_PUMP_DIV_16  /**< Pump clock divided by 16 */
} cy_en_clkpump_divide_t;
/** \} group_sysclk_clk_pump_enums */

/**
* \addtogroup group_sysclk_clk_pump_funcs
* \{
*/
__STATIC_INLINE void Cy_SysClk_ClkPumpSetSource(cy_en_clkpump_in_sources_t source);
__STATIC_INLINE cy_en_clkpump_in_sources_t Cy_SysClk_ClkPumpGetSource(void);
__STATIC_INLINE void Cy_SysClk_ClkPumpSetDivider(cy_en_clkpump_divide_t divider);
__STATIC_INLINE cy_en_clkpump_divide_t Cy_SysClk_ClkPumpGetDivider(void);
__STATIC_INLINE void Cy_SysClk_ClkPumpEnable(void);
__STATIC_INLINE void Cy_SysClk_ClkPumpDisable(void);

/*******************************************************************************
* Function Name: Cy_SysClk_ClkPumpSetSource
****************************************************************************//**
*
* Sets the source for the pump clock (clk_pump). The pump clock can be used for
* the analog pumps in the CTBm block.
*
* \param source \ref cy_en_clkpump_in_sources_t
*
* \note
* Do not change the source while the pump clock is enabled.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkPumpSetSource
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_ClkPumpSetSource(cy_en_clkpump_in_sources_t source)
{
    CY_ASSERT_L3(source <= CY_SYSCLK_PUMP_IN_CLKPATH15);
    CY_REG32_CLR_SET(SRSS_CLK_SELECT, SRSS_CLK_SELECT_PUMP_SEL, source);
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkPumpGetSource
****************************************************************************//**
*
* Reports the source for the pump clock (clk_pump).
*
* \return \ref cy_en_clkpump_in_sources_t
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkPumpSetSource
*
*******************************************************************************/
__STATIC_INLINE cy_en_clkpump_in_sources_t Cy_SysClk_ClkPumpGetSource(void)
{
    return (cy_en_clkpump_in_sources_t)_FLD2VAL(SRSS_CLK_SELECT_PUMP_SEL, SRSS_CLK_SELECT);
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkPumpSetDivider
****************************************************************************//**
*
* Sets the divider of the pump clock (clk_pump).
*
* \param divider \ref cy_en_clkpump_divide_t
*
* \note
* Do not change the divider value while the pump clock is enabled.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkPumpSetDivider
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_ClkPumpSetDivider(cy_en_clkpump_divide_t divider)
{
    CY_ASSERT_L3(divider <= (SRSS_CLK_SELECT_PUMP_DIV_Msk >> SRSS_CLK_SELECT_PUMP_DIV_Pos));
    CY_REG32_CLR_SET(SRSS_CLK_SELECT, SRSS_CLK_SELECT_PUMP_DIV, divider);
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkPumpGetDivider
****************************************************************************//**
*
* Reports the divider value for the pump clock (clk_pump).
*
* \return \ref cy_en_clkpump_divide_t
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkPumpSetDivider
*
*******************************************************************************/
__STATIC_INLINE cy_en_clkpump_divide_t Cy_SysClk_ClkPumpGetDivider(void)
{
    return (cy_en_clkpump_divide_t)_FLD2VAL(SRSS_CLK_SELECT_PUMP_DIV, SRSS_CLK_SELECT);
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkPumpEnable
****************************************************************************//**
*
* Enables the pump clock (clk_pump). The pump clock can be used for the analog
* pumps in the CTBm block.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkPumpEnable
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_ClkPumpEnable(void)
{
    SRSS_CLK_SELECT |= _VAL2FLD(SRSS_CLK_SELECT_PUMP_ENABLE, 1UL); /* 1 = enable */
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkPumpDisable
****************************************************************************//**
*
* Disables the pump clock (clk_pump).
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkPumpDisable
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_ClkPumpDisable(void)
{
    SRSS_CLK_SELECT &= ~_VAL2FLD(SRSS_CLK_SELECT_PUMP_ENABLE, 1UL); /* 0 = disable */
}
/** \} group_sysclk_clk_pump_funcs */


/* ========================================================================== */
/* ==========================    clk_bak SECTION    ========================= */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_clk_bak_enums
* \{
*/
/**
* Backup domain clock (clk_bak) input sources. See BACKUP->CTL register,
* CLK_SEL bits. Used with functions \ref Cy_SysClk_ClkBakSetSource, and
* \ref Cy_SysClk_ClkBakGetSource.
*/
typedef enum
{
    CY_SYSCLK_BAK_IN_WCO,   /**< Backup domain clock input is WCO */
    CY_SYSCLK_BAK_IN_CLKLF  /**< Backup domain clock input is clkLf */
} cy_en_clkbak_in_sources_t;
/** \} group_sysclk_clk_bak_enums */

/**
* \addtogroup group_sysclk_clk_bak_funcs
* \{
*/
__STATIC_INLINE void Cy_SysClk_ClkBakSetSource(cy_en_clkbak_in_sources_t source);
__STATIC_INLINE cy_en_clkbak_in_sources_t Cy_SysClk_ClkBakGetSource(void);

/*******************************************************************************
* Function Name: Cy_SysClk_ClkBakSetSource
****************************************************************************//**
*
* Sets the source for the backup domain clock (clk_bak).
*
* \param source \ref cy_en_clkbak_in_sources_t
*
* \note
* clkLf is not available in all power modes.  For this reason, WCO is the
* preferred source. If the WCO is routed through the clkLf multiplexer
* (see \ref Cy_SysClk_ClkLfSetSource), select WCO directly - do not select clkLf.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkBakSetSource
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysClk_ClkBakSetSource(cy_en_clkbak_in_sources_t source)
{
    CY_ASSERT_L3(source <= CY_SYSCLK_BAK_IN_CLKLF);
    CY_REG32_CLR_SET(BACKUP_CTL, BACKUP_CTL_CLK_SEL, source);
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkBakGetSource
****************************************************************************//**
*
* Reports the source for the backup domain clock (clk_bak).
*
* \return \ref cy_en_clkbak_in_sources_t
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkBakSetSource
*
*******************************************************************************/
__STATIC_INLINE cy_en_clkbak_in_sources_t Cy_SysClk_ClkBakGetSource(void)
{
    return (cy_en_clkbak_in_sources_t)_FLD2VAL(BACKUP_CTL_CLK_SEL, BACKUP_CTL);
}
/** \} group_sysclk_clk_bak_funcs */

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* __CY_SYSCLK_H__ */

/** \} group_sysclk */


/* [] END OF FILE */
