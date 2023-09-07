.. zephyr:code-sample:: uart-native-tty
   :name: Native TTY UART
   :relevant-api: uart_interface

   Use native TTY driver to send and receive data between two UART-to-USB bridge dongles.

Overview
********

This sample demonstrates the usage of the Native TTY UART driver. It uses two
UART to USB bridge dongles that are wired together, writing demo data to one
dongle and reading it from the other."

The source code for this sample application can be found at:
:zephyr_file:`samples/drivers/uart/native-tty`.

You can learn more about the Native TTY UART driver in the
:ref:`TTY UART <native_tty_uart>` section of the Native posix board
documentation.

Requirements
************

#. Two UART to USB bridge dongles. Each dongle must have its TX pin wired to the
   other dongle's RX pin. The ground connection is not needed. Both dongles must
   be plugged into the host machine.
#. The DT overlay provided with the sample expects the dongles to appear as
   ``/dev/ttyUSB0`` and ``/dev/ttyUSB1`` in the system. You can check what they
   are in your system by running the command ``ls -l /dev/tty*``. If that is not
   the case on your machine you can either change the ``serial-port`` properties
   in the ``boards/native_posix.overlay`` file or using the command line options
   ``-uart_port`` and ``-uart_port2``.

Building and Running
********************

This application can be built and executed on Native Posix as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/uart/native_tty
   :host-os: unix
   :board: native_posix
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

   uart_1 connected to pseudotty: /dev/pts/6
   uart2 connected to the serial port: /dev/ttyUSB1
   uart connected to the serial port: /dev/ttyUSB0
   *** Booting Zephyr OS build v3.4.0-rc2-97-ge586d02c137d ***
   Device uart sent: "Hello from device uart, num 9"
   Device uart2 received: "Hello from device uart, num 9"
   Device uart sent: "Hello from device uart, num 8"
   Device uart2 received: "Hello from device uart, num 8"
   Device uart sent: "Hello from device uart, num 7"
   Device uart2 received: "Hello from device uart, num 7"
   Device uart sent: "Hello from device uart, num 6"
   Device uart2 received: "Hello from device uart, num 6"
   Device uart sent: "Hello from device uart, num 5"
   Device uart2 received: "Hello from device uart, num 5"
   Device uart sent: "Hello from device uart, num 4"
   Device uart2 received: "Hello from device uart, num 4"
   Device uart sent: "Hello from device uart, num 3"
   Device uart2 received: "Hello from device uart, num 3"
   Device uart sent: "Hello from device uart, num 2"
   Device uart2 received: "Hello from device uart, num 2"
   Device uart sent: "Hello from device uart, num 1"
   Device uart2 received: "Hello from device uart, num 1"
   Device uart sent: "Hello from device uart, num 0"
   Device uart2 received: "Hello from device uart, num 0"

   Changing baudrate of both uart devices to 9600!

   Device uart sent: "Hello from device uart, num 9"
   Device uart2 received: "Hello from device uart, num 9"
   Device uart sent: "Hello from device uart, num 8"
   Device uart2 received: "Hello from device uart, num 8"
   Device uart sent: "Hello from device uart, num 7"
   Device uart2 received: "Hello from device uart, num 7"
   Device uart sent: "Hello from device uart, num 6"
   Device uart2 received: "Hello from device uart, num 6"
   Device uart sent: "Hello from device uart, num 5"
   Device uart2 received: "Hello from device uart, num 5"
   Device uart sent: "Hello from device uart, num 4"
   Device uart2 received: "Hello from device uart, num 4"
   Device uart sent: "Hello from device uart, num 3"
   Device uart2 received: "Hello from device uart, num 3"
   Device uart sent: "Hello from device uart, num 2"
   Device uart2 received: "Hello from device uart, num 2"
   Device uart sent: "Hello from device uart, num 1"
   Device uart2 received: "Hello from device uart, num 1"
   Device uart sent: "Hello from device uart, num 0"
   Device uart2 received: "Hello from device uart, num 0"
