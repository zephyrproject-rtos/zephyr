.. _bluetooth-hci-uart-async-sample:

Bluetooth: HCI UART based on ASYNC UART
#######################################

Expose a Zephyr Bluetooth Controller over a standard Bluetooth HCI UART interface.

This sample performs the same basic function as the HCI UART sample, but it uses the UART_ASYNC_API
instead of UART_INTERRUPT_DRIVEN API. Not all boards implement both UART APIs, so the board support
of the HCI UART sample may be different.

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

This sample can be found under :zephyr_file:`samples/bluetooth/hci_uart_async`
in the Zephyr tree and is built as a standard Zephyr application.

Using the controller with emulators and BlueZ
*********************************************

The instructions below show how to use a Nordic nRF5x device as a Zephyr BLE
controller and expose it to Linux's BlueZ.

First, make sure you have a recent BlueZ version installed by following the
instructions in the :ref:`bluetooth_bluez` section.

Now build and flash the sample for the Nordic nRF5x board of your choice.
All of the Nordic Development Kits come with a Segger IC that provides a
debugger interface and a CDC ACM serial port bridge. More information can be
found in :ref:`nordic_segger`.

For example, to build for the nRF52832 Development Kit:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/hci_uart_async
   :board: nrf52dk_nrf52832
   :goals: build flash

.. _bluetooth-hci-uart-async-qemu-posix:

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
   The ``-R`` flag passed to ``btattach`` instructs the kernel to avoid
   interacting with the controller and instead just be aware of it in order
   to proxy it to QEMU later.

If you are running :file:`btmon` you should see a brief log showing how the
Linux kernel identifies the attached controller.

Once the controller is attached follow the instructions in the
:ref:`bluetooth_qemu_posix` section to use QEMU with it.

.. _bluetooth-hci-uart-async-bluez:

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

If you are running :file:`btmon` you should see a comprehensive log showing how
BlueZ loads and initializes the attached controller.

Once the controller is attached follow the instructions in the
:ref:`bluetooth_ctlr_bluez` section to use BlueZ with it.

Debugging the controller
========================

The sample can be debugged using RTT since the UART is reserved used by this
application. To enable debug over RTT the debug configuration file can be used.

.. code-block:: console

   west build samples/bluetooth/hci_uart_async -- -DEXTRA_CONFIG='debug.mixin.conf'

Then attach RTT as described here: :ref:`Using Segger J-Link <Using Segger J-Link>`

Using the controller with the Zephyr host
=========================================

This describes how to hook up a board running this sample to a board running
an application that uses the Zephyr host.

On the controller side, the `zephyr,bt-c2h-uart` DTS property (in the `chosen`
block) is used to select which uart device to use. For example if we want to
keep the console logs, we can keep console on uart0 and the HCI on uart1 like
so:

.. code-block:: dts

   / {
      chosen {
         zephyr,console = &uart0;
         zephyr,shell-uart = &uart0;
         zephyr,bt-c2h-uart = &uart1;
      };
   };

On the host application, some config options need to be used to select the H4
driver instead of the built-in controller:

.. code-block:: kconfig

   CONFIG_BT_HCI=y
   CONFIG_BT_CTLR=n
   CONFIG_BT_H4=y

Similarly, the `zephyr,bt-uart` DTS property selects which uart to use:

.. code-block:: dts

   / {
      chosen {
         zephyr,console = &uart0;
         zephyr,shell-uart = &uart0;
         zephyr,bt-uart = &uart1;
      };
   };
