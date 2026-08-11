.. zephyr:board:: frdm_imxrt700

Overview
********

The FRDM-IMXRT700 is a compact, low-cost development board based on the NXP
i.MX RT700 crossover MCU. The i.MX RT700 is a multicore device composed of a
high-performance main-compute subsystem and a secondary "always-on"
sense-compute subsystem, together with specialized coprocessors.

- The main-compute subsystem has a primary Arm® Cortex®-M33 (CPU0) and an
  integrated Cadence® Tensilica® HiFi 4 DSP.
- The sense-compute subsystem has a second Arm® Cortex®-M33 (CPU1) and an
  integrated Cadence® Tensilica® HiFi 1 DSP.

This Zephyr board port currently provides baseline support for the two
Cortex-M33 cores (``cm33_cpu0`` and ``cm33_cpu1``).

Hardware
********

- MIMXRT798S SoC:

  - Arm Cortex-M33 (CPU0) main-compute core
  - Arm Cortex-M33 (CPU1) sense-compute core
  - HiFi 4 and HiFi 1 DSP cores (not supported by this port)

- On-board MCU-Link debug probe with UART, SWD and SPI/I2C bridging
- Quad SPI flash for code storage (XSPI0)
- RGB user LED and user buttons
- Arduino-compatible and MikroBUS expansion headers

For more information about the i.MX RT700 SoC, see the `i.MX RT700 Website`_.

Supported Features
==================

.. zephyr:board-supported-hw::

The ``frdm_imxrt700`` board targets enable the following features in this
baseline port:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port (debug console)         |
+-----------+------------+-------------------------------------+

Additional peripherals will be enabled in follow-up work.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

This board is flashed and debugged using a J-Link debug probe, for example the
on-board MCU-Link programmed with J-Link firmware, or an external J-Link.

.. note::

   LinkServer is not supported yet. The current LinkServer release has no
   FRDM-IMXRT700 board definition and ships only an octal flashloader for the
   i.MX RT700 XSPI, so it cannot program the board's on-board quad SPI NOR
   flash. Once a LinkServer release adds FRDM-IMXRT700 support, the supported
   version will be recorded here and the LinkServer flashing/debugging steps
   added.

Building
========

The debug console of CPU0 is routed to FLEXCOMM0 (LPUART0) through the
MCU-Link bridge. CPU1 uses FLEXCOMM19 (LPUART19).

Build and the :zephyr:code-sample:`hello_world` sample for CPU0:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_imxrt700/mimxrt798s/cm33_cpu0
   :goals: build flash

Build the :zephyr:code-sample:`blinky` sample for CPU0:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: frdm_imxrt700/mimxrt798s/cm33_cpu0
   :goals: build flash

To target the sense-compute core (CPU1), use the
``frdm_imxrt700/mimxrt798s/cm33_cpu1`` board target.

References
**********

.. target-notes::

.. _i.MX RT700 Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-processors/i-mx-rt-crossover-mcus/i-mx-rt700-crossover-mcu-with-arm-cortex-m33-npu-dsp-and-gpu-cores:i.MX-RT700
