.. stm32wba_default_zephyr_bootloader_start

Default Zephyr bootloader
-------------------------

STM32WBAx chips also support secure boot and secure FOTA with Zephyr using
the default MCUboot-based workflows.
The :zephyr:code-sample:`smp-svr` application is one example that can be used
for this.

Use an MCUmgr-capable client (`tools and libraries <https://docs.zephyrproject.org/latest/services/device_mgmt/mcumgr.html#tools-libraries>`_).
The following procedure uses `newtmgr <https://mynewt.apache.org/latest/newtmgr/index.html>`_.

Once ``smp_svr`` is flashed, you can use ``newtmgr`` for FOTA over BLE.
The examples below assume the HCI interface is ``hci0``. Note that internal
HCI devices can sometimes be unreliable.

.. code-block:: console

  newtmgr -i 0 --conntype ble --connstring peer_name=Zephyr image upload build/smp_svr/zephyr/zephyr.signed.bin
  newtmgr -i 0 --conntype ble --connstring peer_name=Zephyr image list
  newtmgr -i 0 --conntype ble --connstring peer_name=Zephyr image test <image0_slot1_hash>
  newtmgr -i 0 --conntype ble --connstring peer_name=Zephyr reset

.. stm32wba_default_zephyr_bootloader_end

.. stm32wba_low_power_start

|stm32wba_lp_board_name| supports Zephyr power management to enter low-power states
when the application is idle.

To achieve the lowest power consumption, Zephyr ``suspend-to-ram`` leverages
the STM32 Standby mode on this platform.

Power management must be enabled in your application by activating
``CONFIG_PM``:

.. code-block:: none

  CONFIG_PM=y

For suspend-to-ram on STM32WBA, the reference implementation is
:zephyr:code-sample:`ble_peripheral_hr`, used together with
:zephyr_file:`samples/bluetooth/peripheral_hr/boards/stm32wba_pwr_optim.overlay`.

In this overlay, ``sram0`` can be reduced to the minimal usable size to
lower consumption, while ``pm_s2ram`` should remain located at the end of
the usable RAM.

Example build command:

.. parsed-literal::

   west build -b |stm32wba_lp_board_target| samples/bluetooth/peripheral_hr -- \\
     -DDTC_OVERLAY_FILE=boards/stm32wba_pwr_optim.overlay \\
     -DCONFIG_PM=y

.. stm32wba_low_power_end