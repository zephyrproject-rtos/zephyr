.. _ti-io-exp:

TI I/O Expander
################

Overview
********

This snippet enables I/O expander functionality on TI boards


Supported boards
****************

- AM62L EVM

Usage
*****

To enable I/O expander functionality, add this snippet to your build:

.. code-block:: console

   west build -b am62l_evm/am62l3/a53 -S ti-io-exp samples/drivers/pwm/capture
