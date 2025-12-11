.. _p3t1755dp_ard_i3c_shield:

P3T1755DP Arduino® Shield Evaluation Board
##########################################

Overview
********

P3T1755DP is a ±0.5 °C accurate temperature-to-digital converter
with a -40 °C to 125 °C range.

.. figure:: p3t1755dp_ard.webp
   :align: center
   :alt: P3T1755DP ARD

Requirements
************

The temperature register always stores a 12 bit two's complement data,
giving a temperature resolution of 0.0625 °C. P3T1755DP can be configured
for different operation conditions: continuous conversion, one-shot mode
or shutdown mode. The device supports 2-wire serial I3C (up to 12.5 MHz)
and I²C (up to 3.4 MHz) as communication interface.

For more information about P3T1755DP-ARD see these NXP documents:

- `Getting Started with the P3T1755DP-ARD Evaluation Board`_
- `P3T1755DP-ARD Evaluation Board User Manual`_

Hardware Connection
*******************
- Shield board p3t1755dp_ard in I3C mode
   J10, J11, J12 3-5, the i3c addr is 0x4800000236152a0090
   JP2, Jp3 1-2
   I3C is from J13
   The VDD from arduino is 3v3 JP1 2-3

- Shield board p3t1755dp_ard in I2C mode
   J10, J11, J12 3-5, the i2c addr is 0x48
   JP2, Jp3 2-3
   The I2C is from Arduino J5 pin9(SCL_ARD) pin10(SDA_ARD)
   The VDD from arduino is 3v3 JP1 2-3

Programming
***********

Set ``--shield p3t1755dp_ard_i3c`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/thermometer
   :board: mimxrt1180_evk/mimxrt1189/cm33
   :shield: p3t1755dp_ard_i3c
   :goals: build

.. _Getting Started with the P3T1755DP-ARD Evaluation Board:
   https://www.nxp.com/document/guide/getting-started-with-the-p3t1755dp-ard-evaluation-board:GS-P3T1755DP-ARD

.. _P3T1755DP-ARD Evaluation Board User Manual:
   https://www.nxp.com/docs/en/user-manual/UM11834.pdf
