Hash HW Functional + Performance sample
#######################################

This sample validates SHA-256 correctness and prints simple throughput metrics
on Zephyr crypto hash drivers.

What it does
************

- Functional checks (known-answer tests, one-shot ``hash_compute()`` on a
  single session):
  - SHA-256("abc")
  - SHA-256(64 x 'a') - exactly one full 512-bit block, forcing padding into
    a second block
- Performance checks:
  - Benchmarks SHA-256 at message sizes 64, 256, 1024, 4096, 16384 and 65536
    bytes (with a matching loop count per size, decreasing as the size grows)
  - Each benchmark case prints total cycles, cycles/byte (``cpb``),
    throughput in MB/s and CPU load (``CPU=%``, via ``cpu_load_get()``)

Build for STM32MP135F-DK
************************

.. code-block:: bash

   west build -b stm32mp135f_dk samples/drivers/hash_hw_test -p always

Then flash/run with your usual board flow (``west flash`` or board-specific boot).

On STM32MP13, the driver processes SHA-224/SHA-256 in interrupt mode: the
calling thread blocks on a semaphore given from the HASH ISR instead of
busy-polling status registers, freeing the CPU while the peripheral
computes the digest.

Compare HW vs SW
****************

Use two builds:

1. Hardware hash path (default)

.. code-block:: bash

   west build -b stm32mp135f_dk samples/drivers/hash_hw_test -p always

2. Software path through mbedTLS shim

.. code-block:: bash

   west build -b stm32mp135f_dk samples/drivers/hash_hw_test -p always -- \
     -DEXTRA_CONF_FILE=prj_mbedtls.conf

Compare the printed ``cpb``, ``MBps`` and ``CPU=%`` lines.
