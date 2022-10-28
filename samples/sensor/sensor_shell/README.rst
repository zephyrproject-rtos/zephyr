.. _sensor_shell_sample:

Sensor Shell Module Sample
##########################

Overview
********
This is a simple shell module to get data from sensors presented in the system.

Building and Running
********************

Build the sample app by choosing the target board that has sensors drivers
enabled, for example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/sensor_module
   :board: reel_board
   :goals: build flash

Shell Module Command Help
=========================

.. code-block:: console

   sensor - Sensor commands
   Subcommands:
     get  :Get sensor data. Channel names are optional. All channels are read when
           no channels are provided. Syntax:
           <device_name> <channel name 0> .. <channel name N>

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
