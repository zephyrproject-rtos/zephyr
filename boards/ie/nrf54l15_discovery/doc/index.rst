.. zephyr:board:: ie_nrf54l15_discovery

Overview
********

The nRF54L15 Discovery is a slim (21.0x91.2mm) Nordic Semiconductor
nRF54L15 SoC development board with an onboard nRF52820 CMSIS-DAP
debugger that can also be broken away. The module consists of a
main nRF54L15 board with 29 I/O ports (plus 2 PMIC I/O ports) and
a nRF52820 daughterboard with 15 I/O ports. The main board contains
a nPM1300 PMIC, allowing for easy battery charging and power management.

The voltage is variable between 1.8V and 3.3V, with the voltage of each
half of the board capable of being controlled individually. But for
CMSIS-DAP to work, the voltage of the two halfs must match.

The board can also be broken into 2 individual modules by snapping it at
the mousebites. Each part can then function individually, albeit for the
daughterboard the solderjumpers at the bottom right must be bridged in
order for USB-C power to work.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LED
---

* LED0 = P1.04, supports PWM. Needs power from nPM1300 LDO2

BUTTONS
-------

* BUTTON0 = P1.09

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``nrf54l15_discovery/nrf54l15/ns`` board target can be
built in the usual way (see :ref:`build_an_application` for more details).

Flashing
========

The daughter board is factory-programmed with Nordics's IFMCU CMSIS-DAP

#. Compile a Zephyr application; we'll use :zephyr:code-sample:`blinky`.

   .. zephyr-app-commands::
      :app: zephyr/samples/basic/blinky
      :board: nrf64l15_discovery/nrf55l15/ns
      :goals: build

#. Flash it onto the board. You may need to mount the device.

   .. code-block:: console

      west flash -r pyocd

   When this command exits, observe the red LED on the board blinking,

Debugging
=========

You may debug this board using the 3 broken out swd holes.
PyOCD and openOCD can be used to flash and debug this board.
