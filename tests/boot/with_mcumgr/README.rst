Upgrade testing with MCUmgr
###########################

This application is based on :ref:`smp_svr_sample`. It is built
using **sysbuild**. Tests are automated with pytest, a new harness of Twister
(more information can be found :ref:`here <integration_with_pytest>`)

.. note::
   Pytest uses the MCUmgr fixture which requires the ``mcumgr`` available
   in the system PATH.
   More information about MCUmgr can be found here :ref:`mcu_mgr`.

To run tests with Twister on ``nrf52840dk/nrf52840`` platform,
use following command:

.. code-block:: console

    west twister -vv --log-level debug --enable-slow -T tests/boot/with_mcumgr \
    -p nrf52840dk/nrf52840 --device-testing --device-serial /dev/ttyACM0

To test with ``mcumgr`` with Bluetooth, one must add ``usb_hci:hciX`` fixture
where ``hciX`` is the Bluetooth HCI device (e.g. ``hci1``).
Fixture can be added to Twister command: ``-X usb_hci:hci1``,
or added to the hardware map

.. code-block:: yaml

      - connected: true
        fixtures:
          - usb_hci:hci1

Test scripts can be found in ``pytest`` directory. To list available
scenarios with described procedures, one can use a pytest command:

.. code-block:: console

    pytest tests/boot/with_mcumgr/pytest --collect-only -v
