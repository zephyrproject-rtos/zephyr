.. zephyr:code-sample:: remote_shell_log
   :name: Coresight STM with remote shell
   :relevant-api: log_api

Overview
********

This sample combines :ref:`logging_cs_stm` (logging through Coresight STM on the
application core) with :ref:`shell_api` and :ref:`shell_remote`. It shows how to use remote shell
on multiple cores. It also shows runtime filtering with STM Logging.

Shell can be used to adjust runtime filtering for the log modules on all cores.

For example, ``log enable dbg app`` will enable up to debug messages for the ``app`` log module on
the ``cpuapp`` core and ``remote_shell radio log enable dbg app`` will do the same but on the
``cpurad`` core.

The custom ``ping`` command prints one message at each severity (error through debug); run it
after change log filtering configuration to confirm which levels are enabled.

Supported targets:

* ``nrf54h20dk/nrf54h20/cpuapp`` — Coresight STM logging and remote shell on radio, PPR, and FLPR
* ``nrf54l15dk/nrf54l15/cpuapp`` — remote shell on FLPR (UART logging on cpuapp)

Requirements
************

* nRF54H20 DK or nRF54L15 DK

Building and running
**********************

nRF54H20 DK:

.. code-block:: shell

   west build -b nrf54h20dk/nrf54h20/cpuapp samples/boards/nordic/remote_shell_log \
     -T sample.boards.nrf.remote_shell_log

The ``nordic-log-stm`` snippet enables STM log frontend and the Coresight overlay on all cores.

nRF54L15 DK (FLPR is started with the ``nordic-flpr`` snippet):

.. code-block:: shell

   west build -b nrf54l15dk/nrf54l15/cpuapp samples/boards/nordic/remote_shell_log \
     --sysbuild -- -Dremote_shell_log_SNIPPET=nordic-flpr \
     -T sample.boards.nrf.remote_shell_log.nrf54l
