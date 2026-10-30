<!--
SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
SPDX-License-Identifier: Apache-2.0
-->

# nRF54L15 DK two-board UART setup

This guide builds and runs the original `micro_core` communication tests over
UART on two nRF54L15 DKs. UART Port 1 carries protocol data between the boards,
while UART Port 0 remains available for console output on both boards. The
sample links the external `micro_core` communication sources, not Zephyr's
`CONFIG_MC_COMM` or `CONFIG_MC_MSG` implementation.

## Working directory

Run the build and flash commands from the UART sample directory. In every new
build terminal, first run:

```sh
cd /run/media/mahdi/Storage/repositories/rasna/mc_message/zephyr/samples/subsys/mc/uart
```

You can confirm that you are in the correct directory with:

```sh
pwd
ls build_flash.sh prj_sender.conf prj_receiver.conf
```

The expected `pwd` output is:

```text
/run/media/mahdi/Storage/repositories/rasna/mc_message/zephyr/samples/subsys/mc/uart
```

There is intentionally no default `prj.conf`. Every build must explicitly use
either `prj_sender.conf` or `prj_receiver.conf`; the helper script does this
automatically.

## Assigned boards

| Role | Probe label | ID reported by `nrfutil` | Console |
|---|---:|---:|---|
| Sender | `001057753351` | `1057753351` | VCOM0; look up the current port |
| Receiver | `001057744552` | `1057744552` | VCOM0; look up the current port |

Linux device names can change after reconnecting USB. Run
`nrfutil device list` and use the `vcom: 0` entry for each board's console.

Set `MICRO_CORE_ROOT` to a checkout of the MIT-licensed `micro_core` source
that contains `src/io/communication/mc_communication.c`. On the tested host:

```sh
export MICRO_CORE_ROOT=/home/mahdi/workstation/micro_core
```

The original host reference is
`$MICRO_CORE_ROOT/test/io/communication/udp/test.c`. The UART port in this
sample retains its case names, payload generators, validation, and fault rates.

## Required software

Install an nRF Connect SDK toolchain containing:

- `west`
- Zephyr SDK/Arm compiler
- `nrfutil` with device support
- `picocom` or another 115200-baud serial terminal

The tested setup uses the nRF Connect SDK Toolchain v3.4.0 and the local
Zephyr tree in this workspace. Open an nRF Connect SDK terminal before
running the commands. Confirm the tools and boards:

```sh
west --version
nrfutil device list
```

If Linux denies access to `/dev/ttyACM*`, add the current user to the serial
device group, log out, and log back in:

```sh
sudo usermod -aG dialout "$USER"
```

## UART allocation

| Function | Zephyr device | Signals | USB interface |
|---|---|---|---|
| `micro_core` protocol | UART20 / Serial Port 1 | P1.04 TXD1, P1.05 RXD1 | VCOM1; disable it |
| Console | UART30 / Serial Port 0 | P0.00 TXD0, P0.01 RXD0 | VCOM0 |

The protocol UART uses 1000000 baud and the console uses 115200 baud. Both use
8 data bits, no parity, and one stop bit. The
protocol uses only TX, RX, and GND; RTS/CTS hardware flow control is not used.
UART frames use escaped 0xC0 boundaries so the receiver can discard a partial
frame and resynchronize at the next one. The fault simulator still corrupts or
drops the decoded `micro_core` windows.

### Disable VCOM1 on both board controllers

The onboard debugger's VCOM1 interface is electrically connected to
P1.04/P1.05. Merely closing `/dev/ttyACM1` and `/dev/ttyACM3` was not
sufficient on the tested boards: its TX signal corrupted the external UART
link. Disable VCOM1 persistently on both board controllers before testing.
The following preserves the other PCA10156 revision 1.0.0 factory settings
and changes only VCOM1 enable pin 22 to `false`:

```sh
BOARD_CONFIG='{"operations":[{"operation":{"type":"smp","operation":2,"group_id":64,"command_id":0,"sequence_number":0,"data":[[6,true,9,false,20,false,22,false,23,false,42,true,45,true,47,true],[1,1800]]},"operationId":"1"}]}'

nrfutil device x-execute-batch --serial-number 1057753351 \
  --batch-json "$BOARD_CONFIG"
nrfutil device x-execute-batch --serial-number 1057744552 \
  --batch-json "$BOARD_CONFIG"
```

Both commands must finish with an SMP write response whose `operation` is
`3` and whose `data` object is empty. VCOM0 remains enabled for console logs.
This setting persists across resets and firmware flashes.

## Wiring

Disconnect USB power while changing jumper wires. Remove any previous wires
from P0.00/P0.01 or P1.11/P1.12. Connect exactly three wires:

```text
Sender board (1057753351)          Receiver board (1057744552)

P1.04 / TXD1  ------------------> P1.05 / RXD1
P1.05 / RXD1  <------------------ P1.04 / TXD1
GND           ------------------- GND
```

TX must cross to RX. Do not connect TX to TX or RX to RX. Do not connect 3 V,
5 V, or VDD between the boards; each board is powered by its own USB cable.

## Automated build and flash

Change to the sample directory and make the helper executable once:

```sh
cd /run/media/mahdi/Storage/repositories/rasna/mc_message/zephyr/samples/subsys/mc/uart
chmod +x build_flash.sh
```

Build and flash the receiver first:

```sh
MICRO_CORE_ROOT=/home/mahdi/workstation/micro_core ./build_flash.sh receiver
```

Build and flash the sender second:

```sh
MICRO_CORE_ROOT=/home/mahdi/workstation/micro_core ./build_flash.sh sender
```

The script uses these role assignments by default:

```sh
SENDER_DEVICE_ID=1057753351
RECEIVER_DEVICE_ID=1057744552
```

They can be overridden without editing the script, for example:

```sh
SENDER_DEVICE_ID=123456789 ./build_flash.sh sender
```

## Manual build and flash commands

First change to the sample directory:

```sh
cd /run/media/mahdi/Storage/repositories/rasna/mc_message/zephyr/samples/subsys/mc/uart
```

Then run the following commands.

Receiver:

```sh
west -z /run/media/mahdi/Storage/repositories/rasna/mc_message/zephyr \
  build -b nrf54l15dk/nrf54l15/cpuapp \
  . -d build_receiver --pristine=always \
  -- -DCONF_FILE=prj_receiver.conf -DMICRO_CORE_ROOT="$MICRO_CORE_ROOT"

west flash -d build_receiver --dev-id 1057744552
```

Sender:

```sh
west -z /run/media/mahdi/Storage/repositories/rasna/mc_message/zephyr \
  build -b nrf54l15dk/nrf54l15/cpuapp \
  . -d build_sender --pristine=always \
  -- -DCONF_FILE=prj_sender.conf -DMICRO_CORE_ROOT="$MICRO_CORE_ROOT"

west flash -d build_sender --dev-id 1057753351
```

## Run the test

Run `nrfutil device list` and identify each board's `vcom: 0` port from its
serial number. Sender is `1057753351`; receiver is `1057744552`. The
`/dev/ttyACM*` names can change after flashing or reconnecting. Close other
serial monitors, then capture each VCOM0 from separate terminals:

```sh
RECEIVER_VCOM0=/dev/serial/by-id/usb-SEGGER_J-Link_001057744552-if00
stty -F "$RECEIVER_VCOM0" 115200 raw -echo
timeout 1800 cat "$RECEIVER_VCOM0" | tee receiver.log
```

```sh
SENDER_VCOM0=/dev/serial/by-id/usb-SEGGER_J-Link_001057753351-if00
stty -F "$SENDER_VCOM0" 115200 raw -echo
timeout 1800 cat "$SENDER_VCOM0" | tee sender.log
```

Start both captures before resetting the receiver, then reset the sender.
The high-loss phase can take several minutes. Elapsed microseconds and the
frame counters vary between runs; the case order and format should not.
Each completed communication case prints progress at 0%, 5%, 10%, ..., 100%.
The high-loss case may stop earlier when its allowed timeout is reached.

The sender and receiver both print:

```text
*** Booting Zephyr OS build ... ***
[MICRO CORE 1.0.0 - IO - COMMUNICATION] Sender
[invalid_creation]
PASSED - <elapsed>(us)

[invalid_argument]
PASSED - <elapsed>(us)

[valid_creation]
PASSED - <elapsed>(us)

[singly_direction]
PASSED - <elapsed>(us)

[singly_repetitive]
PASSED - <elapsed>(us)

[singly_low_lossy]
PASSED - <elapsed>(us)

[singly_high_lossy]
PASSED - <elapsed>(us)

[singly_timed_out]
PASSED - <elapsed>(us)

MC UART communication test PASS
```

Both boards additionally print a plain-ASCII progress bar on every transfer
iteration and, before each
transport case result, a line like:

```text
Sender Progress: [##########----------] 50%
Receiving Progress: [##########----------] 50%
```

```text
[IO] Completed{Recv: 966, Send: 952} - Failed{Recv: 0(0.00%), Send: 0(0.00%)} - Throughput: 1028.57 KBps
```

`singly_direction` transfers 201 sets of a nine-byte string, a variable-size
array of 27–1023 32-bit words, and a one-byte value. `singly_repetitive`
duplicates every transport frame. `singly_low_lossy` applies a 20% fault rate
and 100 ms periodic link changes; `singly_high_lossy` uses 96% and five-second
changes. Faults alternate between drops and bit flips; every 31st receive
fault corrupts the header and shortens the logical frame to 74%. The final
case checks send, receive, and flush timeouts. Its faults are local, matching
the original UDP test, not injected into the physical link.

## Troubleshooting

- `MC UART communication test FAIL` means a case failed or timed out. Check
  that VCOM1 is disabled on both board controllers, then check crossed TX/RX
  wiring, common GND, and reset order.
- If the receiver fails `singly_direction`, check that P1.04 on the sender
  reaches P1.05 on the receiver and that both UARTs share ground.
- Do not open VCOM1 after disabling it. This includes serial monitors in VS
  Code.
- Use `nrfutil device list` after reconnecting USB because `/dev/ttyACM*`
  numbers may change.
- If a build reports undefined Kconfig symbols, ensure the local Zephyr tree
  is selected with `west -z .../mc_message/zephyr`.
- If flashing reports an invalid CMake cache, rebuild successfully before
  running `west flash`.
