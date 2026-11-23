.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

MCUX ADC16 ISR tests
###################

This suite compiles the ADC16 driver and ADC context with a mock ADC16 HAL on
``native_sim/native/64``. It injects ISR calls through ``irq_offload()`` and uses
the real Zephyr semaphores, poll signals, and sampling callbacks. It checks idle
interrupt handling, channel sequencing, disarming before the callback,
immediate repeat and extra sampling, and asynchronous completion.

Run it from a configured Zephyr west workspace:

.. code-block:: console

   west twister -p native_sim/native/64 -T tests/drivers/adc/mcux_adc16 -i

The injected idle interrupt tests containment of an unexpected entry state.
They do not establish a hardware source for that interrupt. The mock does not
simulate ADC timing, shared group interrupts, or DMA. Hardware triggering and
EDMA remain disabled; their supported path does not use this ISR.
