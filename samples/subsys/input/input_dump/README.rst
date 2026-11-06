.. zephyr:code-sample:: input-dump
   :name: Input dump
   :relevant-api: input_events

   Print all input events.

Overview
********

The Input Dump sample prints any input event using the :ref:`input` APIs.

Requirements
************

The samples works on any board with an input driver defined in the board devicetree.

Building and Running
********************

Build and flash as follows, changing ``nrf52dk/nrf52832`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/input/input_dump
   :board: nrf52dk/nrf52832
   :goals: build flash
   :compact:

After starting, the sample will print any input event in the console.

USB HID boot devices
=====================

To use a USB HID boot protocol keyboard or mouse with the SparkFun MAX3421E shield, build with
the shield and the additional configuration fragment:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/input/input_dump
   :board: stm32h747i_disco/stm32h747xx/m7
   :shield: sparkfun_max3421e
   :gen-args: -DEXTRA_CONF_FILE=usb_hid_boot.conf
   :goals: build flash
   :compact:
