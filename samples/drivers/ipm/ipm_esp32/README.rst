.. _ipm_esp32:

ESP32 Soft-IPM example
######################

Overview
********
This simple example can be used with multicore ESP32 Soc, and demonstrates
the software intercore messaging mechanism to be used in AMP applications.

ESP32 has two CPU named APP and PRO, in this simple example PRO send a
message to the APP using the IPM driver, and waits for the APP response
message and prints its contents on console.

ESP32 intercore messaging has up two four channels, the 0 and 1 are
reserved for BT and WIFI messages, and channels 2 and 3 is free to
any application, each channel supports up to 64 bytes of data per
message, so high level protocol is responsible to fragment larger
messages in chunks of 64bytes.

Building and Running the Zephyr Code
************************************

The sample requires two build commands to run, first of all
you need to build the esp32_net firmware as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/ipm/ipm_esp32/ipm_esp32_net
   :board: esp32_net
   :goals: build
   :compact:

Copy output file build/zephyr/esp32_net_firmware.c to samples/drivers/ipm/ipm_esp32/src,
then build the main project and flash as stated earlier, and you would see the following output:

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.0.0-1911-g610f489c861e  ***
   PRO_CPU is sending a fake request, waiting remote response...
   PRO_CPU received a message from APP_CPU : APP_CPU: This is a response
   PRO_CPU is sending a fake request, waiting remote response...
   PRO_CPU received a message from APP_CPU : APP_CPU: This is a response
   PRO_CPU is sending a fake request, waiting remote response...
   PRO_CPU received a message from APP_CPU : APP_CPU: This is a response
   PRO_CPU is sending a fake request, waiting remote response...
   PRO_CPU received a message from APP_CPU : APP_CPU: This is a response
   PRO_CPU is sending a fake request, waiting remote response...
   PRO_CPU received a message from APP_CPU : APP_CPU: This is a response
   PRO_CPU is sending a fake request, waiting remote response...
   PRO_CPU received a message from APP_CPU : APP_CPU: This is a response
   PRO_CPU is sending a fake request, waiting remote response...
   PRO_CPU received a message from APP_CPU : APP_CPU: This is a response
