Upgrade testing with MCUmgr
###########################

This application is based on :ref:`smp_svr_sample`. It is built
using **sysbuild**. Tests are automated with pytest, a new harness of Twister
(more information can be found here :ref:`integration-with-pytest`)

.. note::
   Pytest uses the MCUmgr fixture which requires the ``mcumgr`` available
   in the system PATH.
   More information about MCUmgr can be found here :ref:`mcu_mgr`.

To run tests with Twister on ``nrf52840dk_nrf52840`` platform,
use following command:

.. code-block:: console

    ./zephyr/scripts/twister -vv --west-flash --enable-slow -T zephyr/tests/boot/with_mcumgr \
    -p nrf52840dk_nrf52840 --device-testing --device-serial /dev/ttyACM0

.. note::
   Twister requires ``--west-flash`` flag enabled (without additional parameters
   like ``erase``) to use sysbuild.

Test scripts can be found in ``pytest`` directory. To list available
scenarios with described procedures, one can use a pytest command:

.. code-block:: console

    pytest zephyr/tests/boot/with_mcumgr/pytest --collect-only -v
