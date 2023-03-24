.. _hello_bindesc-sample:

hello_bindesc Sample Application
################################

Overview
********

A simple sample of binary descriptor definition and usage.

Building and Running
********************

Follow these steps to build the `hello_bindesc` sample application:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/bindesc/hello_bindesc
   :board: <board to use>
   :goals: build
   :compact:

To see all binary descriptors, run:

.. code-block:: bash

   west bindesc dump build/zephyr/zephyr.bin
