.. zephyr:board:: frdm_a_s32k344

Overview
********

`NXP FRDM-A-S32K344`_ is a development board for general-purpose industrial and
automotive applications. It features an `NXP S32K344`_ general-purpose automotive
microcontroller based on an Arm Cortex-M7 core (Lock-Step).

Hardware
********

- NXP S32K344

  - Arm Cortex-M7 (Lock-Step), 160 MHz (Max.)
  - 4 MB of program flash
  - 512KB (incl. 192KB TCM)
  - Error-Correcting Code (ECC) on all memories
  - Ethernet 100 Mbps, CAN FD, FlexIO, QSPI
  - 12-bit 1 Msps ADC, 16-bit eMIOS timer

- `NXP FS26 Safety System Basis Chip`_

- Interfaces

  - Console UART
  - 6x CAN FD
  - 100Base-T1 Ethernet
  - JST-GH connectors and I/O headers for I2C, SPI, GPIO,
    PWM, etc.

More information about the hardware and design resources can be found at
`NXP FRDM-A-S32K344`_ website.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Each GPIO port is divided into two banks: low bank, from pin 0 to 15, and high
bank, from pin 16 to 31. For example, ``PTA2`` is the pin 2 of ``gpioa_l`` (low
bank), and ``PTA20`` is the pin 4 of ``gpioa_h`` (high bank).

The GPIO controller provides the option to route external input pad interrupts
to either the SIUL2 EIRQ or WKPU interrupt controllers, as supported by the SoC.
By default, GPIO interrupts are routed to SIUL2 EIRQ interrupt controller,
unless they are explicitly configured to be directed to the WKPU interrupt
controller, as outlined in :zephyr_file:`dts/bindings/gpio/nxp,siul2-gpio.yaml`.

To find information about which GPIOs are compatible with each interrupt
controller, refer to the device reference manual.

.. note::

   It is important to highlight that the current board configuration lacks
   support for wake-up events and power-management features. WKPU functionality
   is restricted solely to serving as an interrupt controller.

LEDs
----

The FRDM-A-S32K344 board has one user RGB LED:

=======================  =====  =====  ===================================
Devicetree node          Color  Pin    Pin Functions
=======================  =====  =====  ===================================
led0 / user_led1_red     Red    PTA29  EMIOS1 CH12
led1 / user_led1_green   Green  PTA30  EMIOS1 CH13
led2 / user_led1_blue    Blue   PTA31  FXIO D0 / EMIOS1 CH14
=======================  =====  =====  ===================================

Buttons
-------

The FRDM-A-S32K344 board has two user buttons:

=======================  =====  =====  ==============
Devicetree node          Label  Pin    Pin Functions
=======================  =====  =====  ==============
sw0 / user_button_1      SW2    PTB26  WKUP41
sw1 / user_button_2      SW3    PTB19  WKUP38
=======================  =====  =====  ==============

System Clock
============

The Arm Cortex-M7 (Lock-Step) are configured to run at 160 MHz.

Serial Console
==============

By default, the serial console is provided through ``lpuart6`` on the on-board
OpenSDA debugger (K26 MCU) via the USB-C connector ``J11``.

=========  =====  ============
Connector  Pin    Pin Function
=========  =====  ============
J11        PTA16  LPUART6_TX
J11        PTA15  LPUART6_RX
=========  =====  ============

Console Output
--------------

.. note::

   UART console bring-up has been verified so far by flashing via the
   on-board OpenSDA debugger's stock `P&E Micro`_ firmware using `NXP S32
   Design Studio`_, and connecting to the OpenSDA CDC serial port at
   115200 8N1. J-Link, TRACE32, and pyOCD flashing/debugging (see
   `Programming and Debugging`_ below) are still expected to work and
   remain to be verified.

.. code-block:: console

   *** Booting Zephyr OS build ... ***
   Hello World! frdm_a_s32k344/s32k344

CAN
===

FRDM-A-S32K344 exposes ``flexcan0`` through an on-board `NXP TJA1043`_ High-Speed
CAN Transceiver with Standby mode, routed to connector ``J15``. CAN FD is
supported up to 5 Mbit/s.

===============  =======  ===============  ================
Devicetree node  Pin      Pin Function     Bus Connector
===============  =======  ===============  ================
flexcan0         | PTA6   | PTA6_CAN0_RX   | J15-1 (CANH)
                 | PTA7   | PTA7_CAN0_TX   | J15-2 (CANL)
===============  =======  ===============  ================

Additional TJA1043 control lines (see UM12406 Table 11):

=========  ==========  ==========  ==============================
Signal     Pin         Default     Description
=========  ==========  ==========  ==============================
CAN0_ERRN  PTA11       routed      TJA1043 error output
CAN0_EN    PTC21       routed      TJA1043 mode-select EN
CAN0_STB   PTC20       routed      TJA1043 standby STB
CAN0_INH   FS26 WAKE1  not routed  CAN wake-up path to FS26 WAKE1
CAN0_WAKE  PTE25       not routed  TJA1043 local wake-up input
=========  ==========  ==========  ==============================

.. note::
   CAN signals are referenced to the VSUP voltage rail. By default, when
   using the FS26 VBOOST feature, these lines are referenced to 8 V.
   If power delivery is enabled, it is recommended to supply 9 V, 12 V,
   or 15 V. In such cases, VSUP will follow the input voltage level
   accordingly.

``flexcan0`` is configured with ``number-of-mb = <96>`` in
``dts/arm/nxp/s32/nxp_s32k344_m7.dtsi``. Each 512-byte RAM region holds
32 classic CAN MBs or 7 CAN FD MBs, giving the following effective counts:

===============  ===========  ==========  ======  ======================
Devicetree node  Mode         MB size     MBs     Calculation
===============  ===========  ==========  ======  ======================
flexcan0         Classic CAN  8 bytes     96      3 regions × 32
flexcan0         CAN FD       64 bytes    21      96 × 7 / 32
===============  ===========  ==========  ======  ======================

Set the RX filter count per instance using the ``max-filters`` devicetree
property. When omitted, the driver falls back to
:kconfig:option:`CONFIG_CAN_MCUX_FLEXCAN_MAX_FILTERS` (default 13 for
classic CAN, 5 for CAN FD). ``max-filters`` must be smaller than the
effective MB count.

.. note::
   The `NXP FRDM-A-S32K344`_ board already includes an on-board 120 Ohm
   split termination network (two 60.4 Ohm resistors, R176 and R178) for
   the CAN interface (J15). Therefore, no external termination is required
   when this board is connected at the ends of a CAN bus.

I2C
===

I2C is provided through the LPI2C interface. On the FRDM-A-S32K344 board,
the I2C pins are routed by default to the on-board sensors and USB Power Delivery controllers.

=========  ====  ====================
Interface  Pin   Pin Function
=========  ====  ====================
LPI2C      PTC7  LPI2C_SCL
LPI2C      PTC6  LPI2C_SDA
=========  ====  ====================

The following on-board components are connected to this I2C bus:
* **NXP FXLS8964AF**: Automotive Accelerometer
* **NXP P3T1750**: Automotive Temperature Sensor
* **NXP PTN5110 & NX20P3483**: USB Power Delivery PHY and Power Switch

ADC
===

ADC is provided through 12-bit ADC SAR controller with 3 instances, each supporting up to
24 channels. ADC channels are divided into 3 groups (precision, standard and external).

.. note::
   All channels of an instance only run on 1 group channel at the same time.

FS26 SBC Watchdog
=================

On normal operation after the board is powered on, there is a window of 256 ms
on which the FS26 watchdog must be serviced with a good token refresh, otherwise
the watchdog will signal a reset to the MCU. This board configuration enables
the FS26 watchdog driver that handles this initialization.

External Flash
==============

The on-board Winbond W25Q64J 64M-bit multi-I/O serial NOR Flash memory is connected
to the QSPI controller. This board configuration selects it as the
default external flash memory for execution-in-place (XiP) or data storage.

Ethernet
========

This board features a single instance of Ethernet Media Access Controller (EMAC)
interfacing with a `KSZ8091RNDIA`_ 100BASE-T Ethernet PHY.

The Ethernet signals are routed directly to the on-board RJ45 connector (J7),
enabling robust network communication and evaluation for automotive and industrial applications.

.. todo::
   The Ethernet PHY devicetree node reuses the microchip,ksz8081 driver, since
   Zephyr has no dedicated KSZ8091 binding. Both parts share the same base
   register map; KSZ8091 only adds EEE/WOL on top, which are not used here.
   Verify Ethernet link-up works on FRDM-A-S32K344 board.

Programming and Debugging
=========================

.. todo::

   None of the J-Link, TRACE32, or pyOCD runners below have been verified on
   FRDM-A-S32K344 board yet.

.. zephyr:board-supported-runners::

Applications for the ``frdm_a_s32k344`` board can be built in the usual way as
documented in :ref:`build_an_application`.

This board configuration supports `Lauterbach TRACE32`_, `SEGGER J-Link`_ and `pyOCD`_
West runners for flashing and debugging. Follow the steps described in
:ref:`lauterbach-trace32-debug-host-tools`, :ref:`jlink-debug-host-tools` and
:ref:`pyocd-debug-host-tools`, to set up the required host tools.

If using TRACE32, ensure you have version >= 2024.09 installed

Flashing
========

.. todo::

   Verify ``west flash`` actually works on FRDM-A-S32K344 board via J-Link, TRACE32,
   and pyOCD before publishing.

Run the ``west flash`` command to flash the application using SEGGER J-Link.
Alternatively, run ``west flash -r trace32`` to use Lauterbach TRACE32, or
``west flash -r pyocd`` to use pyOCD.

The Lauterbach TRACE32 runner supports additional options that can be passed
through command line:

.. code-block:: console

   west flash -r trace32 --startup-args elfFile=<elf_path> loadTo=<flash/sram>
      eraseFlash=<yes/no> verifyFlash=<yes/no>

Where:

- ``<elf_path>`` is the path to the Zephyr application ELF in the output
  directory
- ``loadTo=flash`` loads the application to the SoC internal program flash
  (:kconfig:option:`CONFIG_XIP` must be set), and ``loadTo=sram`` load the
  application to SRAM. Default is ``flash``.
- ``eraseFlash=yes`` erases the whole content of SoC internal flash before the
  application is downloaded to either Flash or SRAM. This routine takes time to
  execute. Default is ``no``.
- ``verifyFlash=yes`` verify the SoC internal flash content after programming
  (use together with ``loadTo=flash``). Default is ``no``.

For example, to erase and verify flash content:

.. code-block:: console

   west flash -r trace32 --startup-args elfFile=build/zephyr/zephyr.elf loadTo=flash eraseFlash=yes verifyFlash=yes

MCUboot
=======

This board supports app chain-loading using MCUboot.

Build & Flash
-------------

.. todo::

   Verify this MCUboot build/flash procedure against the FRDM-A-S32K344 board
   and cross-check with UM12406.

To build MCUboot and the ``flash_shell`` sample application together and
generate HEX files suitable for flashing, run:

.. code-block:: console

   west build -p -b frdm_a_s32k344/s32k344/mcuboot samples/drivers/flash_shell --sysbuild
   west flash

The resulting artifacts are:

* MCUboot: ``build/mcuboot/zephyr/zephyr.hex``
* App (unsigned): ``build/flash_shell/zephyr/zephyr.hex``

Troubleshooting
---------------

    If MCUboot prints “Image in the primary slot is not valid” or stalls after
    “Jumping to the first image slot”, the app was likely signed with a 512-byte header.
    Re-sign with --header-size 0x400 and re-flash.

    Do not add an IVT to MCUboot-chainloaded applications;
    it’s only emitted for standalone/XIP images or MCUboot itself.

Debugging
=========

.. todo::

   Verify ``west debug`` actually works on FRDM-A-S32K344 board via J-Link, TRACE32,
   and pyOCD before publishing.

The reset switch ``SW4`` allows a manual reset of the S32K344 MCU. While reset is
asserted, the reset indicator LED ``D17`` stays lit.

Run the ``west debug`` command to start a GDB session using SEGGER J-Link.
Alternatively, run ``west debug -r trace32`` or ``west debug -r pyocd``
to launch the Lauterbach TRACE32 or pyOCD software debugging interface respectively.

Configuring a Console
======================

We will use the on-board OpenSDA interface as a USB-to-serial adapter for the
serial console, reached through the USB-C connector ``J11``.

Use the following settings with your serial terminal of choice (minicom, putty, etc.):

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

.. include:: ../../common/board-footer.rst.inc

References
**********

.. target-notes::

.. _NXP FRDM-A-S32K344:
   https://www.nxp.com/design/design-center/development-boards-and-designs/FRDM-A-S32K344

.. _NXP S32K344:
   https://www.nxp.com/products/processors-and-microcontrollers/s32-automotive-platform/s32k-auto-general-purpose-mcus/s32k3-microcontrollers-for-automotive-general-purpose:S32K3

.. _NXP FS26 Safety System Basis Chip:
   https://www.nxp.com/products/power-management/pmics-and-sbcs/safety-sbcs/safety-system-basis-chip-with-low-power-fit-for-asil-d:FS26

.. _NXP TJA1043:
   https://www.nxp.com/products/interfaces/can-transceivers/can-with-flexible-data-rate/high-speed-can-transceiver-with-standby-and-sleep-mode:TJA1043

.. _UM12406:
   https://docs.nxp.com/bundle/UM12406/page/topics/Overview.html

.. _KSZ8091RNDIA:
   https://www.microchip.com/en-us/product/ksz8091

.. _P&E Micro:
   https://www.pemicro.com/opensda/

.. _NXP S32 Design Studio:
   https://www.nxp.com/design/design-center/software/automotive-software-and-tools/s32-design-studio-ide:S32DS

.. _Lauterbach TRACE32:
   https://www.lauterbach.com

.. _SEGGER J-Link:
   https://wiki.segger.com/NXP_S32K3xx

.. _pyOCD:
   https://pyocd.io/
