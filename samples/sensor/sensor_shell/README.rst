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

.. code-block:: console

         cmake -DBOARD=reel_board


.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/sensor_module
   :goals: run


Shell Module Command Help
=========================

.. code-block:: console

         sensor - Sensor commands
         Options:
                 -h, --help  :Show command help.
         Subcommands:
                 get            :<device_name> [chanel_idx]
                 list           :List configured sensors
                 list_channels  :<device_name>

**list**: lists all the initialized devices on the system.

Output example (reel_board):

.. code-block:: console

         uart:~$ sensor list
         devices:
         - clk_k32src
         - clk_m16src
         - UART_0
         - ENTROPY_0
         - GPIO_0
         - GPIO_1
         - I2C_0
         - APDS9960
         - MMA8652FC
         - HDC1008

**list_channels**: lists the supported channels (index and name) for a given
sensor device name.

Output example (reel_board):

.. code-block:: console

         uart:~$ sensor list_channels MMA8652FC
         idx=0 name=accel_x
         idx=1 name=accel_y
         idx=2 name=accel_z
         idx=3 name=accel_xyz

.. note:: A valid sensor device name should be passed otherwise you will get an
   undefined behavior like hardware exception. This happens because the shell
   subsystem runs in supervisor mode where API callbacks are not checked before
   being called.

**get**: prints all the sensor channels data for a given sensor device name.
Optionally, a single channel index can be passed to be printed out.

Output example (reel_board):

.. code-block:: console

         uart:~$ sensor get MMA8652FC
         channel idx=0 accel_x =  -1.379061
         channel idx=1 accel_y =   1.991975
         channel idx=2 accel_z =  -9.576807
         channel idx=3 accel_xyz =  -1.379061
         channel idx=3 accel_xyz =   1.991975
         channel idx=3 accel_xyz =  -9.576807

.. note:: A valid sensor device name should be passed otherwise you will get an
   undefined behavior like hardware exception. This happens because the shell
   subsystem runs in supervisor mode where API callbacks are not checked before
   being called.
