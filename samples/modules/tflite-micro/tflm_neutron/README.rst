.. zephyr:code-sample:: tflite-neutron
   :name: TensorFlow Lite for Microcontrollers on NXP Neutron

   Run an inference using a MobileNet model on the NXP Neutron NPU.

Overview
********

A sample application that demonstrates how to run an image classification
inference using the TFLM framework and the NXP Neutron NPU.

The sample application runs a MobileNet-based model that has been converted to a
TFLite flatbuffer. The model is compiled into the application as a C header
array. The Neutron NPU accelerates supported operators via a custom
``NEUTRON_GRAPH`` operator registered in the TFLM ``MicroMutableOpResolver``.

Supported Platforms
*******************

This sample supports NXP platforms with the Neutron NPU:

- MIMXRT798S (i.MX RT700 series)
- FRDM-MCXN947 (MCXN series)

A platform-specific model is selected at build time via the CMakeLists.txt.

Generating Neutron-converted model
**********************************

Follow the steps below to convert a quantized TFLite model for the Neutron NPU
and generate the header files used by this sample.

Software requirements
=====================

- `eIQ Toolkit <https://www.nxp.com/design/software/development-software/eiq-ml-development-environment/eiq-toolkit-for-end-to-end-model-development:EIQ-TOOLKIT>`_
  (available for Windows and Linux)
- GCC, Python, and Git dependencies

Converting the model using neutron-converter
=============================================

After installing the eIQ Toolkit, the ``neutron-converter`` tool is located in
the eIQ Toolkit installation directory (for example
``C:\NXP\eIQ_Toolkit_v1.15.1\bin\neutron-converter\MCU_SDK_25.03.00``). Add it
to your executable path.

To convert a quantized TFLite model for the Neutron NPU:

.. code-block:: console

   neutron-converter --input model_quant.tflite --output model_npu.tflite \
   --target imxrt700 --use-sequencer

The ``neutron-converter`` takes a TFLite file as input and produces another
TFLite file as output, where the operators supported by the Neutron NPU have
been replaced by a ``NeutronGraph`` custom operator. Any layers that were not
converted run on the Cortex-M33 core.

Converting to C array
=====================

Use ``xxd`` to convert the TFLite model and input/output data to C header files:

.. code-block:: console

   xxd -c 16 -i model_npu.tflite model_npu.tflite.h
   xxd -c 16 -i input_data.bin input_data.h
   xxd -c 16 -i output_data.bin output_data.h

Alternatively, the ``neutron-converter`` can generate header files directly
using the ``--dump-header-file-input`` and ``--dump-header-file-output``
arguments.

Synchronizing to this sample
=============================

Copy the generated header files to the appropriate model directory:

- For RT700: ``src/models/rt700/model.hpp``
- For MCXN: ``src/models/mcxn/model.hpp``

Building and running
********************

Add the tflite-micro module to your West manifest and pull it:

.. code-block:: console

    west config manifest.project-filter -- +tflite-micro
    west update

Fetching NXP Neutron blobs
--------------------------

This sample requires NXP Neutron NPU driver and firmware binary blobs from the
``hal_nxp`` module. Fetch them before building:

.. code-block:: console

    west blobs fetch hal_nxp

Build the sample for an RT700 board:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/tflm_neutron
   :board: mimxrt798s/mimxrt700_evk
   :goals: build

Then flash the image:

.. code-block:: console

    west flash

Building for MCXN
-----------------

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/tflm_neutron
   :board: frdm_mcxn947/mcxn947
   :goals: build

Sample output
*************

The application prints the top 5 classification results with confidence
percentages to the console:

.. code-block:: text

   === TFLM NXP Neutron Starting ===

   Inference #1
   ----------------------------------------
   Running inference...
   Inference complete

   Top 5 Results:
   ----------------------------------------
   1. ship                           99.61%
   2. automobile                      0.00%
   3. bird                            0.00%
   4. cat                             0.00%
   5. deer                            0.00%
   ----------------------------------------

   Inference complete!
