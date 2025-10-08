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

Generating Vela-compiled model
******************************

Follow the steps below to generate Vela-compiled model and test input/output data.
Use `keyword_spotting_cnn_small_int8`_ model in this sample:

.. _keyword_spotting_cnn_small_int8: https://github.com/Arm-Examples/ML-zoo/tree/master/models/keyword_spotting/cnn_small/model_package_tf/model_archive/TFLite/tflite_int8

.. note:: The default Vela-compiled model is to target Ethos-U55 and 128 MAC
   on MPS3 target. Because one model can add up to hundreds of KB, don't
   attempt to add more models into code base for other targets.

1. Downloading the files below from `keyword_spotting_cnn_small_int8`_:

   - cnn_s_quantized.tflite
   - testing_input/input/0.npy
   - testing_output/identity/0.npy

2. Optimizing the model for Ethos-U using Vela

   Assuming target Ethos-U is U55 and 128 MAC:

   .. code-block:: console

       $ vela cnn_s_quantized.tflite \
       --output-dir . \
       --accelerator-config ethos-u55-128 \
       --system-config Ethos_U55_High_End_Embedded \
       --memory-mode Shared_Sram

3. Removing unnecessary header

   ``testing_input/input/0.npy`` and ``testing_output/0.npy`` have 128-byte header.
   They must be removed for integration with this sample.

   .. code-block:: console

       $ dd if=testing_input/input/0.npy of=testing_input/input/0_no-header.npy bs=1 skip=128
       $ dd if=testing_output/identity/0.npy of=testing_output/identity/0_no-header.npy bs=1 skip=128

4. Converting to C array

   .. code-block:: console

       $ xxd -c 16 -i cnn_s_quantized.tflite cnn_s_quantized.tflite.h
       $ xxd -c 16 -i cnn_s_quantized_vela.tflite cnn_s_quantized_vela.tflite.h
       $ xxd -c 16 -i testing_input/input/0_no-header.npy testing_input/input/0_no-header.npy.h
       $ xxd -c 16 -i testing_output/identity/0_no-header.npy testing_output/identity/0_no-header.npy.h

5. Synchronizing to this sample

   Synchronize the files below to ``keyword_spotting_cnn_small_int8`` directory
   in this sample:

   - cnn_s_quantized_vela.tflite.h > model.h
   - testing_input/input/0_no-header.npy.h > input.h
   - testing_output/identity/0_no-header.npy.h > output.h

   .. note:: To run non-Vela-compiled model (``CONFIG_TAINT_BLOBS_TFLM_ETHOSU=n``),
      synchronize ``cnn_s_quantized.tflite.h`` instead.

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
