.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

STM32N6 CPU clock scaling
########################

Test exact IC1 CPU rates, invalid requests, repeated requests, and transitions
from threads and timer interrupts. Check CPU clock metadata, fixed clock domains,
and timer progress. The stress scenario runs 10000 transitions.

Require ``CONFIG_CLOCK_STM32_N6_CPU_SCALING`` and an independent LPTIM1 timer.
The overlay configures a 600 MHz boot rate from PLL1 through IC1 divider four.

Run on either supported board, for example::

   west twister -p nucleo_n657x0_q/stm32n657xx/sb -T tests/drivers/clock_control/stm32n6_cpu_clock
