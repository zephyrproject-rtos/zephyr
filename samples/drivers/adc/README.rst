.. _adc-sample:

Analog-to-Digital Converter (ADC)
#################################

Overview
********

This sample demonstrates how to use the ADC driver API.

Depending on the target board, it reads ADC samples from one or more channels
and prints the readings on the console. If voltage of the used reference can
be obtained, the raw readings are converted to millivolts.

The pins of the ADC channels are board-specific. Please refer to the board
or MCU datasheet for further details.

Building and Running
********************

The ADC peripheral and pinmux is configured in the board's ``.dts`` file. Make
sure that the ADC is enabled (``status = "okay";``).

In addition to that, this sample requires an ADC channel specified in the
``io-channels`` property of the ``zephyr,user`` node. This is usually done with
a devicetree overlay. The example overlay in the ``boards`` subdirectory for
the ``nucleo_l073rz`` board can be easily adjusted for other boards.

Configuration of channels (settings like gain, reference, or acquisition time)
also needs to be specified in devicetree, in ADC controller child nodes. Also
the ADC resolution and oversampling setting (if used) need to be specified
there. See :zephyr_file:`boards/nrf52840dk_nrf52840.overlay
<samples/drivers/adc/boards/nrf52840dk_nrf52840.overlay>` for an example of
such setup.

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

   ADC reading:
   - ADC_0, channel 7: 36 = 65mV

.. note:: If the ADC is not supported, the output will be an error message.
