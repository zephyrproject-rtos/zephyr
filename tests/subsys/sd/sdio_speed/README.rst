.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

SDIO bus speed tests
####################

These tests exercise SDIO bus speed selection and CMD52 readback with a fake
SDHC device. The test includes the complete subsystem implementation to call
its private speed helpers, using the normal Zephyr structures and SDHC API.
The device callbacks check command encoding and return controlled card data.

The SDIO stack stays disabled by default to avoid a second copy of its
implementation.
The SDMMC stack supplies the common SD configuration, helpers and log module.
The tests cover native mode. Initialization cases call the real SDIO initializer
with a bus already at negotiated 1.8 V; they do not exercise physical switching.

Run all three retry configurations with:

.. code-block:: console

   west twister -p native_sim/native/64 -T tests/subsys/sd/sdio_speed

A retry count of N allows an initial write followed by at most N retries.
The delayed-success and second-write-error cases are skipped at zero retries.

Literal readback cases cover persistent rejection with 0x01, immediate SDR50
acceptance with 0x05 and final-attempt SDR104 acceptance with 0x07. Separate
upper-bit cases verify preservation of non-BSS bits across retries.

Initialization cases cover rejected and accepted SDR50/SDR104 requests and a
tuning error. The fake SDHC responds to CMD5, CMD3, CMD7, CCCR reads, an END-only
CIS chain and four-bit width negotiation. Rejected speed changes retain the
width update and 25 MHz/SDR12 timing without tuning. Accepted modes still tune,
and tuning errors propagate through the real initializer.
