.. _ncp-sample:

OpenThread NCP
##############

Overview
********

OpenThread NCP allows building a Thread Border Router. The code in this
sample is only the MCU target part of a complete Thread Border Router.
The Linux tools from https://openthread.io/guides/border-router
(especially wpantund and wpanctl) are required to get a complete Thread
Border Router.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/openthread/ncp`.

Building and Running
********************

Build the OpenThread NCP sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/ncp
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

There are configuration files for different boards and setups in the
ncp directory:

- :file:`prj.conf`
  Generic config file, normally you should use this.

- :file:`overlay-tri-n4m-br.conf`
  This is an overlay for the dedicated Thread Border Router hardware
  https://www.tridonic.com/com/en/products/net4more-borderROUTER-PoE-Thread.asp.
  The board support is not part of the Zephyr repositories, but the
  product is based on NXP K64 and AT86RF233. This file can be used as an
  example for a development set-up based on development boards.
