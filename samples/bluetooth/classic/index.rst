.. zephyr:code-sample-category:: bluetooth_classic
   :name: Bluetooth Classic
   :show-listing:
   :glob: **/*

   These samples demonstrate the use of Bluetooth Classic in Zephyr.

Prerequisites
*************

* A board with Bluetooth Classic support
* Recommended boards:

  * :zephyr:board:`mimxrt1170_evk@B/mimxrt1176/cm7 <mimxrt1170_evk>`

Building the Samples
*********************

To build any Bluetooth Classic sample, use the following commands:

.. code-block:: bash

   west build -p auto -b <board> samples/bluetooth/classic/<sample_name>

For example, to build for :zephyr:board:`mimxrt1170_evk@B/mimxrt1176/cm7 <mimxrt1170_evk>`:

.. code-block:: bash

   west build -p auto -b mimxrt1170_evk@B/mimxrt1176/cm7 samples/bluetooth/classic/<sample_name>

Refer to :ref:`bluetooth-dev` for more information.

Configuration
*************

The following Kconfig options must be enabled for Bluetooth Classic functionality:

.. code-block:: cfg

   CONFIG_BT=y
   CONFIG_BT_CLASSIC=y

Enable verbose Bluetooth logging:

.. code-block:: cfg

   CONFIG_LOG=y
   CONFIG_LOG_DEFAULT_LEVEL=<log level>

Running the Samples
*******************

Flashing
========

Flash the built sample to your target board:

.. code-block:: bash

   west flash

Monitoring Output
=================

Monitor the sample output using a serial terminal.

.. zephyr:code-sample-listing::
   :categories: bluetooth_classic
   :live-search:
