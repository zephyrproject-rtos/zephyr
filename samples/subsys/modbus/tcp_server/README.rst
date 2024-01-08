.. zephyr:code-sample:: modbus-tcp-server
   :name: Modbus TCP server
   :relevant-api: modbus bsd_sockets

   Implement a Modbus TCP server exposing Modbus commands to control LEDs.

Overview
********

This is a simple application demonstrating a Modbus TCP server implementation
in Zephyr RTOS.

Requirements
************

This sample has been tested with FRDM-K64F board,
but it should work with any board or shield that has a network interface.
Additionally the board should have three LEDs.

TCP server example is running on an evaluation board. Client is running
on a PC or laptop.

The description of this sample uses `PyModbus`_ (Pymodbus REPL).
The user can of course try out other client implementations with this sample.

Building and Running
********************

This sample can be found under
:zephyr_file:`samples/subsys/modbus/tcp_server` in the Zephyr tree.

The following commands build and flash TCP server sample.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/modbus/tcp_server
   :board: frdm_k64f
   :goals: build flash
   :compact:

On the client side, PC or laptop, the following command connects PyModbus
to the TCP server.

.. code-block:: console

   # pymodbus.console tcp --host 192.0.2.1 --port 502

The LEDs on the board are controlled by Modbus commands FC01, FC05, FC15.
For example, to set LED0 on use FC01 command (write_coil).

.. code-block:: console

   > client.connect
   > client.write_coil address=0 value=1 unit=1

Client should confirm successful communication and LED0 should light.

.. code-block:: console

   {
       "address": 0,
       "value": true
   }

To set LED0 off but LED1 and LED2 on use FC15 command (write_coils).

.. code-block:: console

   > client.write_coils address=0 values=0,1,1 unit=1

To read LED0, LED1, LED2 state FC05 command (read_coils) can be used.

.. code-block:: console

   > client.read_coils address=0 count=3 unit=1
   {
       "bits": [
           false,
           true,
           true,
           false,
           false,
           false,
           false,
           false
       ]
   }

It is also possible to write and read the holding registers.
This however does not involve any special interaction
with the peripherals on the board yet.

To write single holding registers use FC06 command (write_register),

.. code-block:: console

   > client.write_register address=0 value=42 unit=1

or FC16 command (write_registers).

.. code-block:: console

   > client.write_registers address=0 values=42,42,42 unit=1

To read holding registers use FC03 command (read_holding_registers).

.. code-block:: console

   > client.read_holding_registers address=0 count=3 unit=1
   {
       "registers": [
           42,
           42,
           42
       ]
   }

.. _`PyModbus`: https://github.com/riptideio/pymodbus
