.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

PI4IOE5V6408 interrupt tests
##########################

The I2C emulator models reset interrupt acknowledgment, input transitions
away from register 09h, masking and read-to-clear interrupt status. Its
active-low output drives a GPIO emulator, which schedules the real driver
on the system workqueue. A second instance exercises polling.

Faults can occur before a transaction takes effect or after the emulated
device applies it. Recovery must not depend on another physical input
edge or repeat an already reported transition. Configuration-error tests
check software subscriptions, not atomic rollback of hardware writes.

These tests do not verify electrical timing, reset timing, or silicon
behavior when the input default changes. Transitions shorter than the
service interval cannot be reconstructed from the current input alone.
