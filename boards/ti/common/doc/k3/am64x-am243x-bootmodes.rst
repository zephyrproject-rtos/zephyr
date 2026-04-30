..
   SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
   SPDX-FileCopyrightText: Copyright 2025 - 2026 Siemens Mobility GmbH
   SPDX-FileCopyrightText: Copyright 2025 Texas Instruments
   SPDX-License-Identifier: Apache-2.0

:orphan:

.. ti-k3-am64x-am243x-bootmodes

The AM64x/AM243x bootflow
-------------------------

The AM64x/AM243x SoC series has it's own bootflow due to supporting secure boot.
After powering on the SoC an immutable bootloader in ROM (called RBL) is
executed on R5F0_0. Based on the MCU+ SDK from Texas Instruments then a Second
Stage Bootloader (called SBL) should be executed which then boots the final
Zephyr firmware. The RBL can get it's image from various sources, such as
external flash or transmitted via UART. An alternative only on AM64x series SoCs
it to use a premade image from a TI SDK that acts as SBL, starts Linux on the
Cortex-A and then use remoteproc to load a Zephyr firmware onto another core.
