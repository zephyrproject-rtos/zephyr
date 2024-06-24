.. _tmp1075_sample:

TMP1075 sample
#############

Description
***********

This sample writes the temperature to the console once every 1 second.
If TMP1075_ALERT_INTERRUPTS is enabled then it is possible to use interrupts
configured by dts entry. See app.overlay and adjust the i2c and alert pins.

Requirements
************

A board with the :dtcompatible:`ti,tmp1075` built in to its :ref:`devicetree <dt-guide>`,
or a devicetree overlay with such a node added.

