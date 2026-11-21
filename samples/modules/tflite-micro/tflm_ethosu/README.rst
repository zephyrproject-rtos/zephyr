.. zephyr:code-sample:: tflite-ethosu
   :name: TensorFlow Lite for Microcontrollers on Arm(R) Ethos(TM)-U55, U65, and U85 NPUs

   Run an inference using an optimized TFLite model on Arm Ethos-U NPU.

Overview
********

A sample application that demonstrates how to run an inference using the TFLM
framework and the Arm Ethos-U NPU.

Testing the TFLM integration, the Ethos-U driver and the NPU does not need a
large model. The sample runs two int8 test models of a few KB each instead, and
compares each output with the one computed by ``src/models/generate.py``:

* ``dft``: the 16-point `discrete Fourier transform`_ of a noisy two-tone
  signal, as one fully connected layer. Its output is requantized with rounding,
  so a match also checks that the NPU rounds like TFLite. The int8 scheme is
  defined in the `LiteRT 8-bit quantization specification`_.
* ``life``: four generations of `Conway's Game of Life`_ on an 8x8 board, as a
  chain of eight convolutions with ReLU. The input glider moves one cell
  diagonally. `Springer and Kenyon`_ show how n generations map to a small
  convolutional network.

.. _discrete Fourier transform: https://en.wikipedia.org/wiki/Discrete_Fourier_transform
.. _LiteRT 8-bit quantization specification: https://ai.google.dev/edge/litert/models/quantization_spec
.. _Conway's Game of Life: https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life
.. _Springer and Kenyon: https://arxiv.org/abs/2009.01398

Each model was compiled with the
`Vela compiler <https://gitlab.arm.com/artificial-intelligence/ethos-u/ethos-u-vela>`_.
Vela takes a tflite file as input and produces another tflite file as output,
where the operators supported by Arm Ethos-U NPU have been replaced by an Ethos-U custom
operator. In an ideal case the complete network would be replaced by a single
Ethos-U custom operator, which is the case for both models.

Generating the models
*********************

``src/models/generate.py`` builds both models, computes their expected output with
TFLite's int8 arithmetic, and compiles them with Vela. It writes one header per
model holding the plain model, used when ``CONFIG_TAINT_BLOBS_TFLM_ETHOSU=n``, and
one Vela-compiled variant per NPU configuration:

* Arm Ethos-U55 NPU with 128 MACs (``CONFIG_ETHOS_U55_128``), such as Corstone-300
* Arm Ethos-U85 NPU with 256 MACs (``CONFIG_ETHOS_U85_256``), such as
  Corstone-1000-A320

The Vela-compiled variants use the ``Shared_Sram`` memory mode. Other NPU
configurations or memory modes need an entry in ``NPU_CONFIGS`` or a change to
``MEMORY_MODE`` in the script. The script needs Vela, which also provides the
TFLite schema it uses:

.. code-block:: console

   $ pip install ethos-u-vela
   $ python3 src/models/generate.py

Building and running
********************

Add the tflite-micro module to your West manifest and pull it:

.. code-block:: console

    west config manifest.project-filter -- +tflite-micro
    west update

This application can be built and run on any Arm Ethos-U NPU capable platform, such
as Corstone(TM)-300 or Corstone-320.

Run target prerequisites
------------------------

When using the CMake ``run`` target (``-t run``), set the FVP binary path for
your platform and, for Arm Ethos-U85 NPU, pass the NPU configuration to the CLI:

.. code-block:: bash

    # Arm Ethos-U55/U65 NPU (Corstone-300 FVP)
    export ARMFVP_BIN_PATH=/path/to/FVP_Corstone_SSE-300_<ver>/models/Linux64_GCC-<gcc>/

    # Arm Ethos-U85 (Corstone-320 FVP)
    export ARMFVP_BIN_PATH=/path/to/FVP_Corstone_SSE-320_<ver>/models/Linux64_GCC-<gcc>/
    export ARMFVP_EXTRA_FLAGS='-C;mps4_board.subsystem.ethosu.num_macs=2048'  # match chosen ETHOS_U85_* Kconfig

Build and run on the FVP
------------------------

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/tflm_ethosu
   :board: mps3/corstone300/fvp
   :goals: run
   :gen-args: -DCONFIG_SAMPLE_TFLM_ETHOSU_MEM_MODE_SHARED_SRAM=y
              -DCONFIG_ETHOS_U55_128=y

Vela memory modes and overlays
******************************

Vela supports several Arm Ethos NPU memory placement modes. This sample reproduces
those at build time using ``CONFIG_SAMPLE_TFLM_ETHOSU_MEM_MODE_*``:

* ``CONFIG_SAMPLE_TFLM_ETHOSU_MEM_MODE_SHARED_SRAM``
* ``CONFIG_SAMPLE_TFLM_ETHOSU_MEM_MODE_DEDICATED_SRAM``
* ``CONFIG_SAMPLE_TFLM_ETHOSU_MEM_MODE_SRAM_ONLY``

**Board overlays**

Use overlays to place TensorFlow Lite buffers in matching memory regions:

- ``boards/mps3_corstone300_fvp.sram_only.overlay``
- ``boards/mps4_corstone320_fvp.sram_only.overlay``
- ``boards/mps3_corstone300_fvp.dedicated_sram.overlay``
- ``boards/mps4_corstone320_fvp.dedicated_sram.overlay``

``Dedicated_Sram`` additionally requires an ``ETHOSU_FAST`` region declared in
the overlay.

**Example builds**

Shared_Sram (default):

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/tflm_ethosu
   :board: mps3/corstone300/fvp
   :goals: run
   :gen-args: -DCONFIG_SAMPLE_TFLM_ETHOSU_MEM_MODE_SHARED_SRAM=y
              -DCONFIG_ETHOS_U55_128=y

Sram_Only (Arm Ethos-U65 NPU on Corstone-300):

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/tflm_ethosu
   :board: mps3/corstone300/fvp
   :goals: run
   :gen-args: -DCONFIG_SAMPLE_TFLM_ETHOSU_MEM_MODE_SRAM_ONLY=y
              -DCONFIG_ETHOS_U65_256=y
              -DDTC_OVERLAY_FILE=boards/mps3_corstone300_fvp.sram_only.overlay

Dedicated_Sram (Arm Ethos-U85 NPU on Corstone-320):

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/tflm_ethosu
   :board: mps4/corstone320/fvp
   :goals: run
   :gen-args: -DCONFIG_SAMPLE_TFLM_ETHOSU_MEM_MODE_DEDICATED_SRAM=y
              -DCONFIG_ETHOS_U85_2048=y
              -DDTC_OVERLAY_FILE=boards/mps4_corstone320_fvp.dedicated_sram.overlay

.. note::

   - **Dedicated_Sram** is not supported on Arm Ethos-U55 NPU.
   - Ensure the model's Vela mode matches the selected build mode.

Memory regions
**************

The Arm Ethos-U NPU command streams address memory regions using **region indices** (0-3),
which correspond to ``REGIONCFG`` values in the NPU configuration.
Vela assigns these indices when compiling the model, and the HAL maps
each index to a physical AXI interface or MEM_ATTR entry depending on the NPU.

The **mapping used in this sample** is defined by the selected
``CONFIG_SAMPLE_TFLM_ETHOSU_MEM_MODE_*`` option. The table below shows the
region index assignments implemented by this sample's CMake and Kconfig logic:

=================  =======  =======  ============  ============
Mode               weights  scratch  fast_scratch  cmd_stream
=================  =======  =======  ============  ============
Shared_Sram        3        0        0             3
Dedicated_Sram     3        3        0             3
Sram_Only          0        0        0             0
=================  =======  =======  ============  ============

These index values are applied through ``zephyr_compile_definitions()`` as
``NPU_REGIONCFG_n`` and ``NPU_QCONFIG`` macros during the build, ensuring the
HAL and driver use a consistent region configuration:

- ``NPU_REGIONCFG_0``: weights
- ``NPU_REGIONCFG_1``: scratch
- ``NPU_REGIONCFG_2``: fast scratch
- ``NPU_QCONFIG``: command stream region

These indices map to hardware interfaces as follows:

**Architectural mapping**

- **Arm Ethos-U55/U65 NPU:**
  Regions 0-1 route to AXI0 (on-chip SRAM), 2-3 to AXI1 (external memory).
  This mapping is fixed by the architecture.

- **Arm Ethos-U85 NPU:**
  Regions 0-3 select programmable ``MEM_ATTR[0-3]`` entries.
  By default, the HAL configures MEM_ATTR[0-1] to map to SRAM and MEM_ATTR[2-3]
  to external DDR. These defaults can be overridden if the system uses a custom
  memory topology.

When using **Dedicated_Sram**, ensure an ``ETHOSU_FAST`` node exists in the
devicetree overlay to provide a valid region for the NPU's fast_scratch area.


Configuring PMU Events via CMake
********************************

If ``CONFIG_SAMPLE_TFLM_ETHOSU_PMU`` is enabled, the sample reports
per-inference PMU counters. You can override which events are counted by setting
``ETHOSU_PMU_EVENT_0..3`` (and 4-7 if supported).

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/tflm_ethosu
   :board: mps3/corstone300/fvp
   :goals: run
   :gen-args: -DCONFIG_SAMPLE_TFLM_ETHOSU_MEM_MODE_SHARED_SRAM=y
              -DCONFIG_SAMPLE_TFLM_ETHOSU_PMU=y
              -DETHOSU_PMU_EVENT_0=ETHOSU_PMU_AXI0_RD_DATA_BEAT_RECEIVED
              -DETHOSU_PMU_EVENT_1=ETHOSU_PMU_AXI1_RD_DATA_BEAT_RECEIVED

PMU report format
*****************

When enabled, the sample prints a short PMU report after each inference:

.. code-block:: text

   Ethos-U PMU report:
   ethosu_pmu_cycle_cntr : 134869
   ethosu_pmu_cntr0 : 133577
   ethosu_pmu_cntr1 : 0
   ethosu_pmu_cntr2 : 111744
   ethosu_pmu_cntr3 : 0
   # (if supported, counters 4..7 are also printed.)


Timing Adapters (TA)
********************

The Timing Adapters allows simulation of memory bandwidth and latency limits on
supported platforms (FVP or FPGA). Apply TA overlays explicitly with
``-DDTC_OVERLAY_FILE=...``. TA overlays can be combined with any Vela memory
mode to explore timing effects.

For complete TA documentation, see the
`Evaluation Kit docs <https://gitlab.arm.com/artificial-intelligence/ethos-u/ml-embedded-evaluation-kit/-/blob/main/docs/sections/timing_adapters.md>`_.

TA overlay with Sram_Only (Corstone-320)
----------------------------------------

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/tflm_ethosu
   :board: mps4/corstone320/fvp
   :goals: run
   :gen-args: -DCONFIG_SAMPLE_TFLM_ETHOSU_MEM_MODE_SRAM_ONLY=y
              -DCONFIG_ETHOS_U85_2048=y
              -DDTC_OVERLAY_FILE="boards/mps4_corstone320_fvp.ta.overlay;boards/mps4_corstone320_fvp.sram_only.overlay"

TA overlay only (Corstone-300)
------------------------------

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/tflm_ethosu
   :board: mps3/corstone300/fvp
   :goals: run
   :gen-args: -DCONFIG_SAMPLE_TFLM_ETHOSU_MEM_MODE_SHARED_SRAM=y
              -DCONFIG_ETHOS_U55_128=y
              -DDTC_OVERLAY_FILE=boards/mps3_corstone300_fvp.ta.overlay


Trademarks
***********

Arm, Ethos, and Corstone are registered trademarks or trademarks of Arm Limited
(or its subsidiaries or affiliates) in the US and/or elsewhere.

TensorFlow, the TensorFlow logo and any related marks are trademarks of Google Inc.
