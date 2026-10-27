.. _lp_em_cc35x1:

.. zephyr:board:: lp_em_cc35x1

TI cc35x1 Launchpad
#####################

Overview
********

The Texas Instruments cc35x1 LaunchPad™ (LP_EM_CC35X1) is a
development kit for the SimpleLink™ multi-standard cc35x1 wireless MCU.

See the `TI cc35x1 LaunchPad Product Page`_ for details.

Hardware
********

The cc35x1 LaunchPad™ development kit features the cc3551e wireless MCU.
The board is equipped with two LEDs and two push buttons

The cc35x1 wireless MCU has a 160 MHz Arm® Cortex®-M33 SoC and an
integrated 2.4 GHz transceiver supporting multiple protocols including Bluetooth®
Low Energy and IEEE® 802.15.4.

See the `TI cc35x1 Product Page`_ for additional details.

Supported Features
==================

The cc35x1 LaunchPad board configuration supports the following hardware
features:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+

Other hardware features have not been enabled yet for this board.

Programming and Debugging
*************************

The LP_EM_CC35X1 requires an external debug probe such as the LP-XDS110 or
LP-XDS110ET.

Currently there is no debug support in Zephyr for the LP_EM_CC35X1. Binaries can be flashed
with the standard ``west flash`` command.

References
**********

.. _TI cc35x1 LaunchPad Product Page:
   https://www.ti.com/tool/LP-EM-CC35X1

.. _TI cc35x1 Product Page:
   https://www.ti.com/product/CC3551E
