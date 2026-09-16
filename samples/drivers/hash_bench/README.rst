.. zephyr:code-sample:: hash-bench
   :name: Hash HW benchmark
   :relevant-api: crypto_hash

   Measure throughput and CPU load of the crypto hash API implementation.

Overview
********

This sample benchmarks :ref:`crypto hash API <crypto_api>` throughput
(cycles/byte, MB/s) and CPU load for SHA-256 and SHA-3-512, across a range of
message sizes, on top of whichever crypto hash driver is enabled on the
board. It is meant to illustrate what a hardware hash accelerator brings
compared to a software-only implementation, and to compare polling vs.
interrupt-driven driver implementations.

Building and Running
*********************

This project outputs to the console. It can be built and executed as
follows, replacing ``<board>`` with a board that has a crypto hash driver
enabled (for example ``stm32mp135f_dk``):

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/hash_bench
   :board: <board>
   :goals: build flash
   :compact:

To benchmark the software-only (mbedTLS) implementation instead of a
hardware accelerator, build with the ``prj_mtls_shim.conf`` overlay. This
also works on ``native_sim``, to compare against a board with a hardware
accelerator:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/hash_bench
   :board: native_sim
   :gen-args: -DEXTRA_CONF_FILE=prj_mtls_shim.conf
   :goals: build run
   :compact:

The console banner reports which implementation ran: ``Hash HW benchmark``
for a hardware accelerator, ``Hash SW benchmark`` for the mbedTLS shim.

Sample Output
=============

.. code-block:: console

    Hash HW benchmark
    BENCH SHA256 len=64 loops=2000 cycles=... cpb=... MBps=...
    ...
    BENCH SHA3-512 len=65536 loops=40 cycles=... cpb=... MBps=...
