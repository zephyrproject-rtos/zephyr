.. zephyr:code-sample:: read-bindesc
   :name: Binary descriptors read
   :relevant-api: bindesc_read

   Define some binary descriptors and read them.

Overview
********

A simple sample of :ref:`binary descriptor <binary_descriptors>` definition and reading.

Building and Running
********************

Follow these steps to build the ``read_bindesc`` sample application:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/bindesc/read_bindesc
   :board: <board to use>
   :goals: build
   :compact:

For more details see :ref:`binary_descriptors` and :ref:`west-bindesc`.
