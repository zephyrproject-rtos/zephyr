<!--
SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: Copyright (c) 2026 BayLibre SAS
-->

# Testing on real boards

The `*.real_hw` scenarios run the test application on a board while the
OpenSSL server runs on the host. The board reaches the host over a
point-to-point link where the host is `192.0.2.1` and the board is
`192.0.2.2`. How this link is set up depends on the board.

In the commands below, the port passed to `--device-serial` is the board's
console. Adjust device names to match your setup.

## PPP connection

Used by `nrf54l15dk/nrf54l15/cpuapp`, where the link is PPP over a UART.

Start `pppd` on the serial port connected to the board's PPP UART and keep it
running for the whole test session:

```sh
sudo pppd /dev/ttyACM0 115200 192.0.2.1:192.0.2.2 local noauth nodetach nocrtscts nodefaultroute persist debug
```

Then, from another terminal, run the tests:

```sh
twister -c -p nrf54l15dk/nrf54l15/cpuapp \
	-T tests/net/socket/tls_configurations \
	--device-testing --device-serial /dev/ttyACM1
```

## Ethernet-over-USB

Used by `nrf54lm20dk/nrf54lm20a/cpuapp`, where the link is Ethernet over USB
(CDC NCM) on the board's USB port.

The host-side network interface is recreated every time the board is reset or
flashed, so its address must be configured automatically. With
NetworkManager, add a connection profile that matches the host-side MAC
address of the link (the `remote-mac-address` in the board overlay):

```sh
nmcli connection add type ethernet \
	con-name zephyr-ncm \
	802-3-ethernet.mac-address 00:00:5E:00:53:01 \
	ipv4.method manual \
	ipv4.addresses 192.0.2.1/24 \
	ipv6.method disabled \
	connection.autoconnect yes
```

Then run the tests:

```sh
twister -c -p nrf54lm20dk/nrf54lm20a/cpuapp \
	-T tests/net/socket/tls_configurations \
	--device-testing --device-serial /dev/ttyACM1
```

Once done, the connection profile can be removed with:

```sh
nmcli connection delete zephyr-ncm
```
