..
   SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
   SPDX-FileCopyrightText: Copyright (c) 2026 Texas Instruments Incorporated
   SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: lp_am13e230

Overview
********

The AM13E230x LaunchPad is a development board based on the AM13E230x SoC, a 200MHz ARM Cortex M33 real-time motor control MCU with edge AI, TMU and 512KB on-chip flash.

See the `TI AM13E230x Product Page`_ for details.

|

Hardware
********

* Processor:

  + Arm Cortex M33 @ 200MHz

* Memory:

  + 128KB SRAM
  + 512KB on-chip flash

* Debug:

  + XDS110 based JTAG

Details present in `AM13E230x Microcontrollers datasheet`_

|

Supported Features
******************

.. zephyr:board-supported-hw::

|

Flashing and Debugging
======================

OpenOCD support patches have been submitted to the OpenOCD Gerrit: https://review.openocd.org/c/openocd/+/9525 (see "Relation chain" for all commits).

Building a patched OpenOCD
--------------------------

OpenOCD can be built with AM13E230x support by following these steps:

1. Clone ``https://github.com/openocd-org/openocd``

2. Apply patches::

    git pull https://review.openocd.org/openocd refs/changes/25/9525/1
    git pull https://review.openocd.org/openocd refs/changes/26/9526/1
    git pull https://review.openocd.org/openocd refs/changes/27/9527/2

3. Initialize submodules: ``git submodule update --init``

4. ``./bootstrap && ./configure --enable-internal-jimtcl``

5. Build: ``make -j$(nproc)``

6. Launch OpenOCD: ``src/openocd -s tcl -f tcl/board/ti/am13e230x-launchpad.cfg``

Flashing and debugging using West
---------------------------------

``west flash`` can be used with the patched OpenOCD to flash a Zephyr application by providing the paths to the OpenOCD binary and TCL directory.::

    ``west flash --openocd ~/openocd/src/openocd --openocd-search ~/openocd/tcl``

``west debug`` can also be used with the same arguments for debugging using GDB.

|

References
**********

*   `TI AM13E230x Product Page`_

*   `AM13E230x Microcontrollers datasheet`_

*   `AM13E230x OpenOCD support patch`_

.. _TI AM13E230x Product Page:
    https://www.ti.com/product/AM13E23019

.. _AM13E230x Microcontrollers datasheet:
    https://www.ti.com/lit/gpn/am13e23019

.. _AM13E230x OpenOCD support patch:
   https://github.com/TexasInstruments/asm-hal_ti/blob/master/am13/openocd_am13e230x_support.patch
