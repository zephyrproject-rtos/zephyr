.. zephyr:board:: rcar_salvator_xs

Overview
********
The R-Car M3-W is an SOC that features the basic functions for next-generation
car navigation systems.

Hardware
********
The R-Car M3-W includes:

* two 1.5-GHz ARM Cortex-A57 MPCore cores;
* four 1.3-GHz ARM Cortex-A53 MPCore cores,
* memory controller for LPDDR4-3200 with 32 bits x 2 channels;
* 1 channels for HDMI1.4b output and 1 channel for RGB888 output and 1channel for LVDS;
* 2 channels MIPI-CSI2 Video Input, 2 channels digital Video Input;
* USB3.0 x 1ch and USB2.0 x 2ch interfaces;
* 800-MHz ARM Cortex-R7 core;
* two- and three-dimensional graphics engines;
* video processing units;
* sound processing units;
* MediaLB interface;
* SD card host interface;
* USB3.0 and USB2.0 interfaces;
* PCI Express interface;
* CAN interface;
* EtherAVB.

Hardware capabilities for the Salvator-XS for can be found on the `eLinux Salvator-XS page`_.

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

References
**********

- `Renesas R-Car Development Support website`_
- `eLinux Salvator-XS page`_

.. _Renesas R-Car Development Support website:
   https://www.renesas.com/us/en/support/partners/r-car-consortium/r-car-development-support

.. _eLinux Salvator-XS page:
   https://elinux.org/R-Car/Boards/Salvator-XS
