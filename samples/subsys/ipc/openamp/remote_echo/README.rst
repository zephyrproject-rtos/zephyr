.. _openAMP_remote_echo_sample:

OpenAMP Remote Echo Sample Application
######################################

Overview
********

This sample application demonstrates how to integrate coding and building of
OpenAMP with Zephyr. Currently this integration is specific to the i.MX7 and
i.MX6 SoCs and the Master side (Cortex-A) should be running Linux with the i.MX
RPMsg and mailbox driver.


Building the application
*************************

.. zephyr-app-commands::
    :zephyr-app: samples/subsys/ipc/openamp/remote_echo
    :board: colibri_imx7d_m4 warp7_m4 udoo_neo_full_m4
    :goals: build flash


On the Linux side (Master) make sure you have the following Linux kernel config
options enabled:

.. code-block:: none

    CONFIG_HAVE_IMX_MU=y
    CONFIG_HAVE_IMX_RPMSG=y
    CONFIG_RPMSG=y
    CONFIG_RPMSG_VIRTIO=y
    CONFIG_IMX_RPMSG_TTY=m


And make sure that you have the following Linux devitree nodes:

.. code-block:: none

    i.MX7 - arch/arm/boot/dts/imx7s.dtsi:
    mu: mu@30aa0000 {
        compatible = "fsl,imx7d-mu", "fsl,imx6sx-mu";
        reg = <0x30aa0000 0x10000>;
        interrupts = <GIC_SPI 88 IRQ_TYPE_LEVEL_HIGH>;
        clocks = <&clks IMX7D_MU_ROOT_CLK>;
        clock-names = "mu";
        status = "okay";
    };

    rpmsg: rpmsg{
        compatible = "fsl,imx7d-rpmsg";
        status = "disabled";
    };


    i.MX6SX - arch/arm/boot/dts/imx6sx.dtsi:
    mu: mu@02294000 {
        compatible = "fsl,imx6sx-mu";
        reg = <0x02294000 0x4000>;
        interrupts = <0 90 0x04>;
        status = "okay";
    };

    rpmsg: rpmsg{
        compatible = "fsl,imx6sx-rpmsg";
        status = "disabled";
    };


    BOARD - arch/arm/boot/dts/<board>.dts:
    reserved-memory {
        #address-cells = <1>;
        #size-cells = <1>;
        ranges;

        rpmsg_reserved: rpmsg@8fff0000 {
            No-map;
            reg = <0x8fff0000 0x10000>;
        };
    };

    &rpmsg {
        vdev-nums = <1>;
        reg = <0x8fff0000 0x10000>;
        status = "okay";
    };

    &uart2 { // adjust to the Zephyr uart console number of your board
        status = "disabled";
    };


You can use this repository as a reference:

https://github.com/diegosueiro/linux-fslc/tree/4.9-1.0.x-imx


Open a serial terminal (minicom, putty, etc.) and connect the board with the
following settings for UART_2 for the colibri_imx7d_m4 and warp7_m4 boards, and
UART_5 for the udoo_neo_full_m4 board:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Here are the instructions to load and run Zephyr on M4 from A7 using u-boot.

Copy the compiled ``zephyr.bin`` to the first EXT partition of the SD card and
plug the SD card into the board. Power up the board and stop the u-boot
execution. Set the u-boot environment variables and run the ``zephyr.bin`` from
the TCML memory, as described here:

.. code-block:: console

	setenv bootm4 'ext4load mmc 0:1 $m4addr $m4fw && dcache flush && bootaux $m4addr'
	setenv m4tcml 'setenv m4fw zephyr.bin; setenv m4addr 0x007F8000'
	setenv bootm4tcml 'run m4tcml && run bootm4'
	run bootm4tcml
	run bootcmd

In the Linux console load the imx_rpmsg_tty module:

.. code-block:: console

	modprobe imx_rpmsg_tty

And in the Zephyr console you should see:

.. code-block:: console

	***** Booting Zephyr OS zephyr-v1.13.0-2189-gdda6786 *****
	Starting application thread!

	OpenAMP remote echo demo started
	echo message: hello world!

You can use microcom on the Linux side to send and receive the characters:

.. code-block:: console

	microcom /dev/ttyRPMSG
