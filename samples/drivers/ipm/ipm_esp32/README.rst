.. zephyr:code-sample:: ipm-esp32
   :name: IPM on ESP32
   :relevant-api: ipm_interface

   Implement inter-processor mailbox (IPM) between ESP32 APP and PRO CPUs.

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
messages in chunks of 64 bytes.

Building and Running the Zephyr Code
************************************

Build the ESP32 IPM sample code as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/ipm/ipm_esp32
   :board: esp32s3_devkitm/esp32s3/procpu
   :west-args: --sysbuild
   :goals: build
   :compact:

Sample Output
*************

To check the output of this sample, run ``west espressif monitor`` or any other serial
console program (e.g., minicom, putty, screen, etc).

.. code-block:: console

   *** Booting Zephyr OS build v4.0.0-rc2-61-ga24efebe15e2 ***
   PRO_CPU is sending a request, waiting remote response...
   PRO_CPU received a message from APP_CPU : APP_CPU uptime ticks 502
   PRO_CPU is sending a request, waiting remote response...
   PRO_CPU received a message from APP_CPU : APP_CPU uptime ticks 10502
   PRO_CPU is sending a request, waiting remote response...
   PRO_CPU received a message from APP_CPU : APP_CPU uptime ticks 20503
   PRO_CPU is sending a request, waiting remote response...
   PRO_CPU received a message from APP_CPU : APP_CPU uptime ticks 30504
   PRO_CPU is sending a request, waiting remote response...
   PRO_CPU received a message from APP_CPU : APP_CPU uptime ticks 40505
   PRO_CPU is sending a request, waiting remote response...
   PRO_CPU received a message from APP_CPU : APP_CPU uptime ticks 50506
   PRO_CPU is sending a request, waiting remote response...
   PRO_CPU received a message from APP_CPU : APP_CPU uptime ticks 60507
   PRO_CPU is sending a request, waiting remote response...
   PRO_CPU received a message from APP_CPU : APP_CPU uptime ticks 70508
