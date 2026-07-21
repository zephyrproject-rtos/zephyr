# STM32U5 DWC2 and LAN78xx integration report

## Scope and status

This document describes the experimental integration branch
`feature/stm32-dwc2-lan78xx-combined`. The branch is based on current Zephyr
main and combines:

- the generic Synopsys DWC2 USB host controller;
- STM32U5 OTG_HS vendor quirks for its embedded high-speed PHY and wrapper;
- DWC2 interrupt-IN support and host-controller correctness improvements;
- the Microchip LAN78xx USB Ethernet driver; and
- the fixes found while validating a LAN7800 on XXX hardware.

The hardware run recorded below demonstrates an end-to-end working path using
the default DWC2 internal buffer-DMA mode:

```text
STM32U5 OTG_HS -> generic DWC2 UHC -> USB host stack -> LAN78xx driver
               -> LAN7800 (VID 0x0424, PID 0x7800) -> Zephyr eth0
```

The adapter enumerated, its PHY reported carrier, IPv4 was configured, and
three ICMP echo requests completed without packet loss. One DWC2 warning was
observed during the ping: `HCINT 0x00000202`, which decodes as channel halted
plus frame overrun. The warning did not stop traffic in this run. The same log
message exists in the bulk/control-IN and interrupt-IN handlers, so identifying
the emitting endpoint and implementing its recovery remains a follow-up item.

This branch is an integration and hardware-validation vehicle. It is not a
proposal to merge all component changes as one upstream pull request.

## Branch contents

The branch contains the following contributions after its Zephyr base, in
order:

| Contribution | Purpose |
| --- | --- |
| STM32U5 DWC2 vendor quirks | Integrate the embedded high-speed PHY and STM32 wrapper with the generic UHC/UDC drivers. |
| Nucleo STM32U5 DWC2 enablement | Exercise the native DWC2 host and device drivers on an upstream board. |
| STM32U5 VBUS override | Add the optional `st,force-vbus-valid` host-session policy. |
| DWC2 interrupt transfers | Add interrupt-IN support required by LAN78xx. |
| DWC2 DMA synchronization | Maintain cache ownership and report the actual interrupt-IN completion length. |
| DWC2 slave/FIFO mode | Permit operation without the internal buffer-DMA engine. |
| DWC2 MMIO adaptation | Use the current generic DWC2 MMIO accessor in imported paths. |
| LAN78xx USB Ethernet | Add LAN7800/LAN7850 Ethernet, MDIO, and PHY support, including the corrected empty-IN-buffer contract. |
| LAN78xx build coverage | Compile the driver in the Ethernet build-all test. |

Current main already supplies the GUSBCFG host-mode fix, generic DWC2 MMIO
API, and suspend/resume implementation that were previously carried as local
integration commits. They are intentionally not duplicated in this branch.

## Implementation details

### STM32U5 DWC2 vendor adaptation

The generic DWC2 transfer engine remains the sole owner of host channels,
FIFOs, interrupts, and transfer state. The STM32U5 vendor quirk is limited to
the surrounding SoC integration:

- enabling the STM32U5 USB power domain and EPOD booster;
- enabling the OTG_HS controller and embedded PHY clocks;
- selecting the PHY reference clock through SYSCFG;
- deasserting the embedded PHY reset;
- applying USB pin control;
- configuring the STM32 wrapper pull-down and VBUS/session behavior;
- optionally driving an external `vbus-gpios` power switch; and
- shutting down the PHY, clocks, and power resources.

The `st,stm32u5-hsotg` compatible selects these hooks while the fallback
`snps,dwc2` compatible selects the generic DWC2 core. STM32 HAL/LL is used for
the wrapper, PHY, clock, and power sequences only. `HAL_HCD_*` does not run
beside the generic DWC2 host engine.

STM32U5 does not expose every DWC2 hardware-configuration register used by the
generic core. The validated devicetree therefore supplies the endpoint counts
and `GHWCFG1`, `GHWCFG2`, and `GHWCFG4` values explicitly.

### VBUS and session validity

The branch adds the boolean devicetree property `st,force-vbus-valid`. On the
validated host-only board it:

- keeps the STM32U5 VBUS detector enabled;
- asserts the standard DWC2 A-session-valid and B-session-valid override
  enable/value bits before the host port is enabled; and
- clears those session overrides when the host controller is disabled.

Boards that provide usable hardware VBUS sensing can omit the property and
retain the default vendor-quirk behavior.

### Generic DWC2 host improvements

The combined branch extends the generic host driver with:

- interrupt-IN validation, programming, interrupt handling, completion, and
  actual-length reporting;
- cache clean/invalidate operations at every buffer-DMA ownership boundary for
  control, bulk, and interrupt transfers;
- current MMIO accessor use in the imported vendor and interrupt code;
- a non-DMA slave/FIFO path that services `RXFLVL`, moves OUT data through the
  FIFO, preserves data PID state, handles NAK/NYET retries, and supports
  high-speed OUT PING flow control.

The branch uses the SOF, root-port suspend/resume, and remote-wakeup handling
now provided by current Zephyr main.

`CONFIG_UHC_DWC2_DMA` defaults to `y`. That is the mode exercised by the UART
validation in this document. The slave/FIFO implementation is present in the
branch but was not validated by this hardware log.

### LAN78xx USB Ethernet support

The LAN78xx driver:

- matches LAN7800 and LAN7850 USB VID/PID pairs through the USB host class;
- discovers the active bulk-IN, bulk-OUT, and interrupt-IN endpoints;
- initializes the adapter through vendor control requests;
- queues multiple bulk-IN RX transfers and bulk-OUT TX transfers;
- consumes the LAN78xx RX/TX command headers and Ethernet frame alignment;
- supports an optional zero-copy TX path when packet layout permits it;
- exposes Clause 22 PHY register access through Zephyr's MDIO API;
- polls and reports PHY link state, speed, and duplex;
- supports runtime MAC-address configuration; and
- optionally enables selected LAN78xx LED outputs through `led-enable-mask`.

The driver is devicetree-instantiated as an Ethernet device below the active
USB host controller, while its USB class instances probe and bind matching
adapters at runtime.

### Receive-buffer contract fixes

Generic Zephyr UHC drivers receive IN data into `net_buf` tailroom and append
the actual byte count when the transfer completes. OUT transfers instead use
the existing `net_buf->len` as the number of bytes to send.

The original LAN78xx implementation pre-populated the logical length of its
control-IN, interrupt-IN, and bulk-IN buffers. That left zero tailroom, so DWC2
correctly rejected the transfers. The older STM32 HAL shim reset the lengths
internally and masked the mistake.

The current LAN78xx driver commit leaves all LAN78xx IN buffers empty after
`net_buf_reset()`. Their allocation capacity is consequently available as
tailroom, and DWC2 appends the actual receive length on completion. The TX
length assignment is retained because it follows the correct OUT-transfer
contract.

## XXX devicetree example

The following is the USB-host-related portion of the private
`XXX.dts` used for validation. The clock selection, synthetic
hardware-configuration values, pin assignments, VBUS policy, MAC address, and
PHY address are board- and SoC-specific; they must not be copied blindly to a
different target.

```dts
&otghs_phy {
	status = "okay";
	clocks = <&rcc STM32_CLOCK(AHB2, 15)>,
		 <&rcc STM32_SRC_PLL1_P OTGHS_SEL(1)>;
	clock-reference = "SYSCFG_OTG_HS_PHY_CLK_32MHz";
};

&usbotg_hs {
	compatible = "st,stm32u5-hsotg", "snps,dwc2";
	/delete-property/ num-bidir-endpoints;
	/delete-property/ ram-size;
	/delete-property/ maximum-speed;
	num-in-eps = <9>;
	num-out-eps = <9>;
	ghwcfg1 = <0x00000000>;
	ghwcfg2 = <0x228fe052>;
	ghwcfg4 = <0xe2103e30>;
	st,force-vbus-valid;
	pinctrl-0 = <&usb_otg_hs_dm_pa11 &usb_otg_hs_dp_pa12>;
	pinctrl-names = "default";
	clocks = <&rcc STM32_CLOCK(AHB2, 14)>;
	phys = <&otghs_phy>;
	status = "okay";

	lan78xx: ethernet {
		compatible = "microchip,lan78xx";
		local-mac-address = [54 10 EC 00 11 22];
		phy-handle = <&lan78xx_phy>;
		status = "okay";
		led-enable-mask = <0x4>;

		lan78xx_mdio: mdio {
			compatible = "microchip,lan78xx-mdio";
			#address-cells = <1>;
			#size-cells = <0>;
			status = "okay";

			lan78xx_phy: ethernet-phy@1 {
				compatible = "ethernet-phy";
				reg = <1>;
				status = "okay";
			};
		};
	};
};
```

The XXX application creates and starts its USB host context directly from
the `usbotg_hs` node:

```c
#define USBHS_NODE DT_NODELABEL(usbotg_hs)

USBH_CONTROLLER_DEFINE(XXX_ctx, DEVICE_DT_GET(USBHS_NODE));
```

It then calls `usbh_init()` and `usbh_enable()` during application
initialization. A different application must provide an equivalent USB host
context/startup path or use the host startup mechanism appropriate to that
application.

## XXX configuration example

These are the relevant explicit options from the validated application's
`prj.conf`:

```conf
# Shell commands used for USB and network validation
CONFIG_SHELL=y
CONFIG_SHELL_CMDS=y
CONFIG_SHELL_HISTORY=y
CONFIG_SHELL_LOG_BACKEND=y
CONFIG_SHELL_LOG_LEVEL_INF=y
CONFIG_SHELL_PROMPT_UART="vtipmi:$ "

# Networking used by the LAN7800 interface
CONFIG_NETWORKING=y
CONFIG_NET_L2_ETHERNET=y
CONFIG_NET_TCP=y
CONFIG_NET_UDP=y

# USB host shell and diagnostics
CONFIG_USBH_SHELL=y
CONFIG_UHC_DRIVER_LOG_LEVEL_DBG=y
CONFIG_USBH_LOG_LEVEL_ERR=y

# Host-stack and transfer resources
CONFIG_ISR_STACK_SIZE=16384
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=16384
CONFIG_USBH_STACK_SIZE=8192
CONFIG_USBH_USB_DEVICE_HEAP=8192
CONFIG_UHC_XFER_COUNT=16
CONFIG_UHC_BUF_POOL_SIZE=16384

# Generic Synopsys DWC2 host controller
CONFIG_UHC_DWC2=y
```

Because the enabled devicetree contains `microchip,lan78xx`,
`CONFIG_ETH_LAN78XX` defaults to `y` and selects the USB host stack, MDIO, and
the Ethernet reserved-header support needed by the driver. The tested config
does not explicitly assign `CONFIG_UHC_DWC2_DMA`; its default `y` selects the
internal buffer-DMA path.

For an explicit, reproducible DMA-mode selection, the equivalent setting is:

```conf
CONFIG_UHC_DWC2_DMA=y
```

Setting it to `n` selects the experimental slave/FIFO implementation and does
not describe the mode validated by the UART output below.

## Build used before hardware validation

The combined DWC2 and LAN78xx configuration previously built successfully in
the prepared Zephyr environment with:

```sh
west build -p auto -b XXX app -- -DBOARD_ROOT=$PWD/app
```

That build linked `zephyr.elf` with 295892 bytes of flash and 440808 bytes of
RAM reported by the linker. The UART transcript below is the subsequent
hardware evidence supplied from the XXX board.

## UART validation output

The following output is preserved from the working hardware session. It shows
USB enumeration, the LAN7800 descriptor, an operational Ethernet interface,
manual IPv4 configuration, and three successful ping replies. The console line
around the second ping reply is interleaved with shell input exactly as
captured.

```text
vtipmi:$ usbh device list
1
vtipmi:$  usbh device descriptor device 1
Device Descriptor:
  bLength                        18
  bDescriptorType                 1
  bcdUSB                       2.10
  bDeviceClass                  255
  bDeviceSubClass                 0
  bDeviceProtocol               255
  bMaxPacketSize0                64
  idVendor                   0x0424
  idProduct                  0x7800
  bcdDevice                    3.00
  iManufacturer                   0
  iProduct                        0
  iSerial                         0
  bNumConfigurations              1
vtipmi:$ usbh device descriptor configuration 1 0
Configuration Descriptor:
  bLength                         9
  bDescriptorType                 2
  wTotalLength               0x0027
  bNumInterfaces                  1
  bConfigurationValue             1
  iConfiguration                  0
  bmAttributes                 0xE0
  bMaxPower                       2 mA
vtipmi:$ net iface
Hostname: zephyr
Default interface: 1


Interface eth0 (0x20009378) (Ethernet) [1]
===================================
Link addr : 54:10:EC:00:11:22
MTU       : 1500
Flags     : AUTO_START,IPv4,IPv6
Device    : ethernet (0x8160438)
Status    : oper=UP, admin=UP, carrier=ON
Ethernet capabilities supported:
        10 Mbits
        100 Mbits
        1 Gbits
Ethernet PHY device: ethernet-phy@1 (0x8160478)
Ethernet link speed: 10 Mbits half-duplex
IPv6 unicast addresses (max 3):
        fe80::5610:ecff:fe00:1122 autoconf preferred infinite
IPv6 multicast addresses (max 3):
        ff02::1
        ff02::1:ff00:1122
IPv6 prefixes (max 2):
        <none>
IPv6 hop limit           : 64
IPv6 base reachable time : 30000
IPv6 reachable time      : 16953
IPv6 retransmit timer    : 0
IPv4 unicast addresses (max 1):
        <none>
IPv4 multicast addresses (max 1):
        224.0.0.1
IPv4 gateway : 0.0.0.0

Interface lo (0x200094c8) (Dummy) [2]
================================
Link addr : 00:00:5E:00:53:FF
MTU       : 1500
Flags     : AUTO_START,IPv4,IPv6
Device    : lo (0x81602d8)
Status    : oper=UP, admin=UP, carrier=ON
IPv6 unicast addresses (max 3):
        ::1 autoconf preferred infinite
        fe80::5eff:fe00:53ff autoconf preferred infinite
IPv6 multicast addresses (max 3):
        ff02::1
        ff02::1:ff00:1
        ff02::1:ff00:53ff
IPv6 prefixes (max 2):
        <none>
IPv6 hop limit           : 64
IPv6 base reachable time : 30000
IPv6 reachable time      : 31189
IPv6 retransmit timer    : 0
IPv4 unicast addresses (max 1):
        127.0.0.1/255.0.0.0 autoconf preferred infinite
IPv4 multicast addresses (max 1):
        224.0.0.1
IPv4 gateway : 0.0.0.0
vtipmi:$ net ipv4 add 1 192.168.144.90 255.255.255.0
vtipmi:$ net ipv4 gateway 1 192.168.144.1
vtipmi:$ net ping 10.0.0.11
PING 10.0.0.11
28 bytes from 10.0.0.11 to 127.0.0.1: icmp_seq=1 ttl=64 time=0 ms
[00:02:55.452,000] <wrn> uhc_dwc2: IN halted, unhandled HCINT 0x00000202
vtipmi:$ net28 bytes from 10.0.0.11 to 127.0.0.1: icmp_seq=2 ttl=64 time=0 ms
28 bytes from 10.0.0.11 to 127.0.0.1: icmp_seq=3 ttl=64 time=0 ms
--- ping statistics ---
3 packets transmitted, 3 received, 0% packet loss
rtt min/avg/max = 0/0/0 ms
vtipmi:$ net iface
Hostname: zephyr
Default interface: 1


Interface eth0 (0x20009378) (Ethernet) [1]
===================================
Link addr : 54:10:EC:00:11:22
MTU       : 1500
Flags     : AUTO_START,IPv4,IPv6
Device    : ethernet (0x8160438)
Status    : oper=UP, admin=UP, carrier=ON
Ethernet capabilities supported:
        10 Mbits
        100 Mbits
        1 Gbits
Ethernet PHY device: ethernet-phy@1 (0x8160478)
Ethernet link speed: 10 Mbits half-duplex
IPv6 unicast addresses (max 3):
        fe80::5610:ecff:fe00:1122 autoconf preferred infinite
IPv6 multicast addresses (max 3):
        ff02::1
        ff02::1:ff00:1122
IPv6 prefixes (max 2):
        <none>
IPv6 hop limit           : 64
IPv6 base reachable time : 30000
IPv6 reachable time      : 16953
IPv6 retransmit timer    : 0
IPv4 unicast addresses (max 1):
        192.168.144.90/255.255.255.0 manual preferred infinite
IPv4 multicast addresses (max 1):
        224.0.0.1
IPv4 gateway : 192.168.144.1

Interface lo (0x200094c8) (Dummy) [2]
================================
Link addr : 00:00:5E:00:53:FF
MTU       : 1500
Flags     : AUTO_START,IPv4,IPv6
Device    : lo (0x81602d8)
Status    : oper=UP, admin=UP, carrier=ON
IPv6 unicast addresses (max 3):
        ::1 autoconf preferred infinite
        fe80::5eff:fe00:53ff autoconf preferred infinite
IPv6 multicast addresses (max 3):
        ff02::1
        ff02::1:ff00:1
        ff02::1:ff00:53ff
IPv6 prefixes (max 2):
        <none>
IPv6 hop limit           : 64
IPv6 base reachable time : 30000
IPv6 reachable time      : 31189
IPv6 retransmit timer    : 0
IPv4 unicast addresses (max 1):
        127.0.0.1/255.0.0.0 autoconf preferred infinite
IPv4 multicast addresses (max 1):
        224.0.0.1
IPv4 gateway : 0.0.0.0
```

## Result and remaining work

The log validates the following on STM32U5/XXX with a LAN7800:

- DWC2 host-mode initialization through the STM32U5 vendor quirk;
- the board's 32 MHz embedded-PHY reference-clock configuration;
- forced session-valid operation on this host-only VBUS topology;
- USB control transfers and descriptor access;
- bulk-IN, bulk-OUT, and interrupt-IN transfers through DWC2;
- LAN7800 register initialization and MDIO/PHY access;
- Ethernet carrier, MAC address, IPv4 address/gateway configuration; and
- bidirectional network traffic sufficient for a 3/3 ICMP exchange.

Remaining validation and cleanup items are:

- trace the channel that produced
  `HCINT.FRMOVRUN | HCINT.CHHLTD` (`0x202`) and implement recovery instead of
  only logging it;
- make interrupt-IN scheduling honor the endpoint interval rather than merely
  selecting the next odd/even frame;
- validate disconnect/reconnect, sustained traffic, error recovery, and link
  renegotiation;
- validate suspend/resume on hardware;
- validate cache maintenance under longer DMA traffic; and
- separately validate `CONFIG_UHC_DWC2_DMA=n` slave/FIFO mode before treating
  that path as hardware-proven.
