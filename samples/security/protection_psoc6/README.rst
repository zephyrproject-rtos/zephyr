.. _prot-psoc6-sample:

Sample secure domain application
################################

Overview
********

The :ref:`cy8ckit_062_wifi_bt` board has two core processors (Cortex-M4 and
Cortex-M0+). This sample application creates a demo with a protected area
created on the Cortex-M0+ core (secure domain). On start CM0+ core protects its
Flash, SRAM (using SMPU structures) from CM4 access and restricts access to some
peripheral resources (using PPU structures).
When CM4 tries to access protected SRAM area the system breaks it into
a Hardware Fault exception.

There are two protection configuration in the example project:

- secure/src/cyprotection_simple_config.h
- secure/src/cyprotection_config.h

The simple configuration is enabled by default. It implements an elementary
memory and peripheral protection.
The second configuration implements more sophisticated scheme - in this case CM0
core fully controls critical peripheral and memory, and can provide secure
services for user applications on CM4. CM4 has not any access to the CM0
controlled resources.
To enable the second protection configuration the SIMPLE_RESOURCES_PROTECTION
define should be commented in the ''secure/src/main.c'' file.

Requirements
************

- :ref:`cy8ckit_062_wifi_bt` board

Building and Running
********************

#. Build the project

   .. code-block:: console

      $ cd samples/security/protection_psoc6
      $ mkdir build
      $ cd build
      $ cmake -G"Eclipse CDT4 - Ninja" ..
      $ ninja

#. Switch the DevKit into CMSIS-DAP mode using SW3 (LED2 should blink) and
   flash the board:

   .. code-block:: console

      $<openocd_path>\bin\openocd -c "source [find interface/cmsis-dap.cfg]" \
         -c "transport select swd" -c "source [find target/psoc6.cfg]" \
         -c "if [catch {program {<zephyr_path>\samples\security\protection_psoc6\build\zephyr\zephyr.elf}} ] \
            { echo {** Program operation failed **} } \
            else { echo {** Program operation completed successfully **} }" \
         -c "reset_config srst_only;reset run;psoc6.dap dpreg 0x04 0x00;shutdown"

      $<openocd_path>/bin/openocd -c "source [find interface/cmsis-dap.cfg]" \
         -c "transport select swd" -c "source [find target/psoc6.cfg]" \
         -c "if [catch {program {<zephyr_path>\samples\security\protection_psoc6\build\prot_psoc6_unsecure-prefix\src\prot_psoc6_unsecure-build\zephyr\zephyr.elf}} ] \
            { echo {** Program operation failed **} } \
            else { echo {** Program operation completed successfully **} }" \
         -c "reset_config srst_only;reset run;psoc6.dap dpreg 0x04 0x00;shutdown"


#. Switch the DevKit back using SW3. Open two serial terminals (minicom, putty,
   etc.) and connect to the board with the following settings:

   - Speed: 115200
   - Data: 8 bits
   - Parity: None
   - Stop bits: 1

#. Reset the board and the following message will appear on the corresponding
   serial port:

   - Secure core UART

   .. code-block:: console

      ***** Booting Zephyr OS zephyr-v1.13.0-2203-gbf11698ed9 *****
      PSoC6 Secure core has started

      PSoC6 CM0+ resources protected

      Check access rights before executing CM4 requests

      CM4 privileged unsecured access to SCB2 is forbidden
      CM0+ unprivileged secured access to SCB2 is forbidden
      CM0+ privileged secured access to SCB2 is allowed

      CM4 unprivileged unsecured access to CM4 memory is allowed
      CM0 privileged secured access to CM4 memory is allowed

      CM4 unprivileged unsecured access to CM0 memory is forbidden
      CM0 privileged secured access to CM0 memory is allowed

      Read from CM4 memory
      Content of CM4 memory by address 0x8024500 is 0x8b5d78bf

      Read from CM0 memory
      Content of CM0 memory by address 0x8000500 is 0x10cb3bb8


   - Unsecure core UART

   .. code-block:: console

      ***** Booting Zephyr OS zephyr-v1.13.0-2203-gbf11698ed9 *****
      PSoC6 Unsecure core has started

      Read from CM4 memory
      Content of CM4 memory by address 0x8024500 is 0x8b5d78bf

      Read from CM0 memory
      ***** BUS FAULT *****
        Precise data bus error
        BFAR Address: 0x8000500
      ***** Hardware exception *****
      Current thread ID = 0x0802421c
      Faulting instruction address = 0x1006026a
      Fatal fault in essential thread! Spinning...

