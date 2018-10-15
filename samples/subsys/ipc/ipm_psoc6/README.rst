.. _ipm-psoc6-sample:

Sample mailbox application
##########################

Overview
********

The :ref:`cy8ckit_062_wifi_bt` board has two core processors (Cortex-M4
and Cortex-M0+). This sample application uses a mailbox to send messages
from Cortex-M4 processor core (client) to Cortex-M0+ (server).

Requirements
************

- :ref:`cy8ckit_062_wifi_bt` board

Building and Running
********************

#. Build the project

   .. code-block:: console

      $ cd samples/subsys/ipc/ipm_psoc6
      $ mkdir build
      $ cd build
      $ cmake -G"Eclipse CDT4 - Ninja" ..
      $ ninja

#. Switch the DevKit into CMSIS-DAP mode using SW3 (LED2 should blink) and
   flash the board:

   .. code-block:: console

      $<openocd_path>\bin\openocd -c "source [find interface/cmsis-dap.cfg]" \
         -c "transport select swd" -c "source [find target/psoc6.cfg]" \
         -c "if [catch {program {<zephyr_path>\samples\subsys\ipc\ipm_psoc6\build\zephyr\zephyr.elf}} ] \
            { echo {** Program operation failed **} } \
            else { echo {** Program operation completed successfully **} }" \
         -c "reset_config srst_only;reset run;psoc6.dap dpreg 0x04 0x00;shutdown"

      $<openocd_path>/bin/openocd -c "source [find interface/cmsis-dap.cfg]" \
         -c "transport select swd" -c "source [find target/psoc6.cfg]" \
         -c "if [catch {program {<zephyr_path>\samples\subsys\ipc\ipm_psoc6\build\ipm_psoc6_client-prefix\src\ipm_psoc6_client-build\zephyr\zephyr.elf}} ] \
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
   serial port (10 messages is sent by default - see COUNTER_MAX_VAL define in
   the main_client.c file):

   - Server UART

   .. code-block:: console

      ***** Booting Zephyr OS zephyr-v1.13.0-5925-g3b1efc0d5626 *****
      PSoC6 IPM Server example has started
      Received 0 via IPC, sending it back
      Received 1 via IPC, sending it back
      ...
      Received 9 via IPC, sending it back

   - Client UART

   .. code-block:: console

      ***** Booting Zephyr OS zephyr-v1.13.0-5925-g3b1efc0d5626 *****
      PSoC6 IPM Client example has started
      Sending first message " 0 " from 10 messages via IPM
      Received counter value: 0
      Sending next message: 1
      ...
      Sending next message: 9
      Received counter value: 9
      The last message cycle finished
