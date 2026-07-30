:orphan:

.. espressif-building-flashing

Simple Boot
===========

The board could be loaded using the single binary image, without 2nd stage bootloader.
It is the default option when building the application without additional configuration.

.. note::

   Simple boot does not provide any security features nor OTA updates.

MCUboot Bootloader
==================

User may choose to use MCUboot bootloader instead. In that case the bootloader
must be built (and flashed) at least once.

There are two options to be used when building an application:

1. Sysbuild
2. Manual build

.. note::

   User can select the MCUboot bootloader by adding the following line
   to the board default configuration file.

   .. code:: cfg

      CONFIG_BOOTLOADER_MCUBOOT=y

Sysbuild
========

The sysbuild makes possible to build and flash all necessary images needed to
bootstrap the board with the ESP32 SoC.

To build the sample application using sysbuild use the command:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/hello_world
   :board: <board>
   :goals: build
   :west-args: --sysbuild
   :compact:

By default, the ESP32 sysbuild creates bootloader (MCUboot) and application
images. But it can be configured to create other kind of images.

Build directory structure created by sysbuild is different from traditional
Zephyr build. Output is structured by the domain subdirectories:

.. code-block::

  build/
  ├── hello_world
  │   └── zephyr
  │       ├── zephyr.elf
  │       └── zephyr.bin
  ├── mcuboot
  │    └── zephyr
  │       ├── zephyr.elf
  │       └── zephyr.bin
  └── domains.yaml

.. note::

   With ``--sysbuild`` option the bootloader will be re-build and re-flash
   every time the pristine build is used.

For more information about the system build please read the :ref:`sysbuild` documentation.

.. _hardware_flash_encryption:

Hardware Flash Encryption
=========================

Espressif SoCs can enable hardware flash encryption in the Zephyr MCUboot
sysbuild flow. On the first successful boot after a full chip erase and
bootloader and application image flashing, MCUboot bootloader generates
the encryption key, encrypts flash regions in place, burns the required
eFuses and resets so the encrypted flash cache takes effect.

This is distinct from MCUboot software-based image encryption
(``SB_CONFIG_BOOT_ENCRYPTION``). Hardware flash encryption referred here
is related to Espressif's hardware flash encryption feature.
Hardware flash encryption is not available with Simple Boot.

.. note::
   When enabling hardware flash encryption, ``write-block-size`` **must**
   be set to 32 bytes on the SoC or board's overlay file for **both**
   application and MCUboot bootloader. See
   :zephyr_file:`samples/boards/espressif/flash_encryption/boards/esp32_devkitc_procpu.overlay`
   for an example.

Enable it at sysbuild time:

.. code-block:: shell

   west build -b <board> samples/hello_world --sysbuild \
     -DSB_CONFIG_MCUBOOT_ESP_FLASH_ENCRYPTION=y \
     -DSB_CONFIG_MCUBOOT_ESP_FLASH_ENCRYPTION_DEVELOPMENT=y

Development mode keeps UART download / encryption paths usable for
re-flashing and is intended for testing only. It may be desirable to
also keep JTAG access enabled by setting
``SB_CONFIG_MCUBOOT_ESP_SECURE_BOOT_ALLOW_JTAG``.
Release mode is aimed for production and permanently disables those
insecure paths:

.. code-block:: shell

   west build -b <board> samples/hello_world --sysbuild \
     -DSB_CONFIG_MCUBOOT_ESP_FLASH_ENCRYPTION=y \
     -DSB_CONFIG_MCUBOOT_ESP_FLASH_ENCRYPTION_RELEASE=y

.. note::

   Release mode and eFuse burns are irreversible.

For testing without burning real eFuses, enable virtual eFuse emulation
(optionally persisted in a ``sys_partition`` with
``SB_CONFIG_MCUBOOT_ESP_EFUSE_VIRTUAL_KEEP_IN_FLASH`` option):

.. code-block:: shell

   west build -b <board> samples/hello_world --sysbuild \
     -DSB_CONFIG_MCUBOOT_ESP_FLASH_ENCRYPTION=y \
     -DSB_CONFIG_MCUBOOT_ESP_FLASH_ENCRYPTION_DEVELOPMENT=y \
     -DSB_CONFIG_MCUBOOT_ESP_EFUSE_VIRTUAL=y \
     -DSB_CONFIG_MCUBOOT_ESP_EFUSE_VIRTUAL_KEEP_IN_FLASH=y

Additional options under the sysbuild menu
**Espressif hardware secure features** control XTS-AES key size, UART ROM
download mode, XTS-AES pseudo-rounds (where supported), and other development
overrides (JTAG, UART bootloader encryption, and so on). Prefer defaults for
production builds.

.. note::

   Before enabling hardware flash encryption on a physical device, perform a
   full chip erase, this is important because flash erased value (``0xFF`` in
   most cases) read from the trailer registers are part of MCUboot's
   update-state checking mechanism, thus unknown data or garbage could be
   potentially interpreted as a valid state and lead to an unexpected
   behavior.
   After the first successful boot and flash regions encryption,
   subsequent device re-flashing must be manually done with the
   encryption-aware flow (see
   :ref:`manual_flashing_when_flash_encryption`).
   OTA updates may be performed sending plaintext images, as the flash
   read/write operations are encrypted in runtime.

When hardware flash encryption is enabled, sysbuild configures the
application image, sets :kconfig:option:`CONFIG_ESP_FLASH_ENCRYPTION` and
sets extra ``imgtool`` arguments for the padding and correct alignment, so
the flash driver and tools match the encrypted layout.

Manual Build
============

During the development cycle, it is intended to build & flash as quickly possible.
For that reason, images can be built one at a time using traditional build.

The instructions following are relevant for both manual build and sysbuild.
The only difference is the structure of the build directory.

.. note::

   Remember that bootloader (MCUboot) needs to be flash at least once.

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: <board>
   :goals: build

The usual ``flash`` target will work with the board configuration.
Here is an example for the :zephyr:code-sample:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: <board>
   :goals: flash

.. note::

   On targets that expose the built-in USB Serial/JTAG controller, the chip can
   stay in download mode after ``west flash`` and will not boot the new image
   until it is power cycled. If that happens, flash with a watchdog reset so the
   chip restarts on its own:

   .. code-block:: shell

      west flash --reset-type watchdog-reset

Faster Flashing
===============

To speed up the development cycle, ``--esp-skip-flashed`` skips writing the image
when the binary already in flash matches the one being flashed, verified with an
MD5 check on the device:

.. code-block:: shell

   west flash --esp-skip-flashed

For an even faster reflash, ``--esp-diff`` writes only the regions that differ
from the previously flashed image. It compares against a locally cached copy
rather than reading the device, so use it only when the flash was not modified
by another tool, board, or manual write since the last ``west flash``:

.. code-block:: shell

   west flash --esp-diff

Progress output can be suppressed for cleaner logs, which is useful in CI:

.. code-block:: shell

   west flash --esp-no-progress

Open the serial monitor using the following command:

.. code-block:: shell

   west espressif monitor

After the board has automatically reset and booted, you should see the following
message in the monitor:

.. code-block:: console

   ***** Booting Zephyr OS vx.x.x-xxx-gxxxxxxxxxxxx *****
   Hello World! <board>

.. _manual_flashing_when_flash_encryption:

Manual flashing when Flash Encryption already enabled
=====================================================

While developing, it may be desirable to re-flash the bootloader/application
images after having Hardware Flash Encryption already enabled and the flash
regions already encrypted (only possible when working with
``SB_CONFIG_MCUBOOT_ESP_FLASH_ENCRYPTION_DEVELOPMENT`` mode enabled). This
is possible by using the Espressif's ``esptool`` tool manually:

.. code-block:: shell

   esptool -p <port> -b <baudrate> --no-stub --after no-reset write-flash \
     --flash-mode dio --flash-freq keep --encrypt <slot0_partition_offset> \
     <build_directory>/smp_svr/zephyr/zephyr.signed.bin --force
   esptool -p <port> -b <baudrate> --no-stub --after no-reset write-flash \
     --flash-mode dio --flash-freq keep --encrypt <boot_partition_offset> \
     <build_directory>/mcuboot/zephyr/zephyr.bin --force

Manually reset the board only after both images are flashed.

.. note::

   As mentioned in the :ref:`hardware_flash_encryption` section, MCUboot's
   update-state checking mechanism is based on the flash erased value, thus
   invalid data left on ``slot1_partition`` or ``scratch_partition`` trailer
   regions could potentially lead to an unexpected behavior. In order to
   workaround that when re-flashing images, a binary sized the same as the
   partition filled with ``0xFF`` bytes can be manually flashed using
   ``esptool``.

.. _`Zephyr Support Status`: https://developer.espressif.com/software/zephyr-support-status/
