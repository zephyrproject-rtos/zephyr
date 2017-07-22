.. _msp_exp432p401r_launchxl:

MSP-EXP432P401R LaunchXL
########################

Overview 
********

The SimpleLink MSP‐EXP432P401R LaunchPad development kit is an easy-to-use evaluation
module for the SimpleLink MSP432P401R microcontroller. It contains everything needed to start
developing on the SimpleLink MSP432 low-power + performance ARM |reg| 32-bit Cortex |reg|-M4F
microcontroller (MCU). The MSP432P401R device supports low-power applications requiring increased CPU
speed, memory, analog, and 32-bit performance.

.. figure:: img/msp_exp432p401r_launchxl.jpg
     :width: 1032px
     :align: center
     :height: 1663px
     :alt: MSP-EXP432P401R LaunchXL development board

Features:
=========

* Low-power ARM Cortex-M4F MSP432P401R
* 40-pin LaunchPad development kit standard that leverages the BoosterPack plug-in module ecosystem
* XDS110-ET, an open-source onboard debug probe featuring EnergyTrace+ technology and application
  UART
* Two buttons and two LEDs for user interaction
* Backchannel UART through USB to PC

Details on the MSP-EXP432P401R LaunchXL development board can be found in the
`MSP-EXP432P401R LaunchXL User's Guide`_.

Supported Features
==================

* The on-board 32-kHz crystal allows for lower LPM3 sleep currents and a higher-precision clock source than the
  default internal 32-kHz REFOCLK. Therefore, the presence of the crystal allows the full range of low-
  power modes to be used.
* The on-board 48-MHz crystal allows the device to run at its maximum operating speed for MCLK and HSMCLK.

The MSP-EXP432P401R LaunchXL development board configuration supports the following hardware features:

+-----------+------------+-----------------------+
| Interface | Controller | Driver/Component      |
+===========+============+=======================+
| NVIC      | on-chip    | nested vectored		 |
|						 | interrupt controller	 |
+-----------+------------+-----------------------+
| SYSTICK   | on-chip    | system clock          |
+-----------+------------+-----------------------+
| UART   	| on-chip    | serial port           |
+-----------+------------+-----------------------+

More details about the supported periperals are available in `MSP432P4XX TRM`_
Other hardware features are not currently supported by the Zephyr kernel.

Building and Flashing
*********************

Building
========

Follow the :ref:`getting_started` instructions for Zephyr application
development.

To build for the MSP-EXP432P401R LaunchXL:

.. code-block:: console
	
	$ make -C samples/hello_world BOARD=msp_exp432p401r_launchxl

This will produce zephyr.elf which could be flashed onto MSP-EXP432P401R LaunchXL
using the command line utility mentioned below.

Flashing
========

For Linux:
----------

`UniFlash`_ command line utility could be used to program the flash memory. Only
elf loading is supported as of now.

The following command has been known to work:

.. code-block:: console

   % ./dslite.sh --config=MSP432P401R.ccxml zephyr.elf

.. note:: ccxml file is included in the MSP432 SDK

References
**********

TI MSP432 Wiki:
   https://en.wikipedia.org/wiki/TI_MSP432

TI MSP432P401R Product Page:
   http://www.ti.com/product/msp432p401r

TI MSP432 SDK:
   http://www.ti.com/tool/SIMPLELINK-MSP432-SDK

.. _MSP-EXP432P401R LaunchXL User's Guide:
   http://www.ti.com/lit/ug/slau597c/slau597c.pdf

.. _MSP432P4XX TRM:
   http://www.ti.com/lit/ug/slau356f/slau356f.pdf

.. _UniFlash:
   http://processors.wiki.ti.com/index.php/UniFlash_v4_Quick_Guide#Command_Line_Interface
