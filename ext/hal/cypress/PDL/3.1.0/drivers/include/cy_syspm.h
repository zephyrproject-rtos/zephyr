/***************************************************************************//**
* \file cy_syspm.h
* \version 2.30
*
* Provides the function definitions for the power management API.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*
*******************************************************************************/

/**
* \defgroup group_syspm System Power Management (SysPm)
* \{
*
* Use the System Power Management (SysPm) driver to enter low-power modes and 
* reduce system power consumption in power sensitive designs. For multi-core 
* devices, this library allows you to individually enter low-power modes for 
* each core.
*
* This document contains the following topics:
*
* * \ref group_syspm_power_modes
* * \ref group_syspm_device_power_modes
*   - \ref group_syspm_switching_into_lpactive
*   - \ref group_syspm_switching_from_lpactive
*   - \ref group_syspm_switching_into_sleep_deepsleep
*   - \ref group_syspm_wakingup_from_sleep_deepsleep
*   - \ref group_syspm_switching_into_hibernate
*   - \ref group_syspm_wakingup_from_hibernate
* * \ref group_syspm_managing_core_regulators
*   - \ref group_syspm_ulp_limitations
*   - \ref group_syspm_lp_limitations
* * \ref group_syspm_managing_pmic
* * \ref group_syspm_managing_backup_domain
* * \ref group_syspm_cb
*   - \ref group_syspm_cb_example
*   - \ref group_syspm_cb_config_consideration
*     - \ref group_syspm_cb_parameters
*     - \ref group_syspm_cb_structures
*     - \ref group_syspm_cb_function_implementation
*     - \ref group_syspm_cb_flow
*     - \ref group_syspm_cb_uregistering
* * \ref group_syspm_definitions
*
* \section group_syspm_section_configuration Configuration Considerations
* \subsection group_syspm_power_modes Power Modes
* PSoC 6 MCUs support the following power modes (in the order of high-to-low 
* power consumption): Active, Low-Power (LPActive), Deep Sleep, and Hibernate. 
* The core(s) can also be in Arm-defined power modes - Active, Sleep, 
* and Deep Sleep.
* 
* \subsection group_syspm_device_power_modes Device Power Modes
* * <b>Active</b> - In this mode the code is executed, and all logic and 
* memories are powered. Firmware may disable clocks for specific peripherals 
* and power down specific analog power domains.
*
* * <b>LPActive</b> - Low-Power mode is like Active mode, but with clock 
* restrictions and limited/slower peripherals to achieve a lower system current.
* Refer to \ref group_syspm_switching_into_lpactive in Configuration 
* considerations.
*
* * <b>Deep Sleep</b> - is a lower power mode where high-frequency clocks are 
* disabled. Deep-Sleep-capable peripherals are available. A normal wakeup from 
* Deep Sleep returns to either LPActive, Active, or Sleep, depending on the 
* previous state and programmed behavior for the configured wakeup interrupt. 
* Likewise, a debug wakeup from Deep Sleep returns to Sleep, depending on which 
* mode was used to enter the Deep Sleep power mode. 
*
* * <b>Hibernate</b> - is an even lower power mode that is entered from 
* firmware, just like Deep Sleep. However, on a wakeup the core and all 
* peripherals go through a full reset. The I/O's state is frozen so that the 
* output driver state is held. Note that in this mode, the core(s) and all 
* peripherals lose their states, so the system and firmware reboot on a wakeup 
* event. Backup memory (if present) can be used to store system states for use 
* on the next reboot.
*
* \subsubsection group_syspm_switching_into_lpactive Switching Device into LPActive
* Before switching into the LPActive power mode, ensure that the device meets 
* the current load limitation. Decrease the clock frequencies, and slow or 
* disable peripherals. Refer to the \ref group_syspm_managing_core_regulators
* Section. <br>
* * The IMO is set to the Clk_HF
*
* * Turn off unused peripherals, or decrease their operating frequencies to 
*   achieve total current consumption less than or equal to 20 mA.
*
* * Call the Cy_SysPm_EnterLowPowerMode() function that will put references 
*   such as POR and BOD into Low-Power mode.
*
* * If the core is sourced by the LDO Core Voltage Regulator, then the 
*   0.9 V (nominal) mode must be set. Refer \ref group_syspm_functions_ldo API 
*   in \ref group_syspm_functions_core_regulators.
*
* * If the core is sourced by the Buck Core Voltage Regulator, then it is 
*   recommended, but not required, to set CY_SYSPM_BUCK_OUT1_VOLTAGE_0_9V. 
*   Decide whether your application can meet the requirements for the 
*   CY_SYSPM_BUCK_OUT1_VOLTAGE_0_9V. 
*   See \ref group_syspm_managing_core_regulators.
*
* \subsubsection group_syspm_switching_from_lpactive Switching Device from LPActive
* To switch a device from LPActive to Active mode, just 
* call Cy_SysPm_ExitLowPowerMode().
*
* \subsubsection group_syspm_switching_into_sleep_deepsleep Switching Device or Core to Sleep or Deep Sleep
* For multi-core devices, the Cy_SysPm_Sleep() and Cy_SysPm_DeepSleep() 
* functions switch only the core that calls the function into the Sleep or 
* the Deep Sleep power mode. To set the whole device in the Sleep or Deep Sleep
* power mode, ensure that each core calls the Cy_SysPm_Sleep() or 
* Cy_SysPm_DeepSleep() function.
*
* There are situations when the device does not switch into the Deep Sleep 
* power mode immediately after the second core calls Cy_SysPm_DeepSleep(). 
* The device will switch into Deep Sleep mode automatically a little bit later, 
* after the low-power circuits are ready to switch into Deep Sleep. Refer to 
* the Cy_SysPm_DeepSleep() description for more details.
*
* All pending interrupts should be cleared before the device is put into a 
* Sleep or Deep Sleep mode, even if they are masked.
*
* For single-core devices, SysPm functions that return the status of the 
* unsupported core always return CY_SYSPM_STATUS_<CORE>_DEEPSLEEP.
*
* \subsubsection group_syspm_wakingup_from_sleep_deepsleep Waking Up from Sleep or Deep Sleep
* For Arm-based devices, an interrupt is required for the core to wake up. 
* For multi-core devices, one core can wake up the other core by sending the 
* event instruction. Use the CMSIS function __SEV() to sent event from one core
* to another.
*
* \subsubsection group_syspm_switching_into_hibernate Switching Device to Hibernate
* If you call Cy_SysPm_Hibernate() from either core, the device will be switched
* into the Hibernate power mode directly, as there is no 
* handshake between cores.
*
* \subsubsection group_syspm_wakingup_from_hibernate Waking Up from Hibernate
*
* The system can wake up from Hibernate mode by configuring:
*
* * Wakeup pin
*
* * LP Comparator
*
* * RTC alarm
*
* * WDT interrupt
*
* Wakeup is supported from device specific pin(s) with programmable polarity. 
* Additionally, unregulated peripherals can wake the device under some 
* conditions. For example, a low-power comparator can wake the device by 
* comparing two external voltages but may not support comparison to an 
* internally-generated voltage. The Backup domain remains functional, and if 
* present it can schedule an alarm to wake the device from Hibernate using RTC. 
* Alternatively, the Watchdog Timer (WDT) can be configured to wake-up the 
* device by WDT interrupt.
* Refer to \ref Cy_SysPm_SetHibernateWakeupSource() for more details.
*
* \subsection group_syspm_managing_core_regulators Managing Core Voltage Regulators
* The SysPm driver provides functionality to manage the power modes of the 
* low-dropout (LDO) and Buck Core Voltage Regulators.
* For both core regulators, two voltages are possible:
*
* * <b>0.9 V (nominal)</b> - core is sourced by 0.9 V (nominal). This core regulator 
*   power mode is called Ultra Low-Power (ULP). In this mode, the device 
*   functionality and performance is limited. You must decrease the operating 
*   frequency and current consumption to meet the 
*   \ref group_syspm_ulp_limitations, shown below.
* * <b>1.1 V (nominal)</b> - core is sourced by 1.1 V (nominal). This core regulator 
*   power mode is called low-power mode (LP). In this mode, you must meet the 
*   \ref group_syspm_lp_limitations, shown below.
*
* \subsubsection group_syspm_ulp_limitations ULP Limitations
* When the core voltage is <b>0.9 V (nominal)</b> the next limitations must be 
* meet: <br>
*   - the maximum operating frequency for all Clk_HF paths must not exceed 
*     <b>50 MHz</b>, whereas the peripheral and slow clock must not exceed <b>25 MHz</b>
*     (refer to \ref group_sysclk driver documentation).
*   - the total current consumption must be less than or equal to <b>20 mA</b><br>
*
* \subsubsection group_syspm_lp_limitations LP Limitations
* When the core voltage is <b>1.1V (nominal)</b> the next limitations must be meet:
*   - the maximum operating frequency for all Clk_HF paths must not exceed 
*     <b>150 MHz</b>, whereas the peripheral and slow clock must not exceed <b>100 MHz</b>
*     (refer to \ref group_sysclk driver documentation). <br>
*   - the total current consumption must be less than or equal to <b>250 mA</b>
*
* \subsection group_syspm_managing_pmic Managing PMIC
*
* The SysPm driver also provides an API to configure the external power 
* management integrated circuit (PMIC) that supplies Vddd.
* Use the API to enable the PMIC output that is routed to pmic_wakeup_out pin, 
* and configure the polarity of the PMIC input (pmic_wakeup_in) that is used to 
* wake up the PMIC.
*
* The PMIC is automatically enabled when:
*
* * the PMIC is locked by a call to Cy_SysPm_PmicLock()
*
* * the configured polarity of the PMIC input and the polarity driven to 
* pmic_wakeup_in pin matches.
*
* Because a call to Cy_SysPm_PmicLock() automatically enables the PMIC, the PMIC
* can remain disabled only when it is unlocked. See Cy_SysPm_PmicUnlock() 
* for more details.
*
* Use Cy_SysPm_PmicIsLocked() to read the current PMIC lock status.
* 
* To enable the PMIC, use these functions in this order:<br>
* \code{.c}
* 1 Cy_SysPm_PmicUnlock();
* 2 Cy_SysPm_PmicEnable();
* 3 Cy_SysPm_PmicLock();
* \endcode
*
* To disable the PMIC block, unlock the PMIC. Then call Cy_SysPm_PmicDisable() 
* with the inverted value of the current active state of the pmic_wakeup_in pin. 
* For example, assume the current state of the pmic_wakeup_in pin is active low.
* To disable the PMIC, call these functions in this order:<br>
* \code{.c}
* 1 Cy_SysPm_PmicUnlock();
* 2 Cy_SysPm_PmicDisable(CY_SYSPM_PMIC_POLARITY_HIGH);
* \endcode
* Note that you do not call Cy_SysPm_PmicLock(), because that automatically 
* enables the PMIC.
* 
* While disabled, the PMIC block is automatically enabled when the 
* pmic_wakeup_in pin state is changed into high state.
*
* To disable the PMIC output, call these functions in this order:
* Cy_SysPm_PmicUnlock();
* Cy_SysPm_PmicDisableOutput();
*
* Do not call Cy_SysPm_PmicLock() (which automatically enables the PMIC output).
* 
* When disabled, the PMIC output is enabled when the PMIC is locked, or by 
* calling Cy_SysPm_PmicEnableOutput().
*
* \subsection group_syspm_managing_backup_domain Managing the Backup Domain
* The SysPm driver provide functions to:
*
* * Configure supercapacitor charge
*
* * Select power supply (Vbackup or Vddd) for the Vddbackup
*
* * Measure Vddbackup using the ADC
* 
* Refer to the \ref group_syspm_functions_backup functions for more details.
*
* \subsection group_syspm_cb SysPm Callbacks
* The SysPm driver handles the low power callbacks declared in the application. 
*
* If there are no callbacks registered, the device just executes the power mode 
* transition. However, it is often the case that your firmware must prepare for 
* low power mode. For example, you may need to disable a peripheral, or ensure 
* that a message is not being transmitted or received.
*
* To enable this, the SysPm driver implements a callback mechanism. When a lower
* power mode transition is about to take place (either entering or exiting 
* \ref group_syspm_device_power_modes), the registered callbacks are called.
*
* The SysPm driver organizes all the callbacks into a linked list. While 
* entering a low-power mode, SysPm goes through that linked list from first to 
* last, executing the callbacks one after another. While exiting low-power mode,
* SysPm goes through that linked list again, but in the opposite direction from 
* last to first.
*
* For example, the picture below shows three callback structures organized into 
* a linked list: myDeepSleep1, mySleep1, myDeepSleep2 (represented with the 
* \ref cy_stc_syspm_callback_t configuration structure). Each structure 
* contains, among other fields, the address of the callback function. The code 
* snippets below set this up so that myDeepSleep1 is called first when entering 
* the low-power mode. This also means that myDeepSleep1 will be the last one to 
* execute when exiting the low-power mode.
*
* The callback structures after registration:
* \image html syspm_2_10_after_registration.png
*
* Your application must register each callback, so that SysPm can execute it. 
* Upon registration, the linked list is built by the SysPm driver. Notice 
* the &mySleep1 address in the myDeepSleep1 
* \ref cy_stc_syspm_callback_t structure. This is filled in by the SysPm driver 
* when you register mySleep1. The order in which the callbacks are registered in
* the application defines the order of their execution by the SysPm driver. You
* may have up to 32 callback functions registered. 
* Call \ref Cy_SysPm_RegisterCallback() to register each callback function. 
* 
* A callback function is typically associated with a particular driver that 
* handles the peripheral. So the callback mechanism enables a peripheral to 
* prepare for a low-power mode (for instance, shutting down the analog part); 
* or to perform tasks while exiting a low-power mode (like enabling the analog 
* part again).
*
* With the callback mechanism you can prevent switching into a low-power mode if
* a peripheral is not ready. For example, driver X is in the process of 
* receiving a message. In the callback function implementation simply return 
* CY_SYSPM_FAIL in a response to CY_SYSPM_CHECK_READY.
*
* If success is returned while executing a callback, the SysPm driver calls the 
* next callback and so on to the end of the list. If at some point a callback 
* returns CY_SYSPM_FAIL in response to the CY_SYSPM_CHECK_READY step, all the 
* callbacks that have already executed are executed in reverse order, with the 
* CY_SYSPM_CHECK_FAIL step. This allows each callback to know that entering the 
* low-power mode has failed. The callback can then undo whatever it did to 
* prepare for low power mode. For example, if the driver X callback shut down 
* the analog part, it can re-enable the analog part.
* 
* Let's switch to an example explaining the implementation, setup, and 
* registration of three callbacks (myDeepSleep1, mySleep1, myDeepSleep2) in the
* application. The \ref group_syspm_cb_config_consideration are provided after 
* the \ref group_syspm_cb_example.
* 
* \subsection group_syspm_cb_example SysPm Callbacks Example
*
* The following code snippets demonstrate how use the SysPm callbacks mechanism.
* We will build the prototype for an application that registers 
* three callback functions: <br>
*    1. mySleep1 - handles Sleep <br>
*    2. myDeepSleep1 - handles Deep Sleep and is associated with peripheral 
*       HW1_address (see <a href="..\..\pdl_user_guide.pdf">PDL Design</a> 
*       section to learn about the base hardware 
*       address) <br>
*    3. myDeepSleep2 - handles entering and exiting Deep Sleep and is 
*       associated with peripheral HW2_address <br>
*
* We set things up so that the mySleep1 and myDeepSleep1 callbacks do nothing 
* while entering the low power mode (skip on CY_SYSPM_SKIP_BEFORE_TRANSITION - 
* see \ref group_syspm_cb_function_implementation in 
* \ref group_syspm_cb_config_consideration).
* Skipping the actions while entering low power might be useful if you need 
* to save the time while switching low-power modes. This is because the callback
* function with skipped mode is not even called.
* 
* Let's first declare the callback functions. Each gets the pointer to the 
* \ref cy_stc_syspm_callback_params_t structure as the argument.
*
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_Callback_Func_Declaration
*
* Now we setup the \ref cy_stc_syspm_callback_params_t structures that we will
* pass to callback functions. Note that for the myDeepSleep1 and myDeepSleep2 
* callbacks we also pass the pointers to the peripherals related to that 
* callback (see <a href="..\..\pdl_user_guide.pdf">PDL Design</a> section to 
* learn about the base hardware address). 
* The configuration considerations related to this structure are described 
* in \ref group_syspm_cb_parameters in \ref group_syspm_cb_config_consideration.
*
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_Callback_Params_Declaration
*
* Now we setup the actual callback configuration structures. Each of these 
* contains, among the other fields, the address of the 
* \ref cy_stc_syspm_callback_params_t we just set up. We will use the callback 
* configuration structures later in the code to register the callbacks in the 
* SysPm driver. Again, we set things up so that the mySleep1 and myDeepSleep1 
* callbacks do nothing while entering the low power mode 
* (skip on CY_SYSPM_SKIP_BEFORE_TRANSITION) - see 
* \ref group_syspm_cb_function_implementation in 
* \ref group_syspm_cb_config_consideration.
*
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_Callback_Structure_Declaration
*
* Note that in each case the last two fields are NULL. These are fields used by 
* the SysPm driver to set up the linked list of callback functions.
* 
* The callback structures are now defined and allocated in the user's 
* memory space:
* \image html syspm_2_10_before_registration.png
*
* Now we implement the callback functions. See 
* \ref group_syspm_cb_function_implementation in 
* \ref group_syspm_cb_config_consideration for the instructions on how the 
* callback functions should be implemented.
*
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_Callback_Func_Implementation
*
* Finally, we register the callbacks so that the SysPm driver knows about them. 
* The order in which the callbacks will be called depends upon the order in 
* which the callbacks are registered. If there are no callbacks registered, 
* the device just executes the power mode transition.
*
* Callbacks that reconfigure global resources, such as clock frequencies, should
* be registered last. They then modify global resources as the final step before
* entering the low power mode, and restore those resources first, as the system
* returns from low-power mode.
*
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_RegisterCallback
*
* We are done configuring three callbacks. Now the SysPm driver will execute the
* callbacks appropriately whenever there is a call to a power mode transition 
* function: \ref Cy_SysPm_Sleep(), \ref Cy_SysPm_DeepSleep(), 
* \ref Cy_SysPm_EnterLowPowerMode(), \ref Cy_SysPm_ExitLowPowerMode(), and 
* \ref Cy_SysPm_Hibernate().
* \note On a wakeup from hibernate the device goes through a reset, so the 
* callbacks with CY_SYSPM_AFTER_TRANSITION are not executed. Refer to 
* \ref Cy_SysPm_Hibernate() for more details.
* 
* Refer to \ref group_syspm_cb_uregistering in 
* \ref group_syspm_cb_config_consideration to learn what to do if you need to 
* remove the callback from the linked list. You might want to unregister the 
* callback for debug purposes.
* 
* Refer to \ref group_syspm_cb_flow in \ref group_syspm_cb_config_consideration 
* to learn about how the SysPm is processing the callbacks.
*
* \subsection group_syspm_cb_config_consideration Callback Configuration Considerations
*
* \subsubsection group_syspm_cb_parameters Callback Function Parameters
* 
* The <b>callbackParams</b> parameter of the callback function is a 
* \ref cy_stc_syspm_callback_params_t structure. The first field in this 
* structure (<b>mode</b>) is for internal use. In the example code we used a 
* dummy value CY_SYSPM_CHECK_READY to eliminate compilation errors associated 
* with the enumeration. The driver sets the <b>mode</b> field to the correct 
* value when calling the callback functions (the mode is referred to as step in 
* the \ref group_syspm_cb_function_implementation). The callback function reads 
* the value and acts based on the mode set by the SysPm driver. The <b>base</b> 
* and <b>context</b> fields are optional and can be NULL. Some drivers require a
* base hardware address and a context. If your callback routine needs access to 
* the driver registers or context, provide those values (see 
* <a href="..\..\pdl_user_guide.pdf">PDL Design</a> section 
* to learn about Base Hardware Address). Be aware of MISRA warnings if these 
* parameters are NULL.
*
* \subsubsection group_syspm_cb_structures Callback Function Structure
* For each callback, provide a \ref cy_stc_syspm_callback_t structure. Some 
* fields in this structure are maintained by the driver. Use NULL for 
* <b>prevItm</b> and <b>nextItm</b>. The driver uses these fields to build a 
* linked list of callback functions.
*
* \warning The Cy_SysPm_RegisterCallback() function stores a pointer to the 
* cy_stc_syspm_callback_t variable. Do not modify elements of the 
* cy_stc_syspm_callback_t structure after the callback is registered. 
* You are responsible for ensuring that the variable remains in scope. 
* Typically the structure is declared as a global or static variable, or as a 
* local variable in the main() function.
*
* \subsubsection group_syspm_cb_function_implementation Callback Function Implementation
*
* Every callback function should handle four possible steps (referred to as 
* "mode") defined in \ref cy_en_syspm_callback_mode_t : <br>
*    * CY_SYSPM_CHECK_READY - prepare for entering a low power mode<br>
*    * CY_SYSPM_BEFORE_TRANSITION - The actions to be done before entering 
*      the low-power mode <br>
*    * CY_SYSPM_AFTER_TRANSITION - The actions to be done after exiting the 
*      low-power mode <br>
*    * CY_SYSPM_CHECK_FAIL - roll back the actions done in the callbacks 
*      executed previously with CY_SYSPM_CHECK_READY <br>
*
* A callback function can skip steps (see \ref group_syspm_skip_callback_modes).
* In our example mySleep1 and myDeepSleep1 callbacks do nothing while entering 
* the low power mode (skip on CY_SYSPM_BEFORE_TRANSITION). If there is anything 
* preventing low power mode entry - return CY_SYSPM_FAIL in response to 
* CY_SYSPM_CHECK_READY in your callback implementation. Note that the callback 
* should return CY_SYSPM_FAIL only in response to CY_SYSPM_CHECK_READY. The 
* callback function should always return CY_SYSPM_PASS for other modes: 
* CY_SYSPM_CHECK_FAIL, CY_SYSPM_BEFORE_TRANSITION, and CY_SYSPM_AFTER_TRANSITION
* (see \ref group_syspm_cb_flow).
* 
* \subsubsection group_syspm_cb_flow Callbacks Execution Flow
*
* This section explains what happens during a power transition, when callbacks 
* are implemented and set up correctly. The following discussion assumes: <br>
* * All required callback functions are defined and implemented <br>
* * All cy_stc_syspm_callback_t structures are filled with required values <br>
* * All callbacks are successfully registered 
*
* User calls one of the power mode transition functions: \ref Cy_SysPm_Sleep(), 
* \ref Cy_SysPm_DeepSleep(), \ref Cy_SysPm_EnterLowPowerMode(), 
* \ref Cy_SysPm_ExitLowPowerMode(), and \ref Cy_SysPm_Hibernate(). 
* It calls each callback with the mode set to CY_SYSPM_CHECK_READY. This 
* triggers execution of the code for that step inside of each user callback.
* 
* If that process is successful for all callbacks, then 
* \ref Cy_SysPm_ExecuteCallback() calls each callback with the mode set to 
* CY_SYSPM_BEFORE_TRANSITION. This triggers execution of the code for that step 
* inside each user callback. We then enter the low power mode.
* 
* When exiting the low power mode, the SysPm driver executes 
* \ref Cy_SysPm_ExecuteCallback() again. This time it calls each callback in 
* reverse order, with the mode set to CY_SYSPM_AFTER_TRANSITION. This triggers 
* execution of the code for that step inside each user callback. When complete, 
* we are back to Active state.
* 
* A callback can return CY_SYSPM_FAIL only while executing the 
* CY_SYSPM_CHECK_READY step. If that happens, then the remaining callbacks are 
* not executed. Any callbacks that have already executed are called again, in 
* reverse order, with CY_SYSPM_CHECK_FAIL. This allows the system to return to 
* the previous state. Then any of the functions (\ref Cy_SysPm_Sleep(), 
* \ref Cy_SysPm_DeepSleep(), \ref Cy_SysPm_EnterLowPowerMode(), 
* \ref Cy_SysPm_ExitLowPowerMode(), and \ref Cy_SysPm_Hibernate()) that 
* attempted to switch the device into a low power mode will 
* return CY_SYSPM_FAIL.
*
* Callbacks that reconfigure global resources, such as clock frequencies, 
* should be registered last. They then modify global resources as the final 
* step before entering the low power mode, and restore those resources first, 
* as the system returns from Low-power mode.
*
* \subsubsection group_syspm_cb_uregistering Callback Unregistering
*
* Unregistering the callback might be useful when you need dynamically manage
* the callbacks.
*
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_UnregisterCallback
* The callback structures after mySleep1 callback is unregistered:
* \image html syspm_2_10_unregistration.png
*
* \section group_syspm_definitions Definitions
*
* <table class="doxtable">
*   <tr>
*     <th>Term</th>
*     <th>Definition</th>
*   </tr>
*
*   <tr>
*     <td>LDO</td>
*     <td>Low Dropout Linear Regulator (LDO). The functions that manage this 
*         block are grouped as \ref group_syspm_functions_ldo under 
*         \ref group_syspm_functions_core_regulators</td>
*   </tr>
*
*   <tr>
*     <td>SIMO Buck</td>
*     <td>Single Inductor Multiple Output Buck Regulator, referred as 
*         "Buck regulator" throughout the documentation. The functions that 
*         manage this block are grouped as \ref group_syspm_functions_buck under
*         \ref group_syspm_functions_core_regulators</td>
*   </tr>
*
*   <tr>
*     <td>PMIC</td>
*     <td>Power Management Integrated Circuit. The functions that manage this 
*         block are grouped as \ref group_syspm_functions_pmic</td>
*   </tr>
*
*   <tr>
*     <td>LPActive</td>
*     <td>Low-Power Active mode. The MCU power mode. 
*         See the \ref group_syspm_switching_into_lpactive 
*         section for details</td>
*   </tr>
* </table>
*
* \section group_syspm_section_more_information More Information
* For more information on the Power Management (SysPm) driver,
* refer to the technical reference manual (TRM).
*
* \section group_syspm_MISRA MISRA-C Compliance
* The SysPm driver does not have any specific deviations.
*
* \section group_syspm_changelog Changelog
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td>2.30</td>
*     <td>Flattened the driver source code organization into the single
*         source directory and the single include directory
*     </td>
*     <td>Simplified the driver library directory structure</td>
*   </tr>
*   <tr>
*     <td>2.20</td>
*     <td> <br>
*          * Added support for changing core voltage when the protection context
*            is other that zero. Such support is available only for devices 
*            that support modifying registers via syscall.
*
*          * For preproduction PSoC 6 devices the changing core voltage
*            is prohibited when the protection context is other than zero.
*
*          * Updated the following functions. They now have a 
*            \ref cy_en_syspm_status_t return value and use a syscall: <br>
*            Cy_SysPm_LdoSetVoltage <br>
*            Cy_SysPm_BuckSetVoltage1 <br>
*            Cy_SysPm_BuckEnable <br>
*            No backward compatibility issues
*
*          * Added new CY_SYSPM_CANCELED element in 
*            the \ref cy_en_syspm_status_t
*
*          * Documentation updates
*
*          * Added warning that
*            Cy_SysPm_PmicDisable(CY_SYSPM_PMIC_POLARITY_LOW) is not 
*            supported by hardware.
*     </td>
*     <td>Added support for changing the core voltage in protection context 
*         higher that zero (PC > 0) <br>
*         Documentation update and clarification
*     </td>
*   </tr>
*   <tr>
*     <td>2.10</td>
*     <td> <br>
*          * Changed names for all Backup, Buck-related functions, defines, 
*            and enums <br>
*          * Changed next power mode function names: <br>
*            Cy_SysPm_EnterLpMode <br>
*            Cy_SysPm_ExitLpMode <br>
*            Cy_SysPm_SetHibWakeupSource <br>
*            Cy_SysPm_ClearHibWakeupSource <br>
*            Cy_SysPm_GetIoFreezeStatus <br>
*          * Changed following enumeration names: <br>
*            cy_en_syspm_hib_wakeup_source_t <br>
*            cy_en_syspm_simo_buck_voltage1_t <br>
*            cy_en_syspm_simo_buck_voltage2_t <br>
*          * Updated Power Modes documentation section <br>
*          * Added Low Power Callback Managements section <br>
*          * Documentation edits 
*     </td>
*     <td>Improvements made based on usability feedback <br>
*         Documentation update and clarification
*     </td>
*   </tr>
*   <tr>
*     <td>2.0</td>
*     <td>Enhancement and defect fixes: <br> 
*         * Added input parameter(s) validation to all public functions <br>
*         * Removed "_SysPm_" prefixes from the internal functions names <br>
*         * Changed the type of elements with limited set of values, from 
*           uint32_t to enumeration
*         * Enhanced syspm callback mechanism
*         * Added functions to control: <br>
*           * Power supply for the Vddbackup <br>
*           * Supercapacitor charge <br>
*           * Vddbackup measurement by ADC <br>
*     </td>
*     <td></td>
*   </tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*
* \defgroup group_syspm_macros Macros
* \defgroup group_syspm_functions Functions
* \{
*   \defgroup group_syspm_functions_power          Power Modes
*   \defgroup group_syspm_functions_power_status   Power Status 
*   \defgroup group_syspm_functions_iofreeze       I/Os Freeze
*   \defgroup group_syspm_functions_core_regulators    Core Voltage Regulation 
*   \{
*     \defgroup group_syspm_functions_ldo      LDO
*     \defgroup group_syspm_functions_buck     Buck
*   \}
*   \defgroup group_syspm_functions_pmic       PMIC
*   \defgroup group_syspm_functions_backup     Backup Domain
*   \defgroup group_syspm_functions_callback   Low Power Callbacks
* \}
* \defgroup group_syspm_data_structures Data Structures
* \defgroup group_syspm_data_enumerates Enumerated Types
*/

#if !defined (CY_SYSPM_H)
#define CY_SYSPM_H

#include <stdbool.h>
#include <stddef.h>

#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_syslib.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
*       Register Constants
*******************************************************************************/

/**
* \addtogroup group_syspm_macros
* \{
*/

/** Driver major version */
#define CY_SYSPM_DRV_VERSION_MAJOR       2

/** Driver minor version */
#define CY_SYSPM_DRV_VERSION_MINOR       30

/** syspm driver identifier */
#define CY_SYSPM_ID                      (CY_PDL_DRV_ID(0x10U))


/*******************************************************************************
*       Internal Defines
*******************************************************************************/

/** \cond INTERNAL */

/** The internal define of the unlock value for the PMIC functions */
#define CY_SYSPM_PMIC_UNLOCK_KEY                         (0x3AU)

/* Macro to validate parameters in Cy_SysPm_SetHibernateWakeupSource() and for Cy_SysPm_ClearHibernateWakeupSource() function */
#define CY_SYSPM_IS_WAKE_UP_SOURCE_VALID(wakeupSource)   (0UL == ((wakeupSource) & \
                                                         ((uint32_t) ~(CY_SYSPM_HIB_WAKEUP_SOURSE_MASK))))

/* Macro to validate parameters in Cy_SysPm_PmicDisable() function */
#define CY_SYSPM_IS_POLARITY_VALID(polarity)            (((polarity) == CY_SYSPM_PMIC_POLARITY_LOW) || \
                                                         ((polarity) == CY_SYSPM_PMIC_POLARITY_HIGH))

/* Macro to validate parameters in Cy_SysPm_BuckSetVoltage1() function */
#define CY_SYSPM_IS_BUCK_VOLTAGE1_VALID(voltage)        (((voltage) == CY_SYSPM_BUCK_OUT1_VOLTAGE_0_9V) || \
                                                         ((voltage) == CY_SYSPM_BUCK_OUT1_VOLTAGE_1_1V))

/* Macro to validate parameters in Cy_SysPm_BuckSetVoltage2() function */
#define CY_SYSPM_IS_BUCK_VOLTAGE2_VALID(voltage)        (((voltage) == CY_SYSPM_BUCK_OUT2_VOLTAGE_1_15V) || \
                                                         ((voltage) == CY_SYSPM_BUCK_OUT2_VOLTAGE_1_2V)  || \
                                                         ((voltage) == CY_SYSPM_BUCK_OUT2_VOLTAGE_1_25V) || \
                                                         ((voltage) == CY_SYSPM_BUCK_OUT2_VOLTAGE_1_3V)  || \
                                                         ((voltage) == CY_SYSPM_BUCK_OUT2_VOLTAGE_1_35V) || \
                                                         ((voltage) == CY_SYSPM_BUCK_OUT2_VOLTAGE_1_4V)  || \
                                                         ((voltage) == CY_SYSPM_BUCK_OUT2_VOLTAGE_1_45V) || \
                                                         ((voltage) == CY_SYSPM_BUCK_OUT2_VOLTAGE_1_5V))

/* Macro to validate parameters in Cy_SysPm_BuckIsOutputEnabled() function */
#define CY_SYSPM_IS_BUCK_OUTPUT_VALID(output)           (((output) == CY_SYSPM_BUCK_VBUCK_1) || \
                                                         ((output) == CY_SYSPM_BUCK_VRF))

/* Macro to validate parameters in Cy_SysPm_LdoSetVoltage() function */
#define CY_SYSPM_IS_LDO_VOLTAGE_VALID(voltage)          (((voltage) == CY_SYSPM_LDO_VOLTAGE_0_9V) || \
                                                         ((voltage) == CY_SYSPM_LDO_VOLTAGE_1_1V))

/* Macro to validate parameters in Cy_SysPm_ExecuteCallback() function */
#define CY_SYSPM_IS_CALLBACK_TYPE_VALID(type)           (((type) == CY_SYSPM_SLEEP) || \
                                                         ((type) == CY_SYSPM_DEEPSLEEP) || \
                                                         ((type) == CY_SYSPM_HIBERNATE) || \
                                                         ((type) == CY_SYSPM_ENTER_LOWPOWER_MODE) || \
                                                         ((type) == CY_SYSPM_EXIT_LOWPOWER_MODE))

/* Macro to validate parameters in Cy_SysPm_ExecuteCallback() function */
#define CY_SYSPM_IS_CALLBACK_MODE_VALID(mode)           (((mode) == CY_SYSPM_CHECK_READY) || \
                                                         ((mode) == CY_SYSPM_CHECK_FAIL) || \
                                                         ((mode) == CY_SYSPM_BEFORE_TRANSITION) || \
                                                         ((mode) == CY_SYSPM_AFTER_TRANSITION))

/* Macro to validate parameters in Cy_SysPm_Sleep() and for Cy_SysPm_DeepSleep() function */
#define CY_SYSPM_IS_WAIT_FOR_VALID(waitFor)             (((waitFor) == CY_SYSPM_WAIT_FOR_INTERRUPT) || \
                                                         ((waitFor) == CY_SYSPM_WAIT_FOR_EVENT))

/* Macro to validate parameters in Cy_SysPm_BackupSetSupply() function */
#define CY_SYSPM_IS_VDDBACKUP_VALID(vddBackControl)      (((vddBackControl) == CY_SYSPM_VDDBACKUP_DEFAULT) || \
                                                          ((vddBackControl) == CY_SYSPM_VDDBACKUP_VBACKUP))
                                                          
/* Macro to validate parameters in Cy_SysPm_BackupSuperCapCharge() function */
#define CY_SYSPM_IS_SC_CHARGE_KEY_VALID(key)            (((key) == CY_SYSPM_SC_CHARGE_ENABLE) || \
                                                         ((key) == CY_SYSPM_SC_CHARGE_DISABLE))

/** The maximum callbacks number */
#define CY_SYSPM_CALLBACKS_NUMBER_MAX           (32U)

/** The mask to unlock the Hibernate power mode */
#define CY_SYSPM_PWR_HIBERNATE_UNLOCK             ((uint32_t) 0x3Au << SRSS_PWR_HIBERNATE_UNLOCK_Pos)

/** The mask to set the Hibernate power mode */
#define CY_SYSPM_PWR_SET_HIBERNATE                 ((uint32_t) CY_SYSPM_PWR_HIBERNATE_UNLOCK |\
                                                                   SRSS_PWR_HIBERNATE_FREEZE_Msk |\
                                                                   SRSS_PWR_HIBERNATE_HIBERNATE_Msk)

/** The mask to retain the Hibernate power mode status */
#define CY_SYSPM_PWR_RETAIN_HIBERNATE_STATUS      ((uint32_t) SRSS_PWR_HIBERNATE_TOKEN_Msk |\
                                                              SRSS_PWR_HIBERNATE_MASK_HIBALARM_Msk |\
                                                              SRSS_PWR_HIBERNATE_MASK_HIBWDT_Msk |\
                                                              SRSS_PWR_HIBERNATE_POLARITY_HIBPIN_Msk |\
                                                              SRSS_PWR_HIBERNATE_MASK_HIBPIN_Msk)

/** The mask for the wakeup sources */
#define CY_SYSPM_PWR_WAKEUP_HIB_MASK              ((uint32_t) SRSS_PWR_HIBERNATE_MASK_HIBALARM_Msk |\
                                                              SRSS_PWR_HIBERNATE_MASK_HIBWDT_Msk |\
                                                              SRSS_PWR_HIBERNATE_POLARITY_HIBPIN_Msk |\
                                                              SRSS_PWR_HIBERNATE_MASK_HIBPIN_Msk)

/** The define to update the token to indicate the transition into Hibernate */
#define CY_SYSPM_PWR_TOKEN_HIBERNATE      ((uint32_t) 0x1BU << SRSS_PWR_HIBERNATE_TOKEN_Pos)

/** The internal define of the first wakeup pin bit used in the
* Cy_SysPm_SetHibernateWakeupSource() function
*/
#define CY_SYSPM_WAKEUP_PIN0_BIT                         (1UL)

/** The internal define of the second wakeup pin bit 
* used in the Cy_SysPm_SetHibernateWakeupSource() function
*/
#define CY_SYSPM_WAKEUP_PIN1_BIT                         (2UL)

/**
* The internal define of the first LPComparator bit 
* used in the Cy_SysPm_SetHibernateWakeupSource() function
*/
#define CY_SYSPM_WAKEUP_LPCOMP0_BIT                      (4UL)

/**
* The internal define for the second LPComparator bit 
* used in the Cy_SysPm_SetHibernateWakeupSource() function
*/
#define CY_SYSPM_WAKEUP_LPCOMP1_BIT                      (8UL)

/**
* The internal define of the first LPComparator value 
* used in the Cy_SysPm_SetHibernateWakeupSource() function
*/
#define CY_SYSPM_WAKEUP_LPCOMP0    ((uint32_t) CY_SYSPM_WAKEUP_LPCOMP0_BIT << \
                                               SRSS_PWR_HIBERNATE_MASK_HIBPIN_Pos)

/**
* The internal define of the second LPComparator value 
* used in the Cy_SysPm_SetHibernateWakeupSource() function
*/
#define CY_SYSPM_WAKEUP_LPCOMP1    ((uint32_t) CY_SYSPM_WAKEUP_LPCOMP1_BIT << \
                                               SRSS_PWR_HIBERNATE_MASK_HIBPIN_Pos)

/**
* The internal define of the first wake-up pin value 
* used in the Cy_SysPm_SetHibernateWakeupSource() function
*/
#define CY_SYSPM_WAKEUP_PIN0       ((uint32_t) CY_SYSPM_WAKEUP_PIN0_BIT << \
                                               SRSS_PWR_HIBERNATE_MASK_HIBPIN_Pos)

/**
* The internal define of the second wake-up pin value used 
* in the Cy_SysPm_SetHibernateWakeupSource() function
*/
#define CY_SYSPM_WAKEUP_PIN1       ((uint32_t) CY_SYSPM_WAKEUP_PIN1_BIT << \
                                               SRSS_PWR_HIBERNATE_MASK_HIBPIN_Pos)

/** The internal define for the first LPComparator polarity configuration */
#define CY_SYSPM_WAKEUP_LPCOMP0_POLARITY_HIGH    ((uint32_t) CY_SYSPM_WAKEUP_LPCOMP0_BIT << \
                                                             SRSS_PWR_HIBERNATE_POLARITY_HIBPIN_Pos)

/** The internal define for the second LPComparator polarity configuration */
#define CY_SYSPM_WAKEUP_LPCOMP1_POLARITY_HIGH    ((uint32_t) CY_SYSPM_WAKEUP_LPCOMP1_BIT << \
                                                             SRSS_PWR_HIBERNATE_POLARITY_HIBPIN_Pos)

/** The internal define for the first wake-up pin polarity configuration */
#define CY_SYSPM_WAKEUP_PIN0_POLARITY_HIGH       ((uint32_t) CY_SYSPM_WAKEUP_PIN0_BIT << \
                                                             SRSS_PWR_HIBERNATE_POLARITY_HIBPIN_Pos)

/** The internal define for the second wake-up pin polarity configuration */
#define CY_SYSPM_WAKEUP_PIN1_POLARITY_HIGH      ((uint32_t) CY_SYSPM_WAKEUP_PIN1_BIT << \
                                                             SRSS_PWR_HIBERNATE_POLARITY_HIBPIN_Pos)

/* Internal macro of all possible wakeup sources from hibernate power mode */
#define CY_SYSPM_HIB_WAKEUP_SOURSE_MASK        (CY_SYSPM_HIBERNATE_LPCOMP0_HIGH | CY_SYSPM_HIBERNATE_LPCOMP1_HIGH | \
                                                CY_SYSPM_HIBERNATE_RTC_ALARM     | CY_SYSPM_HIBERNATE_WDT | \
                                                CY_SYSPM_HIBERNATE_PIN0_HIGH | CY_SYSPM_HIBERNATE_PIN1_HIGH)
/** \endcond */

/**
* \defgroup group_syspm_return_status The Power Mode Status Defines
* \{
* The defines of the core(s) and device power mode status.
*/

/** The CM4 is Active */
#define CY_SYSPM_STATUS_CM4_ACTIVE       (0x01U)

/** The CM4 is in Sleep */
#define CY_SYSPM_STATUS_CM4_SLEEP        (0x02U)

/** The CM4 is in Deep Sleep */
#define CY_SYSPM_STATUS_CM4_DEEPSLEEP    (0x04U)

/** The CM4 is Low-Power mode */
#define CY_SYSPM_STATUS_CM4_LOWPOWER     (0x80U)

/** The CM0 is Active */
#define CY_SYSPM_STATUS_CM0_ACTIVE       ((uint32_t) ((uint32_t)0x01U << 8U))

/** The CM0 is in Sleep */
#define CY_SYSPM_STATUS_CM0_SLEEP        ((uint32_t) ((uint32_t)0x02U << 8U))

/** The CM0 is in Deep Sleep */
#define CY_SYSPM_STATUS_CM0_DEEPSLEEP    ((uint32_t) ((uint32_t)0x04U << 8U))

/** The CM0 is in Low-Power mode */
#define CY_SYSPM_STATUS_CM0_LOWPOWER     ((uint32_t) ((uint32_t)0x80U << 8U))

/** The device is in the Low-Power mode define */
#define CY_SYSPM_STATUS_SYSTEM_LOWPOWER    ((uint32_t) (CY_SYSPM_STATUS_CM0_LOWPOWER | (CY_SYSPM_STATUS_CM4_LOWPOWER)))

/** \} group_syspm_return_status */

/** \} group_syspm_macros */

/*******************************************************************************
*       Configuration Structures
*******************************************************************************/

/**
* \addtogroup group_syspm_data_enumerates
* \{
*/

/** The SysPm status definitions */
typedef enum
{
    CY_SYSPM_SUCCESS        = 0x00U,                                        /**< Successful */
    CY_SYSPM_BAD_PARAM      = CY_SYSPM_ID | CY_PDL_STATUS_ERROR | 0x01U,    /**< One or more invalid parameters */
    CY_SYSPM_TIMEOUT        = CY_SYSPM_ID | CY_PDL_STATUS_ERROR | 0x02U,    /**< A time-out occurs */
    CY_SYSPM_INVALID_STATE  = CY_SYSPM_ID | CY_PDL_STATUS_ERROR | 0x03U,    /**< The operation is not setup or is in an
                                                                                 improper state */
    CY_SYSPM_CANCELED       = CY_SYSPM_ID | CY_PDL_STATUS_ERROR | 0x04U,    /**< Operation canceled */
    CY_SYSPM_FAIL           = CY_SYSPM_ID | CY_PDL_STATUS_ERROR | 0xFFU     /**< An unknown failure */
} cy_en_syspm_status_t;

/**
* This enumeration is used to initialize a wait action - an interrupt or
* an event. Refer to the CMSIS for the WFE and WFI instruction explanations
*/
typedef enum
{
    CY_SYSPM_WAIT_FOR_INTERRUPT,      /**< Wait for an interrupt */
    CY_SYSPM_WAIT_FOR_EVENT           /**< Wait for an event */
} cy_en_syspm_waitfor_t;

/** This enumeration is used to configure sources for wakeup from the Hibernate 
*   power mode
*/
typedef enum
{
    /** Configure a low level for the first LPComp */
    CY_SYSPM_HIBERNATE_LPCOMP0_LOW = 
    ((uint32_t) CY_SYSPM_WAKEUP_LPCOMP0_BIT << SRSS_PWR_HIBERNATE_MASK_HIBPIN_Pos),

    /** Configure a high level for the first LPComp */
    CY_SYSPM_HIBERNATE_LPCOMP0_HIGH = 
    ((uint32_t) ((uint32_t) CY_SYSPM_WAKEUP_LPCOMP0_BIT << SRSS_PWR_HIBERNATE_POLARITY_HIBPIN_Pos) | 
                ((uint32_t) CY_SYSPM_WAKEUP_LPCOMP0_BIT << SRSS_PWR_HIBERNATE_MASK_HIBPIN_Pos)),

     /** Configure a low level for the second LPComp */
    CY_SYSPM_HIBERNATE_LPCOMP1_LOW = ((uint32_t) CY_SYSPM_WAKEUP_LPCOMP1_BIT << SRSS_PWR_HIBERNATE_MASK_HIBPIN_Pos),
    
    /** Configure a high level for the second LPComp */
    CY_SYSPM_HIBERNATE_LPCOMP1_HIGH =
    ((uint32_t) ((uint32_t) CY_SYSPM_WAKEUP_LPCOMP1_BIT << SRSS_PWR_HIBERNATE_POLARITY_HIBPIN_Pos) | 
                ((uint32_t) CY_SYSPM_WAKEUP_LPCOMP1_BIT << SRSS_PWR_HIBERNATE_MASK_HIBPIN_Pos)),

    /** Configure the RTC alarm */
    CY_SYSPM_HIBERNATE_RTC_ALARM = SRSS_PWR_HIBERNATE_MASK_HIBALARM_Msk,
    
    /** Configure the WDT interrupt */
    CY_SYSPM_HIBERNATE_WDT = SRSS_PWR_HIBERNATE_MASK_HIBWDT_Msk,

    /** Configure a low level for the first wakeup-pin */
    CY_SYSPM_HIBERNATE_PIN0_LOW = ((uint32_t) CY_SYSPM_WAKEUP_PIN0_BIT << SRSS_PWR_HIBERNATE_MASK_HIBPIN_Pos),
    
    /** Configure a high level for the first wakeup-pin */
    CY_SYSPM_HIBERNATE_PIN0_HIGH =
    ((uint32_t) ((uint32_t) CY_SYSPM_WAKEUP_PIN0_BIT << SRSS_PWR_HIBERNATE_POLARITY_HIBPIN_Pos) | 
                ((uint32_t) CY_SYSPM_WAKEUP_PIN0_BIT << SRSS_PWR_HIBERNATE_MASK_HIBPIN_Pos)),

    /** Configure a low level for the second wakeup-pin */
    CY_SYSPM_HIBERNATE_PIN1_LOW = ((uint32_t) CY_SYSPM_WAKEUP_PIN1_BIT << SRSS_PWR_HIBERNATE_MASK_HIBPIN_Pos),
    
   /** Configure a high level for the second wakeup-pin */
    CY_SYSPM_HIBERNATE_PIN1_HIGH =
    ((uint32_t) ((uint32_t) CY_SYSPM_WAKEUP_PIN1_BIT << SRSS_PWR_HIBERNATE_POLARITY_HIBPIN_Pos) | 
                ((uint32_t) CY_SYSPM_WAKEUP_PIN1_BIT << SRSS_PWR_HIBERNATE_MASK_HIBPIN_Pos)),
} cy_en_syspm_hibernate_wakeup_source_t;

/** The enumeration is used to select output voltage for the LDO */
typedef enum
{
    CY_SYSPM_LDO_VOLTAGE_0_9V,       /**< 0.9 V nominal LDO voltage */
    CY_SYSPM_LDO_VOLTAGE_1_1V        /**< 1.1 V nominal LDO voltage */
} cy_en_syspm_ldo_voltage_t;

/**
*  The enumeration is used to select the output voltage for the Buck
*  output 1, which can supply a core(s).
*/
typedef enum
{
    CY_SYSPM_BUCK_OUT1_VOLTAGE_0_9V = 0x02U,    /**< 0.9 V nominal Buck voltage */
    CY_SYSPM_BUCK_OUT1_VOLTAGE_1_1V = 0x05U     /**< 1.1 V nominal Buck voltage */
} cy_en_syspm_buck_voltage1_t;

/**
* The enumerations are used to select the Buck regulator outputs
*/
typedef enum
{
    CY_SYSPM_BUCK_VBUCK_1,    /**< Buck output 1 Voltage (Vbuck1) */
    CY_SYSPM_BUCK_VRF         /**< Buck out 2 Voltage (Vbuckrf) */
} cy_en_syspm_buck_out_t;

/**
* The enumeration is used to select the output voltage for the Buck
* output 2, which can source the BLE HW block.
*/
typedef enum
{
    CY_SYSPM_BUCK_OUT2_VOLTAGE_1_15V = 0U,    /**< 1.15 V nominal voltage */
    CY_SYSPM_BUCK_OUT2_VOLTAGE_1_2V  = 1U,    /**< 1.20 V nominal voltage */
    CY_SYSPM_BUCK_OUT2_VOLTAGE_1_25V = 2U,    /**< 1.25 V nominal voltage */
    CY_SYSPM_BUCK_OUT2_VOLTAGE_1_3V  = 3U,    /**< 1.3 V nominal voltage  */
    CY_SYSPM_BUCK_OUT2_VOLTAGE_1_35V = 4U,    /**< 1.35 V nominal voltage */
    CY_SYSPM_BUCK_OUT2_VOLTAGE_1_4V  = 5U,    /**< 1.4 V nominal voltage */
    CY_SYSPM_BUCK_OUT2_VOLTAGE_1_45V = 6U,    /**< 1.45 V nominal voltage */
    CY_SYSPM_BUCK_OUT2_VOLTAGE_1_5V  = 7U     /**< 1.5 V nominal voltage */
} cy_en_syspm_buck_voltage2_t;

/**
* This enumeration is used to set a polarity for the PMIC input. The PMIC is 
* automatically enabled when configured polarity of PMIC input and the polarity
* driven to pmic_wakeup_in pin matches.
*
* \warning 
* Do not use CY_SYSPM_PMIC_POLARITY_LOW as it is not supported by hardware.
*/
typedef enum
{
    CY_SYSPM_PMIC_POLARITY_LOW   = 0U,    /**< Set active low state for the PMIC input */
    CY_SYSPM_PMIC_POLARITY_HIGH  = 1U     /**< Set active high state for the PMIC input */
} cy_en_syspm_pmic_wakeup_polarity_t;

/**
* This enumeration sets a switch to select  Vbackup or Vddd to supply Vddbackup
*/
typedef enum
{
    CY_SYSPM_VDDBACKUP_DEFAULT         = 0U,    /**< Automatically selects Vddd or Vbackup to supply 
                                                     Vddbackup */
    CY_SYSPM_VDDBACKUP_VBACKUP         = 2U     /**< Set Vbackup to supply Vddbackup */
} cy_en_syspm_vddbackup_control_t;

/**
* This enumeration configures supercapacitor charging.
*/
typedef enum
{
    CY_SYSPM_SC_CHARGE_ENABLE     = 0x3CU,    /**< Enables supercapacitor charging */
    CY_SYSPM_SC_CHARGE_DISABLE    = 0x00U     /**< Disables supercapacitor charging */
} cy_en_syspm_sc_charge_key_t;

/**
* This enumeration is used for selecting the low power mode on which the
* appropriate registered callback handler will be executed. For example,
* the registered callback of the type CY_SYSPM_SLEEP will be executed while
* switching into the Sleep power mode.
*/
typedef enum
{
    CY_SYSPM_SLEEP,          /**< The Sleep enum callback type */
    CY_SYSPM_DEEPSLEEP,      /**< The Deep Sleep enum callback type */
    CY_SYSPM_HIBERNATE,      /**< The Hibernate enum callback type */
    CY_SYSPM_ENTER_LOWPOWER_MODE,  /**< The enter into the LPActive mode enum callback type */
    CY_SYSPM_EXIT_LOWPOWER_MODE,   /**< The exit out of the LPActive mode enum callback type */
} cy_en_syspm_callback_type_t;

/** The callback mode enumeration. This enum defines the callback mode */
typedef enum
{
    CY_SYSPM_CHECK_READY        = 0x01U,    /**< Callbacks with this mode are executed before entering into the 
                                                 low-power mode. Callback function check if the device is ready
                                                 to enter the low-power mode */
    CY_SYSPM_CHECK_FAIL         = 0x02U,    /**< Callbacks with this mode are executed after the previous callbacks
                                                 execution with CY_SYSPM_CHECK_READY return CY_SYSPM_FAIL. The callback
                                                 with the CY_SYSPM_CHECK_FAIL mode should roll back the actions done in
                                                 the callbacks executed previously with CY_SYSPM_CHECK_READY */
    CY_SYSPM_BEFORE_TRANSITION  = 0x04U,    /**< The actions to be done before entering into the low-power mode */
    CY_SYSPM_AFTER_TRANSITION   = 0x08U,    /**< The actions to be done after exiting the low-power mode */
} cy_en_syspm_callback_mode_t;

/** \} group_syspm_data_enumerates */

/**
* \addtogroup group_syspm_macros
* \{
*/
/**
* \defgroup group_syspm_skip_callback_modes The Defines to skip the callbacks modes
* \{
* The defines of the SysPm callbacks modes that can be skipped during execution.
* For more information about callbacks modes refer 
* to \ref cy_en_syspm_callback_mode_t.
*/
#define CY_SYSPM_SKIP_CHECK_READY                   (0x01U)   /**< The define to skip the check ready mode in the syspm callback */
#define CY_SYSPM_SKIP_CHECK_FAIL                    (0x02U)   /**< The define to skip the check fail mode in the syspm callback */
#define CY_SYSPM_SKIP_BEFORE_TRANSITION             (0x04U)   /**< The define to skip the before transition mode in the syspm callback */
#define CY_SYSPM_SKIP_AFTER_TRANSITION              (0x08U)   /**< The define to skip the after transition mode in the syspm callback */
/** \} group_syspm_skip_callback_modes */
/** \} group_syspm_macros */

/**
* \addtogroup group_syspm_data_structures
* \{
*/

/** The structure with the syspm callback parameters */
typedef struct
{
    cy_en_syspm_callback_mode_t mode;   /**< The callback mode. You can skip assigning as this element is for 
                                             internal usage, see \ref cy_en_syspm_callback_mode_t. This element 
                                             should not be defined as it is updated every time before the 
                                             callback function is executed */
    void *base;                         /**< The base address of a HW instance, matches name of the driver in
                                             the API for the base address. Can be not defined if not required */
    void *context;                      /**< The context for the handler function. This item can be 
                                             skipped if not required. Can be not defined if not required */

} cy_stc_syspm_callback_params_t;

/** The type for syspm callbacks */
typedef cy_en_syspm_status_t (*Cy_SysPmCallback) (cy_stc_syspm_callback_params_t *callbackParams);

/** The structure with the syspm callback configuration elements */
typedef struct cy_stc_syspm_callback
{
    Cy_SysPmCallback callback;                         /**< The callback handler function */
    cy_en_syspm_callback_type_t type;                  /**< The callback type, see \ref cy_en_syspm_callback_type_t */
    uint32_t skipMode;                                 /**< The mask of modes to be skipped during callback 
                                                             execution, see \ref group_syspm_skip_callback_modes. The
                                                             corresponding callback mode won't execute if the 
                                                             appropriate define is set. These values can be ORed.
                                                             If all modes are required to be executed this element
                                                             should be equal to zero. */

    cy_stc_syspm_callback_params_t *callbackParams;    /**< The address of a cy_stc_syspm_callback_params_t, 
                                                            the callback is executed with these parameters  */

    struct cy_stc_syspm_callback *prevItm;              /**< The previous list item. This element should not be 
                                                             defined, or defined as NULL. It is for internal 
                                                             usage to link this structure to the next registered structure.
                                                             It will be updated during callback registering. Do not 
                                                             modify this element in run-time */
                                                             
    struct cy_stc_syspm_callback *nextItm;              /**< The next list item. This element should not be 
                                                             defined, or defined as NULL. It is for internal usage to 
                                                             link this structure to the previous registered structure.
                                                             It will be updated during callback registering. Do not 
                                                             modify this element in run-time */
} cy_stc_syspm_callback_t;

/** \} group_syspm_data_structures */


/**
* \addtogroup group_syspm_functions
* \{
*/

/**
* \addtogroup group_syspm_functions_power_status
* \{
*/

__STATIC_INLINE bool Cy_SysPm_Cm4IsActive(void);
__STATIC_INLINE bool Cy_SysPm_Cm4IsSleep(void);
__STATIC_INLINE bool Cy_SysPm_Cm4IsDeepSleep(void);
__STATIC_INLINE bool Cy_SysPm_Cm4IsLowPower(void);


__STATIC_INLINE bool Cy_SysPm_Cm0IsActive(void);
__STATIC_INLINE bool Cy_SysPm_Cm0IsSleep(void);
__STATIC_INLINE bool Cy_SysPm_Cm0IsDeepSleep(void);
__STATIC_INLINE bool Cy_SysPm_Cm0IsLowPower(void);
__STATIC_INLINE bool Cy_SysPm_IsLowPower(void);
uint32_t Cy_SysPm_ReadStatus(void);
/** \} group_syspm_functions_power_status */

/**
* \addtogroup group_syspm_functions_power
* \{
*/
cy_en_syspm_status_t Cy_SysPm_Sleep(cy_en_syspm_waitfor_t waitFor);
cy_en_syspm_status_t Cy_SysPm_DeepSleep(cy_en_syspm_waitfor_t waitFor);
cy_en_syspm_status_t Cy_SysPm_Hibernate(void);
void Cy_SysPm_SetHibernateWakeupSource(uint32_t wakeupSource);
void Cy_SysPm_ClearHibernateWakeupSource(uint32_t wakeupSource);
cy_en_syspm_status_t Cy_SysPm_EnterLowPowerMode(void);
cy_en_syspm_status_t Cy_SysPm_ExitLowPowerMode(void);
void Cy_SysPm_SleepOnExit(bool enable);
/** \} group_syspm_functions_power */

/**
* \addtogroup group_syspm_functions_iofreeze
* \{
*/

/** \cond INTERNAL */
void Cy_SysPm_IoFreeze(void);
/** \endcond */

void Cy_SysPm_IoUnfreeze(void);
__STATIC_INLINE bool Cy_SysPm_IoIsFrozen(void);
/** \} group_syspm_functions_iofreeze */

/**
* \addtogroup group_syspm_functions_ldo
* \{
*/
cy_en_syspm_status_t Cy_SysPm_LdoSetVoltage(cy_en_syspm_ldo_voltage_t voltage);
__STATIC_INLINE cy_en_syspm_ldo_voltage_t Cy_SysPm_LdoGetVoltage(void);
__STATIC_INLINE bool Cy_SysPm_LdoIsEnabled(void);
/** \} group_syspm_functions_ldo */

/**
* \addtogroup group_syspm_functions_pmic
* \{
*/
__STATIC_INLINE void Cy_SysPm_PmicEnable(void);
__STATIC_INLINE void Cy_SysPm_PmicDisable(cy_en_syspm_pmic_wakeup_polarity_t polarity); 
__STATIC_INLINE void Cy_SysPm_PmicAlwaysEnable(void);
__STATIC_INLINE void Cy_SysPm_PmicEnableOutput(void);
__STATIC_INLINE void Cy_SysPm_PmicDisableOutput(void); 
__STATIC_INLINE void Cy_SysPm_PmicLock(void);
__STATIC_INLINE void Cy_SysPm_PmicUnlock(void);
__STATIC_INLINE bool Cy_SysPm_PmicIsEnabled(void);
__STATIC_INLINE bool Cy_SysPm_PmicIsOutputEnabled(void);
__STATIC_INLINE bool Cy_SysPm_PmicIsLocked(void);
/** \} group_syspm_functions_pmic */

/**
* \addtogroup group_syspm_functions_backup
* \{
*/
__STATIC_INLINE void Cy_SysPm_BackupSetSupply(cy_en_syspm_vddbackup_control_t vddBackControl);
__STATIC_INLINE cy_en_syspm_vddbackup_control_t Cy_SysPm_BackupGetSupply(void);
__STATIC_INLINE void Cy_SysPm_BackupEnableVoltageMeasurement(void);
__STATIC_INLINE void Cy_SysPm_BackupDisableVoltageMeasurement(void);
__STATIC_INLINE void Cy_SysPm_BackupSuperCapCharge(cy_en_syspm_sc_charge_key_t key);
/** \} group_syspm_functions_backup */

/**
* \addtogroup group_syspm_functions_buck
* \{
*/

__STATIC_INLINE bool Cy_SysPm_BuckIsEnabled(void);
__STATIC_INLINE cy_en_syspm_buck_voltage1_t Cy_SysPm_BuckGetVoltage1(void);

cy_en_syspm_status_t Cy_SysPm_BuckEnable(cy_en_syspm_buck_voltage1_t voltage);
cy_en_syspm_status_t Cy_SysPm_BuckSetVoltage1(cy_en_syspm_buck_voltage1_t voltage);
bool Cy_SysPm_BuckIsOutputEnabled(cy_en_syspm_buck_out_t output);

__STATIC_INLINE cy_en_syspm_buck_voltage2_t Cy_SysPm_BuckGetVoltage2(void);
__STATIC_INLINE void Cy_SysPm_BuckDisableVoltage2(void);
__STATIC_INLINE void Cy_SysPm_BuckSetVoltage2HwControl(bool hwControl);
__STATIC_INLINE bool Cy_SysPm_BuckIsVoltage2HwControlled(void);
void Cy_SysPm_BuckEnableVoltage2(void);
void Cy_SysPm_BuckSetVoltage2(cy_en_syspm_buck_voltage2_t voltage, bool waitToSettle);

/** \} group_syspm_functions_buck */

/**
* \addtogroup group_syspm_functions_callback
* \{
*/
bool Cy_SysPm_RegisterCallback(cy_stc_syspm_callback_t *handler);
bool Cy_SysPm_UnregisterCallback(cy_stc_syspm_callback_t const *handler);
cy_en_syspm_status_t Cy_SysPm_ExecuteCallback(cy_en_syspm_callback_type_t type, cy_en_syspm_callback_mode_t mode);
/** \} group_syspm_functions_callback */

/**
* \addtogroup group_syspm_functions_power_status
* \{
*/

/*******************************************************************************
* Function Name: Cy_SysPm_Cm4IsActive
****************************************************************************//**
*
* Checks whether CM4 is in Active mode.
*
* \return
* true - if CM4 is in Active mode; false - if the CM4 is not in Active mode.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_Cm4IsActive
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysPm_Cm4IsActive(void)
{
    return((Cy_SysPm_ReadStatus() & CY_SYSPM_STATUS_CM4_ACTIVE) != 0U);
}


/*******************************************************************************
* Function Name: Cy_SysPm_Cm4IsSleep
****************************************************************************//**
*
* Checks whether the CM4 is in Sleep mode.
*
* \return
* true - if the CM4 is in Sleep mode; 
* false - if the CM4 is not in Sleep mode.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_Cm4IsSleep
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysPm_Cm4IsSleep(void)
{
    return((Cy_SysPm_ReadStatus() & CY_SYSPM_STATUS_CM4_SLEEP) != 0U);
}


/*******************************************************************************
* Function Name: Cy_SysPm_Cm4IsDeepSleep
****************************************************************************//**
*
* Checks whether the CM4 is in the Deep Sleep mode.
*
* \return
* true - if CM4 is in Deep Sleep mode; false - if the CM4 is not in 
* Deep Sleep mode.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_Cm4IsDeepSleep
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysPm_Cm4IsDeepSleep(void)
{
    return((Cy_SysPm_ReadStatus() & CY_SYSPM_STATUS_CM4_DEEPSLEEP) != 0U);
}


/*******************************************************************************
* Function Name: Cy_SysPm_Cm4IsLowPower
****************************************************************************//**
*
* Checks whether the CM4 is in Low-Power mode. Note that Low-Power mode is a 
* status of the device.
*
* \return
* true - if the CM4 is in Low-Power mode; 
* false - if the CM4 is not in Low-Power mode.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_Cm4IsLowPower
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysPm_Cm4IsLowPower(void)
{
    return((Cy_SysPm_ReadStatus() & CY_SYSPM_STATUS_CM4_LOWPOWER) != 0U);
}
/** \} group_syspm_functions_power_status */


/**
* \addtogroup group_syspm_functions_power_status
* \{
*/

/*******************************************************************************
* Function Name: Cy_SysPm_Cm0IsActive
****************************************************************************//**
*
* Checks whether the CM0+ is in Active mode.
*
* \return
* true - if the CM0+ is in Active mode; 
* false - if the CM0+ is not in Active mode.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_Cm0IsActive
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysPm_Cm0IsActive(void)
{
    return((Cy_SysPm_ReadStatus() & CY_SYSPM_STATUS_CM0_ACTIVE) != 0U);
}


/*******************************************************************************
* Function Name: Cy_SysPm_Cm0IsSleep
****************************************************************************//**
*
* Checks whether the CM0+ is in Sleep mode.
*
* \return
* true - if the CM0+ is in Sleep mode; 
* false - if the CM0+ is not in Sleep mode.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_Cm0IsSleep
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysPm_Cm0IsSleep(void)
{
    return((Cy_SysPm_ReadStatus() & CY_SYSPM_STATUS_CM0_SLEEP) != 0U);
}


/*******************************************************************************
* Function Name: Cy_SysPm_Cm0IsDeepSleep
****************************************************************************//**
*
* Checks whether the CM0+ is in Deep Sleep mode.
*
* \return
* true - if the CM0+ is in Deep Sleep mode; 
* false - if the CM0+ is not in Deep Sleep mode.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_Cm0IsDeepSleep
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysPm_Cm0IsDeepSleep(void)
{
    return((Cy_SysPm_ReadStatus() & CY_SYSPM_STATUS_CM0_DEEPSLEEP) != 0U);
}


/*******************************************************************************
* Function Name: Cy_SysPm_Cm0IsLowPower
****************************************************************************//**
*
* Checks whether the CM0+ is in Low-Power mode.  Note that Low-Power mode is a
* status of the device.
*
* \return
* true - if the CM0+ is in Low-Power mode; 
* false - if the CM0+ is not in Low-Power mode.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_Cm0IsLowPower
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysPm_Cm0IsLowPower(void)
{
    return((Cy_SysPm_ReadStatus() & CY_SYSPM_STATUS_CM0_LOWPOWER) != 0U);
}


/*******************************************************************************
* Function Name: Cy_SysPm_IsLowPower
****************************************************************************//**
*
* Checks whether the device is in Low-Power mode.
*
* \return
* true - the system is in Low-Power mode;
* false - the system is is not in Low-Power mode.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_IsLowPower
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysPm_IsLowPower(void)
{
    return((Cy_SysPm_ReadStatus() & CY_SYSPM_STATUS_SYSTEM_LOWPOWER) != 0U);
}

/** \} group_syspm_functions_power_status */


/**
* \addtogroup group_syspm_functions_buck
* \{
*/

/*******************************************************************************
* Function Name: Cy_SysPm_BuckIsEnabled
****************************************************************************//**
*
* Get the current status of the Buck regulator.
*
* \return
* The current state of the Buck regulator. True if the Buck regulator
* is enabled; false if it is disabled.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_VoltageRegulator
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysPm_BuckIsEnabled(void)
{
    return((0u !=_FLD2VAL(SRSS_PWR_BUCK_CTL_BUCK_EN, SRSS_PWR_BUCK_CTL)) ? true : false);
}

/*******************************************************************************
* Function Name: Cy_SysPm_BuckGetVoltage1
****************************************************************************//**
*
* Gets the current nominal output 1 voltage (Vccbuck1) of 
* the Buck regulator.
*
* \note The actual device output 1 voltage (Vccbuck1) can be different from
* the nominal voltage because the actual voltage value depends on the 
* conditions including the load current.
*
* \return
* The nominal output voltage 1 (Vccbuck1) of the Buck regulator.
* See \ref cy_en_syspm_buck_voltage1_t.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_VoltageRegulator
*
*******************************************************************************/
__STATIC_INLINE cy_en_syspm_buck_voltage1_t Cy_SysPm_BuckGetVoltage1(void)
{
    uint32_t retVal;
    retVal = _FLD2VAL(SRSS_PWR_BUCK_CTL_BUCK_OUT1_SEL, SRSS_PWR_BUCK_CTL);

    return((cy_en_syspm_buck_voltage1_t) retVal);
}


/*******************************************************************************
* Function Name: Cy_SysPm_BuckGetVoltage2
****************************************************************************//**
*
* Gets the current output 2 nominal voltage (Vbuckrf) of the SIMO 
* Buck regulator.
*
* \note The actual device output 2 voltage (Vbuckrf) can be different from the 
* nominal voltage because the actual voltage value depends on the conditions 
* including the load current.
*
* \return
* The nominal output voltage of the Buck SIMO regulator output 2 
* voltage (Vbuckrf).
* See \ref cy_en_syspm_buck_voltage2_t.
*
* The function is applicable for devices with the SIMO Buck regulator.
* Refer to device datasheet about information if device contains 
* SIMO Buck.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_BuckGetVoltage2
*
*******************************************************************************/
__STATIC_INLINE cy_en_syspm_buck_voltage2_t Cy_SysPm_BuckGetVoltage2(void)
{
    uint32_t retVal;
    retVal = _FLD2VAL(SRSS_PWR_BUCK_CTL2_BUCK_OUT2_SEL, SRSS_PWR_BUCK_CTL2);

    return((cy_en_syspm_buck_voltage2_t) retVal);
}


/*******************************************************************************
* Function Name: Cy_SysPm_BuckDisableVoltage2
****************************************************************************//**
*
* Disables the output 2 voltage (Vbuckrf) of the SIMO Buck regulator. The 
* output 2 voltage (Vbuckrf) of the Buck regulator is used to supply 
* the BLE HW block.
*
* \note The function does not have effect if the Buck regulator is
* switched off.
*
* \note Ensures that the new voltage supply for the BLE HW block is settled 
* and is stable before calling the Cy_SysPm_BuckDisableVoltage2() function.
*
* The function is applicable for devices with the SIMO Buck regulator.
* Refer to device datasheet about information if device contains 
* SIMO Buck.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_BuckDisableVoltage2
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysPm_BuckDisableVoltage2(void)
{
    /* Disable the Vbuck2 output */
    SRSS_PWR_BUCK_CTL2 &= ((uint32_t) ~ (_VAL2FLD(SRSS_PWR_BUCK_CTL2_BUCK_OUT2_EN, 1U)));
}


/*******************************************************************************
* Function Name: Cy_SysPm_BuckSetVoltage2HwControl
****************************************************************************//**
*
* Sets the hardware control for the SIMO Buck output 2 (Vbuckrf).
*
* The hardware control for the Vbuckrf output. When this bit is set, the 
* value in BUCK_OUT2_EN is ignored and the hardware signal is used instead. If the 
* product has supporting hardware, it can directly control the enable signal
* for Vbuckrf.
*
* \param hwControl
* Enables/disables hardware control for the SIMO Buck output 2.
*
* Function does not have an effect if SIMO Buck regulator is disabled.
*
* The function is applicable for devices with the SIMO Buck regulator.
* Refer to device datasheet about information if device contains 
* SIMO Buck.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_BuckSetVoltage2HwControl
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysPm_BuckSetVoltage2HwControl(bool hwControl)
{
    if(Cy_SysPm_BuckIsEnabled())
    {
        if(hwControl)
        {
            SRSS_PWR_BUCK_CTL2 |= _VAL2FLD(SRSS_PWR_BUCK_CTL2_BUCK_OUT2_HW_SEL, 1U);
        }
        else
        {
            SRSS_PWR_BUCK_CTL2 &= ((uint32_t)~(_VAL2FLD(SRSS_PWR_BUCK_CTL2_BUCK_OUT2_HW_SEL, 1U)));
        }
    }
}


/*******************************************************************************
* Function Name: Cy_SysPm_BuckIsVoltage2HwControlled
****************************************************************************//**
*
* Gets the hardware control for Vbuckrf.
*
* The hardware control for the Vbuckrf output. When this bit is set, the 
* value in BUCK_OUT2_EN is ignored and the hardware signal is used instead. 
* If the product has supporting hardware, it can directly control the enable 
* signal for Vbuckrf.
*
* \return
* True if the HW control is set; false if the FW control is set for the 
* Buck output 2.
*
* The function is applicable for devices with the SIMO Buck regulator.
* Refer to device datasheet about information if device contains 
* SIMO Buck.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_BuckIsVoltage2HwControlled
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysPm_BuckIsVoltage2HwControlled(void)
{
    return((0U != _FLD2VAL(SRSS_PWR_BUCK_CTL2_BUCK_OUT2_HW_SEL, SRSS_PWR_BUCK_CTL2)) ? true : false);
}

/** \} group_syspm_functions_buck */

/**
* \addtogroup group_syspm_functions_ldo
* \{
*/

/*******************************************************************************
* Function Name: Cy_SysPm_LdoGetVoltage
****************************************************************************//**
*
* Gets the current output voltage value of the LDO.
*
* \note The actual device Vccd voltage can be different from the 
* nominal voltage because the actual voltage value depends on the conditions 
* including the load current.
*
* \return
* The nominal output voltage of the LDO.
* See \ref cy_en_syspm_ldo_voltage_t.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_VoltageRegulator
*
*******************************************************************************/
__STATIC_INLINE cy_en_syspm_ldo_voltage_t Cy_SysPm_LdoGetVoltage(void)
{
    uint32_t curVoltage;

    curVoltage = _FLD2VAL(SRSS_PWR_TRIM_PWRSYS_CTL_ACT_REG_TRIM, SRSS_PWR_TRIM_PWRSYS_CTL);

    return((curVoltage == ( SFLASH_LDO_0P9V_TRIM)) ? CY_SYSPM_LDO_VOLTAGE_0_9V : CY_SYSPM_LDO_VOLTAGE_1_1V);
}


/*******************************************************************************
* Function Name: Cy_SysPm_LdoIsEnabled
****************************************************************************//**
*
* Reads the current status of the LDO.
*
* \return
* True means the LDO is enabled. False means it is disabled.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_VoltageRegulator
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysPm_LdoIsEnabled(void)
{
    return((0U != _FLD2VAL(SRSS_PWR_CTL_LINREG_DIS, SRSS_PWR_CTL)) ? false : true);
}

/** \} group_syspm_functions_ldo */


/**
* \addtogroup group_syspm_functions_iofreeze
* \{
*/

/*******************************************************************************
* Function Name: Cy_SysPm_IoIsFrozen
****************************************************************************//**
*
* Checks whether IOs are frozen.
*
* \return Returns True if IOs are frozen. <br> False if IOs are unfrozen.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_IoUnfreeze
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysPm_IoIsFrozen(void)
{
    return(0U != _FLD2VAL(SRSS_PWR_HIBERNATE_FREEZE, SRSS_PWR_HIBERNATE));
}

/** \} group_syspm_functions_iofreeze */


/**
* \addtogroup group_syspm_functions_pmic
* \{
*/

/*******************************************************************************
* Function Name: Cy_SysPm_PmicEnable
****************************************************************************//**
*
* Enable the external PMIC that supplies Vddd (if present).
*
* For information about the PMIC input and output pins and their assignment in 
* the specific families devices, refer to the appropriate device TRM.
* 
* The function is not effective when the PMIC is locked. Call 
* Cy_SysPm_PmicUnlock() before enabling the PMIC.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_PmicEnable
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysPm_PmicEnable(void)
{
    if(CY_SYSPM_PMIC_UNLOCK_KEY == _FLD2VAL(BACKUP_PMIC_CTL_UNLOCK, BACKUP_PMIC_CTL))
    {
        BACKUP_PMIC_CTL =
        _VAL2FLD(BACKUP_PMIC_CTL_UNLOCK, CY_SYSPM_PMIC_UNLOCK_KEY) |
        _VAL2FLD(BACKUP_PMIC_CTL_PMIC_EN_OUTEN, 1U) |
        _VAL2FLD(BACKUP_PMIC_CTL_PMIC_EN, 1U);
    }
}


/*******************************************************************************
* Function Name: Cy_SysPm_PmicDisable
****************************************************************************//**
*
* Disables the PMIC. This function does not affect the output pin. Configures 
* the PMIC input pin polarity. The PMIC input pin has programmable polarity to 
* enable the PMIC using different input polarities. The PMIC is 
* automatically enabled when input polarity and configured polarity matches.
* The function is not effective when the active level of PMIC input pin 
* is equal to configured PMIC polarity.
*
* The function is not effective when the PMIC is locked. Call 
* Cy_SysPm_PmicUnlock() before enabling the PMIC.
*
* \param polarity
* Configures the PMIC wakeup input pin to be active low or active 
* high. The PMIC will be automatically enabled when set polarity and the active 
* level of PMIC input pin matches. See \ref cy_en_syspm_pmic_wakeup_polarity_t.
*
* The PMIC will be enabled automatically by any of RTC alarm or PMIC wakeup 
* event regardless of the PMIC lock state.
*
* \note
* Before disabling the PMIC, ensure that PMIC input and PMIC output pins are 
* configured in the correct way not to obtain unexpected PMIC enabling.
*
* \warning
* The PMIC is enabled automatically when you call Cy_SysPm_PmicLock(). 
* To keep the PMIC disabled, the PMIC must remain unlocked.
*
* \warning 
* Do not call Cy_SysPm_PmicDisable(CY_SYSPM_PMIC_POLARITY_LOW) because this 
* is not supported by hardware.
*
* For information about the PMIC input and output pins and their assignment in 
* the specific families devices, refer to the appropriate device TRM.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_PmicDisable
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysPm_PmicDisable(cy_en_syspm_pmic_wakeup_polarity_t polarity)
{
    CY_ASSERT_L3(CY_SYSPM_IS_POLARITY_VALID(polarity));
    
    if(CY_SYSPM_PMIC_UNLOCK_KEY == _FLD2VAL(BACKUP_PMIC_CTL_UNLOCK, BACKUP_PMIC_CTL))
    {
        BACKUP_PMIC_CTL = 
        (_VAL2FLD(BACKUP_PMIC_CTL_UNLOCK, CY_SYSPM_PMIC_UNLOCK_KEY) | 
         _CLR_SET_FLD32U(BACKUP_PMIC_CTL, BACKUP_PMIC_CTL_POLARITY, (uint32_t) polarity)) &
        ((uint32_t) ~ _VAL2FLD(BACKUP_PMIC_CTL_PMIC_EN, 1U));
    }
}


/*******************************************************************************
* Function Name: Cy_SysPm_PmicAlwaysEnable
****************************************************************************//**
*
* Enables the signal through the PMIC output pin. This is a Write once API, 
* ensure that the PMIC cannot be disabled or polarity changed until a next 
* device reset.
* 
* For information about the PMIC input and output pins and their assignment in 
* the specific families devices, refer to the appropriate device TRM.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_PmicAlwaysEnable
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysPm_PmicAlwaysEnable(void)
{
    BACKUP_PMIC_CTL |= _VAL2FLD(BACKUP_PMIC_CTL_PMIC_ALWAYSEN, 1U);
}


/*******************************************************************************
* Function Name: Cy_SysPm_PmicEnableOutput
****************************************************************************//**
*
* Enables the PMIC output. 
*
* The function is not effective when the PMIC is locked. Call 
* Cy_SysPm_PmicUnlock() before enabling the PMIC.
*
* For information about the PMIC output pin and it assignment in 
* the specific families devices, refer to the appropriate device TRM.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_PmicEnableOutput
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysPm_PmicEnableOutput(void)
{
    if(CY_SYSPM_PMIC_UNLOCK_KEY == _FLD2VAL(BACKUP_PMIC_CTL_UNLOCK, BACKUP_PMIC_CTL))
    {
        BACKUP_PMIC_CTL |=
        _VAL2FLD(BACKUP_PMIC_CTL_UNLOCK, CY_SYSPM_PMIC_UNLOCK_KEY) | _VAL2FLD(BACKUP_PMIC_CTL_PMIC_EN_OUTEN, 1U);
    }
}


/*******************************************************************************
* Function Name: Cy_SysPm_PmicDisableOutput
****************************************************************************//**
*
* Disables the PMIC output. 
*
* When PMIC output pin is disabled and is unlocked the PMIC output pin can be 
* used for the another purpose.
*
* The function is not effective when the PMIC is locked. Call 
* Cy_SysPm_PmicUnlock() before enabling the PMIC.
*
* For information about the PMIC output pin and it assignment in 
* the specific families devices, refer to the appropriate device TRM.
*
* \note
* After the PMIC output is disabled, the PMIC output pin returns to its 
* setup configuration. 
*
* \warning
* The PMIC output is enabled automatically when you call Cy_SysPm_PmicLock(). 
* To keep PMIC output disabled, the PMIC must remain unlocked.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_PmicDisableOutput
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysPm_PmicDisableOutput(void)
{
    if(CY_SYSPM_PMIC_UNLOCK_KEY == _FLD2VAL(BACKUP_PMIC_CTL_UNLOCK, BACKUP_PMIC_CTL))
    {
        BACKUP_PMIC_CTL = 
        (BACKUP_PMIC_CTL | _VAL2FLD(BACKUP_PMIC_CTL_UNLOCK, CY_SYSPM_PMIC_UNLOCK_KEY)) &
        ((uint32_t) ~ _VAL2FLD(BACKUP_PMIC_CTL_PMIC_EN_OUTEN, 1U));
    } 
}


/*******************************************************************************
* Function Name: Cy_SysPm_PmicLock
****************************************************************************//**
*
* Locks the PMIC control register so that no changes can be made. The changes 
* are related to the PMIC enabling/disabling and PMIC output signal 
* enabling/disabling.
*
* \warning
* The PMIC and/or the PMIC output are enabled automatically when 
* you call Cy_SysPm_PmicLock(). To keep the PMIC or PMIC output disabled,
* the PMIC must remain unlocked.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_PmicLock
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysPm_PmicLock(void)
{
    BACKUP_PMIC_CTL = _CLR_SET_FLD32U(BACKUP_PMIC_CTL, BACKUP_PMIC_CTL_UNLOCK, 0U);
}


/*******************************************************************************
* Function Name: Cy_SysPm_PmicUnlock
****************************************************************************//**
*
* Unlocks the PMIC control register so that changes can be made. The changes are
* related to the PMIC enabling/disabling and PMIC output signal 
* enabling/disabling.
*
* \warning
* The PMIC and/or the PMIC output are enabled automatically when 
* you call Cy_SysPm_PmicLock(). To keep the PMIC or PMIC output disabled,
* the PMIC must remain unlocked.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_PmicEnable
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysPm_PmicUnlock(void)
{
    BACKUP_PMIC_CTL = _CLR_SET_FLD32U(BACKUP_PMIC_CTL, BACKUP_PMIC_CTL_UNLOCK, CY_SYSPM_PMIC_UNLOCK_KEY);
}


/*******************************************************************************
* Function Name: Cy_SysPm_PmicIsEnabled
****************************************************************************//**
* 
* The function returns the status of the PMIC.
*
* \return
* True - The PMIC is enabled. <br>
* False - The PMIC is disabled. <br>
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_PmicLock
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysPm_PmicIsEnabled(void)
{
    return(0U != _FLD2VAL(BACKUP_PMIC_CTL_PMIC_EN, BACKUP_PMIC_CTL));
}


/*******************************************************************************
* Function Name: Cy_SysPm_PmicIsOutputEnabled
****************************************************************************//**
* 
* The function returns the status of the PMIC output.
*
* \return
* True - The PMIC output is enabled. <br>
* False - The PMIC output is disabled. <br>
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_PmicDisable
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysPm_PmicIsOutputEnabled(void)
{
    return(0U != _FLD2VAL(BACKUP_PMIC_CTL_PMIC_EN_OUTEN, BACKUP_PMIC_CTL));
}


/*******************************************************************************
* Function Name: Cy_SysPm_PmicIsLocked
****************************************************************************//**
*
* Returns the PMIC lock status
*
* \return
* True - The PMIC is locked. <br>
* False - The PMIC is unlocked. <br>
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_PmicLock
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SysPm_PmicIsLocked(void)
{
    return((_FLD2VAL(BACKUP_PMIC_CTL_UNLOCK, BACKUP_PMIC_CTL) == CY_SYSPM_PMIC_UNLOCK_KEY) ? false : true);
}

/** \} group_syspm_functions_pmic */


/**
* \addtogroup group_syspm_functions_backup
* \{
*/

/*******************************************************************************
* Function Name: Cy_SysPm_BackupSetSupply
****************************************************************************//**
*
* Sets the Backup Supply (Vddback) operation mode.
*
* \param
* vddBackControl 
* Selects Backup Supply (Vddback) operation mode.
* See \ref cy_en_syspm_vddbackup_control_t.
*
* Refer to device TRM for more details about Backup supply modes.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_BackupSetSupply
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysPm_BackupSetSupply(cy_en_syspm_vddbackup_control_t vddBackControl)
{
    CY_ASSERT_L3(CY_SYSPM_IS_VDDBACKUP_VALID(vddBackControl));

    BACKUP_CTL = _CLR_SET_FLD32U((BACKUP_CTL), BACKUP_CTL_VDDBAK_CTL, (uint32_t) vddBackControl);
}


/*******************************************************************************
* Function Name: Cy_SysPm_BackupGetSupply
****************************************************************************//**
*
* Returns the current Backup Supply (Vddback) operation mode.
*
* \return
* The current Backup Supply (Vddback) operation mode, 
* see \ref cy_en_syspm_status_t.
*
* Refer to device TRM for more details about Backup supply modes.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_BackupGetSupply
*
*******************************************************************************/
__STATIC_INLINE cy_en_syspm_vddbackup_control_t Cy_SysPm_BackupGetSupply(void)
{
    uint32_t retVal;
    retVal = _FLD2VAL(BACKUP_CTL_VDDBAK_CTL, BACKUP_CTL);

    return((cy_en_syspm_vddbackup_control_t) retVal);
}


/*******************************************************************************
* Function Name: Cy_SysPm_BackupEnableVoltageMeasurement
****************************************************************************//**
*
* This function enables the Vbackup supply measurement by the ADC. The function 
* connects the Vbackup supply to the AMUXBUSA. Note that measured signal is 
* scaled by 40% to allow being measured by the ADC.
*
* Refer to device TRM for more details about Vbackup supply measurement.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_BackupEnableVoltageMeasurement
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysPm_BackupEnableVoltageMeasurement(void)
{
    BACKUP_CTL |= BACKUP_CTL_VBACKUP_MEAS_Msk;
}


/*******************************************************************************
* Function Name: Cy_SysPm_BackupDisableVoltageMeasurement
****************************************************************************//**
*
* The function disables the Vbackup supply measurement by the ADC. The function 
* disconnects the Vbackup supply from the AMUXBUSA.
*
* Refer to device TRM for more details about Vbackup supply measurement.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_BackupDisableVoltageMeasurement
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysPm_BackupDisableVoltageMeasurement(void)
{
    BACKUP_CTL &= ((uint32_t) ~BACKUP_CTL_VBACKUP_MEAS_Msk);
}


/*******************************************************************************
* Function Name: Cy_SysPm_BackupSuperCapCharge
****************************************************************************//**
*
* Configures the supercapacitor charger circuit.
*
* \param key 
* Passes the key to enable or disable the supercapacitor charger circuit.
* See \ref cy_en_syspm_sc_charge_key_t.
*
* \warning
* This function is used only for charging the supercapacitor.
* Do not use this function to charge a battery. Refer to device TRM for more 
* details.
*
* \funcusage
* \snippet syspm/syspm_2_20_sut_01.cydsn/main_cm4.c snippet_Cy_SysPm_BackupSuperCapCharge
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysPm_BackupSuperCapCharge(cy_en_syspm_sc_charge_key_t key)
{
    CY_ASSERT_L3(CY_SYSPM_IS_SC_CHARGE_KEY_VALID(key));
    
    if(key == CY_SYSPM_SC_CHARGE_ENABLE)
    {
        BACKUP_CTL = _CLR_SET_FLD32U((BACKUP_CTL), BACKUP_CTL_EN_CHARGE_KEY, (uint32_t) CY_SYSPM_SC_CHARGE_ENABLE);
    }
    else
    {
        BACKUP_CTL &= ((uint32_t) ~BACKUP_CTL_EN_CHARGE_KEY_Msk);
    }
}

/** \} group_syspm_functions_backup */
/** \} group_syspm_functions*/

/** \cond INTERNAL */

/*******************************************************************************
* Backward compatibility macro. The following code is DEPRECATED and must 
* not be used in new projects
*******************************************************************************/

/* BWC defines for Buck related functions */
typedef cy_en_syspm_buck_voltage1_t          cy_en_syspm_simo_buck_voltage1_t;
typedef cy_en_syspm_buck_voltage2_t          cy_en_syspm_simo_buck_voltage2_t;

#define Cy_SysPm_SimoBuckGetVoltage2         Cy_SysPm_BuckGetVoltage2
#define Cy_SysPm_DisableVoltage2             Cy_SysPm_BuckDisableVoltage2
#define Cy_SysPm_EnableVoltage2              Cy_SysPm_BuckEnableVoltage2
#define Cy_SysPm_SimoBuckSetHwControl        Cy_SysPm_BuckSetVoltage2HwControl
#define Cy_SysPm_SimoBuckGetHwControl        Cy_SysPm_BuckIsVoltage2HwControlled
#define Cy_SysPm_SimoBuckSetVoltage2         Cy_SysPm_BuckSetVoltage2

#define CY_SYSPM_SIMO_BUCK_OUT2_VOLTAGE_1_15V   CY_SYSPM_BUCK_OUT2_VOLTAGE_1_15V
#define CY_SYSPM_SIMO_BUCK_OUT2_VOLTAGE_1_2V    CY_SYSPM_BUCK_OUT2_VOLTAGE_1_2V
#define CY_SYSPM_SIMO_BUCK_OUT2_VOLTAGE_1_25V   CY_SYSPM_BUCK_OUT2_VOLTAGE_1_25V
#define CY_SYSPM_SIMO_BUCK_OUT2_VOLTAGE_1_3V    CY_SYSPM_BUCK_OUT2_VOLTAGE_1_3V 
#define CY_SYSPM_SIMO_BUCK_OUT2_VOLTAGE_1_35V   CY_SYSPM_BUCK_OUT2_VOLTAGE_1_35V
#define CY_SYSPM_SIMO_BUCK_OUT2_VOLTAGE_1_4V    CY_SYSPM_BUCK_OUT2_VOLTAGE_1_4V 
#define CY_SYSPM_SIMO_BUCK_OUT2_VOLTAGE_1_45V   CY_SYSPM_BUCK_OUT2_VOLTAGE_1_45V
#define CY_SYSPM_SIMO_BUCK_OUT2_VOLTAGE_1_5V    CY_SYSPM_BUCK_OUT2_VOLTAGE_1_5V

#define CY_SYSPM_SIMO_BUCK_OUT1_VOLTAGE_0_9V     CY_SYSPM_BUCK_OUT1_VOLTAGE_0_9V
#define CY_SYSPM_SIMO_BUCK_OUT1_VOLTAGE_1_1V     CY_SYSPM_BUCK_OUT1_VOLTAGE_1_1V

#define Cy_SysPm_SwitchToSimoBuck()          (Cy_SysPm_BuckEnable(CY_SYSPM_BUCK_OUT1_VOLTAGE_0_9V))
#define Cy_SysPm_SimoBuckGetVoltage1         Cy_SysPm_BuckGetVoltage1
#define Cy_SysPm_SimoBuckIsEnabled           Cy_SysPm_BuckIsEnabled
#define Cy_SysPm_SimoBuckSetVoltage1         Cy_SysPm_BuckSetVoltage1
#define Cy_SysPm_SimoBuckOutputIsEnabled     Cy_SysPm_BuckIsOutputEnabled

#define CY_SYSPM_LPCOMP0_LOW                    CY_SYSPM_HIBERNATE_LPCOMP0_LOW
#define CY_SYSPM_LPCOMP0_HIGH                   CY_SYSPM_HIBERNATE_LPCOMP0_HIGH
#define CY_SYSPM_LPCOMP1_LOW                    CY_SYSPM_HIBERNATE_LPCOMP1_LOW
#define CY_SYSPM_LPCOMP1_HIGH                   CY_SYSPM_HIBERNATE_LPCOMP1_HIGH
#define CY_SYSPM_HIBALARM                       CY_SYSPM_HIBERNATE_RTC_ALARM
#define CY_SYSPM_HIBWDT                         CY_SYSPM_HIBERNATE_WDT
#define CY_SYSPM_HIBPIN0_LOW                    CY_SYSPM_HIBERNATE_PIN0_LOW
#define CY_SYSPM_HIBPIN0_HIGH                   CY_SYSPM_HIBERNATE_PIN0_HIGH
#define CY_SYSPM_HIBPIN1_LOW                    CY_SYSPM_HIBERNATE_PIN1_LOW
#define CY_SYSPM_HIBPIN1_HIGH                   CY_SYSPM_HIBERNATE_PIN1_HIGH

#define CY_SYSPM_ENTER_LP_MODE                  CY_SYSPM_ENTER_LOWPOWER_MODE
#define CY_SYSPM_EXIT_LP_MODE                   CY_SYSPM_EXIT_LOWPOWER_MODE

/* BWC defines for functions related to low power transition*/
#define Cy_SysPm_EnterLpMode                 Cy_SysPm_EnterLowPowerMode
#define Cy_SysPm_ExitLpMode                  Cy_SysPm_ExitLowPowerMode

typedef cy_en_syspm_hibernate_wakeup_source_t  cy_en_syspm_hib_wakeup_source_t;

/* BWC defines related to hibernation functions */
#define Cy_SysPm_SetHibWakeupSource          Cy_SysPm_SetHibernateWakeupSource
#define Cy_SysPm_ClearHibWakeupSource        Cy_SysPm_ClearHibernateWakeupSource
#define Cy_SysPm_GetIoFreezeStatus           Cy_SysPm_IoIsFrozen

/* BWC defines for Backup related functions */
#define Cy_SysPm_SetBackupSupply             Cy_SysPm_BackupSetSupply
#define Cy_SysPm_GetBackupSupply             Cy_SysPm_BackupGetSupply
#define Cy_SysPm_EnableBackupVMeasure        Cy_SysPm_BackupEnableVoltageMeasurement
#define Cy_SysPm_DisableBackupVMeasure       Cy_SysPm_BackupDisableVoltageMeasurement

/* BWC defines for PMIC related functions */
#define Cy_SysPm_EnablePmic                  Cy_SysPm_PmicEnable
#define Cy_SysPm_DisablePmic                 Cy_SysPm_PmicDisable
#define Cy_SysPm_AlwaysEnablePmic            Cy_SysPm_PmicAlwaysEnable
#define Cy_SysPm_EnablePmicOutput            Cy_SysPm_PmicEnableOutput
#define Cy_SysPm_DisablePmicOutput           Cy_SysPm_PmicDisableOutput
#define Cy_SysPm_LockPmic                    Cy_SysPm_PmicLock
#define Cy_SysPm_UnlockPmic                  Cy_SysPm_PmicUnlock
#define Cy_SysPm_IsPmicEnabled               Cy_SysPm_PmicIsEnabled
#define Cy_SysPm_IsPmicOutputEnabled         Cy_SysPm_PmicIsOutputEnabled
#define Cy_SysPm_IsPmicLocked                Cy_SysPm_PmicIsLocked

/** \endcond */

#ifdef __cplusplus
}
#endif

#endif /* CY_SYSPM_H */

/** \} group_syspm */


/* [] END OF FILE */
