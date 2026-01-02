<!--
SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: Copyright (c) 2025 Synopsys, Inc.
-->

# UART Echo - RISC-V AIA Demo

Demonstrates RISC-V AIA (Advanced Interrupt Architecture) using UART interrupts.

## Overview

Shows the interrupt flow: UART -> APLIC -> MSI -> IMSIC -> CPU

- Configures APLIC source routing
- Routes UART interrupt via MSI to IMSIC
- Handles interrupts and echoes typed characters

## Building

```bash
west build -p -b qemu_riscv32/qemu_virt_riscv32_aia samples/drivers/interrupt_controller/intc_riscv_aia
```

## Running

```bash
west build -t run
```

Type characters to see them echoed. Press Ctrl+A, X to exit QEMU.

## Configuration

- **Board**: qemu_riscv32/qemu_virt_riscv32_aia
- **UART IRQ**: APLIC source 10
- **Target EIID**: 32
- **Mode**: Edge-triggered MSI delivery
