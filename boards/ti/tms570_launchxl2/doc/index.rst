.. zephyr:board:: tms570_launchxl2

Overview
********

The Texas Instruments Hercules |trade| TMS570LC43x LaunchPad |trade| (LAUNCHXL2-570LC43) is a
development kit for the TMS570LC4357 MCU.

See the `TI LAUNCHXL2-570LC43 Product Page`_ for details.

Hardware
********

This development kit features the TMS570LC4357 MCU, which is a lockstep cached 300MHz
ARM® Cortex®-R5F based TMS570 series automotive-grade MCU designed to aid in the
development of ISO 26262 and IEC 61508 functional safety applications.

The board is equipped with two LEDs, two push buttons and BoosterPack connectors
for expansion. It also includes an integrated (XDS110) debugger.

See the `TI TMS570LC4357 Product Page`_ for additional details.

Supported Features
==================

.. zephyr:board-supported-hw::


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Programming and debugging is supported via either on-board XDS110 debugger, or
via another debugger such as the SEGGER J-Link connected on the 20 pin JTAG
connector.

References
**********

.. target-notes::

.. _TI LAUNCHXL2-570LC43 Product Page: https://www.ti.com/tool/LAUNCHXL2-570LC43

.. _TI TMS570LC4357 Product Page: https://www.ti.com/product/TMS570LC4357
