.. zephyr:code-sample:: tflite-ethosu
   :name: TensorFlow Lite for Microcontrollers on Arm Ethos-U

   Run an inference using an optimized TFLite model on Arm Ethos-U NPU.

Overview
********

A sample application that demonstrates how to run an inference using the TFLM
framework and the Arm Ethos-U NPU.

The sample application runs a model that has been downloaded from the
`Arm model zoo <https://github.com/ARM-software/ML-zoo>`_. This model has then
been optimized using the
`Vela compiler <https://git.mlplatform.org/ml/ethos-u/ethos-u-vela.git>`_.

Vela takes a tflite file as input and produces another tflite file as output,
where the operators supported by Ethos-U have been replaced by an Ethos-U custom
operator. In an ideal case the complete network would be replaced by a single
Ethos-U custom operator.

Building and running
********************

Add the tflite-micro module to your West manifest and pull it:

.. code-block:: console

    west config manifest.project-filter -- +tflite-micro
    west update

This application can be built and run on any Arm Ethos-U capable platform, for
example Corstone(TM)-300. A reference implementation of Corstone-300 can be
downloaded either as a FPGA bitfile for the
`MPS3 FPGA prototyping board <https://developer.arm.com/tools-and-software/development-boards/fpga-prototyping-boards/mps3>`_,
or as a
`Fixed Virtual Platform <https://developer.arm.com/tools-and-software/open-source-software/arm-platforms-software/arm-ecosystem-fvps>`_
that can be emulated on a host machine.

Assuming that the Corstone-300 FVP has been downloaded, installed and added to
the ``PATH`` variable, then building and testing can be done with following
commands.

.. code-block:: bash

    $ west build -b mps3/corstone300/fvp zephyr/samples/modules/tflite-micro/tflm_ethosu
    $ FVP_Corstone_SSE-300_Ethos-U55 build/zephyr/zephyr.elf
