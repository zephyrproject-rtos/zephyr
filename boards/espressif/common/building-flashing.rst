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
sysbuild flow. It is a hardware-assisted feature that encrypts off-chip
flash contents so they are protected against physical readout. After eFuse
burning enables the feature, flash read/write operations are
decrypted/encrypted at runtime.

The burning of the eFuses can be done manually using the ``espefuse`` tool,
or can happen automatically during the first boot of the device.

This is distinct from MCUboot software-based image encryption
(``SB_CONFIG_BOOT_ENCRYPTION``). Hardware flash encryption referred here
is related to Espressif's hardware flash encryption feature.
Hardware flash encryption is not available with Simple Boot.

.. important::

   Flash encryption provides **confidentiality** of flash contents. It does
   **not** authenticate images. Trust that only signed/verified firmware
   runs still depends on MCUboot image signature verification and, when
   used, Espressif Secure Boot. Use those together with flash encryption
   for a full security chain.

Further background: `ESP32 Flash Encryption`_.

.. note::
   When enabling hardware flash encryption, ``write-block-size`` **must**
   be set to 32 bytes on the SoC or board's overlay file for **both**
   application and MCUboot bootloader. The build fails at configure time if
   it is missing or if it is not a multiple of 32. See
   :zephyr_file:`samples/boards/espressif/flash_encryption/boards/esp32_devkitc_procpu.overlay`
   for an example.

Enable it at sysbuild time (development mode):

.. code-block:: shell

   west build -b <board> samples/hello_world --sysbuild \
     -DSB_CONFIG_MCUBOOT_ESP_FLASH_ENCRYPTION=y \
     -DSB_CONFIG_MCUBOOT_ESP_FLASH_ENCRYPTION_DEVELOPMENT=y

Development mode keeps UART download / encryption paths usable for
re-flashing and is intended for testing only. It may be desirable to
also keep JTAG access enabled by setting
``SB_CONFIG_MCUBOOT_ESP_SECURE_BOOT_ALLOW_JTAG``.

Release mode is aimed for production:

.. code-block:: shell

   west build -b <board> samples/hello_world --sysbuild \
     -DSB_CONFIG_MCUBOOT_ESP_FLASH_ENCRYPTION=y \
     -DSB_CONFIG_MCUBOOT_ESP_FLASH_ENCRYPTION_RELEASE=y

.. note::

   Release mode and eFuse burns are irreversible. Unless development
   overrides are explicitly kept enabled, the first boot permanently
   changes device security state.

What release mode locks
-----------------------

With ``SB_CONFIG_MCUBOOT_ESP_FLASH_ENCRYPTION_RELEASE``, the first boot
(unless overridden under **Espressif hardware secure features**) typically:

* Enables flash encryption and burns the related eFuses (including crypt
  count / enable bits appropriate to the SoC).
* Disables UART ROM bootloader access to flash encryption/decryption and
  cache (development options that leave those paths open are not selected).
* Disables JTAG (``SB_CONFIG_MCUBOOT_ESP_SECURE_BOOT_ALLOW_JTAG`` is for
  development only).
* May permanently disable UART ROM download mode or switch it to Secure
  Download Mode (``SB_CONFIG_MCUBOOT_ESP_SECURE_DISABLE_ROM_DL_MODE`` /
  ``SB_CONFIG_MCUBOOT_ESP_SECURE_ENABLE_SECURE_ROM_DL_MODE``). Disabling
  download mode blocks later use of ``esptool`` / ``espefuse`` over UART.
* On supported targets, may permanently enable XTS-AES pseudo-rounds.

Flash a production application (with an update agent if OTA is required)
**before** the first reset that completes release-mode enablement.

Device-generated key
--------------------

Default first-boot flow when no flash-encryption key is already burned:

1. Flash MCUboot and the application as plaintext (after a full chip erase).
2. On first boot, MCUboot generates the encryption key in eFuse (device
   key remains unknown/unreadable), encrypts flash regions in place
   (bootloader, image slots, scratch), burns eFuses that enable encryption,
   and resets so the encrypted flash cache takes effect.

Host-generated key (``espsecure`` / ``espefuse``)
-------------------------------------------------

Alternatively, burn a host-generated key **before** the first encrypting
boot. Keep a private copy of the key; burning is one-time and irreversible
on real eFuses (virtual eFuse options do **not** apply to ``espefuse``
``burn_key`` — that command programs physical eFuses).

Generate a key:

.. code-block:: shell

   espsecure.py generate_flash_encryption_key <FLASH_ENCRYPTION_KEY.bin>

Burn it (UART ROM download mode must still be available, or Secure
Download Mode must allow the needed operations):

* ESP32:

  .. code-block:: shell

     espefuse.py --port <PORT> burn_key flash_encryption <FLASH_ENCRYPTION_KEY.bin>

* Other Espressif targets (key block + purpose; names vary by SoC), for
  example:

  .. code-block:: shell

     espefuse.py --port <PORT> burn_key BLOCK <FLASH_ENCRYPTION_KEY.bin> <KEYPURPOSE>

  ``BLOCK`` is a free key block (for example ``BLOCK_KEY0`` …
  ``BLOCK_KEY5``). ``KEYPURPOSE`` is typically ``XTS_AES_128_KEY``, or
  ``XTS_AES_256_KEY_1`` / ``XTS_AES_256_KEY_2`` where AES-XTS-256 is
  supported.

Then flash MCUboot and the application as plaintext. On first boot MCUboot
encrypts in place using the already-burned host key (it does not generate a
new one), enables flash encryption eFuses, and resets.

Host-side pre-encryption (optional, when re-flashing with a known key) uses
``espsecure.py encrypt_flash_data``. Prefer AES-XTS on targets that use it:

.. code-block:: shell

   # ESP32 (AES-128 flash encryption)
   espsecure.py encrypt_flash_data --keyfile <FLASH_ENCRYPTION_KEY.bin> \
     --address <FLASH_OFFSET> --output <OUTPUT_DATA> <INPUT_DATA>

   # ESP32-S2/S3/C3 and similar (AES-XTS)
   espsecure.py encrypt_flash_data --aes_xts --keyfile <FLASH_ENCRYPTION_KEY.bin> \
     --address <FLASH_OFFSET> --output <OUTPUT_DATA> <INPUT_DATA>

OTA updates must still be sent as **plaintext** images: after encryption is
enabled, runtime flash writes are encrypted by hardware; sending
pre-encrypted OTA payloads would double-encrypt them. To update with a
host-encrypted image, flash it over serial with encryption-aware tooling
(see :ref:`manual_flashing_when_flash_encryption`); UART download must not
be fully disabled.

Verifying eFuses (``espefuse summary``)
---------------------------------------

After key burn and/or the first encrypting boot, inspect eFuses:

.. code-block:: shell

   espefuse.py --port <PORT> summary

Check in particular:

* Flash encryption enable / crypt-count fields for the SoC (for example
  ``SPI_BOOT_CRYPT_CNT`` / related flash-encryption bits).
* Key block purpose (for example ``XTS_AES_128_KEY``) when using a host key.
* Download-mode and JTAG-related disables after release-mode first boot.

Virtual eFuses (testing)
------------------------

For testing without burning real eFuses, enable virtual eFuse emulation
(persisted in a ``sys_partition`` when
``SB_CONFIG_MCUBOOT_ESP_EFUSE_VIRTUAL_KEEP_IN_FLASH`` is set — selected
together with ``SB_CONFIG_MCUBOOT_ESP_EFUSE_VIRTUAL``):

.. code-block:: shell

   west build -b <board> samples/hello_world --sysbuild \
     -DSB_CONFIG_MCUBOOT_ESP_FLASH_ENCRYPTION=y \
     -DSB_CONFIG_MCUBOOT_ESP_FLASH_ENCRYPTION_DEVELOPMENT=y \
     -DSB_CONFIG_MCUBOOT_ESP_EFUSE_VIRTUAL=y

Additional options under the sysbuild menu
**Espressif hardware secure features** control XTS-AES key size, UART ROM
download mode, XTS-AES pseudo-rounds (where supported), and other development
overrides (JTAG, UART bootloader encryption, and so on). Prefer defaults for
production builds.

.. note::

   Before enabling hardware flash encryption on a physical device, perform a
   full chip erase. This matters because the flash erased value (``0xFF`` in
   most cases) read from trailer regions is part of MCUboot's update-state
   checking; garbage in those regions can look like a valid state.

   .. code-block:: shell

      esptool -p <PORT> erase-flash

   After the first successful boot and flash regions encryption,
   subsequent device re-flashing must use the encryption-aware flow (see
   :ref:`manual_flashing_when_flash_encryption`).
   OTA updates may be performed sending plaintext images, as the flash
   read/write operations are encrypted in runtime.

When hardware flash encryption is enabled, sysbuild configures the
application image and sets :kconfig:option:`CONFIG_ESP_FLASH_ENCRYPTION`
so the flash driver matches the encrypted layout. Image signing must use
32-byte alignment (``--align`` / ``--max-align`` consistent with
``write-block-size``).

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

.. _`ESP32 Flash Encryption`:
   https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/flash-encryption.html
