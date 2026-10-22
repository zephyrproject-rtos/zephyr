.. zephyr:code-sample:: npm10xx_uicr
   :name: nPM10xx UICR

   Demonstration of the UICR programming sequence for Nordic's nPM10 Series PMICs.

Overview
********

Nordic's nPM10 Series PMICs feature one-time-programmable UICR (User Information Configuration
Registers). The UICR is a set of non-volatile bits that define the reset/default values for some of
the PMIC's control registers. This allows more flexible and efficient designs by eliminating the
need for some of the run-time software intervention during the startup sequence.

Requirements
************

This sample can run on any target featuring an I2C/TWI bus through which it can communicate with an
nPM10 Series PMIC. An overlay for :zephyr:board:`nrf54l15dk` is included for reference assuming it
is wired to an **nPM1012 EK** in the following way:

.. list-table:: nRF54L15 DK and nPM1012 EK wiring
   :header-rows: 1

   * - nPM1012 EK pins
     - nRF54L15 DK pins
   * - SDA
     - P1.11
   * - SCL
     - P1.12
   * - VOUT
     - P6 VDDM current measure, VDD:nRF pin
   * - GND
     - GND

In addition to that, the following must be done on the EK:

- Disconnect all USB cables
- On the **BUCK SET** header connect VSET to either 1.8V or 3.0V with a jumper
- On the **VDDIO REF** header connect VDDIO to BUCK with a jumper
- On header **P1** connect VBAT and VBATIN with a jumper
- Connect a suitable battery to the **VBAT** connector

Building and Running
********************

Build and flash as follows, replacing ``nrf54l15dk/nrf54l15/cpuapp`` with your board:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/otp/npm10xx_uicr
   :board: nrf54l15dk/nrf54l15/cpuapp
   :goals: build flash
   :compact:

Testing
*******

.. note::

   Dry-run programming (:kconfig:option:`CONFIG_OTP_NPM10XX_DRY_RUN`) is enabled by default for
   safety. Remember to follow the datasheet guidelines on supply voltages and junction temperature
   if you want to actually burn some UICR bits.

Upon flashing you should be able to see the following output in the target's console:

.. code-block:: console

   [00:00:00.008,488] <wrn> main: This is a dry run - nothing will actually get programmed.
   [00:00:00.017,227] <wrn> main: VBUS connection is required for UICR programming to proceed.

After supplying VBUS (connecting a USB cable to the **USB PMIC** port on the EK), the programming
will proceed automatically. During the dry run every bit index to be burned is logged:

.. code-block:: console

   [00:00:08.536,627] <inf> main: VBUS connected, parameters within range, starting UICR programming...
   [00:00:08.546,227] <inf> otp_npm10xx: Dry run: would program UICR bit 6
   [00:00:08.553,975] <inf> otp_npm10xx: Dry run: would program UICR bit 9
   <...>

Once the driver completes programming of all required bits, the following message will be printed:

.. code-block:: console

   [00:00:08.655,851] <inf> main: UICR programming success. Remove VBUS...

As soon as VBUS is removed, the MCU will issue a SW reset to the PMIC which will also power cycle
the MCU itself.

.. code-block:: console

   [00:00:11.666,473] <inf> main: VBUS disconnected, resetting PMIC...
