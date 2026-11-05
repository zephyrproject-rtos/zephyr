.. SPDX-License-Identifier: Apache-2.0
.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors

ISR table registration and device interrupts
############################################

This test compares linked table entries with C references to their original
handlers and parameters. It covers arrays, generated switches, shared clients
and runtime installation in writable arrays.

The timer test waits for three periodic expirations and checks ISR context,
timer identity and user data. The UART scenarios enable TX interrupts and send
a byte through the console device. They require a callback in ISR context with
the original device and user-data pointers. A level-triggered controller can
already have another notification pending when TX interrupts are disabled, so
the UART test requires at least one callback and prints the observed count.

The shared UART scenarios register a second client on the same device IRQ. This
client only records ISR context, its argument and the number of observations;
the UART driver acknowledges the device. Both clients must observe a real IRQ.
The APBUART driver also issues a synchronous bootstrap notification when TX is
started; the test waits for the device IRQ and does not count that notification.

OpenRISC's system timer has a separate trap entry. Use its UART scenarios to
exercise the generated table through the peripheral interrupt controller.

Run, for example, with a toolchain and QEMU installation supporting the board::

   python scripts/twister -T tests/arch/common/isr_table_runtime \
     -s arch.isr_table_runtime.array.static -p qemu_arc/qemu_arc_hs

   python scripts/twister -T tests/arch/common/isr_table_runtime \
     -s arch.isr_table_runtime.uart.switch.shared -p qemu_or1k

The registered inspection IRQs are never enabled or raised. On RISC-V they are
local CPU IRQs, rather than the final flat indices of an aggregated table. Entry
lookups use the common IRQ-to-table mapping, including the configured offset
and interrupt-controller aggregation.
