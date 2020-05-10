.. _adc-sample:

Analog-to-Digital Converter (ADC)
#################################

Overview
********

This sample demonstrates how to use the ADC driver API.

Depending on the MCU type, it reads the ADC samples of one or two ADC channels
and prints the readings to the console. If supported by the driver, the raw
readings are converted to millivolts.

The pins of the ADC channels are board-specific. Please refer to the board
or MCU datasheet for further details.

.. note::

   This sample does not work on Nordic platforms where there is a distinction
   between channel and analog input that requires additional configuration. See
   :zephyr_file:`samples/boards/nrf/battery` for an example of using the ADC
   infrastructure on Nordic hardware.


Building and Running
********************

The ADC peripheral and pinmux is configured in the board's ``.dts`` file. Make
sure that the ADC is enabled (``status = "okay";``).

In addition to that, this sample requires an ADC channel specified in the
``io-channels`` property of the ``zephyr,user`` node. This is usually done with
a devicetree overlay. The example overlay in the ``boards`` subdirectory for
the :ref:`nucleo_l073rz_board` can be easily adjusted for other boards.

Building and Running for ST Nucleo L073RZ
=========================================

The sample can be built and executed for the
:ref:`nucleo_l073rz_board` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/adc
   :board: nucleo_l073rz
   :goals: build flash
   :compact:

To build for another board, change "nucleo_l073rz" above to that board's name
and provide a corresponding devicetree overlay.

Sample output
=============

You should get a similar output as below, repeated every second:

.. code-block:: console

   ADC reading(s): 42 (raw)

.. note:: If the ADC is not supported, the output will be an error message.
