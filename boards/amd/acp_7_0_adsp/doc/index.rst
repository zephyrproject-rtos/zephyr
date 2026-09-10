.. SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: acp_7_0_adsp

Overview
********

ACP 7.0 is Audio co-processor in AMD SoC based on HiFi5 DSP Xtensa Architecture,
Zephyr OS is ported to run various audio and speech use cases on
the SOF based framework.

SOF can be built with either Zephyr or XCC toolchain and run on a ACP 7.0 AMD platforms.

Hardware
********

- Board features:

  - RAM: 2.5MB HP SRAM & 512KB configurable IRAM/DRAM
  - Audio Interfaces:

      - 1 x SDW,
      - 1 x HS (I2S, PCM),
      - DMIC

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The ACP 7.0 SoC operates with an audio clock frequency ranging from 200 to 800 MHz.

System requirements
*******************

Xtensa Toolchain (optional)
===========================

The Zephyr SDK provides GCC-based toolchains necessary to build Zephyr for
the AMD ACP boards. For users looking for higher optimization levels,
building with the proprietary Xtensa toolchain from Cadence
might be preferable.

The following instructions assume you have purchased and
installed the toolchain(s) and core(s) for your board following
instructions from Xtensa documentation.

If you choose to build with the Xtensa toolchain instead of the Zephyr SDK, set
the following environment variables specific to the board in addition to the
Xtensa toolchain environment variable listed below.

First, make sure, the necessary license is available from
Cadence and set the license variables as per the instruction from Cadence.
Next, set the following environment variables:

The bottom three variables are specific to acp_7_0.

.. code-block:: shell

   export XTENSA_TOOLCHAIN_PATH="tools installed path"
   export XTENSA_BUILDS_DIR="user build directory path"
   export ZEPHYR_TOOLCHAIN_VARIANT=Zephyr GCC
   export TOOLCHAIN_VER=RI-2023.11-linux
   export XTENSA_CORE=ACP_7_0_HiFi5_NNE_PROD



Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Building
========

Build as usual.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: acp_7_0_adsp/acp_7_0
   :goals: build

Flashing
========

AMD supports only signed images flashing on ACP 7.0 platforms
through ACP Linux Driver.

The following boot sequence messages can be observed in dmesg

   -  booting DSP firmware
   -  ACP_DSP0_RUNSTALL : 0x0
   -  ipc rx: 0x70000000
   -  Firmware info: version 2:14:99-b5961
   -  Firmware: ABI 3:29:1 Kernel ABI 3:23:0
   -  mailbox upstream 0x0 - size 0x400
   -  mailbox downstream 0x400 - size 0x400
   -  stream region 0x1000 - size 0x400
   -  debug region 0x800 - size 0x400
   -  fw_state change: 3 -> 6
   -  ipc rx done: 0x70000000
   -  firmware boot complete
