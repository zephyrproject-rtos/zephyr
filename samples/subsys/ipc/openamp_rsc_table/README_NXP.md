# Inter-core communication between Linux and Zephyr using OpenAMP and Resource Table

<!----- Boards ----->
[![License badge](https://img.shields.io/badge/License-Apache%202.0-red)](https://github.com/search?q=org%3Anxp-appcodehub+vision+in%3Areadme&type=Repositories)
[![Language badge](https://img.shields.io/badge/Language-C-yellow)](https://www.nxp.com/docs/en/user-guide/IMX-MACHINE-LEARNING-UG.pdf)
[![Board badge](https://img.shields.io/badge/Board-EVK&ndash;MIMX8MP-blue)](https://www.nxp.com/pip/8MPLUSLPD4-EVK)
[![Board badge](https://img.shields.io/badge/Board-MEK&ndash;MIMX8QM-blue)](https://www.nxp.com/pip/MCIMX8QM-CPU)
[![Board badge](https://img.shields.io/badge/Board-MEK&ndash;MIMX8QX-blue)](https://www.nxp.com/pip/MCIMX8QXP-CPU)
[![Board badge](https://img.shields.io/badge/Board-EVK&ndash;MIMX8ULP-blue)]()
[![Board badge](https://img.shields.io/badge/Board-IMX95LPD5EVK&ndash;19-blue)]()

This example demonstrates inter-core communication between: Linux (Cortex-A cores) running the rpmsg drivers ([rpmsg_client_sample](https://elixir.bootlin.com/linux/latest/source/samples/rpmsg/rpmsg_client_sample.c), [rpmsg_tty](https://elixir.bootlin.com/linux/latest/source/drivers/tty/rpmsg_tty.c)), and Zephyr RTOS running on:
- Cortex-M cores (i.MX8 and i.MX9 families), or
- HiFi4 DSP (i.MX8M Plus and similar).

It shows how to send messages between Linux and Zephyr using the remoteproc / rpmsg / virtio framework with an OpenAMP resource table.
It leverages:
- [OpenAMP](https://openamp.readthedocs.io/en/latest/index.html) on Zephyr
- remoteproc and rpmsg on Linux.

The application responds to:
- [rpmsg_client_sample Linux kernel module](https://elixir.bootlin.com/linux/latest/source/samples/rpmsg/rpmsg_client_sample.c)
- [rpmsg_tty Linux kernel driver](https://elixir.bootlin.com/linux/latest/source/drivers/tty/rpmsg_tty.c)

This enables developers to run real-time tasks on Cortex-M or DSP cores while exchanging messages seamlessly with Linux on the main Cortex-A cores.

Additional documentation:
- [i.MX Linux User Guide](https://www.nxp.com/docs/en/user-guide/UG10163.pdf)
- [Application Note AN13970: Running Zephyr RTOS on Cadence Tensilica HiFi 4 DSP](https://www.nxp.com/docs/en/application-note/AN13970.pdf)
- [Application Note AN5317: Loading Code on Cortex-M from U-Boot/Linux for the i.MX Asymmetric MultiProcessing Application Processors](https://www.nxp.com/docs/en/application-note/AN5317.pdf)
- [YouTube Tutorial – RPMsg, OpenAMP, DTS for i.MX8M Plus (HiFi4 core example)](https://www.youtube.com/watch?v=JqwPljnm2_k)
- [Zephyr DTS API zephyr,ipc_shm](https://docs.zephyrproject.org/latest/build/dts/api/api.html)

## Table of Contents
1. [Software](#step1)
2. [Hardware](#step2)
3. [Setup](#step3)
4. [Results](#step4)
5. [FAQs](#step5)
6. [Support](#step6)
7. [Release Notes](#step7)

## 1. Software<a name="step1"></a>

This application uses a combination of Linux and Zephyr-based environments.

- Linux: Kernel version 5.x+ with CONFIG_RPMSG enabled. Key modules include [rpmsg_client_sample](https://elixir.bootlin.com/linux/latest/source/samples/rpmsg/rpmsg_client_sample.c) and [rpmsg_tty](https://elixir.bootlin.com/linux/latest/source/drivers/tty/rpmsg_tty.c).
- Toolchain: Linaro GCC or a Yocto-built SDK.
- Zephyr:  Zephyr 4.1+ and Zephyr SDK v0.17+ are used, along with the sample located at [samples/subsys/ipc/openamp_rsc_table](https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/subsys/ipc/openamp_rsc_table). This runs on Cortex-M cores (e.g., i.MX8, i.MX9) or HiFi4 DSP (i.MX8).
- Bootloaders: U-Boot is used for Cortex-M startup via `prepare_mcore`. For HiFi4 DSP, Linux provides remoteproc support (see [imx_dsp_rproc](https://elixir.bootlin.com/linux/latest/source/drivers/remoteproc/imx_dsp_rproc.c) driver). For Cortex-M, in addition to U-Boot, the Linux remoteproc driver [imx_rproc](https://elixir.bootlin.com/linux/latest/source/drivers/remoteproc/imx_rproc.c) can also be used.
- Tools: A serial console is required - options include Minicom, PuTTY, or Picocom.


## 2. Hardware<a name="step2"></a>

Supported Boards:
- [EVK–MIMX8MP (Cortex-M + HiFi4 DSP)](https://www.nxp.com/design/design-center/development-boards-and-designs/8MPLUSLPD4-EVK)
- [MEK–MIMX8QM](https://www.nxp.com/design/design-center/development-boards-and-designs/i-mx-evaluation-and-development-boards/i-mx-8quadmax-multisensory-enablement-kit-mek:MCIMX8QM-CPU)
- [MEK–MIMX8QX](https://www.nxp.com/design/design-center/development-boards-and-designs/i-mx-evaluation-and-development-boards/i-mx-8quadxplus-multisensory-enablement-kit-mek:MCIMX8QXP-CPU)
- [EVK–MIMX8ULP](https://www.nxp.com/design/design-center/development-boards-and-designs/i-mx-8ulp-evaluation-kit:MCIMX8ULP-EVK)
- [IMX95LPD5EVK–19](https://www.nxp.com/docs/en/quick-reference-guide/IMX95LPD5EVK-19CM.pdf)

Connections:
- USB-UART for Cortex-M/DSP Zephyr console

Successful porting of the `openamp_rsc_table` sample requires defining shared memory between host and remote cores via the `zephyr,ipc_shm` property in a device tree overlay. For instance, platforms such as i.MX93 require both a memory region definition and Messaging Unit (MU) configuration.

The linker script must also be updated to include the resource table section when `CONFIG_OPENAMP_RSC_TABLE` is enabled.

Platform-specific configuration may be necessary, such as overriding default RX/TX IDs, adjusting IPM settings, and enabling cache support. These changes should be applied through appropriate `.conf` files.

Refer to board-specific DTS files and documentation to ensure correct memory mapping, peripheral setup, and runtime behavior.

See reference board configurations and DTS overlay examples for guidance [here](https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/subsys/ipc/openamp_rsc_table/boards).

## 3. Setup<a name="step3"></a>

For Linux setup:
- for rpmsg client sample, enable `SAMPLE_RPMSG_CLIENT` configuration.
- for rpmsg TTY demo, enable `RPMSG_TTY` in kernel config.

### 3.1 Cortex-M Setup
1. Prepare Cortex-M core in U-Boot by running `prepare_mcore`
2. Boot Linux with `-rpmsg` device tree blob
Ensure carveouts, vrings, and mailbox are defined in DTS (see [AN5317](https://www.nxp.com/docs/en/application-note/AN5317.pdf)).
3. Build Zephyr sample
```
$ west build -b <board> samples/subsys/ipc/openamp_rsc_table
```
4.	Flash the Zephyr image using U-Boot or, to use the remoteproc interface, follow these steps:

4.1. Write the firmware name to the appropriate remoteproc node:

```
$ echo -n zephyr_openamp_rsc_table.elf > /sys/class/remoteproc/remoteprocX/firmware
$ echo start > /sys/class/remoteproc/remoteprocX/state
```
Replace remoteprocX with the correct remote processor index corresponding to the Cortex-M core.

4.2. Identify the correct remoteproc index

Use the following command to list available remote processors:
```
$ ls -la /sys/class/remoteproc/
```

4.3. Example output on i.MX8ULP:

```
root@imx8ulpevk:~# ls -la /sys/class/remoteproc/
total 0
drwxr-xr-x  2 root root 0 Oct  7 07:52 .
drwxr-xr-x 89 root root 0 Oct  7 06:59 ..
lrwxrwxrwx  1 root root 0 Oct  7 06:59 remoteproc0 -> ../../devices/platform/remoteproc-cm33/remoteproc/remoteproc0
lrwxrwxrwx  1 root root 0 Oct  7 07:00 remoteproc1 -> ../../devices/platform/soc@0/21170000.dsp/remoteproc/remoteproc1
```
In this example:
- `remoteproc0` corresponds to the Cortex-M33 core.
- `remoteproc1` corresponds to the DSP core.

### 3.2 HiFi4 DSP Setup

1. Boot Linux with `-rpmsg` device tree blob

Make sure Linux DTS defines:
- DSP memory carveouts
- Mailbox resources
- Remoteproc entry for DSP

2. Build Zephyr sample for HiFi4 DSP

```
$ west build -b imx8mp_evk//adsp samples/subsys/ipc/openamp_rsc_table
```
3. Load firmware from Linux

Once Linux is running, use remoteproc to load Zephyr firmware:

```
$ echo -n zephyr_openamp_rsc_table.elf > /sys/class/remoteproc/remoteprocX/firmware
$ echo start > /sys/class/remoteproc/remoteprocX/state
```
remoteprocX corresponds to the HiFi4 DSP instance.

### 3.3 Linux Console

1. Open a Linux shell

Use any terminal interface such as `minicom`, `ssh`, or a direct console.

2. For **RPMsg Sample Client Demo** insert the `rpmsg_client_sample` kernel module:
```
$ modprobe rpmsg_client_sample
```

This module enables basic RPMsg communication. Once inserted, you should see kernel logs indicating:
- RPMsg channel creation or destruction
- Incoming messages from the remote processor

 To view these logs, use:
```
$ dmesg
```
The output should confirm that the Zephyr firmware was successfully loaded and that communication channels were established:
```
[  230.645569] remoteproc remoteproc1: powering up imx-dsp-rproc
[  230.812436] remoteproc remoteproc1: Booting fw image zephyr_openamp_rsc_table.elf, size 1800572
[  230.828248] rproc-virtio rproc-virtio.6.auto: assigned reserved memory node vdev0buffer@8ff00000
[  230.833688] virtio_rpmsg_bus virtio2: rpmsg host is online
[  230.834229] rproc-virtio rproc-virtio.6.auto: registered virtio2 (type 7)
[  230.834258] remoteproc remoteproc1: remote processor imx-dsp-rproc is now up
[  230.834868] virtio_rpmsg_bus virtio2: creating channel rpmsg-tty addr 0x400
[  230.835339] virtio_rpmsg_bus virtio2: creating channel rpmsg-client-sample addr 0x401
[  230.835535] virtio_rpmsg_bus virtio2: creating channel rpmsg-tty addr 0x402
[  231.104700] rpmsg_client_sample virtio2.rpmsg-client-sample.-1.1025: new channel: 0x400 -> 0x401!
[  231.105388] rpmsg_client_sample virtio2.rpmsg-client-sample.-1.1025: incoming msg 1 (src: 0x401)
[  231.106916] rpmsg_client_sample virtio2.rpmsg-client-sample.-1.1025: incoming msg 2 (src: 0x401)
...
[  231.126858] rpmsg_client_sample virtio2.rpmsg-client-sample.-1.1025: incoming msg 97 (src: 0x401)
[  231.127041] rpmsg_client_sample virtio2.rpmsg-client-sample.-1.1025: incoming msg 98 (src: 0x401)
[  231.127223] rpmsg_client_sample virtio2.rpmsg-client-sample.-1.1025: incoming msg 99 (src: 0x401)
[  231.127425] rpmsg_client_sample virtio2.rpmsg-client-sample.-1.1025: incoming msg 100 (src: 0x401)
[  231.127447] rpmsg_client_sample virtio2.rpmsg-client-sample.-1.1025: goodbye!
[  231.127482] virtio_rpmsg_bus virtio2: destroying channel rpmsg-client-sample addr 0x401
[  231.130503] rpmsg_client_sample virtio2.rpmsg-client-sample.-1.1025: rpmsg sample client driver is removed
```
3. For **RPMsg TTY Demo**, a TTY-like communication channel between Linux and Zephyr, insert the `rpmsg_tty` module:
```
$ modprobe rpmsg_tty
```
On the Linux console, send a message to Zephyr.
If `CONFIG_SHELL` option is enabled use the `ttyRPMSG1` for RPMsg TTY demo, as `ttyRPMSG0` is used for shell. Otherwise, use `ttyRPMSG0`.
```
$ cat /dev/ttyRPMSG0 &
$ echo "Hello Zephyr" > /dev/ttyRPMSG0
```

### 3.4 Zephyr Console

1. Open a serial terminal (minicom, putty, etc.) and connect to the board using default serial port settings.
2. For **RPMsg Sample Client Demo**, by default, the Zephyr console only prints:
```
*** Booting Zephyr OS build v#.#.#-####-g########## ***
```
If `CONFIG_SHELL` is disabled from the project configuration, for each message received, its content is displayed as shown down below:
```
*** Booting Zephyr OS build v#.#.#-####-g########## ***
[00:00:00.007,000] <inf> openamp_rsc_table: Starting application threads!
...
[00:00:00.012,000] <dbg> openamp_rsc_table: mailbox_notify: mailbox_notify: msg received
--- 287 messages dropped ---
[00:00:00.012,000] <inf> openamp_rsc_table: [Linux sample client] incoming msg 97: hello world!
[00:00:00.012,000] <dbg> openamp_rsc_table: mailbox_notify: mailbox_notify: msg received
[00:00:00.012,000] <dbg> openamp_rsc_table: platform_ipm_callback: platform_ipm_callback: msg received from mb 0
[00:00:00.012,000] <inf> openamp_rsc_table: [Linux sample client] incoming msg 98: hello world!
[00:00:00.012,000] <dbg> openamp_rsc_table: mailbox_notify: mailbox_notify: msg received
[00:00:00.012,000] <dbg> openamp_rsc_table: platform_ipm_callback: platform_ipm_callback: msg received from mb 0
[00:00:00.012,000] <inf> openamp_rsc_table: [Linux sample client] incoming msg 99: hello world!
[00:00:00.012,000] <dbg> openamp_rsc_table: mailbox_notify: mailbox_notify: msg received
[00:00:00.012,000] <dbg> openamp_rsc_table: platform_ipm_callback: platform_ipm_callback: msg received from mb 0
[00:00:00.012,000] <inf> openamp_rsc_table: [Linux sample client] incoming msg 100: hello world!
[00:00:00.012,000] <dbg> openamp_rsc_table: mailbox_notify: mailbox_notify: msg received
[00:00:00.012,000] <dbg> openamp_rsc_table: mailbox_notify: mailbox_notify: msg received
[00:00:00.012,000] <inf> openamp_rsc_table: OpenAMP Linux sample client responder ended
```
3. For **RPMsg TTY Demo**, the received message is displayed as shown below:
```
*** Booting Zephyr OS build zephyr-v#.#.#-####-g########## ***
Starting application threads!

OpenAMP[remote] Linux responder demo started

OpenAMP[remote] Linux sample client responder started

OpenAMP[remote] Linux TTY responder started
[Linux TTY] incoming msg: Hello Zephyr
```

## 4. Results<a name="step4"></a>
1. Linux Console
```
root@imx8mpevk:~# echo -n zephyr_openamp_rsc_table.elf > /sys/class/remoteproc/remoteproc0/firmware
root@imx8mpevk:~# echo start > /sys/class/remoteproc/remoteproc0/state
[  200.630824] remoteproc remoteproc0: powering up imx-dsp-rproc
[  200.637393] remoteproc remoteproc0: Booting fw image zephyr_openamp_rsc_table.elf, size 999412
[  200.649895] rproc-virtio rproc-virtio.2.auto: assigned reserved memory node vdev0buffer@94300000
[  200.662289] virtio_rpmsg_bus virtio0: rpmsg host is online
[  200.667889] rproc-virtio rproc-virtio.2.auto: registered virtio0 (type 7)
[  200.674715] remoteproc remoteproc0: remote processor imx-dsp-rproc is now up
[  200.681908] virtio_rpmsg_bus virtio0: creating channel rpmsg-client-sample addr 0x400
[  200.689959] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: new channel: 0x400 -> 0x400!
[  200.700409] virtio_rpmsg_bus virtio0: creating channel rpmsg-tty addr 0x401
[  200.707894] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: incoming msg 1 (src: 0x400)
[  200.717703] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: incoming msg 2 (src: 0x400)
....
[  201.591742] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: incoming msg 100 (src: 0x400)
[  201.600716] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: goodbye!
root@imx8mpevk:~# cat /dev/ttyRPMSG0 &
root@imx8mpevk:~# echo "Hello Zephyr" > /dev/ttyRPMSG0
TTY 0x0401: Hello Zephyr
```
2. Zephyr Console
```
*** Booting Zephyr OS build zephyr-v#.#.#-####-g########## ***
Starting application threads!

OpenAMP[remote] Linux responder demo started

OpenAMP[remote] Linux sample client responder started

OpenAMP[remote] Linux TTY responder started
...
<inf> openamp_rsc_table: [Linux sample client] incoming msg 99: hello world!
<dbg> openamp_rsc_table: mailbox_notify: mailbox_notify: msg received
<dbg> openamp_rsc_table: platform_ipm_callback: platform_ipm_callback: msg received from mb 0
<inf> openamp_rsc_table: [Linux sample client] incoming msg 100: hello world!
<dbg> openamp_rsc_table: mailbox_notify: mailbox_notify: msg received
<dbg> openamp_rsc_table: mailbox_notify: mailbox_notify: msg received
<inf> openamp_rsc_table: OpenAMP Linux sample client responder ended
...
[Linux TTY] incoming msg: Hello Zephyr
...
<inf> openamp_rsc_table: OpenAMP Linux TTY responder ended
```

## 5. FAQs<a name="step5"></a>
*Q: Can Zephyr run only on Cortex-M cores?*

A: No. Zephyr also runs on HiFi4 DSP cores on i.MX8 in addition to Cortex-M cores on i.MX8 and i.MX9.

*Q: How do I choose between Cortex-M and DSP for Zephyr?*

A: Use Cortex-M for control/real-time tasks, and DSP (HiFi4) for signal-processing–heavy tasks. Both support OpenAMP messaging with Linux.

## 6. Support<a name="step6"></a>
- [NXP Community Forum](https://community.nxp.com/)
- [Zephyr Project Documentation](https://docs.zephyrproject.org/latest/index.html)
- [Application Note AN5317](https://www.nxp.com/docs/en/application-note/AN5317.pdf)
- [Application Note AN13970](https://www.nxp.com/docs/en/application-note/AN13970.pdf)

Questions regarding the content/correctness of this example can be entered as Issues within this GitHub repository.

>**Warning**: For more general technical questions regarding NXP Microcontrollers and the difference in expected functionality, enter your questions on the [NXP Community Forum](https://community.nxp.com/)

[![Follow us on Youtube](https://img.shields.io/badge/Youtube-Follow%20us%20on%20Youtube-red.svg)](https://www.youtube.com/NXP_Semiconductors)
[![Follow us on LinkedIn](https://img.shields.io/badge/LinkedIn-Follow%20us%20on%20LinkedIn-blue.svg)](https://www.linkedin.com/company/nxp-semiconductors)
[![Follow us on Facebook](https://img.shields.io/badge/Facebook-Follow%20us%20on%20Facebook-blue.svg)](https://www.facebook.com/nxpsemi/)
[![Follow us on Twitter](https://img.shields.io/badge/X-Follow%20us%20on%20X-black.svg)](https://x.com/NXP)

## 7. Release Notes<a name="step7"></a>
| Version | Description / Update                           | Date                        |
|:-------:|------------------------------------------------|----------------------------:|
| 1.0     | Initial release on Application Code Hub        | October 15<sup>th</sup> 2025 |
