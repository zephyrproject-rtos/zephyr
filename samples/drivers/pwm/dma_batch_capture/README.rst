.. zephyr:code-sample:: pwm-dma-batch-capture
   :name: PWM DMA batch capture
   :relevant-api: pwm_interface

   Capture batches of PWM samples using DMA.

Overview
********

This sample demonstrates the DMA-based batch capture feature of the PWM API
with :kconfig:option:`CONFIG_PWM_CAPTURE_BATCH`. Instead of handling
individual edges, the timer capture register is streamed by DMA into a
circular buffer. In the STM32 implementation used by this sample, each buffer
is split into two equal halves. When one half is ready, a callback processes
those samples while DMA fills the other half.

The sample uses two PWM channels: one to generate a test signal, and another to capture it.

Requirements
************

This sample requires a board with DMA-capable PWM timers.


Sample Output
=============

.. code-block:: console

   batch of 256 samples: avg period: 2328 cycles, avg frequency: 6185 Hz, avg duty cycle: 69 %
   batch of 256 samples: avg period: 2331 cycles, avg frequency: 6177 Hz, avg duty cycle: 69 %
   batch of 256 samples: avg period: 2357 cycles, avg frequency: 6109 Hz, avg duty cycle: 69 %
   batch of 256 samples: avg period: 2360 cycles, avg frequency: 6101 Hz, avg duty cycle: 69 %
   <repeats with triangular frequency sweep>
