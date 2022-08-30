.. _sink-sample:

Basic USBC SINK
###############

Overview
********

This example demonstrates how to create a USBC Power Delivery application
using a TypeC Port Controller (TCPC) driver. The application implements a
USBC Sink device.

After the USBC Sink device is powered, an LED begins to blink and
when the USBC Sink device is plugged into a Power Delivery charger, it
negotiates with the charger to provide 5V@100mA and displays all
Power Delivery Objects (PDOs) provided by the charger.

Please note that this example does not implement any of the features and
requirements outlined in the USBC Specification needed to create a robust
USBC Sink device. It is meant for demonstration purposes only.

.. _sink-sample-requirements:

Requirements
************

The TCPC device that's used by the sample is specified by defining a devicetree
node label named ``tcpc``.
The sample has been tested on :ref:`stm32g081b_eval_board` and provides an
overlay file for both board.

For the :ref:`stm32g081b_eval_board`, Port 2 is configured as a Sink and Port 1
is unused. So, the charger must be plugged into Port 2.

Building and Running
********************

Build and flash Sink as follows, changing ``stm32g081b_eval_board`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/usbc/sink
   :board: stm32g081b_eval_board
   :goals: build flash
   :compact:

After flashing, the LED starts to blink. Connect a charger and see console output:

Sample Output
=============

.. code-block:: console

 UnAttached.SNK
 AttachedWait.SNK
 Attached.SNK
 Got PD_CTRL_ACCEPT
 Got PD_CTRL_PS_RDY
 Source Caps:
 PDO 0:
        Type:     FIXED
        DRP:      0
        Suspend:  0
        UP:       1
        USB Comm: 0
        DRD:      0
        Voltage:  5000 mV
        Current:  2400 mA
 PDO 1:
        Type:     FIXED
        DRP:      0
        Suspend:  0
        UP:       0
        USB Comm: 0
        DRD:      0
        Voltage:  9000 mV
        Current:  3000 mA
 PDO 2:
        Type:     FIXED
        DRP:      0
        Suspend:  0
        UP:       0
        USB Comm: 0
        DRD:      0
        Voltage:  20000 mV
        Current:  3000 mA
 SNK_READY
