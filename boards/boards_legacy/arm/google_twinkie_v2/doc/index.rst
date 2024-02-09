.. _google_twinkie_v2_board:

Google Twinkie V2
#################

Overview
********

Google Twinkie V2 is a reference board for the google power delivery analyzer
(PDA) Twinkie V2.

Hardware
********

- STM32G0B1REI6

Supported Features
==================

The following features are supported:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| CLOCK     | on-chip    | reset and clock control             |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | flash memory                        |
+-----------+------------+-------------------------------------+

The default configuration can be found in the defconfig file:
``boards/arm/google_twinkie_v2/google_twinkie_v2_defconfig``

Pin Mapping
===========

Default Zephyr Peripheral Mapping:
----------------------------------
- CC1_BUF : PA1
- CC2_BUF : PA3
- VBUS_READ_BUF : PB11
- CSA_VBUS : PC4
- CSA_CC2 : PC5

Programming and Debugging
*************************

Build application as usual for the ``google_twinkie_v2`` board, and flash
using dfu-util or J-Link.

Debugging
=========

Use SWD with a J-Link or ST-Link.
