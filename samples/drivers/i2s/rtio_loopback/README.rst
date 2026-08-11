.. zephyr:code-sample:: i2s-rtio-loopback
   :name: I2S RTIO loopback
   :relevant-api: rtio i2s_interface

   Perform I2S transaction using RTIO, looping back the data for validation.

Overview
********

This sample demonstrates how to perform I2S transactions using RTIO, with I2S devices acting as
controller or target. The sample supports two setups:

Requirements
************

* A single I2S device supporting bidirectional transfers, with its TX output looped back to its RX
  input.
* Two I2S devices, one acting as controller and doing write transfers, the other acting as target,
  doing a read transfers, connected to each other.

.. note::

   Remember to connect the wire up the relevant I2S controllers physically.

Board support
*************

Any board which meets the requirements must use an overlay to specify which I2S devices which will
be used for the sample. This is done using the devicetree by configuring the I2S devices and
specifying them by their role, using the following devicetree aliases:

* ``sample-i2s-txrx``: The single I2S device which will perform bidirectional transfers.
* ``sample-i2s-rx``: The I2S device which will act as target, performing read transfers.
* ``sample-i2s-tx``: The I2S device which will act as controller, performing write transfers.

For example:

.. code-block:: devicetree

   / {
           aliases {
                   sample-i2s-txrx = &i2s20;
           };
   };

If necessary, add any board specific configs to the board specific conf fragment.

Sample configurations
*********************

The sample can be configured to align with device capabilities, or to simply test various
configurations, using the following sample specific configuration options:

* :kconfig:option:`CONFIG_SAMPLE_BUFFER_COUNT`
* :kconfig:option:`CONFIG_SAMPLE_WORD_SIZE_8`
* :kconfig:option:`CONFIG_SAMPLE_WORD_SIZE_16`
* :kconfig:option:`CONFIG_SAMPLE_FRAME_CLK_FREQ`
* :kconfig:option:`CONFIG_SAMPLE_FRAME_COUNT`
* :kconfig:option:`CONFIG_SAMPLE_STREAM_COUNT`
* :kconfig:option:`CONFIG_SAMPLE_STREAM_BUFFER_COUNT`
* :kconfig:option:`CONFIG_SAMPLE_SIGNAL_FREQ`
* :kconfig:option:`CONFIG_SAMPLE_SIGNAL_ATTENUATION`
