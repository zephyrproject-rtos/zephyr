.. _bluetooth-hci-uart-sample:

Bluetooth: HCI UART
####################

Overview
*********

Expose the Zephyr Bluetooth controller support over UART to another device/CPU
using the H:4 HCI transport protocol (requires HW flow control from the UART).

Requirements
************

* A board with BLE support

Default UART settings
*********************

By default the controller builds use the following settings:

* Baudrate: 1Mbit/s
* 8 bits, no parity, 1 stop bit
* Hardware Flow Control (RTS/CTS) enabled

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/hci_uart` in the
Zephyr tree, and it is built as a standard Zephyr application.

Using the controller with emulators and BlueZ
*********************************************

The instructions below show how to use a Nordic nRF5x device as a Zephyr BLE
controller and expose it to Linux's BlueZ. This can be very useful for testing
the Zephyr Link Layer with the BlueZ Host. The Zephyr BLE controller can also
provide a modern BLE 5.0 controller to a Linux-based machine for native
BLE support or QEMU-based development.

First, make sure you have a recent BlueZ version installed by following the
instructions in the :ref:`bluetooth_bluez` section.

Now build and flash the sample for the Nordic nRF5x board of your choice.
All of the Nordic Development Kits come with a Segger IC that provides a
debugger interface and a CDC ACM serial port bridge. More information can be
found in :ref:`nordic_segger`.

For example, to build for the nRF52832 Development Kit:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/hci_uart
   :board: nrf52dk_nrf52832
   :goals: build flash

.. _bluetooth-hci-uart-qemu-posix:

Using the controller with QEMU and Native POSIX
===============================================

In order to use the HCI UART controller with QEMU or Native POSIX you will need
to attach it to the Linux Host first. To do so simply build the sample and
connect the UART to the Linux machine, and then attach it with this command:

.. code-block:: console

   sudo btattach -B /dev/ttyACM0 -S 1000000 -R

.. note::
   Depending on the serial port you are using you will need to modify the
   ``/dev/ttyACM0`` string to point to the serial device your controller is
   connected to.

.. note::
   If using the BBC micro:bit you will need to modify the baudrate argument
   from ``1000000`` to ``115200``.

.. note::
   The ``-R`` flag passed to ``btattach`` instructs the kernel to avoid
   interacting with the controller and instead just be aware of it in order
   to proxy it to QEMU later.

If you are running :file:`btmon` you should see a brief log showing how the
Linux kernel identifies the attached controller.

Once the controller is attached follow the instructions in the
:ref:`bluetooth_qemu_posix` section to use QEMU with it.

.. _bluetooth-hci-uart-bluez:

Using the controller with BlueZ
===============================

In order to use the HCI UART controller with BlueZ you will need to attach it
to the Linux Host first. To do so simply build the sample and connect the
UART to the Linux machine, and then attach it with this command:

.. code-block:: console

   sudo btattach -B /dev/ttyACM0 -S 1000000

.. note::
   Depending on the serial port you are using you will need to modify the
   ``/dev/ttyACM0`` string to point to the serial device your controller is
   connected to.

.. note::
   If using the BBC micro:bit you will need to modify the baudrate argument
   from ``1000000`` to ``115200``.

If you are running :file:`btmon` you should see a comprehensive log showing how
BlueZ loads and initializes the attached controller.

Once the controller is attached follow the instructions in the
:ref:`bluetooth_ctlr_bluez` section to use BlueZ with it.

Debugging the controller
========================

The sample can be debugged using RTT since the UART is otherwise used by this
application. To enable debug over RTT the debug configuration file can be used.

.. code-block:: console

   west build samples/bluetooth/hci_uart -- -DOVERLAY_CONFIG='debug.conf'

Then attach RTT as described here: :ref:`Using Segger J-Link <Using Segger J-Link>`

Support for the Direction Finding
=================================

The sample can be built with the support for the BLE Direction Finding.
To enable this feature build this sample for specific board variants that provide
required hardware configuration for the Radio.

.. code-block:: console

   west build samples/bluetooth/hci_uart -b nrf52833dk_nrf52833@df -- -DCONFIG_BT_CTLR_DF=y

You can use following targets:

* ``nrf5340dk_nrf5340_cpunet@df``
* ``nrf52833dk_nrf52833@df``

Check the :ref:`bluetooth_direction_finding_connectionless_rx` and the :ref:`bluetooth_direction_finding_connectionless_tx` for more details.
