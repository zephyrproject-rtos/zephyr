.. zephyr:code-sample:: tflite-hello-world
   :name: Hello World

   Replicate a sine wave using TensorFlow Lite for Microcontrollers.

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

The sample comes in two flavors. One with TensorFlow Lite Micro
reference kernels and one with CMSIS-NN optimized kernels.

.. Note::
   This README and sample have been modified from
   `the TensorFlow Hello World sample for Zephyr`_.

.. _the TensorFlow Hello World sample for Zephyr:
   https://github.com/tensorflow/tflite-micro/tree/main/tensorflow/lite/micro/examples/hello_world

Building and Running
********************

The sample should work on most boards since it does not rely
on any sensors.

Add the tflite-micro module to your West manifest and pull it:

.. code-block:: console

    west config manifest.project-filter -- +tflite-micro
    west update

The reference kernel application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/hello_world
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

The CMSIS-NN kernel application can be built and executed on any Arm(R) Cortex(R)-M core based platform,
for example based on Arm Corstone(TM)-300 software. A reference implementation of Corstone-300
can be downloaded either as a FPGA bitfile for the
[MPS3 FPGA prototyping board](https://developer.arm.com/tools-and-software/development-boards/fpga-prototyping-boards/mps3),
or as a
[Fixed Virtual Platform](https://developer.arm.com/tools-and-software/open-source-software/arm-platforms-software/arm-ecosystem-fvps)
that can be emulated on a host machine.

Assuming that the Corstone-300 FVP has been downloaded, installed and added to
the `PATH` variable, then building and testing can be done with following
commands.

```
$ west build -p auto -b mps3/an547 samples/modules/tflite-micro/hello_world/ -T sample.tensorflow.helloworld.cmsis_nn
$ FVP_Corstone_SSE-300_Ethos-U55 build/zephyr/zephyr.elf
```

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

.. code-block:: cfg

    CONFIG_CPP=y
    CONFIG_REQUIRES_FULL_LIBC=y
    CONFIG_TENSORFLOW_LITE_MICRO=y

Note that the CMSIS-NN kernel sample demonstrates how to use CMSIS-NN optimized kernels with
TensorFlow Lite Micro, in that is sets below Kconfig option. Note also that this
Kconfig option is only set for Arm Cortex-M cores, i.e. option CPU_CORTEX_M is set.

.. code-block:: cfg

    CONFIG_TENSORFLOW_LITE_MICRO_CMSIS_NN_KERNELS=y

Training
********
Follow the instructions in the :file:`train/` directory to train your
own model for use in the sample.
