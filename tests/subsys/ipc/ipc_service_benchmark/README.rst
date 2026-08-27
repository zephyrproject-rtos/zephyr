.. _ipc_service_benchmark_test:

IPC Service Benchmark Test
##########################

Overview
********

This test benchmarks Zephyr :ref:`IPC Service <ipc_service_api>` backends on
heterogeneous multi-core SoCs. The application core runs the ztest suite; a
companion image on the remote core provides the peer endpoint. Results are
printed to the console (throughput in kB/s, round-trip latency, and CPU load).

The same test sources are built against different backends so their performance
can be compared under similar workloads:

* **icmsg** — Inter-core messaging backend (:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_ICMSG`)
* **icbmsg** — Inter-core buffer messaging backend (zero-copy TX where supported)
* **static_vrings** — RPMsg over statically allocated virtio rings

Benchmarks
**********

* **Turnaround time** — Ping-pong exchange for 16- and 32-byte packets; reports
  average round-trip time in microseconds.
* **TX performance** — Sustained one-way traffic for 32-, 64-, and 128-byte
  packets over one second; reports throughput and buffer exhaustion counts.
  Variants with ``_no_copy`` use :c:func:`ipc_service_get_tx_buffer` and
  :c:func:`ipc_service_send_nocopy` when the backend supports it.
* **Stress** — Multiple concurrent producers (ztress) sending variable-size
  packets with payload verification; reports aggregate throughput and per-context
  statistics.

Building and Testing
********************

The test uses sysbuild. Select the backend via the Twister scenario name or
``FILE_SUFFIX``:

.. code-block:: console

   # icmsg on nRF5340 DK
   ./scripts/twister -p nrf5340dk/nrf5340/cpuapp \
       -T tests/subsys/ipc/ipc_service_benchmark \
       --test tests.ipc.ipc_service.benchmark.icmsg

   # icbmsg on nRF54L15 DK
   ./scripts/twister -p nrf54l15dk/nrf54l15/cpuapp \
       -T tests/subsys/ipc/ipc_service_benchmark \
       --test tests.ipc.ipc_service.benchmark.icbmsg

Supported platforms are listed in ``testcase.yaml`` (for example
``nrf5340dk/nrf5340/cpuapp``, ``nrf54l15dk/nrf54l15/cpuapp``, and
``nrf54h20dk/nrf54h20/cpuapp`` with remote ``cpurad``).
