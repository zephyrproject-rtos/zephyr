.. SPDX-License-Identifier: Apache-2.0
.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors

Cortex-A/R floating point context switching
##########################################

Exercises ``USE_SWITCH`` with FPU sharing enabled on the Cortex-A9 and Cortex-R5
QEMU boards with SMP disabled. The preemption test requires both threads to
share one CPU. Cortex-R5 covers the D16 register bank; Cortex-A9 also covers
d16-d31. This suite does not claim SMP or user-mode FP coverage.
Reusing a thread object must clear its old callee FP registers.
Cooperative switches preserve d8-d15 and FPSCR rounding control.
A higher-priority thread wakes twice from timer sleeps while the interrupted
thread spins without kernel calls; all implemented FP registers and FPSCR must
survive those preemptions. Another case disables the current thread's VFP,
switches to an FP-using peer, and checks that its first FP instruction enables
the VFP through the undefined-instruction retry path. Explicit FP enabling
continues to return ``-ENOTSUP``, as specified by the Cortex-A/R implementation.
