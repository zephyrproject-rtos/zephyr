.. zephyr:board:: acp_6_0_adsp

Overview
********

ACP 6.0 is Audio co-processor in AMD SoC based on HiFi5 DSP Xtensa Architecture,
Zephyr OS is ported to run various audio and speech use cases on
the SOF based framework.

SOF can be built with either Zephyr or Cadence's proprietary
Xtensa OS (XTOS) and run on a ACP 6.0 AMD platforms.

Hardware
********

- Board features:

  - RAM: 1.75MB HP SRAM & 512KB configurable IRAM/DRAM
  - Audio Interfaces:

      - 1 x SP (I2S, PCM),
      - 1 x BT (I2S, PCM),
      - 1 x HS (I2S, PCM),
      - DMIC

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The ACP 6.0 SoC operates with an audio clock frequency ranging from 200 to 800 MHz.

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

The bottom three variables are specific to acp_6_0.

.. code-block:: shell

   export XTENSA_TOOLCHAIN_PATH="tools installed path"
   export XTENSA_BUILDS_DIR="user build directory path"
   export ZEPHYR_TOOLCHAIN_VARIANT=xcc
   export TOOLCHAIN_VER=RI-2019.1-linux
   export XTENSA_CORE=LX7_HiFi5_PROD

Programming and Debugging
*************************

Building
========

Build as usual.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: acp_6_0_adsp/acp_6_0
   :goals: build

Flashing
========

AMD supports only signed images flashing on ACP 6.0 platforms
through ACP Linux Driver.

The following boot sequence messages can be observed in dmesg

   -  booting DSP firmware
   -  ACP_DSP0_RUNSTALL : 0x0
   -  ipc rx: 0x70000000
   -  Firmware info: version 2:11:99-03a9d
   -  Firmware: ABI 3:29:1 Kernel ABI 3:23:0
   -  mailbox upstream 0x0 - size 0x400
   -  mailbox downstream 0x400 - size 0x400
   -  stream region 0x1000 - size 0x400
   -  debug region 0x800 - size 0x400
   -  fw_state change: 3 -> 6
   -  ipc rx done: 0x70000000
   -  firmware boot complete
