.. zephyr:code-sample:: sensor_shell
   :name: Sensor shell
   :relevant-api: sensor_interface

   Interact with sensors using the shell module.

Overview
********
This is a simple shell module to get data from sensors presented in the system.

Building and Running
********************

Build the sample app by choosing the target board that has sensors drivers
enabled, for example:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sensor_shell
   :board: reel_board
   :goals: build flash

For boards that do not have a sensor, a simple fake sensor driver is provided, for example:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sensor_shell
   :board: qemu_riscv64
   :goals: run
   :gen-args: -DCONFIG_SAMPLES_SENSOR_SHELL_FAKE_SENSOR=y

Shell Module Command Help
=========================

.. code-block:: console

   sensor - Sensor commands
   Subcommands:
     get   :Get sensor data. Channel names are optional. All channels are read when
            no channels are provided. Syntax:
            <device_name> <channel name 0> .. <channel name N>
     info  :Get sensor info, such as vendor and model name, for all sensors.


**get**: prints all the sensor channels data for a given sensor device name.
Optionally, a single channel name or index can be passed to be printed out. The
device name and channel name are autocompleted.

Output example (reel_board):

.. code-block:: console

   uart:~$ sensor get mma8652fc@1d
   channel idx=0 accel_x =   0.000000
   channel idx=1 accel_y =   0.268150
   channel idx=2 accel_z =   9.959878
   channel idx=3 accel_xyz x =   0.000000 y =   0.268150 z =   9.959878

   uart:~$ sensor get mma8652fc@1d accel_z
   channel idx=2 accel_z =   9.959878

   uart:~$ sensor get mma8652fc@1d 2
   channel idx=2 accel_z =   9.959878

.. note::
   A valid sensor device name should be passed otherwise you will get an
   undefined behavior like hardware exception. This happens because the shell
   subsystem runs in supervisor mode where API callbacks are not checked before
   being called.

**info**: prints vendor, model, and friendly name information for all sensors.
This command depends on enabling :kconfig:option:`CONFIG_SENSOR_INFO`.

.. code-block:: console

   uart:~$ sensor info
   device name: apds9960@39, vendor: Avago Technologies, model: apds9960, friendly name: (null)
   device name: mma8652fc@1d, vendor: NXP Semiconductors, model: fxos8700, friendly name: (null)
   device name: ti_hdc@43, vendor: Texas Instruments, model: hdc, friendly name: (null)
   device name: temp@4000c000, vendor: Nordic Semiconductor, model: nrf-temp, friendly name: (null)
