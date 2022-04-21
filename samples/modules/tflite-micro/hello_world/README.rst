.. _tensorflow_hello_world:

TensorFlow Lite Micro Hello World sample
########################################

Overview
********

This sample TensorFlow application replicates a sine wave and
demonstrates the absolute basics of using TensorFlow Lite Micro.

The model included with the sample is trained to replicate a
sine function and generates x values to print alongside the
y values predicted by the model. The x values iterate from 0 to
an approximation of 2π.

The sample also includes a full end-to-end workflow of training
a model and converting it for use with TensorFlow Lite Micro for
running inference on a microcontroller.

.. Note::
   This README and sample have been modified from
   `the TensorFlow Hello World sample for Zephyr`_.

.. _the TensorFlow Hello World sample for Zephyr:
   https://github.com/tensorflow/tflite-micro/tree/main/tensorflow/lite/micro/examples/hello_world

Building and Running
********************

This sample should work on most boards since it does not rely
on any sensors.

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tensorflow/hello_world
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Sample Output
=============

.. code-block:: console

    ...

    x_value: 1.0995567*2^1, y_value: 1.6951603*2^-1

    x_value: 1.2566366*2^1, y_value: 1.1527088*2^-1

    x_value: 1.4137159*2^1, y_value: 1.1527088*2^-2

    x_value: 1.5707957*2^1, y_value: -1.0849024*2^-6

    x_value: 1.7278753*2^1, y_value: -1.0509993*2^-2

    ...

The modified sample prints 10 generated-x-and-predicted-y pairs. To see
the full period of the sine curve, increase the number of loops in :file:`main.c`.

Modifying Sample for Your Own Project
*************************************

It is recommended that you copy and modify one of the two TensorFlow
samples when creating your own TensorFlow project. To build with
TensorFlow, you must enable the below Kconfig options in your :file:`prj.conf`:

.. code-block:: kconfig

    CONFIG_CPLUSPLUS=y
    CONFIG_NEWLIB_LIBC=y
    CONFIG_TENSORFLOW_LITE_MICRO=y

Training
********
Follow the instructions in the :file:`train/` directory to train your
own model for use in the sample.
