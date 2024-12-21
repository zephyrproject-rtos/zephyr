.. zephyr:code-sample:: dsa
   :name: DSA (Distributed Switch Architecture)
   :relevant-api: DSA

   Test and debug Distributed Switch Architecture

Overview
********

Example on testing/debugging Distributed Switch Architecture

The source code for this sample application can be found at:
:zephyr_file:`samples/net/dsa`.

Requirements
************

- :ref:`networking_with_host`

Building and Running
********************

Host machine with :zephyr:board:`ip_k66f` board from Segger.

Follow these steps to build the DSA sample application:

.. zephyr-app-commands::
   :zephyr-app: samples/net/dsa
   :board: <board to use>
   :conf: prj.conf
   :goals: build
   :compact:
