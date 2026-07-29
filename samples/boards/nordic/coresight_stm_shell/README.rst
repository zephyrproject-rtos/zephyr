.. zephyr:code-sample:: coresight_stm_shell
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

It is only supported on ``nrf54h20dk/nrf54h20/cpuapp``.

Requirements
************

* nRF54H20 DK

Building and running
**********************

.. code-block:: shell

   west build -b nrf54h20dk/nrf54h20/cpuapp samples/boards/nordic/coresight_stm_shell \
     -T sample.boards.nrf.coresight_stm_shell

The ``nordic-log-stm`` snippet enables STM log frontend and the Coresight overlay on all cores.
