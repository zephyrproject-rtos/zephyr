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

On the RT2660 (MIMXRT2663) the Neutron NPU can only access the on-chip SRAM
and the external PSRAM; it cannot reach the TCMs or execute-in-place flash.
The NPU also DMAs directly into the tensor arena and reads the Neutron
driver's DMA-shared data without any cache maintenance, so both of those must
live in a non-cacheable region to stay coherent with the CPU.

This sample uses the following layout (see
``boards/mimxrt2660_evk_mimxrt2663_cm85.overlay`` and ``nocache.ld``):

.. list-table::
   :header-rows: 1
   :widths: 20 15 12 15 38

   * - Region
     - Address
     - Size
     - Attribute
     - Contents
   * - ``SRAM01_NPU`` (SRAM0+SRAM1)
     - ``0x22000000``
     - 512 KB
     - non-cacheable
     - Tensor arena the NPU DMAs into, and the Neutron driver's DMA-shared
       data (``.data.neutron_driver``).
   * - ``SRAM2``
     - ``0x22080000``
     - 256 KB
     - cacheable
     - Model (microcode/weights) copied out of flash, the libc malloc arena
       and the k_heap pool. Read-only or CPU-only, so cacheable is fine.
   * - ``PSRAM``
     - ``0x80000000``
     - 32 MB
     - (chosen sram)
     - Application data/BSS/heap/stack. Also reachable by the NPU, so it can
       hold a large model or, if needed, the tensor arena and driver data, at
       a lower access speed than on-chip SRAM.

The model is placed in the fast on-chip ``SRAM2`` to get the best inference
performance. Both the CPU and the NPU then read the model from tightly
coupled on-chip RAM instead of external XSPI flash. The NPU can also reach the
PSRAM, so any of these buffers can be moved there when the on-chip SRAM is too
small, trading throughput for capacity.

Running a larger model
----------------------

``SRAM2`` is only 256 KB and it also holds the libc malloc arena and the
k_heap pool, so a model larger than what fits there must be placed in the
32 MB PSRAM instead. The PSRAM is reachable by both the CPU and the NPU, so
this trades access speed for capacity: the same option exists for the tensor
arena and the driver data if the 512 KB ``SRAM01_NPU`` region runs out.
Steps:

#. Point the model buffer at the PSRAM region instead of ``SRAM2``. In
   ``src/main_functions.cpp``, the RT266x branch defines::

      #define SRAM2_SECTION \
          __attribute__((section(LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(sram2)))))
      ...
      static uint8_t __ALIGNED(16) SRAM2_SECTION g_model_sram[sizeof(g_model)];

   Change the section to the PSRAM node so the copy target lands in PSRAM::

      #define MODEL_SECTION \
          __attribute__((section(LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(psram)))))
      ...
      static uint8_t __ALIGNED(16) MODEL_SECTION g_model_sram[sizeof(g_model)];

   The existing ``setup()`` code already copies ``g_model`` into
   ``g_model_sram`` and cleans the cache over it with ``cleanCache_by_Addr()``;
   keep that call so the NPU sees the copied model.

#. Keep the tensor arena and the Neutron driver data in the non-cacheable
   ``SRAM01_NPU`` region for best performance. They can be moved to PSRAM if
   the on-chip SRAM is too small, but PSRAM access is slower, so inference
   throughput drops. Either way they must stay non-cacheable and coherent with
   the NPU: the PSRAM window used here (``0x80000000``) is deliberately the
   uncached alias (nothing manages LLC coherency in tree), so a non-cacheable
   MPU region over it keeps the arena and driver data coherent.

#. If the model no longer needs ``SRAM2``, you can grow the system heap and the
   libc malloc arena in ``boards/mimxrt2660_evk_mimxrt2663_cm85.conf``
   (``CONFIG_HEAP_MEM_POOL_SIZE`` and ``CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE``)
   to use the freed SRAM2, as long as the totals still fit in its 256 KB.

#. Enlarge the tensor arena for the bigger model if needed by raising
   ``kTensorArenaSize`` in ``src/main_functions.cpp``, keeping it within the
   512 KB ``SRAM01_NPU`` region.

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
