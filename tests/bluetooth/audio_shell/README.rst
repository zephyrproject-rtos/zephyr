.. _bt_audio:

Bluetooth: Audio
################

Overview
********
This test application is used to manually test the following Bluetooth Audio
features: ISO, PACS, ASCS, BAP, VCS, VOCS, AICS, VCP, TBS, CCP, CSIS, CSIP and
MCS.

Requirements
************

* A board with Bluetooth LE support, preferably with ISO support, or
* QEMU/native_posix with BlueZ running on the host.

Building and Running
********************

This test can be found under :zephyr_file:`tests/bluetooth/audio_shell` in the
Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details on how
to run an application inside QEMU or as a native POSIX application.

For other boards, build and flash the application as follows:

.. zephyr-app-commands::
   :zephyr-app: tests/bluetooth/audio_shell
   :board: <board>
   :goals: flash
   :compact:

Refer to your :ref:`board's documentation <boards>` for alternative
flash instructions if your board doesn't support the ``flash`` target.

Notes for Running with Native POSIX
===================================
Depending on your setup, it may be necessary to build the :code:`native_posix`
board with optimizations disabled (:code:`CONFIG_NO_OPTIMIZATIONS=y`).

When the application has been built, it can be run as a regular executable.
Depending on your setup, you may need to run the executable with elevated
privileges (e.g. sudo).

It can e.g. be run as following for the hci0 device:

.. code-block:: console

   zephyr.exe -bt-dev=hci0 -rt -attach_uart

Commands
********

.. code-block:: console

   tbs          :TBS related commands
   ccp          :CCP related commands
   csis         :CSIS related commands
   csip         :CSIP related commands
   mpc          :MCC related commands
   mpl          :MCP related commands
   bass         :BASS related commands
   bass_client  :BASS client related commands
   mics         :MICS related commands
   mics_client  :MICS client related commands
   vcs          :VCS related commands
   vcs_client   :VCS client related commands
   bap          :BAP related commands
   iso          :ISO related commands

Common steps:

1. Init Bluetooth subsystem:

.. code-block:: console

   uart:~$ bt init
   Bluetooth initialized
   ...

2. Init services and register audio endpoints:

.. code-block:: console

   uart:~$ bap init
   [00:00:09.480,000] <dbg> bt_pacs.bt_audio_register: cap 0x00168cc0 type 0x02 codec 0x01 freq 0x007f
   [00:00:09.480,000] <dbg> bt_pacs.bt_audio_register: cap 0x00168cf0 type 0x01 codec 0x01 freq 0x007f

3. [Slave] Listen to incoming ISO connections:

.. code-block:: console

   uart:~$ iso listen
   [00:00:14.300,000] <dbg> bt_iso.bt_iso_server_register: 0x00169e44

4. [Slave] Advertise:

.. code-block:: console

   uart:~$ bt advertise on

5. [Master Optional] Scan:

If the device address is not known it can be scanned with the following
commands:

.. code-block:: console

   uart:~$ bt scan on
   ...(stop as soon as the device show up)
   uart:~$ bt scan off

5. [Master] Connect:

.. code-block:: console

   uart:~$ bt connect <address> (public/random)

6. [Master Optional] Exchange MTU:

Some profiles may require an MTU other than the default, to change that use
the following command:

.. code-block:: console

   uart:~$ gatt exchange-mtu

Subcommands
***********
<tba>
