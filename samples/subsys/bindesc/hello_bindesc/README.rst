.. zephyr:code-sample:: hello-bindesc
   :name: Binary descriptors "Hello World"
   :relevant-api: bindesc_define

   Set and access binary descriptors for a basic Zephyr application.

Overview
********

A simple sample of :ref:`binary descriptor <binary_descriptors>` definition and usage.

Building and Running
********************

Follow these steps to build the ``hello_bindesc`` sample application:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/bindesc/hello_bindesc
   :board: <board to use>
   :goals: build
   :compact:

To dump all binary descriptors in the image, run:

.. code-block:: bash

   west bindesc dump build/zephyr/zephyr.bin

(Note: you can also dump the contents of ``zephyr.elf``, if your build system
does not produce a ``*.bin`` file, e.g. compiling for ``native_posix``.)

For more details see :ref:`binary_descriptors` and :ref:`west-bindesc`.
