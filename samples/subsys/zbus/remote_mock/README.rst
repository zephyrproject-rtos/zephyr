.. _zbus-remote-mock-sample:

Remote mock sample
##################

Overview
********

This application demonstrates how a host script can publish to the zbus in an embedded device using the UART as a bridge.
The device sends information to the script running on a computer host. Then, the script sends back information when it would simulate a publish to some channel. Finally, the script decodes the data exchanged and prints it to the stdout.

With this approach, developers can implement tests using any language on a computer to talk directly via channels with threads running on a device. Furthermore, it gives the tests more controllability and observability since we can control and access what is sent and received by the script.

Building and Running
********************

This project outputs to the console. It can be built and executed
on native_posix as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/zbus/remote_mock
   :host-os: unix
   :board: native_posix
   :goals: run

Sample Output
=============

.. code-block:: console

    -- west build: running target run
    [0/1] cd /.../zephyr/build/zephyr/zephyr.exe
    uart_1 connected to pseudotty: /dev/pts/2
    uart connected to pseudotty: /dev/pts/3
    *** Booting Zephyr OS build zephyr-v3.1.0 ***
    D: [Mock Proxy] Started.
    D: [Mock Proxy RX] Started.
    D: [Mock Proxy RX] Found sentence
    D: Channel start_measurement claimed
    D: Channel start_measurement finished
    D: Publishing channel start_measurement
    D: START processing channel start_measurement change
    D: Channel start_measurement claimed
    D: discard loopback event (channel start_measurement)
    D: Channel start_measurement finished
    D: FINISH processing channel start_measurement change
    D: START processing channel sensor_data change
    D: Channel sensor_data claimed
    D: sending message to host (channel sensor_data)
    D: Channel sensor_data finished
    D: FINISH processing channel sensor_data change
    D: sending sensor data err = 0

    <repeats endlessly>

Exit execution by pressing :kbd:`CTRL+C`.

The :file:`remote_mock.py` script can be executed using the following command:

.. code-block:: bash

    python3.8 samples/subsys/zbus/remote_mock/remote_mock.py /dev/pts/2


Note the run command above prints the value of pts port because it is running in ``native_posix``. Look at the line indicating ``uart_1 connected to pseudotty: /dev/pts/2``. It can be different in your case. If you are using a board, read the documentation to get the correct port destination (in Linux is something like ``/dev/tty...`` or in Windows ``COM...``).

From the remote mock (Python script), you would see something like this:

.. code-block:: shell

    Proxy PUB [start_measurement] -> start measurement
    Proxy NOTIFY: [sensor_data] -> sensor value 1
    Proxy PUB [start_measurement] -> start measurement
    Proxy NOTIFY: [sensor_data] -> sensor value 2
    Proxy PUB [start_measurement] -> start measurement
    Proxy NOTIFY: [sensor_data] -> sensor value 3
    Proxy PUB [start_measurement] -> start measurement
    Proxy NOTIFY: [sensor_data] -> sensor value 4
    Proxy PUB [start_measurement] -> start measurement
    Proxy NOTIFY: [sensor_data] -> sensor value 5
    Proxy PUB [start_measurement] -> start measurement
    Proxy NOTIFY: [sensor_data] -> sensor value 6
    Proxy PUB [start_measurement] -> start measurement
    Proxy NOTIFY: [sensor_data] -> sensor value 7
    Proxy PUB [start_measurement] -> start measurement
    Proxy NOTIFY: [sensor_data] -> sensor value 8
    Proxy PUB [start_measurement] -> start measurement
    Proxy NOTIFY: [sensor_data] -> sensor value 9
    Proxy PUB [start_measurement] -> start measurement
    Proxy NOTIFY: [sensor_data] -> sensor value 10

    <continues>

Exit the remote mock script by pressing :kbd:`CTRL+C`.
