.. zephyr:code-sample:: smp-bt-client
   :name: SMP client (Bluetooth)
   :relevant-api: mcumgr_transport_bt_client mcumgr_img_mgmt_client mcumgr_os_mgmt_client

   Upload a firmware image from a file system to a Bluetooth peer that runs the SMP service.

Overview
********

This sample scans for a Bluetooth peer that advertises the MCUmgr SMP service, connects to
it, attaches the Bluetooth client transport to it with :c:func:`smp_client_bt_attach`, checks
the link with an ``os_mgmt`` echo command, and uploads a firmware image into the peer's MCUboot
secondary slot with the ``img_mgmt`` upload client.

The sample enables :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_CLIENT` and
:kconfig:option:`CONFIG_SMP_CLIENT` only. It does not enable the Bluetooth MCUmgr server
(:kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT`), so the device runs no SMP GATT service of its own
and does not need :kconfig:option:`CONFIG_BT_PERIPHERAL`. The transport only processes responses
from the peer, and drops any request the peer sends. See :ref:`mcumgr_smp_transport_ble_client`.
The transport is experimental.

The image is read from a littlefs file system on the board's external flash, so it can be
replaced without rebuilding the application.

Requirements
************

* An :zephyr:board:`nrf54lm20dk` or an :zephyr:board:`nrf52840dk`. The board overlays in
  :file:`boards/` add a 1 MiB littlefs partition mounted at ``/lfs1`` on the on-board MX25R64
  flash. Other boards need an overlay providing an equivalent ``lfs1``
  :dtcompatible:`zephyr,fstab,littlefs` entry.

* A second board running the :zephyr:code-sample:`smp-svr` sample, built with sysbuild so that
  MCUboot provides a secondary slot, and with :file:`bt.conf`.

The peer's :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE` must be at least this
sample's value of 2048, because the upload client sizes its frames from it.

Building and Running
********************

Build and flash the peer:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/subsys/mgmt/mcumgr/smp_svr
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :west-args: --sysbuild
   :gen-args: -DEXTRA_CONF_FILE="bt.conf" -Dsmp_svr_CONFIG_IMG_ERASE_PROGRESSIVELY=y
   :compact:

Uploading a real image needs :kconfig:option:`CONFIG_IMG_ERASE_PROGRESSIVELY` on the peer: without
it, ``img_mgmt`` erases the whole secondary slot when it receives offset 0, and the retries that
the SMP client sends every :kconfig:option:`CONFIG_SMP_CMD_RETRY_TIME` restart that erase until the
upload times out. The 4 KiB placeholder does not need it.

Then build and flash this sample on a second board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mgmt/mcumgr/smp_bt_client
   :board: nrf54lm20dk/nrf54lm20a/cpuapp
   :goals: build flash
   :compact:

Start the peer first. The sample scans for only 30 seconds; if it stops before the peer logs
``Advertising successfully started``, reset the board running the sample.

Getting an image onto the file system
=====================================

The sample uploads :file:`/lfs1/update.bin`. If the file does not exist, the sample writes a
4 KiB placeholder consisting of a valid MCUboot image header followed by zeros. The
placeholder is not bootable, so do not mark it pending on the peer.

To upload a real image, write :file:`build/smp_svr/zephyr/zephyr.signed.bin` from the peer's
build to :file:`/lfs1/update.bin`. Building with the following options makes the board an SMP
server over the serial shell, so the file can be written as described in the *Download files
from/upload files to file system* section of :zephyr:code-sample:`smp-svr`:

.. code-block:: cfg

   CONFIG_BASE64=y
   CONFIG_SHELL=y
   CONFIG_SHELL_BACKEND_SERIAL=y
   CONFIG_MCUMGR_TRANSPORT_SHELL=y
   CONFIG_MCUMGR_GRP_FS=y
   CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=2304

This gives any host on the serial port access to the file system; restrict it with the
MCUmgr file access hooks described in :ref:`mcumgr_callbacks` if that is not wanted.

Sample Output
*************

This output was captured on an nRF54LM20 DK with no :file:`/lfs1/update.bin` on its file system,
with :zephyr:code-sample:`smp-svr` running on an nRF52840 DK as the peer. Timestamps, the boot
banner and the flash and file system lines are removed. The addresses, RSSI and MTU depend on the
boards and the link, and on first boot with unformatted external flash the file system is
formatted first.

.. code-block:: console

   <inf> smp_bt_client_sample: MCUmgr SMP client Bluetooth sample
   <inf> smp_bt_client_sample: No /lfs1/update.bin yet, writing a placeholder image
   <inf> smp_bt_client_sample: Image /lfs1/update.bin is 4096 bytes
   <inf> bt_hci_core: HW Platform: Nordic Semiconductor (0x0002)
   <inf> bt_hci_core: HW Variant: nRF54Lx (0x0005)
   <inf> bt_hci_core: Controller: Zephyr Bluetooth Controller (0x00) manufacturer 0x05f1 Version 4.5 Build 0
   <inf> bt_hci_core: HCI transport: Controller
   <inf> bt_hci_core: Identity: R:D4:5C:51:D6:08:B3
   <inf> bt_hci_core: HCI: version 5.4 (0x0d) revision 0x0000, manufacturer 0x05f1
   <inf> bt_hci_core: LMP: version 5.4 (0x0d) subver 0xffff
   <inf> smp_bt_client_sample: Scanning for a peer that advertises the SMP service
   <inf> smp_bt_client_sample: SMP server found at R:F1:63:F1:10:90:76, RSSI -44
   <inf> smp_bt_client_sample: Connected
   <inf> mcumgr_smp: SMP client transport attached (value handle 0x000e, MTU 498)
   <inf> smp_bt_client_sample: Transport attached to the peer's SMP service
   <inf> smp_bt_client_sample: Peer answered the echo command
   <inf> smp_bt_client_sample: Uploaded 1024/4096 bytes
   <inf> smp_bt_client_sample: Uploaded 2048/4096 bytes
   <inf> smp_bt_client_sample: Uploaded 3072/4096 bytes
   <inf> smp_bt_client_sample: Uploaded 4096/4096 bytes
   <inf> smp_bt_client_sample: Image written to the peer's secondary slot
   <inf> smp_bt_client_sample: Disconnected (reason 0x16)

An MTU of 23 in the ``attached`` line means the ATT MTU exchange did not happen, and the upload
runs in 20 byte fragments.

If the attach fails, the preceding ``mcumgr_smp`` log line gives the reason. A peer built with
:kconfig:option:`CONFIG_BT_SMP` defaults to
:kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_PERM_RW_AUTHEN` and rejects the subscription from an
unauthenticated link. This sample does not pair, so build such a peer with
:kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_PERM_RW`.
