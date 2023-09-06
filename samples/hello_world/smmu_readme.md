# Zephyr Run SMMU demo

## Pre

In our public RFC, there might be some different details between RFC and
implementation as this is in a very early stage. See
[#Issues60289](github.com/zephyrproject-rtos/zephyr/issues/60289)

Here are the simple instructions for building the Zephyr, assuming the users
are familiar with the Zephyr project.

- Following the official instructions [Getting Started](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) to start
- Download the FVP: [Armv-A Base RevC AEM FVP](https://developer.arm.com/-/media/Files/downloads/ecosystem-models/FVP_Base_RevC-2xAEMvA_11.22_14_Linux64.tgz?rev=838d36a4f4884b9f8d18eb215e3fc13c&hash=598281951FEF577A12677DF1313A09A294A95D8B)
- Following the instructions to start with [FVP base RevC board](https://docs.zephyrproject.org/latest/boards/arm64/fvp_base_revc_2xaemv8a/doc/index.html)

## Development plan

- Phase 1 adds basic SMMU driver, a borrowed page table walker, ahci example for
  Cortex A profile devices
- Phase 2 enhances page table walker, completes the page table management.
- Phase 3 Improve the SMMU IRQ management, completes the SMMUv3 driver.
- Phase 4 SMMU framework implementation once the RFC is accepted (It depends,
  might be we send PR first, collecting and fixing comments, then got approved).
  We are expecting this to be long term work with multiple iterations.

## Demo

1. Checkout this PR: https://github.com/zephyrproject-rtos/zephyr/pull/62374.
1. Prepare a disk image file to the `$ZEPHYR_BASE` directory.

    ```shell
    dd if=/dev/zero of=pci_disk.img bs=4M count=8
    ```

1. Build and run.

    ```shell
    west update
    west build -b fvp_base_revc_2xaemv8a samples/hello_world
    ```

The following log snippet indicates the AHCI controller read the device identity
of `pci_disk.img` successfully, meaning SMMU is on, and the DMA device (AHCI
controller) can access the RAM (otherwise, the AHCI controller cannot access the
RAM by default).

```shell
$ west build -t run
-- west build: running target run
[0/1] FVP_Base_RevC-2xAEMvA:
terminal_0: Listening for serial connection on port 5000
terminal_1: Listening for serial connection on port 5001
terminal_2: Listening for serial connection on port 5002
terminal_3: Listening for serial connection on port 5003

Info: FVP_Base_RevC_2xAEMvA: FVP_Base_RevC_2xAEMvA.bp.flashloader0: FlashLoader: Loaded 6 MB from file '/home/huizha01/workspace/arm/zephyr-base/zephyr/build/tfa/fvp/release/fip.bin'

Info: FVP_Base_RevC_2xAEMvA: FVP_Base_RevC_2xAEMvA.bp.secureflashloader: FlashLoader: Loaded 25 kB from file '/home/huizha01/workspace/arm/zephyr-base/zephyr/build/tfa/fvp/release/bl1.bin'
NOTICE:  Booting Trusted Firmware
NOTICE:  BL1: v2.9(release):v2.7.0-1861-g421dc0502
NOTICE:  BL1: Built : 15:41:16, Aug 28 2023
NOTICE:  BL1: Booting BL2
NOTICE:  BL2: v2.9(release):v2.7.0-1861-g421dc0502
NOTICE:  BL2: Built : 15:41:19, Aug 28 2023
NOTICE:  BL1: Booting BL31
NOTICE:  BL31: v2.9(release):v2.7.0-1861-g421dc0502
NOTICE:  BL31: Built : 15:41:23, Aug 28 2023


[    0.408000] <inf> pcie_core: [00:00.0] 1af4:1001 class 1 subclass 80 progif 0 rev 0 Type0 multifunction false
[    0.408000] <inf> pcie_core: [00:00.0] BAR0 size 0x1000 assigned [mem 0x50000000-0x50000fff -> 0x50000000-0x50000fff]
[    0.408000] <inf> pcie_core: [00:00.0] BAR2 size 0x1000 assigned [mem 0x50001000-0x50001fff -> 0x50001000-0x50001fff]
[    0.408000] <inf> pcie_core: [00:00.0] BAR4 size 0x1000 assigned [mem 0x50002000-0x50002fff -> 0x50002000-0x50002fff]
[    0.408000] <inf> pcie_core: [00:02.0] 1af4:1001 class 1 subclass 80 progif 0 rev 0 Type0 multifunction false
[    0.409000] <inf> pcie_core: [00:02.0] BAR0 size 0x1000 assigned [mem 0x50003000-0x50003fff -> 0x50003000-0x50003fff]
[    0.409000] <inf> pcie_core: [00:02.0] BAR2 size 0x1000 assigned [mem 0x50004000-0x50004fff -> 0x50004000-0x50004fff]
[    0.409000] <inf> pcie_core: [00:02.0] BAR4 size 0x1000 assigned [mem 0x50005000-0x50005fff -> 0x50005000-0x50005fff]
[    0.409000] <inf> pcie_core: [00:03.0] 0abc:aced class 1 subclass 6 progif 1 rev 1 Type0 multifunction false
[    0.409000] <inf> pcie_core: [00:03.0] BAR0 size 0x2000 assigned [mem 0x50006000-0x50007fff -> 0x50006000-0x50007fff]
[    0.409000] <inf> pcie_core: [00:03.0] BAR1 size 0x2000 assigned [mem 0x50008000-0x50009fff -> 0x50008000-0x50009fff]
[    0.409000] <inf> pcie_core: [00:03.0] BAR2 size 0x1000 assigned [mem 0x5000a000-0x5000afff -> 0x5000a000-0x5000afff]
[    0.409000] <inf> pcie_core: [00:03.0] BAR3 size 0x2000 assigned [mem 0x5000c000-0x5000dfff -> 0x5000c000-0x5000dfff]
[    0.409000] <inf> pcie_core: [00:03.0] BAR4 size 0x1000 assigned [mem 0x5000e000-0x5000efff -> 0x5000e000-0x5000efff]
[    0.409000] <inf> pcie_core: [00:03.0] BAR5 size 0x2000 assigned [mem 0x50010000-0x50011fff -> 0x50010000-0x50011fff]
[    0.011000] <inf> arm_smmu_v3: 2-level stream table supported.
[    0.014000] <inf> ata_ahci: SATA drive found at port 0

[    0.115000] <dbg> ata_ahci: port_read: transfer bytes: 512
[    0.115000] <inf> ata_ahci: Serial Number: ARM-SATA-Model
[    0.115000] <inf> ata_ahci: Firmware revision: 0.1.0
[    0.115000] <inf> ata_ahci: Model Number: pci_disk.img
*** Booting Zephyr OS build zephyr-v3.4.0-2183-g693aa79894ad ***
Hello World! fvp_base_revc_2xaemv8a
uart:~$
```

## Debug

SMMU driver provides some utility functions to dump SMMU internal data
structure, for now it supports dumping command queue and event queue. If you
want to try it, you must install telent first.

1. Connect to the FVP terminal port via telnet.

    > Noted: The telnet port is 5000, which is listed during the FVP booting
      terminal_0: Listening for serial connection on port 5000

    ```shell
    telnet localhost 5000
    ```
1. Execute smmu commands in the telnet.

    ```shell
    smmu dump cmdq
    ```

    ```shell
    smmu dump evtq
    ```

Below shows the outputs after executing the previous two commands.

```shell
$ telnet localhost 5000
Trying ::1...
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.

uart:~$ smmu dump cmdq
PROD points to 0xb0
CONS points to 0xb0
00000000: 04 00 00 00 00 00 00 00  1f 00 00 00 00 00 00 00 |........ ........|
00000010: 46 20 c0 0f 00 00 00 00  00 00 00 00 00 00 00 00 |F ...... ........|
00000020: 30 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |0....... ........|
00000030: 46 20 c0 0f 00 00 00 00  00 00 00 00 00 00 00 00 |F ...... ........|
00000040: 03 00 00 00 18 00 00 00  01 00 00 00 00 00 00 00 |........ ........|
00000050: 46 20 c0 0f 00 00 00 00  00 00 00 00 00 00 00 00 |F ...... ........|
00000060: 03 00 00 00 18 00 00 00  01 00 00 00 00 00 00 00 |........ ........|
00000070: 46 20 c0 0f 00 00 00 00  00 00 00 00 00 00 00 00 |F ...... ........|
00000080: 01 00 00 00 18 00 00 00  00 00 00 00 00 00 00 00 |........ ........|
00000090: 46 20 c0 0f 00 00 00 00  00 00 00 00 00 00 00 00 |F ...... ........|
000000A0: 46 20 c0 0f 00 00 00 00  00 00 00 00 00 00 00 00 |F ...... ........|
000000B0: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
000000C0: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
000000D0: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
000000E0: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
000000F0: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
uart:~$ smmu dump evtq
PROD points to 0x0
CONS points to 0x0
00000000: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
00000010: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
00000020: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
00000030: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
00000040: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
00000050: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
00000060: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
00000070: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
00000080: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
00000090: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
000000A0: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
000000B0: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
000000C0: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
000000D0: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
000000E0: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
000000F0: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
00000100: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
00000110: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
00000120: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
00000130: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
00000140: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
00000150: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
00000160: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
00000170: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
00000180: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
00000190: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
000001A0: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
000001B0: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
000001C0: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
000001D0: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
000001E0: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
000001F0: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........|
uart:~$
```
