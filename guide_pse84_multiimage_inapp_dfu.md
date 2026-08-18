# PSE84 DFU User Guide: Multi-Image In-App DFU (kit_pse84_eval)

A step-by-step guide to update both application images (CM33 Non-Secure
and CM55) at run time from a single `smp_svr` application over
`mcumgr`/SMP, with no bootloader recovery button required.

One overlay pair (`smp_app.overlay` + `dfu_multi.overlay`) gives the hosting
core a 2-image slot layout with a fixed identity:

* `-n 0` = CM33 Non-Secure image
* `-n 1` = CM55 image

`smp_svr` can run on either core; whichever core hosts it can update both
images. The *other* core runs a plain application built to free the mcumgr UART.

All commands assume the `pse846gps2dbzc4a` (epc4) board variant, an OpenOCD
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
* The Go `mcumgr` client at `~/go/bin/mcumgr`.

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

## 2. Choose a hosting core

Decide which core runs `smp_svr` (the DFU host). This guide's flashing example
uses CM55 as the host and CM33-NS as the plain peer; the reverse case is
noted inline.

| | Host = CM33-NS | Host = CM55 |
|---|---|---|
| smp_svr build | `build_ns_host` | `build_cm55_host` |
| plain peer build | `build_cm55_plain` (CM55) | `build_ns_plain` (CM33-NS) |

The host owns SCB2 (mcumgr); the peer is built with `scb2_disable.overlay` so it
does not re-init SCB2 and moves its own console to SCB1.

---

## 3. Build

### 3.1 Bootloader (MCUboot)

```bash
west build -p always -b kit_pse84_eval/pse846gps2dbzc4a/m33 -d build_mcuboot \
  bootloader/mcuboot/boot/zephyr \
  -- -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_mcuboot.conf" \
     -DEXTRA_DTC_OVERLAY_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_mcuboot_bl.overlay"
```

### 3.2 Build the `smp_svr` host

Overlays: `smp_app.overlay` (mcumgr on SCB2 + console on SCB1) plus
`dfu_multi.overlay` (the 2-image slot layout).
Confs: `slot.conf` (chain-load) + the sample's `serial.conf` (SMP-over-UART) +
`smp_app.conf` (large mcumgr buffers/MTU 2048) + `dfu_multi.conf`.

> **Required on *every* multicore build (host, peer, both payloads):**
> `-DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y`. It compiles in the CM33-NS
> `Cy_SysEnableCM55()` (releases CM55) and the CM55 SRF client. Omit it on any
> image and the cores disagree — a CM33-NS image without it never starts the
> CM55, so a CM55 host (`smp_svr`) never runs.

Option A (CM33-NS hosts `smp_svr`):
```bash
west build -p always -b kit_pse84_eval/pse846gps2dbzc4a/m33/ns -d build_ns_host \
  $ZEPHYR_BASE/samples/subsys/mgmt/mcumgr/smp_svr \
  -- -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_slot.conf;$ZEPHYR_BASE/samples/subsys/mgmt/mcumgr/smp_svr/serial.conf;$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_smp_app.conf;$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_dfu_multi.conf" \
     -DEXTRA_DTC_OVERLAY_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_smp_app.overlay;$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_dfu_multi.overlay" \
     -DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y
```

Option B (CM55 hosts `smp_svr`) (also needs `PSE84_CM33_BUILD_DIR`
pointing at the CM33-NS peer build from §3.3, built *first*):
```bash
west build -p always -b kit_pse84_eval/pse846gps2dbzc4a/m55 -d build_cm55_host \
  $ZEPHYR_BASE/samples/subsys/mgmt/mcumgr/smp_svr \
  -- -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_slot.conf;$ZEPHYR_BASE/samples/subsys/mgmt/mcumgr/smp_svr/serial.conf;$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_smp_app.conf;$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_dfu_multi.conf" \
     -DEXTRA_DTC_OVERLAY_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_smp_app.overlay;$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_dfu_multi.overlay" \
     -DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y \
     -DPSE84_CM33_BUILD_DIR=build_ns_plain
```

> **Build order (CM55 host case).** CM55 consumes the CM33-NS build's TF-M PSA
> headers — build the CM33-NS peer (§3.3) first and point `PSE84_CM33_BUILD_DIR`
> at it.

### 3.3 Build the plain peer (the core NOT hosting `smp_svr`)

`scb2_disable.overlay` frees the SCB2 mcumgr UART for the host and moves the
peer console to SCB1. The CM33-NS peer also needs
`-DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y` (it starts the CM55).

Peer = CM55 (use when CM33-NS hosts):
```bash
west build -p always -b kit_pse84_eval/pse846gps2dbzc4a/m55 -d build_cm55_plain \
  $ZEPHYR_BASE/samples/basic/blinky \
  -- -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_slot.conf" \
     -DEXTRA_DTC_OVERLAY_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_scb2_disable.overlay" \
     -DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y \
     -DPSE84_CM33_BUILD_DIR=build_ns_host
```

Peer = CM33-NS (use when CM55 hosts):
```bash
west build -p always -b kit_pse84_eval/pse846gps2dbzc4a/m33/ns -d build_ns_plain \
  $ZEPHYR_BASE/samples/basic/blinky \
  -- -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_slot.conf" \
     -DEXTRA_DTC_OVERLAY_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_scb2_disable.overlay" \
     -DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y
```

### 3.4 Build observable DFU payloads (one per image)

Build a *different* app per image so the swap is visible. Both payloads also
pass `-DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y` so the swapped-in images keep the
second core running.

```bash
# New CM33-NS payload (image 0):
west build -p always -b kit_pse84_eval/pse846gps2dbzc4a/m33/ns -d build_ns_payload \
  $ZEPHYR_BASE/samples/hello_world \
  -- -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_slot.conf" \
     -DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y

# New CM55 payload (image 1):
west build -p always -b kit_pse84_eval/pse846gps2dbzc4a/m55 -d build_cm55_payload \
  $ZEPHYR_BASE/samples/hello_world \
  -- -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_slot.conf" \
     -DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y \
     -DPSE84_CM33_BUILD_DIR=build_ns_payload
```

> **Payload console.** With only `slot.conf` (+ SRF flag), payload consoles stay
> on board-default **SCB2** (`/dev/ttyACM1`); MCUboot swap logs are on SCB1
> (`/dev/ttyACM0`). Add `scb2_disable.overlay` (peer) / `smp_app.overlay` (host)
> to route payload output to SCB1.

---

## 4. Flash the baseline

Flash MCUboot, the plain peer, and the `smp_svr` host. `west flash` selects the
right signed artifact automatically (NS uses `tfm_merged.hex`).

```bash
# Example: CM55 hosts smp_svr, CM33-NS is the plain peer.
west flash -d build_mcuboot     --openocd ~/Downloads/openocd/bin/openocd --openocd-search ~/Downloads/openocd/scripts
west flash -d build_ns_plain    --openocd ~/Downloads/openocd/bin/openocd --openocd-search ~/Downloads/openocd/scripts
west flash -d build_cm55_host   --openocd ~/Downloads/openocd/bin/openocd --openocd-search ~/Downloads/openocd/scripts

# For the CM33-NS-hosts case instead flash:
#   build_mcuboot, build_ns_host, build_cm55_plain
```

---

## 5. Update both images over mcumgr

Open the console on `/dev/ttyACM0` (115200) to watch
swaps while you drive DFU from another terminal.

> In-app image numbering (app `UPDATEABLE_IMAGE_NUMBER=2`):
> | `-n` | Image |
> |------|-------|
> | `-n 0` | CM33 Non-Secure |
> | `-n 1` | CM55 |

### 5.1 Confirm the transport and list images

```bash
~/go/bin/mcumgr --conntype serial \
  --connstring "dev=/dev/ttyACM1,baud=115200,mtu=512" image list
# Expect: image=0 (CM33-NS) and image=1 (CM55), each slot=0 active.
```

> The first `mcumgr` call after boot often returns `NMP timeout`; just
> retry it once.

### 5.2 Update the CM55 image (`-n 1`)

```bash
~/go/bin/mcumgr --conntype serial \
  --connstring "dev=/dev/ttyACM1,baud=115200,mtu=512" \
  image upload -e -n 1 build_cm55_payload/zephyr/zephyr.signed.bin

~/go/bin/mcumgr --conntype serial \
  --connstring "dev=/dev/ttyACM1,baud=115200,mtu=512" image list
# Copy the image=1 slot=1 hash printed above, then mark it for test:
~/go/bin/mcumgr --conntype serial \
  --connstring "dev=/dev/ttyACM1,baud=115200,mtu=512" image test <image1-slot1-hash>
```

### 5.3 Update the CM33-NS image (`-n 0`)

```bash
~/go/bin/mcumgr --conntype serial \
  --connstring "dev=/dev/ttyACM1,baud=115200,mtu=512" \
  image upload -e -n 0 build_ns_payload/zephyr/zephyr.signed.bin

~/go/bin/mcumgr --conntype serial \
  --connstring "dev=/dev/ttyACM1,baud=115200,mtu=512" image list
# Copy the image=0 slot=1 hash, then:
~/go/bin/mcumgr --conntype serial \
  --connstring "dev=/dev/ttyACM1,baud=115200,mtu=512" image test <image0-slot1-hash>
```

### 5.4 Reset to trigger the swap

Re-flash the host image; `west flash` resets the board on completion, which
triggers MCUboot to apply the pending swaps. (Primary-slot re-write only; the
staged payloads in the secondary slots are untouched.)

```bash
west flash -d build_cm55_host --openocd ~/Downloads/openocd/bin/openocd --openocd-search ~/Downloads/openocd/scripts
```

> Update the HOST core's own image LAST. Swapping the image that runs
> `smp_svr` tears down the mcumgr transport, so finish the *peer* image first,
> then update the host image and reset.

---

## 6. Verify

On reset, MCUboot performs the pending swaps and boots the new payloads. On
`/dev/ttyACM0` you should see (no assert):

```
*** Booting MCUboot v2.4.0-... ***
I: Starting bootloader
I: Image index: 1, Swap type: test     (CM33-NS) -> copying 0x9f00 bytes
I: Image index: 2, Swap type: test     (CM55)    -> copying 0xc500 bytes
I: Bootloader chainload address offset: 0x100000
...
Hello World! kit_pse84_eval/pse846gps2dbzc4a/m33/ns
Hello World! kit_pse84_eval/pse846gps2dbzc4a/m55
```

(The `Image index` values above are MCUboot's absolute bootloader indices:
CM33-NS is bootloader index 1 and CM55 is index 2, which is why the swap lines
read 1 and 2 even though you addressed them over mcumgr as `-n 0` and `-n 1`.)

To make the swap permanent (avoid revert on the next reset), confirm the running
images:

```bash
~/go/bin/mcumgr --conntype serial \
  --connstring "dev=/dev/ttyACM1,baud=115200,mtu=512" image confirm
```

---

## 7. Multi-Core DFU over BLE (Bluetooth Low Energy)

To update over Bluetooth (BLE) instead of a physical UART, you must build the CM33-NS host with the `cyw55513` Bluetooth stack enabled, and disable the MCUboot serial recovery transport to avoid hardware conflicts.

### 7.1 Bootloader (MCUboot) without Serial Recovery

Use the BLE companion configurations to explicitly disable serial recovery:

```bash
west build -p always -b kit_pse84_eval/pse846gps2dbzc4a/m33 -d build_mcuboot \
  bootloader/mcuboot/boot/zephyr \
  -- -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_mcuboot.conf;$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_mcuboot_ble.conf" \
     -DEXTRA_DTC_OVERLAY_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_mcuboot_bl.overlay;$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_mcuboot_ble.overlay"
```

### 7.2 Build the `smp_svr` host with Bluetooth

Overlays: `dfu_multi.overlay`.
Confs: `slot.conf` + the sample's `bt.conf` + `smp_app.conf` + `dfu_multi.conf`.

```bash
west build -p always -b kit_pse84_eval/pse846gps2dbzc4a/m33/ns -d build_ns_host \
  $ZEPHYR_BASE/samples/subsys/mgmt/mcumgr/smp_svr \
  -- -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_slot.conf;$ZEPHYR_BASE/samples/subsys/mgmt/mcumgr/smp_svr/bt.conf;$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_smp_app.conf;$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_dfu_multi.conf" \
     -DEXTRA_DTC_OVERLAY_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_dfu_multi.overlay" \
     -DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y
```

### 7.3 Flash the baseline and connect

Build the plain peer payload exactly as outlined in §3.3 and flash all three binaries. The host will begin advertising as **Zephyr**.

Use the BLE transport in `mcumgr` to upload your payloads. For example, to upload the CM55 payload (`-n 1`):

> **Note:** The `-w 1` (window size 1) flag is highly recommended for BLE. It forces `mcumgr` to wait for a response before sending the next packet, controlling the speed and preventing Bluetooth buffer overflows.

```bash
~/go/bin/mcumgr --conntype ble --connstring "peer_name=Zephyr" \
  image upload -e -n 1 -w 1 build_cm55_payload/zephyr/zephyr.signed.bin
```

### 7.4 Permanent Swap Workaround for XIP (CRITICAL)

When the CM55 executes code (XIP) directly from its primary flash partition, the host core cannot safely write the `image_ok` trailer to that partition to confirm the swap (this results in `Error: 1` / `IMG_MGMT_ERR_UNKNOWN`).

To avoid this flash collision, **you must confirm the payload while it is still in the secondary slot**, *before* triggering the swap:

```bash
# 1. Confirm the payload BEFORE rebooting!
~/go/bin/mcumgr --conntype ble --connstring "peer_name=Zephyr" image confirm <hash-of-new-payload>

# 2. Reset to apply the permanent swap
~/go/bin/mcumgr --conntype ble --connstring "peer_name=Zephyr" reset
```

> MCUboot will apply the permanent swap and handle the primary slot confirmation itself during boot, safely bypassing the XIP conflict.
