.. zephyr:code-sample:: zbus-uart-bridge
   :name: UART bridge
   :relevant-api: zbus_apis

   Redirect channel events to the host over UART.

Overview
********

This simple application demonstrates a UART redirection of all channel events to the host.
The device sends information to the script running on a computer host. The script decodes it and prints it to the stdout.

Building and Running
********************

This project outputs to the console. It can be built and executed
on native_sim as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/zbus/uart_bridge
   :host-os: unix
   :board: native_sim
   :goals: run

Sample Output
=============

.. code-block:: console

    [0/1] cd .../zephyr/build/zephyr/zephyr.exe
    uart_1 connected to pseudotty: /dev/pts/2
    uart connected to pseudotty: /dev/pts/3
    *** Booting Zephyr OS build zephyr-v3.1.0 ***
    D: Core sending start measurement with status 0
    D: START processing channel start_measurement change
    D: FINISH processing channel start_measurement change
    D: Peripheral sending sensor data
    D: START processing channel sensor_data change
    D: FINISH processing channel sensor_data change
    D: Bridge Started
    D: Channel start_measurement claimed
    D: Channel start_measurement finished
    D: Bridge send start_measurement
    D: Channel sensor_data claimed
    D: Channel sensor_data finished
    D: Bridge send sensor_data
    D: Core sending start measurement with status 1
    D: START processing channel start_measurement change
    D: FINISH processing channel start_measurement change
    D: Channel start_measurement claimed
    D: Channel start_measurement finished
    D: Bridge send start_measurement
    D: Peripheral sending sensor data
    D: START processing channel sensor_data change
    D: FINISH processing channel sensor_data change
    D: Channel sensor_data claimed
    D: Channel sensor_data finished
    D: Bridge send sensor_data

    <repeats endlessly>

Exit execution by pressing :kbd:`CTRL+C`.

The :file:`decoder.py` script can be executed using the following command:

.. code-block:: bash

    python3.8 samples/subsys/zbus/uart_bridge/decoder.py /dev/pts/2


Note the run command above prints the value of pts port because it is running in
:zephyr:board:`native_sim <native_sim>`.
Look at the line indicating ``uart_1 connected to pseudotty: /dev/pts/2``.
It can be different in your case. If you are using a board, read the documentation to get the
correct port destination (in Linux is something like ``/dev/tty...`` or in Windows ``COM...``).

From the serial decoder (Python script), you would see something like this:

.. code-block:: shell

    PUB [sensor_data] -> b'\xc5\x01\x00\x00\xb2\x11\x00\x00'
    PUB [start_measurement] -> b'\x00'
    PUB [sensor_data] -> b'\xc6\x01\x00\x00\xbc\x11\x00\x00'
    PUB [start_measurement] -> b'\x01'
    PUB [sensor_data] -> b'\xc7\x01\x00\x00\xc6\x11\x00\x00'
    PUB [start_measurement] -> b'\x00'
    PUB [sensor_data] -> b'\xc8\x01\x00\x00\xd0\x11\x00\x00'
    PUB [start_measurement] -> b'\x01'
    PUB [sensor_data] -> b'\xc9\x01\x00\x00\xda\x11\x00\x00'
    PUB [start_measurement] -> b'\x00'
    PUB [sensor_data] -> b'\xca\x01\x00\x00\xe4\x11\x00\x00'
    PUB [start_measurement] -> b'\x01'
    PUB [sensor_data] -> b'\xcb\x01\x00\x00\xee\x11\x00\x00'
    PUB [start_measurement] -> b'\x00'
    PUB [sensor_data] -> b'\xcc\x01\x00\x00\xf8\x11\x00\x00'
    PUB [start_measurement] -> b'\x01'
    PUB [sensor_data] -> b'\xcd\x01\x00\x00\x02\x12\x00\x00'
    PUB [start_measurement] -> b'\x00'
    PUB [sensor_data] -> b'\xce\x01\x00\x00\x0c\x12\x00\x00'
    PUB [start_measurement] -> b'\x01'
    PUB [sensor_data] -> b'\xcf\x01\x00\x00\x16\x12\x00\x00'
    PUB [start_measurement] -> b'\x00'

Exit the decoder script by pressing :kbd:`CTRL+C`.
