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
- MIMXRT2663 (i.MX RT2600 series)
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

Converting the model using neutron-compiler
===========================================

After installing the eIQ neutron SDK, the ``neutron-compiler`` tool is located in
the eIQ Neutron SDK directory (for example
``C:\eiq-neutron-sdk-windows-3.2.2\bin``). Add it
to your executable path.

To list the available Neutron targets:

.. code-block:: console

   neutron-compiler --show-targets

Some of the supported targets are:

- ``imxrt700`` : NXP i.MX RT700 MCU
- ``imxrt2660`` : NXP i.MX RT2660 Application Processor
- ``mcxn94x`` : NXP MCX N94x MCU

To convert a quantized TFLite model for the Neutron NPU:

.. code-block:: console

   neutron-compiler --input model_quant.tflite --output model_npu.tflite \
   --target imxrt700 --use-sequencer

The ``neutron-compiler`` takes a TFLite file as input and produces another
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

Alternatively, the ``neutron-compiler`` can generate header files directly
using the ``--dump-header-file-input`` and ``--dump-header-file-output``
arguments.

Synchronizing to this sample
=============================

Copy the generated header files to the appropriate model directory:

- For RT700: ``src/models/rt700/model.hpp``
- For RT2660: ``src/models/rt2660/model.hpp``
- For MCXN: ``src/models/mcxn/model.hpp``

Building and running
********************

Add the tflite-micro module to your West manifest and pull it:

.. code-block:: console

    west config manifest.project-filter -- +tflite-micro
    west update

Fetching NXP Neutron blobs
==========================

This sample requires NXP Neutron NPU driver and firmware binary blobs from the
``hal_nxp`` module. Fetch them before building:

.. code-block:: console

    west blobs fetch hal_nxp

Build the sample for an RT700 board:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/tflm_neutron
   :board: mimxrt798s/mimxrt700_evk
   :goals: build

Then flash the image:

.. code-block:: console

    west flash

Building for RT2660
===================

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/tflm_neutron
   :board: mimxrt2660_evk/mimxrt2663/cm85
   :goals: build

RT2660 memory map
-----------------

The MIMXRT2663 NPU cannot access TCM. The sample uses the
following memory layout:

.. list-table::
   :header-rows: 1
   :widths: 20 18 14 48

   * - Region
     - Address
     - Attribute
     - Contents
   * - ``SRAM01_NPU`` (512 KB)
     - ``0x22000000``
     - non-cacheable
     - Tensor arena and Neutron DMA-shared data
   * - ``SRAM2`` (256 KB)
     - ``0x22080000``
     - cacheable
     - Model, libc malloc arena, and system heap
   * - ``PSRAM`` (32 MB)
     - ``0x80000000``
     - uncached alias
     - Application data, BSS, heap, and stacks

The board overlay combines SRAM0 and SRAM1 into ``SRAM01_NPU``. The
``nocache.ld`` fragment places ``.data.neutron_driver`` in this region, while
the model is copied from flash to SRAM2 before inference.

Running a larger model
----------------------

If the model does not fit in SRAM2, place ``g_model_sram`` in the ``psram``
devicetree region in ``src/main_functions.cpp`` and keep the existing
``cleanCache_by_Addr()`` call. Keep the tensor arena and driver data in
``SRAM01_NPU`` for best performance. If necessary, increase
``kTensorArenaSize`` without exceeding the region's 512 KB capacity.

Building for MCXN
=================

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/tflm_neutron
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
