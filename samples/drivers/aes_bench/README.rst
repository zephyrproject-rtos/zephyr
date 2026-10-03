.. zephyr:code-sample:: aes-bench
   :name: AES HW benchmark
   :relevant-api: crypto_cipher

   Measure throughput and CPU load of the crypto cipher API implementation.

Overview
********

This sample benchmarks :ref:`crypto cipher API <crypto_api>` throughput
(cycles/byte, MB/s) and CPU load for AES-256-CBC across a range of message
sizes, on top of whichever crypto cipher driver is enabled on the board. It
is meant to illustrate what a hardware AES accelerator brings compared to a
software-only implementation, and to compare polling vs. interrupt-driven
driver implementations.

On drivers with an interrupt-driven mode (e.g. STM32 CRYP), the fixed cost
of an interrupt round trip is paid on every call regardless of message
size: negligible for large messages (freed-up CPU time with no throughput
cost), but a measurable throughput hit for small ones (a few tens of bytes)
where that fixed cost dominates.

Building and Running
*********************

This project outputs to the console. It can be built and executed as
follows, replacing ``<board>`` with a board that has a crypto cipher driver
enabled (for example ``stm32mp135f_dk``):

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/aes_bench
   :board: <board>
   :goals: build flash
   :compact:

To benchmark the software-only (mbedTLS) implementation instead of a
hardware accelerator, build with the ``prj_mtls_shim.conf`` overlay. This
also works on ``native_sim``, to compare against a board with a hardware
accelerator:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/aes_bench
   :board: native_sim
   :gen-args: -DEXTRA_CONF_FILE=prj_mtls_shim.conf
   :goals: build run
   :compact:

The console banner reports which implementation ran: ``AES HW benchmark``
for a hardware accelerator, ``AES SW benchmark`` for the mbedTLS shim.

Sample Output
=============

.. code-block:: console

    AES HW benchmark
    BENCH AES256-CBC len=64 loops=2000 cycles=... cpb=... MBps=...
    ...
    BENCH AES256-CBC len=65520 loops=40 cycles=... cpb=... MBps=...
