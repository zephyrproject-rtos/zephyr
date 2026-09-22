.. SPDX-FileCopyrightText: Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
.. SPDX-License-Identifier: Apache-2.0

:orphan:

.. espressif-qemu

Using Espressif QEMU
====================

Espressif boards can be run under `Espressif's QEMU fork
<https://github.com/espressif/qemu>`_ for local, hardware-free testing.
:kconfig:option:`CONFIG_ESPRESSIF_QEMU` generates a merged SPI flash image.
Enabling it also selects the hidden
:kconfig:option:`CONFIG_ESPRESSIF_QEMU_TARGET` board marker.
A ``west build -t run`` target is available only when the board opts into the
``espressif_qemu`` emu platform (the DevKitC boards below do this via a shared
``board.cmake`` helper).

There are two ways to enable Espressif QEMU on the reference DevKitC boards:

1. **Opt-in** with ``-DCONFIG_ESPRESSIF_QEMU=y`` on the hardware board target.
   The same firmware image can then be flashed to hardware (without that
   option) or run under QEMU. Other boards on a supported SoC can set the
   Kconfig as well (flash layout export), but they need the same
   ``SUPPORTED_EMU_PLATFORMS espressif_qemu`` wiring for ``-t run``.
2. **Board variant** on the reference DevKitC targets below (``/qemu``
   qualifier). The variant defconfig sets
   :kconfig:option:`CONFIG_ESPRESSIF_QEMU`, so no ``-D`` is required. Use
   these for local and CI ``west build -t run`` workflows. Twister
   ``simulation:`` metadata for Espressif QEMU is not wired yet; treat
   ``/qemu`` as a board identifier plus emu platform, not a drop-in for
   stock ``qemu_*`` Twister simulation.

Supported SoCs and boards
-------------------------

.. list-table::
   :header-rows: 1

   * - SoC
     - Hardware board
     - QEMU variant (CI)
     - QEMU binary
     - ``-machine``
   * - ESP32
     - ``esp32_devkitc/esp32/procpu``
     - ``esp32_devkitc/esp32/procpu/qemu``
     - ``qemu-system-xtensa``
     - ``esp32``
   * - ESP32-S3
     - ``esp32s3_devkitc/esp32s3/procpu``
     - ``esp32s3_devkitc/esp32s3/procpu/qemu``
     - ``qemu-system-xtensa``
     - ``esp32s3``
   * - ESP32-C3
     - ``esp32c3_devkitc/esp32c3``
     - ``esp32c3_devkitc/esp32c3/qemu``
     - ``qemu-system-riscv32``
     - ``esp32c3``
   * - ESP32-C6
     - ``esp32c6_devkitc/esp32c6/hpcore``
     - ``esp32c6_devkitc/esp32c6/hpcore/qemu``
     - ``qemu-system-riscv32``
     - ``esp32c6``

ESP32-C3 and ESP32-C6 require ``-icount 3`` (added automatically by the
``run`` target). Free-running mode is not supported for these machines.

.. note::

   Pre-built Espressif QEMU releases as of ``esp-develop-9.2.2-20260417``
   include ``esp32``, ``esp32s3`` and ``esp32c3`` only. ESP32-C6 support was
   merged into Espressif's ``esp-develop`` branch in
   https://github.com/espressif/qemu/commit/febae182e132e4055529be423a818225ebddaa3a
   but is not yet in a published release binary. Build QEMU from the
   ``esp-develop`` branch to run ESP32-C6.

Emulated peripherals
--------------------

Per-SoC capability is documented in the `Espressif QEMU feature matrix
<https://github.com/espressif/esp-toolchain-docs/blob/main/qemu/README.md>`_.
There is no published matrix column for ESP32-C6; the C6 values below follow
the current esp-develop ``esp32c6`` machine, not a blind copy of ESP32-C3.

Usable in Zephyr on ``/qemu`` DevKitC boards
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1

   * - Peripheral
     - ESP32
     - ESP32-S3
     - ESP32-C3
     - ESP32-C6
   * - UART console
     - yes
     - yes
     - yes
     - yes
   * - NOR flash (SPI + MMU)
     - yes
     - yes
     - yes
     - yes
   * - eFuse
     - yes (ECO3 image auto)
     - yes
     - yes
     - yes
   * - RNG / TRNG
     - yes
     - yes
     - yes
     - no
   * - AES / SHA / RSA crypto
     - yes
     - yes (+ HMAC, DS)
     - yes (+ HMAC, DS)
     - SHA only
   * - Timer groups / SysTimer
     - yes
     - yes
     - yes
     - yes
   * - TWAI / CAN
     - modeled
     - modeled
     - modeled
     - no
   * - PSRAM (``-m``)
     - QPI 2M/4M
     - QPI/OPI 2M–32M
     - N/A
     - N/A
   * - GDMA
     - no Zephyr smoke test
     - yes (SoC)
     - yes (SoC)
     - yes (SoC)

Not emulated (disabled in ``/qemu`` device trees)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Wi-Fi, Bluetooth, USB, general-purpose SPI, I2C, I2S, RMT, GPIO matrix / IOMUX,
ADC/DAC, touch, MCPWM, pulse counter, ULP, and board GPIO keys. LEDC is emulated
only on ESP32; SD/MMC is not emulated on ESP32-S3. On ESP32-C6, also disable
TRNG, AES, and the HP/LP mailbox (``mbox0``); leave SHA enabled.

Tests that depend on those peripherals will not run under QEMU.

QEMU variant device tree
------------------------

Hardware DevKitC targets keep the full device tree. The ``/qemu`` board variants
include the same hardware ``*.dts`` and overlay ``status = "disabled"`` (or
``/delete-node/``) for nodes QEMU does not model:

* ``boards/espressif/esp32_devkitc/esp32_devkitc_procpu_qemu.dts``
* ``boards/espressif/esp32s3_devkitc/esp32s3_devkitc_procpu_qemu.dts``
* ``boards/espressif/esp32c3_devkitc/esp32c3_devkitc_qemu.dts``
* ``boards/espressif/esp32c6_devkitc/esp32c6_devkitc_hpcore_qemu.dts``

Common overlays disable Wi-Fi, Bluetooth, I2C, I2S, general-purpose SPI, ADC,
and non-emulated PWM-related blocks; remove GPIO key nodes; and clear unsupported
``chosen`` properties. The Xtensa variants also disable touch, pulse counter,
and MCPWM nodes. ESP32-S3 additionally disables its second I2C/SPI/I2S instances,
USB Serial/JTAG and OTG, LEDC, SD/MMC, temperature sensor, and LCD/CAM controller.
ESP32-C3 disables USB Serial/JTAG, LEDC, and its temperature sensor. ESP32-C6
disables IEEE 802.15.4, LP UART, MCPWM, pulse counter, TRNG (and
``zephyr,entropy``), AES, and ``mbox0``; SHA stays available.

UART, flash partitions, timer groups, and watchdog nodes stay available for
smoke tests (for example ``samples/hello_world``). TRNG and full crypto blocks
remain on ESP32 / S3 / C3; ESP32-C6 keeps SHA only.
The GPIO controller nodes and ``CONFIG_GPIO=y`` must remain enabled because the
Espressif UART driver selects the GPIO driver for pin muxing; attempting to
disable them produces a Kconfig dependency error. This is boot infrastructure,
not a claim that QEMU implements application GPIO or the GPIO matrix. GPIO keys
are removed from the device tree.

Automatic vs manual QEMU flags
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``west build -t run`` target appends these flags automatically when
:kconfig:option:`CONFIG_ESPRESSIF_QEMU` is enabled:

* ``-icount 3`` on ESP32-C3 and ESP32-C6
* ESP32 ECO3 eFuse image (see `Flash size`_ / `ESP32 chip revision`_)
* ``-drive file=…/flash_image.bin,if=mtd,format=raw``
* ``-m <size>`` when :kconfig:option:`CONFIG_ESP_SPIRAM` is set; size comes from
  :kconfig:option:`CONFIG_ESP_SPIRAM_SIZE` (default from the board ``psram0``
  ``size`` property in devicetree)
* ``-global driver=ssi_psram,property=is_octal,value=true`` on ESP32-S3 when
  :kconfig:option:`CONFIG_SPIRAM_MODE_OCT` is set

Use the ``QEMU_EXTRA_FLAGS`` environment variable for optional cases (watchdog
disable, SD card on ESP32, and similar).

Installing Espressif QEMU
-------------------------

Download pre-built binaries from
https://github.com/espressif/qemu/releases (Xtensa and RISC-V packages).

Example (Linux x86_64, release ``esp-develop-9.2.2-20260417``)::

   mkdir -p ~/Downloads ~/opt
   cd ~/Downloads
   wget https://github.com/espressif/qemu/releases/download/esp-develop-9.2.2-20260417/qemu-xtensa-softmmu-esp_develop_9.2.2_20260417-x86_64-linux-gnu.tar.xz
   wget https://github.com/espressif/qemu/releases/download/esp-develop-9.2.2-20260417/qemu-riscv32-softmmu-esp_develop_9.2.2_20260417-x86_64-linux-gnu.tar.xz
   tar -xf qemu-xtensa-softmmu-*.tar.xz -C ~/opt --one-top-level=qemu-xtensa-softmmu
   tar -xf qemu-riscv32-softmmu-*.tar.xz -C ~/opt --one-top-level=qemu-riscv32-softmmu

Put both ``bin`` directories on ``PATH``, or set ``ESPRESSIF_QEMU_PATH`` /
``QEMU_BIN_PATH`` to a platform path list containing the directories for the
SoCs you are targeting. A generic distro QEMU without ``-machine esp*`` will
not work.

At CMake configure time each ``qemu-system-xtensa`` / ``qemu-system-riscv32``
found in ``ESPRESSIF_QEMU_PATH``, ``QEMU_BIN_PATH`` and ``PATH`` (in that
order) is probed with ``-machine help``, and the first one that implements the
SoC's machine is used. This skips the Zephyr SDK's own QEMU in
``hosttools``, which is an upstream build without the Espressif machines. The
selected binary is printed as::

   -- Espressif QEMU: /home/user/opt/qemu-xtensa-softmmu/qemu/bin/qemu-system-xtensa (-machine esp32)

Because the lookup happens during configuration, installing QEMU or changing
``ESPRESSIF_QEMU_PATH`` after a build requires re-running CMake
(``west build --pristine``).

Building and running (Simple Boot)
----------------------------------

``west build`` does not use sysbuild unless ``--sysbuild`` is given or
``build.sysbuild`` is set in the west config, so a plain build is a Simple Boot
image with no MCUboot. ``--no-sysbuild`` below makes that explicit and also
overrides a ``build.sysbuild=true`` west config.

**Opt-in** (hardware board + CMake cache entry)::

   west build -b esp32_devkitc/esp32/procpu samples/hello_world \
     --no-sysbuild --pristine \
     -- -DCONFIG_ESPRESSIF_QEMU=y

   west build -t run

**QEMU board variant** (defconfig supplies :kconfig:option:`CONFIG_ESPRESSIF_QEMU`)::

   west build -b esp32_devkitc/esp32/procpu/qemu samples/hello_world \
     --no-sysbuild --pristine

   west build -t run

The build produces ``build/zephyr/flash_image.bin`` (merged SPI image) and
registers the ``espressif_qemu`` emu platform (``run_espressif_qemu`` /
``debugserver_espressif_qemu``, aliased as ``west build -t run`` /
``debugserver``). On ESP32 it also generates
``build/zephyr/qemu_efuse_eco3.bin`` and passes it to QEMU. Equivalent manual
invocation::

   qemu-system-xtensa -nographic -machine esp32 \
     -drive file=build/zephyr/qemu_efuse_eco3.bin,if=none,format=raw,id=efuse \
     -global driver=nvram.esp32.efuse,property=drive,value=efuse \
     -drive file=build/zephyr/flash_image.bin,if=mtd,format=raw

ESP32-C3 opt-in example::

   west build -b esp32c3_devkitc/esp32c3 samples/hello_world \
     --no-sysbuild --pristine \
     -- -DCONFIG_ESPRESSIF_QEMU=y
   west build -t run

ESP32-C3 variant (no ``-D``)::

   west build -b esp32c3_devkitc/esp32c3/qemu samples/hello_world \
     --no-sysbuild --pristine
   west build -t run
   # or:
   qemu-system-riscv32 -nographic -icount 3 -machine esp32c3 \
     -drive file=build/zephyr/flash_image.bin,if=mtd,format=raw

MCUboot / sysbuild
------------------

With sysbuild, :kconfig:option:`CONFIG_ESPRESSIF_QEMU` merges MCUboot
(``boot_partition``) and the signed application (``slot0_partition``) into
the same flash image::

   west build -b esp32_devkitc/esp32/procpu samples/hello_world \
     --sysbuild --pristine \
     -- -DCONFIG_ESPRESSIF_QEMU=y

   west build --domain hello_world -t run

``--domain`` is required here: ``run`` is defined by the application image, and
the sysbuild top-level build has no such target. The image is written to
``build/<app>/zephyr/flash_image.bin`` rather than ``build/zephyr/``.

The merge consumes ``../mcuboot/zephyr/zephyr.bin`` from the bootloader domain.
Sysbuild adds each image as an independent external project, so these DevKitC
boards' ``sysbuild.cmake`` always orders the application after ``mcuboot``
whenever both ExternalProject targets exist (the dependency is not gated on
:kconfig:option:`CONFIG_ESPRESSIF_QEMU`, which is not reliably visible in the
sysbuild CMake context). Without that ordering the two domains build
concurrently and the merge can read an incomplete bootloader binary.

A non-sysbuild build with :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` does not
produce a bootloader binary in the tree (on hardware MCUboot is already
flashed). For QEMU, either use ``--sysbuild`` or pass an existing bootloader
image with ``-DESPRESSIF_QEMU_MCUBOOT_BIN=<path to mcuboot zephyr.bin>``. The
path must be absolute, because the merge runs inside the build directory.

GDB debugging
-------------

Start QEMU with the CPU held in reset and a GDB stub on port 1234::

   west build -t debugserver

Under sysbuild, add ``--domain <app>`` as for ``run``. Then attach from another
terminal, using the GDB that ships with the Zephyr SDK::

   $ZEPHYR_SDK_INSTALL_DIR/gnu/xtensa-espressif_esp32_zephyr-elf/bin/xtensa-espressif_esp32_zephyr-elf-gdb \
     build/zephyr/zephyr.elf \
     -ex "target remote :1234" \
     -ex "tb main" -ex "c"

For the RISC-V SoCs (ESP32-C3, ESP32-C6) use
``gnu/riscv64-zephyr-elf/bin/riscv64-zephyr-elf-gdb`` instead. The ESP-IDF
toolchain GDBs (``xtensa-esp32-elf-gdb``, ``riscv32-esp-elf-gdb``) also work if
you have an ESP-IDF environment on ``PATH``.

Advanced QEMU arguments
-----------------------

Extra flags can be injected via the ``QEMU_EXTRA_FLAGS`` environment
variable (space-separated) read at CMake configure time: set it before the
first build or re-run CMake for a change to take effect. Options **not**
appended automatically:

* **Disable TG watchdogs:**
  ``-global driver=timer.esp32.timg,property=wdt_disable,value=true``
  (ESP32-S3, ESP32-C3, and ESP32-C6 use the
  ``timer.esp32c3.timg`` property name)
* **SD/MMC (ESP32 only):**
  ``-drive file=sdcard.img,if=sd,format=raw``
* **Custom eFuse storage:** configure with
  ``-DESPRESSIF_QEMU_EFUSE_HEX_FILE=/path/to/efuse.hex``. Whitespace is
  allowed in the hex file. The build copies it into the build directory,
  decodes it with Python, and passes the resulting binary to the machine's
  eFuse device. Without this override, ESP32 uses a built-in ECO3 image and
  the other SoCs attach no eFuse drive. The source tree is never written.

  See the per-SoC pages under
  https://github.com/espressif/esp-toolchain-docs/tree/main/qemu for efuse
  layouts and strap modes.

PSRAM (automatic when SPIRAM is enabled)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When :kconfig:option:`CONFIG_ESP_SPIRAM` is enabled, CMake appends ``-m <size>`` where
``<size>`` is derived from :kconfig:option:`CONFIG_ESP_SPIRAM_SIZE` (bytes ÷
1 MiB, suffixed with ``M``). Unsupported sizes fail configure (do not pass a
half-supported ``-m``). The ESP32 ``/qemu`` variant clamps ``psram0`` to 4M
because the hardware WROVER N4R8 tree defaults to 8M.

Documented QEMU sizes:

* **ESP32:** 2M, 4M (PSRAM MMU is not fully emulated)
* **ESP32-S3:** 2M, 4M, 8M, 16M, or 32M; octal (OPI) mode also adds the
  ``ssi_psram`` ``is_octal`` global when
  :kconfig:option:`CONFIG_SPIRAM_MODE_OCT=y`

Override or supplement with ``QEMU_EXTRA_FLAGS`` if needed.

Flash size
----------

The merged image is padded to the board's ``zephyr,flash`` size from
devicetree so the SPI flash header and the QEMU MTD size match. Espressif
QEMU supports 2, 4, 8 and 16 MB; use a board or overlay whose flash size is
one of those values.

ESP32 chip revision
-------------------

Default QEMU efuses report ESP32 revision 0, which Zephyr rejects unless
:kconfig:option:`CONFIG_ESP32_USE_UNSUPPORTED_REVISION` is enabled. With
:kconfig:option:`CONFIG_ESPRESSIF_QEMU`, the ``run`` / ``debugserver`` targets
automatically attach an ECO3 eFuse image (``CHIP_VER_REV1`` and
``CHIP_VER_REV2`` set) so the guest reports chip revision v3.0.

At configure time the ECO3 hex is written under the build directory and
decoded with Python into ``build/zephyr/qemu_efuse_eco3.bin``. Override with
``-DESPRESSIF_QEMU_EFUSE_HEX_FILE=...`` at configure time (or pass the same
``-D`` on a later ``west build`` / ``west build -t run``, which re-runs CMake).
An environment variable alone is not enough: the path is read only during
CMake configure. The layout matches
`Emulating ESP32 ECO3
<https://github.com/espressif/esp-toolchain-docs/blob/main/qemu/esp32/README.md#emulating-esp32-eco3>`_.

Equivalent manual QEMU invocation::

   qemu-system-xtensa -nographic -machine esp32 \
     -drive file=build/zephyr/qemu_efuse_eco3.bin,if=none,format=raw,id=efuse \
     -global driver=nvram.esp32.efuse,property=drive,value=efuse \
     -drive file=build/zephyr/flash_image.bin,if=mtd,format=raw
