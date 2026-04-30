.. zephyr:code-sample:: stm32-system-bootloader
   :name: STM32 system bootloader
   :relevant-api: stm32_bootloader flash_interface

   Program a target STM32 from a host MCU via the target's built-in ROM
   bootloader, using both the bootloader API and its flash-adapter view.

Overview
********

Two device tree nodes cooperate:

``stm32_target`` (``st,stm32-bootloader``)
    The standalone bootloader driver. Exposes the AN3155 command set
    through the typed API in
    ``<zephyr/drivers/misc/stm32_bootloader/stm32_bootloader.h>``:
    ``enter`` / ``exit`` / ``go`` / ``get_info`` /
    ``read_memory`` / ``write_memory`` /
    ``erase_pages`` / ``mass_erase``.

``main_flash`` (``st,stm32-bootloader-flash``, child of the bootloader)
    A thin ``flash_driver_api`` adapter. Delegates ``flash_read`` /
    ``flash_write`` / ``flash_erase`` to the parent's ``read_memory`` /
    ``write_memory`` / ``erase_pages``. Makes the target usable with
    Zephyr's existing DFU stack (``stream_flash``, ``flash_img``,
    ``mcumgr img_mgmt``, UpdateHub) with no changes to those consumers.

The sample uses both APIs side by side: the parent for lifecycle
(``enter``, ``get_info``, ``mass_erase``, ``go``, ``exit``) and the
child via ``stream_flash`` for the actual programming + readback verify.

Session model
*************

The flash adapter holds no session state. Callers must call
``stm32_bootloader_enter(parent)`` before any ``flash_*`` operation on a
child and ``stm32_bootloader_exit(parent, ...)`` afterwards. If the
parent is not entered, the adapter returns ``-ENOTCONN``.

Hardware setup
**************

Host ``stm32h573i_dk`` connected to target ``nucleo_g0B1re``:

+---------------------+----------------------+
| Host (H573)         | Target (G0B1)        |
+=====================+======================+
| USART3 TX/RX        | USART1 RX/TX         |
+---------------------+----------------------+
| PG8  → BOOT0        | BOOT0                |
+---------------------+----------------------+
| PG5  → NRST         | NRST                 |
+---------------------+----------------------+
| GND                 | GND                  |
+---------------------+----------------------+

Building and running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/misc/stm32_bootloader
   :board: stm32h573i_dk
   :goals: build flash

Expected log::

   [00:00:00.xx] <inf> main: === STM32 bootloader sample ===
   [00:00:00.xx] <inf> stm32_sys_bl_uart: Connected: BL v3.1, PID 0x0467, extended erase
   [00:00:00.xx] <inf> main: Target: PID=0x0467, BL v3.1, extended erase
   [00:00:00.xx] <inf> main: Mass-erasing target...
   [00:00:xx.xx] <inf> main: Streaming 12 bytes of firmware...
   [00:00:xx.xx] <inf> main: Verified 12 bytes @0x0
   [00:00:xx.xx] <inf> main: Jumping to 0x08000000...
   [00:00:xx.xx] <inf> stm32_sys_bl_uart: Disconnected
   [00:00:xx.xx] <inf> main: === Done ===

Using the bootloader API directly
*********************************

If your task doesn't fit flash semantics (reading option bytes, sending
a ``Go`` to a custom address, querying the target, ...), use the
bootloader API on the parent device. The flash adapter is entirely
optional and does nothing when ``CONFIG_STM32_BOOTLOADER_FLASH=n``.

Adding other memory regions
***************************

Add sibling ``st,stm32-bootloader-flash`` nodes under the bootloader
node to expose additional regions as flash devices. For read-only
regions (system memory, factory OTP) set the ``read-only`` DT property.
