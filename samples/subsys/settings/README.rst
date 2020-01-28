.. _Settings-sample:

Settings sample application
###########################

Overview
********

This application show how to use Settings subsystem with NVS backends.
It presents how to write more then one key and how to handle loading it.

Requirements
************

MCU or board with flash/NVS controller.

Building and Running
********************

This application was developed and tested with NRF52840 Dev board. But probably
will work with others as well.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/settings
   :board: nrf52840_pca10056
   :goals: build flash
   :compact:


Sample Output
=============

.. code-block:: console

        *** Booting Zephyr OS build zephyr-v2.1.0-1384-gc20a870b6ba8  ***
        Settings sample
        [00:00:00.002,532] <inf> fs_nvs: 8 Sectors of 4096 bytes
        [00:00:00.002,563] <inf> fs_nvs: alloc wra: 0, f30
        [00:00:00.002,563] <inf> fs_nvs: data wra: 0, e8
        [00:00:00.002,563] <dbg> settings.settings_nvs_backend_init: Initialized
        Setup and save variables:
        Number = 33.800000
        Another Number = 2
        Str = This is string

        prefs_export_handler is called
        Cleanup of variables:
        Number = 0.000000
        Another Number = 0
        Str = --- NOTHING :D ---

        Now we are loading settings:
        Restoring key str, result:30
        [00:00:04.003,906] <dbg> settings.settings_call_set_handler: set-value OK. key: my_prefs/str
        Restoring key anthr_number, result:4
        [00:00:04.004,028] <dbg> settings.settings_call_set_handler: set-value OK. key: my_prefs/anthr_number
        Restoring key number, result:8
        [00:00:04.004,272] <dbg> settings.settings_call_set_handler: set-value OK. key: my_prefs/number
        Number = 33.800000
        Another Number = 2
        Str = This is string

        Now we save just a single item:
        Number = 0.000000
        Another Number = 0
        Str = --- NOTHING :D ---

        Now we loading settings with just one value change:
        Restoring key str, result:30
        [00:00:10.005,859] <dbg> settings.settings_call_set_handler: set-value OK. key: my_prefs/str
        Restoring key anthr_number, result:4
        [00:00:10.006,011] <dbg> settings.settings_call_set_handler: set-value OK. key: my_prefs/anthr_number
        Restoring key number, result:8
        [00:00:10.006,225] <dbg> settings.settings_call_set_handler: set-value OK. key: my_prefs/number
        Number = 33.800000
        Another Number = 444
        Str = This is string
