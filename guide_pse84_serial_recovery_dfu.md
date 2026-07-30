# PSE84 DFU User Guide: MCUboot Serial Recovery (kit_pse84_eval)

A step-by-step guide to update either the CM33 Non-Secure or the CM55
application image on the Infineon PSE84 board using MCUboot's
serial recovery mode over `mcumgr`.

In serial recovery the bootloader itself hosts the SMP/mcumgr transport, so no
application needs to be running to accept an update. Uploads land directly in
the target image's primary slot (no swap), which makes this the most robust
way to recover or re-provision a single image.

All commands assume the `pse846gps4dbzc4a` (epc4) board variant, an OpenOCD
install under `~/Downloads/openocd`, the Go `mcumgr` client at `~/go/bin/mcumgr`,
and a west workspace whose Zephyr repo is pointed to by `$ZEPHYR_BASE`. Adjust
those paths to match your host.

---

## 1. Prerequisites

### 1.1 Toolchain
* A built west workspace (`west`, Zephyr SDK, venv) with the Zephyr repo at
  `$ZEPHYR_BASE`.
* OpenOCD with PSE84 support (`~/Downloads/openocd/bin/openocd`,
  scripts under `~/Downloads/openocd/scripts`).
* The Go `mcumgr` client at `~/go/bin/mcumgr`
  (`go install github.com/apache/mynewt-mcumgr-cli/mcumgr@latest`).

### 1.2 Open a shell
```bash
cd <your-west-workspace>          # dir containing .west/
source .venv/bin/activate
# Export explicitly; used verbatim below and avoids a stale CMake-registry clone.
export ZEPHYR_BASE=$PWD/<zephyr-repo-dir>   # e.g. .../ifx-zephyr
```

Console = `/dev/ttyACM0` (SCB1), mcumgr = `/dev/ttyACM1` (SCB2). These map by
USB serial and can swap after a replug — re-derive per session
(`udevadm info -q property -n /dev/ttyACM0 | grep ID_MODEL`).

---

## 2. Build

MCUboot is built for the secure M33 core; its config sets
`UPDATEABLE_IMAGE_NUMBER=3` so it manages all three images (CM33-S, CM33-NS,
CM55). The applications are built with `kit_pse84_eval_slot.conf`, which enables
MCUboot chain-loading.

### 2.1 Bootloader (MCUboot, serial recovery enabled)

```bash
west build -p always -b kit_pse84_eval/pse846gps4dbzc4a/m33 -d build_mcuboot \
  bootloader/mcuboot/boot/zephyr \
  -- -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_mcuboot.conf" \
     -DEXTRA_DTC_OVERLAY_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_mcuboot_bl.overlay"
```

* `kit_pse84_eval_mcuboot.conf`: swap-using-move bootloader,
  `UPDATEABLE_IMAGE_NUMBER=3`, serial recovery over SCB2 with GPIO entrance,
  and `BOOT_SERIAL_UNALIGNED_BUFFER_SIZE=256` (matches the QSPI 256-byte
  write-block to avoid corrupting uploads).
* `kit_pse84_eval_mcuboot_bl.overlay`: console on SCB1, `uart-mcumgr` on
  SCB2, and maps `mcuboot-button0` to SW2 (P8.3) as the recovery
  entrance button.

### 2.2 Baseline CM33 Non-Secure app (image 1)

The CM33-NS core launches the CM55, so it needs
`-DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y` (it compiles in the `Cy_SysEnableCM55()`
release). Both cores must set this symbol.

```bash
west build -p always -b kit_pse84_eval/pse846gps4dbzc4a/m33/ns -d build_cm33ns \
  $ZEPHYR_BASE/samples/hello_world \
  -- -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_slot.conf" \
     -DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y
```

### 2.3 Baseline CM55 app (image 2)

The CM55 build needs `-DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y` (SRF client) and
`PSE84_CM33_BUILD_DIR` pointing at the NS build directory from §2.2 (built
first — the CM55 build consumes its TF-M PSA headers).

```bash
west build -p always -b kit_pse84_eval/pse846gps4dbzc4a/m55 -d build_cm55 \
  $ZEPHYR_BASE/samples/basic/blinky \
  -- -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_slot.conf" \
     -DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y \
     -DPSE84_CM33_BUILD_DIR=build_cm33ns
```

### 2.4 Build the update payload(s)

Build an *observably different* image so you can confirm the update visually.
Pick whichever image you intend to update. A CM33-NS payload keeps the SRF flag
so it still launches the CM55 after the update.

```bash
# New CM33-NS payload (e.g. blinky instead of hello_world):
west build -p always -b kit_pse84_eval/pse846gps4dbzc4a/m33/ns -d build_cm33ns_new \
  $ZEPHYR_BASE/samples/basic/blinky \
  -- -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_slot.conf" \
     -DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y

# New CM55 payload (e.g. hello_world instead of blinky):
west build -p always -b kit_pse84_eval/pse846gps4dbzc4a/m55 -d build_cm55_new \
  $ZEPHYR_BASE/samples/hello_world \
  -- -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_slot.conf" \
     -DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y \
     -DPSE84_CM33_BUILD_DIR=build_cm33ns
```

---

## 3. Flash the baseline

`west flash` picks the correct signed artifact for each build (the NS build
programs `tfm_merged.hex` automatically).

```bash
for d in build_mcuboot build_cm33ns build_cm55; do
  west flash -d $d --openocd ~/Downloads/openocd/bin/openocd \
                   --openocd-search ~/Downloads/openocd/scripts
done
```

### 3.1 Verify a normal boot
Open the console on `/dev/ttyACM0` at 115200
(e.g. `picocom -b 115200 /dev/ttyACM0`). You
should see MCUboot chain-load all three images:

```
*** Booting MCUboot v2.4.0-... ***
*** Using Zephyr OS build v4.4.1-... ***
I: Starting bootloader
I: Image index: 0, Swap type: none
I: Image index: 1, Swap type: none
I: Image index: 2, Swap type: none
I: Bootloader chainload address offset: 0x100000
...
Hello World! kit_pse84_eval/pse846gps4dbzc4a/m33/ns
```

---

## 4. Enter serial recovery mode

Serial recovery is entered by holding the GPIO entrance button while resetting:

1. Locate user button SW2 (P8.3).
2. Press and hold SW2.
3. While still holding, press the physical RESET button (or power-cycle).
4. Release SW2.
5. Watch the console on `/dev/ttyACM0`. It must show:

```
*** Booting MCUboot v2.4.0-... ***
I: Starting bootloader
I: Enter the serial recovery mode
```

The bootloader now stays in the loop and listens for `mcumgr` frames on
`/dev/ttyACM1`. Confirm the link:

```bash
~/go/bin/mcumgr --conntype serial \
  --connstring "dev=/dev/ttyACM1,baud=115200,mtu=512" image list
```

---

## 5. Update the image

Because `CONFIG_MCUBOOT_SERIAL_DIRECT_IMAGE_UPLOAD` is not set, the upload
goes directly into the target image's primary slot. There is no swap and no
`image test`/`image confirm` step.

> Serial-recovery image numbering (bootloader `UPDATEABLE_IMAGE_NUMBER=3`):
> | `-n` | Image | Primary slot |
> |------|-------|--------------|
> | `-n 0` | CM33 Secure (TF-M) | slot0 @ `0x100000` |
> | `-n 1` | CM33 Non-Secure    | slot2 @ `0x340000` |
> | `-n 2` | CM55               | slot4 @ `0x580000` |

### 5.1 Update the CM33 Non-Secure image (`-n 1`)

```bash
~/go/bin/mcumgr --conntype serial \
  --connstring "dev=/dev/ttyACM1,baud=115200,mtu=512" \
  image upload -e -n 1 build_cm33ns_new/zephyr/zephyr.signed.bin
```

### 5.2 Update the CM55 image (`-n 2`)

```bash
~/go/bin/mcumgr --conntype serial \
  --connstring "dev=/dev/ttyACM1,baud=115200,mtu=512" \
  image upload -e -n 2 build_cm55_new/zephyr/zephyr.signed.bin
```

* `-e` erases the destination slot before writing (recommended).
* Wait for the transfer to reach `Done`.

---

## 6. Reset and verify

Reboot normally (press RESET without holding SW2, or re-flash MCUboot to reset —
this only rewrites the bootloader region and leaves the recovered image intact):

```bash
west flash -d build_mcuboot --openocd ~/Downloads/openocd/bin/openocd --openocd-search ~/Downloads/openocd/scripts
```

Because the image was written straight to its primary slot, MCUboot reports
`Swap type: none` and chain-loads the new firmware immediately:

```
I: Image index: 0, Swap type: none
I: Image index: 1, Swap type: none
I: Image index: 2, Swap type: none
I: Bootloader chainload address offset: 0x100000
...
LED state: OFF          <- new CM33-NS blinky payload
LED state: ON
```

The serial-recovery update is complete.
