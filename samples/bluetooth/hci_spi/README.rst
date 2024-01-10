.. _bluetooth-hci-spi-sample:

Bluetooth: HCI SPI
##################

Overview
********

Expose Zephyr Bluetooth Controller support over SPI to another device/CPU using
the Zephyr SPI HCI transport protocol (similar to BlueNRG).

Requirements
************

A board with SPI slave, GPIO and Bluetooth Low Energy support.

Building and Running
********************

You then need to ensure that your :ref:`devicetree <dt-guide>` defines a node
for the HCI SPI slave device with compatible
:dtcompatible:`zephyr,bt-hci-spi-slave`. This node sets an interrupt line to
the host and associates the application with a SPI bus to use.

See :zephyr_file:`boards/nrf51dk_nrf51422.overlay
<samples/bluetooth/hci_spi/boards/nrf51dk_nrf51422.overlay>` in this sample
directory for an example overlay for the :ref:`nrf51dk_nrf51422` board.

You can then build this application and flash it onto your board in
the usual way; see :ref:`boards` for board-specific building and
flashing information.

You will also need a separate chip acting as BT HCI SPI master. This
application is compatible with the HCI SPI master driver provided by
Zephyr's Bluetooth HCI driver core; see the help associated with the
:kconfig:option:`CONFIG_BT_SPI` configuration option for more information.

Refer to :ref:`bluetooth-samples` for general Bluetooth information, and
to :ref:`96b_carbon_nrf51_bluetooth` for instructions specific to the
96Boards Carbon board.

Using the controller with the Zephyr host
=========================================

This describes how to hook up a board running this sample to a board running
an application that uses the Zephyr host.

Here the example is with two nRF52840dk boards. All wires are connected to the
same port/pin on the other side, save for the reset line, which is connected to
a GPIO on the host and the SoC reset line on the controller.

Host board
----------

On the host application, some config options need to be used to select the SPI
driver instead of the built-in controller:

.. code-block:: kconfig

   CONFIG_BT_CTLR=n
   CONFIG_BT_SPI=y

A device with the `zephyr,bt-hci-spi` compatible must be present on the desired
SPI bus node.

.. code-block:: dts

   &spi1 {
      compatible = "nordic,nrf-spim";
      status = "okay";
      pinctrl-0 = <&spi1_default_alt>;
      /delete-property/ pinctrl-1;
      pinctrl-names = "default";

      cs-gpios = <&gpio1 5 GPIO_ACTIVE_LOW>;

      bt-controller@0 {
         status = "okay";
         compatible = "zephyr,bt-hci-spi";
         reg = <0>;
         spi-max-frequency = <2000000>;
         /* different scheme than pinctrl: IRQ is on P1.04 */
         irq-gpios = <&gpio1 4 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>;
         reset-gpios = <&gpio1 10 GPIO_ACTIVE_LOW>;
            reset-assert-duration-ms = <420>;
      };
   };

And its `pinctrl` node:

.. code-block:: dts

   &pinctrl {
      spi1_default_alt: spi1_default_alt {
         group1 {
            /* schema: (name, port, number)
             * only change the port and number.
             * e.g. here: SCK is on P1.08
             */
            psels = <NRF_PSEL(SPIM_SCK, 1, 8)>,
               <NRF_PSEL(SPIM_MOSI, 1, 7)>,
               <NRF_PSEL(SPIM_MISO, 1, 6)>;
         };
      };
   };

Controller board
----------------

A device compatible with `zephyr,bt-hci-spi-slave` has to be present on an SPI
peripheral bus.

.. code-block:: dts

   &spi1 {
      compatible = "nordic,nrf-spis";
      status = "okay";
      def-char = <0x00>;
      pinctrl-0 = <&spi1_default_alt>;
      /delete-property/ pinctrl-1;
      pinctrl-names = "default";

      bt-host@0 {
         compatible = "zephyr,bt-hci-spi-slave";
         reg = <0>;
         /* different scheme than pinctrl: IRQ is on P1.04 */
         irq-gpios = <&gpio1 4 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>;
      };
   };

And the `pinctrl` node:

.. code-block:: dts

   &pinctrl {
      spi1_default_alt: spi1_default_alt {
         group1 {
            /* schema: (name, port, number)
             * only change the port and number.
             * e.g. here: SCK is on P1.08
             */
            psels = <NRF_PSEL(SPIS_SCK, 1, 8)>,
               <NRF_PSEL(SPIS_MOSI, 1, 7)>,
               <NRF_PSEL(SPIS_MISO, 1, 6)>,
               <NRF_PSEL(SPIS_CSN, 1, 5)>;
         };
      };
   };
