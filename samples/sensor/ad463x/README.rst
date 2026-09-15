.. zephyr:code-sample:: ad463x
   :name: AD463x precision SAR ADC

   Capture conversion data from an AD463x ADC over the AXI offload data path.

Overview
********

This sample captures conversion data from an Analog Devices AD463x family
precision SAR ADC (AD4630-x / AD4631-x / AD4632-x) and prints decoded codes
for both channels to the console.

The AD463x is an offload-mode part: every sample is pushed through a hardware
data path with no per-sample CPU cost::

   PWMGEN (CNV trigger) -> SPI Engine (offload) -> DMAC -> DDR

The sample uses the driver's primary interface, :c:func:`ad463x_read_buffer`,
which arms one DMA transfer and blocks until the requested number of samples
has been captured. It first prints one 64-sample batch and then streams
conversion data continuously.

If an AXI System ID core is present in the design, the sample also prints the
FPGA build information (board, product and git hash) decoded from its ROM.

Requirements
************

This sample targets the :ref:`Avnet ZedBoard <zedboard>` with an AD4630-20 FMC
card and the matching AD4630 reference HDL design, which synthesizes the AXI
SPI Engine, DMAC, CLKGEN, PWMGEN and System ID cores in the PL.

The devicetree overlay in :file:`boards/zedboard.overlay` describes these
peripherals. Adapt ``adi,part`` and ``adi,output-mode`` there to match your
part and output mode; the sample reads the resulting real-bit precision from
the driver at runtime.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/ad463x
   :board: zedboard
   :goals: build flash
   :compact:

Sample Output
*************

.. code-block:: console

   FPGA design: AD4630 / ad4630_fmc (git 0123456789ab)

   === Batch capture (64 samples) ===
   channel 0    channel 1
   12043        -3391
   12050        -3402
   ...

   === Continuous stream ===
   channel 0    channel 1
   12047        -3398
   ...
