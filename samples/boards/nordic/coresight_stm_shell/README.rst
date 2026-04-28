.. zephyr:code-sample:: coresight_stm_shell
   :name: Coresight STM with remote shell
   :relevant-api: log_api

Overview
********

This sample combines :ref:`logging_cs_stm` (logging through Coresight STM on the
application core) with :ref:`shell_api`. It shows runtime filtering with STM Logging.

On the main shell you can adjust runtime filtering for the ``app`` log module with
``log enable <level> app``, where ``<level>`` is one of the shell log levels (for example
``dbg``, ``inf``, ``wrn``, ``err``).

The custom ``ping`` command prints one message at each severity (error through debug); run it
after ``log enable`` to confirm which levels still reach the console after STM decode.

It is only supported on ``nrf54h20dk/nrf54h20/cpuapp``.

Requirements
************

* nRF54H20 DK

Building and running
**********************

.. code-block:: shell

   west build -b nrf54h20dk/nrf54h20/cpuapp samples/boards/nordic/coresight_stm_shell \
     -T sample.boards.nrf.coresight_stm_shell

The ``nordic-log-stm`` snippet enables STM log frontend and the Coresight overlay on cpuapp.
