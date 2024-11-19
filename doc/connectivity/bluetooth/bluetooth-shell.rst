.. _bluetooth_shell:

Shell
#####

The Bluetooth Shell is an application based on the :ref:`shell_api` module. It offer a collection of
commands made to easily interact with the Bluetooth stack.

For specific Bluetooth functionality see also the following shell documentation

.. toctree::
   :maxdepth: 1

   shell/audio/bap.rst
   shell/audio/bap_broadcast_assistant.rst
   shell/audio/bap_scan_delegator.rst
   shell/audio/cap.rst
   shell/audio/ccp.rst
   shell/audio/csip.rst
   shell/audio/gmap.rst
   shell/audio/mcp.rst
   shell/audio/tmap.rst
   shell/audio/pbp.rst
   shell/classic/a2dp.rst
   shell/host/gap.rst
   shell/host/gatt.rst
   shell/host/iso.rst
   shell/host/l2cap.rst

Bluetooth Shell Setup and Usage
*******************************

First you need to build and flash your board with the Bluetooth shell. For how to do that, see the
:ref:`getting_started`. The Bluetooth shell itself is located in
:zephyr_file:`tests/bluetooth/shell/`.

When it's done, connect to the CLI using your favorite serial terminal application. You should see
the following prompt:

.. code-block:: console

        uart:~$

For more details on general usage of the shell, see :ref:`shell_api`.

The first step is enabling Bluetooth. To do so, use the :code:`bt init` command. The following
message is printed to confirm Bluetooth has been initialized.

.. code-block:: console

        uart:~$ bt init
        Bluetooth initialized
        Settings Loaded
        [00:02:26.771,148] <inf> fs_nvs: nvs_mount: 8 Sectors of 4096 bytes
        [00:02:26.771,148] <inf> fs_nvs: nvs_mount: alloc wra: 0, fe8
        [00:02:26.771,179] <inf> fs_nvs: nvs_mount: data wra: 0, 0
        [00:02:26.777,984] <inf> bt_hci_core: hci_vs_init: HW Platform: Nordic Semiconductor (0x0002)
        [00:02:26.778,015] <inf> bt_hci_core: hci_vs_init: HW Variant: nRF52x (0x0002)
        [00:02:26.778,045] <inf> bt_hci_core: hci_vs_init: Firmware: Standard Bluetooth controller (0x00) Version 3.2 Build 99
        [00:02:26.778,656] <inf> bt_hci_core: bt_init: No ID address. App must call settings_load()
        [00:02:26.794,738] <inf> bt_hci_core: bt_dev_show_info: Identity: EB:BF:36:26:42:09 (random)
        [00:02:26.794,769] <inf> bt_hci_core: bt_dev_show_info: HCI: version 5.3 (0x0c) revision 0x0000, manufacturer 0x05f1
        [00:02:26.794,799] <inf> bt_hci_core: bt_dev_show_info: LMP: version 5.3 (0x0c) subver 0xffff


Logging
*******

You can configure the logging level per module at runtime. This depends on the maximum logging level
that is compiled in. To configure, use the :code:`log` command. Here are some examples:

* List the available modules and their current logging level

.. code-block:: console

        uart:~$ log status

* Disable logging for *bt_hci_core*

.. code-block:: console

        uart:~$ log disable bt_hci_core

* Enable error logs for *bt_att* and *bt_smp*

.. code-block:: console

        uart:~$ log enable err bt_att bt_smp

* Disable logging for all modules

.. code-block:: console

        uart:~$ log disable

* Enable warning logs for all modules

.. code-block:: console

        uart:~$ log enable wrn
