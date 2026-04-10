EnvDAQ
######

EnvDAQ is a small GPIO control test application for the HIKIT SC1 board.

The sample blinks the user LED continuously and enables the relay output once
every 30 seconds. The relay stays active for 5 seconds and is then turned off.

Runtime state changes are reported through the Zephyr logging subsystem.

Supported board
***************

Only ``hikit_sc1`` is supported.

Build and run
*************

.. code-block:: console

   west build -b hikit_sc1 samples/subsys/shell/envdaq