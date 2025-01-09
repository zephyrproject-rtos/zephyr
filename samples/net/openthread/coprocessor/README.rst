.. zephyr:code-sample:: openthread-coprocessor
   :name: OpenThread co-processor
   :relevant-api: openthread

   Build a Thread border-router using OpenThread's co-processor designs.

Overview
********

OpenThread Co-Processor allows building a Thread Border Router. The code in this
sample is only the MCU target part of a complete Thread Border Router.
The Co-Processor can act in two variants: Network Co-Processor (NCP) and Radio
Co-Processor (RCP), see https://openthread.io/platforms/co-processor.

Additional required host-side tools (e.g. otbr-agent) to build a Thread Border
Router can be obtained by following
https://openthread.io/guides/border-router/build#set-up-the-border-router.

The preferred Co-Processor configuration of OpenThread is RCP now.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/openthread/coprocessor`.

Building and Running
********************

Build the OpenThread NCP sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/coprocessor
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

Build the OpenThread NCP sample application which uses CDC ACM UART device:

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/coprocessor
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :gen-args: -DDTC_OVERLAY_FILE=usb.overlay -DEXTRA_CONF_FILE=overlay-usb-nrf-br.conf
   :compact:

Example building for the nrf52840dk/nrf52840 for RCP:

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/coprocessor
   :host-os: unix
   :board: nrf52840dk/nrf52840
   :conf: "prj.conf overlay-rcp.conf"
   :goals: run
   :compact:

There are configuration files for different boards and setups in the
coprocessor directory:

- :file:`prj.conf`
  Generic NCP config file. Use this, if you want the NCP configuration.

- :file:`overlay-rcp.conf`
   RCP overlay file. Use this in combination with prj.conf, if you want the RCP
   configuration.

- :file:`overlay-tri-n4m-br.conf`
  This is an overlay for the dedicated Thread Border Router hardware
  https://www.tridonic.com/com/en/download/data_sheets/net4more_Borderrouter_PoE-Thread_en.pdf.
  The board support is not part of the Zephyr repositories, but the
  product is based on NXP K64 and AT86RF233. This file can be used as an
  example for a development set-up based on development boards.
