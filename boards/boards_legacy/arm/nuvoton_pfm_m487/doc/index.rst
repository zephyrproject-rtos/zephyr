.. _nuvoton_pfm_m487:

NUVOTON NUMAKER PFM M487
########################

Overview
********

The NuMaker PFM M487 is an Internet of Things (IoT) application focused platform
specially developed by Nuvoton. The PFM-M487 is based on the NuMicro® M487
Ethernet series MCU with ARM® -Cortex®-M4F core.

.. image:: pfm_m487.jpg
   :align: center
   :alt: PFM-M487

Features:
=========
- 32-bit Arm Cortex®-M4 M487JIDAE MCU
- Core clock up to 192 MHz
- 512 KB embedded Dual Bank Flash and 160 KB SRAM
- Audio codec (NAU88L25) with Microphone In and Headphone Out
- Ethernet (IP101GR) for network application
- USB 2.0 High-Speed OTG / Host / Device
- USB 1.1 Full-Speed OTG / Host / Device
- External SPI Flash (Winbond W25Q20) which can be regarded as ROM module
- MicroSD Card slot for T-Flash
- M487 extended interface 4 connector with 36 pins each
- Arduino UNO compatible interface
- Three push-buttons: one is for reset and the other two are for user-defined
- Four LEDs: one is for power indication and the other three are for user-defined
- On-board NU-Link-Me ICE debugger/programmer with SWD connector

More information about the board can be found at the `PFM M487 User Manual`_.

Supported Features
==================

* The on-board 12-MHz crystal allows the device to run at its maximum operating speed of 192MHz.

The development board configuration supports the following hardware features:

+-----------+------------+-----------------------+
| Interface | Controller | Driver/Component      |
+===========+============+=======================+
| NVIC      | on-chip    | nested vectored       |
|           |            | interrupt controller  |
+-----------+------------+-----------------------+
| SYSTICK   | on-chip    | system clock          |
+-----------+------------+-----------------------+
| UART      | on-chip    | serial port           |
+-----------+------------+-----------------------+

Other hardware features are not yet supported on Zephyr porting.

More details about the supported peripherals are available in `M480 TRM`_
Other hardware features are not currently supported by the Zephyr kernel.

Building and Flashing
*********************
Flashing
========

Here is an example for the :ref:`hello_world` application.

On board debugger Nu-link-Me can emulate UART0 as a virtual COM port over usb,
To enable this, set ISW1 DIP switch 1-3 (TXD RXD VOM) to ON.
Connect the PFM M487 IoT to your host computer using the USB port, then
run a serial host program to connect with your board. For example:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nuvoton_pfm_m487
   :goals: flash

Debugging
=========

Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nuvoton_pfm_m487
   :goals: debug

Step through the application in your debugger.

References
**********

.. _PFM M487 User Manual:
   https://www.nuvoton.com/export/resource-files/UM_NuMaker-PFM-M487_User_Manual_EN_Rev1.01.pdf
.. _M480 TRM:
   https://www.nuvoton.com/export/resource-files/TRM_M480_Series_EN_Rev2.02.pdf
