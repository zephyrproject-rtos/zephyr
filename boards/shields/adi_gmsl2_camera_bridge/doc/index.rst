.. _adi_gmsl2_camera_bridge:

ADI GMSL2 Camera Bridge
#######################

Overview
********

The shield brings a camera attached to an ADI GMSL2 serializer board to the
CSI-2 camera connector of a host board, over a coaxial cable, through the
MAX96724 quad GMSL2 deserializer of the MAX96724-BAK-EVK evaluation kit.

It describes the deserializer on the host's camera I2C bus, its CSI-2 output
towards the host, GMSL link A, and the serializer board behind that link: the
AD-GMSL717MIPI-EVK with a MAX96717 serializer at I2C address 0x40, running the
link at 6 Gbps. The serializer's camera-side I2C bus and its MFP pins are
exposed as an I2C controller and a GPIO controller, so a camera sensor can be
placed behind them in the devicetree and have its enable line driven from the
serializer.

The camera sensor itself is not part of the shield. It comes from a sensor
shield such as :ref:`raspberry_pi_camera_module_2` and an application overlay
that moves it behind the serializer, or from the deserializer's pattern
generator when no camera is connected.

Requirements
************

A host board with a CSI-2 camera connector described by the node labels used
for :dtcompatible:`raspberrypi,csi-connector` boards: ``csi_i2c``,
``csi_interface``, ``csi_capture_port`` and ``csi_ep_in``. The shield was
developed on the :zephyr:board:`stm32n6570_dk`.

The I2C transactions the host sends to the serializer and the camera travel
over the GMSL link, and the deserializer occasionally refuses a transaction
that starts right after the previous one. The shield sets
:kconfig:option:`CONFIG_I2C_GMSL_GATE_RETRY_NUM` to 5, retries in the
deserializer's I2C gate for every device behind the link, and
:kconfig:option:`CONFIG_VIDEO_I2C_RETRY_NUM` to 5 for the deserializer's own
registers, which it refuses rarely.

Usage
*****

The shield describes no camera. A sensor shield such as
:ref:`raspberry_pi_camera_module_2` declares the sensor on the host bus, and an
application overlay moves it behind the serializer: under ``ser_i2c`` for its
I2C bus, with its CSI-2 output connected to port 0 of ``max96717``. For the
IMX219 of the Raspberry Pi Camera Module 2:

.. code-block:: devicetree

   #include <zephyr/dt-bindings/video/video-interfaces.h>

   /delete-node/ &imx219;

   &ser_i2c {
           imx219: imx219@10 {
                   compatible = "sony,imx219";
                   reg = <0x10>;
                   clocks = <&imx219_input_clock>;

                   port {
                           imx219_ep_out: endpoint {
                                   remote-endpoint-label = "ser_csi_in";
                                   bus-type = <VIDEO_BUS_TYPE_CSI2_DPHY>;
                                   data-lanes = <1 2>;
                           };
                   };
           };
   };

   &max96717 {
           ports {
                   port@0 {
                           reg = <0>;

                           ser_csi_in: endpoint {
                                   remote-endpoint-label = "imx219_ep_out";
                                   bus-type = <VIDEO_BUS_TYPE_CSI2_DPHY>;
                                   data-lanes = <1 2>;
                           };
                   };
           };
   };

The sensor shield comes first, for example
``--shield raspberry_pi_camera_module_2 --shield adi_gmsl2_camera_bridge``, so
that this shield can reconnect the host's CSI-2 input to the deserializer.

The camera module's enable pin is wired to serializer MFP0, pin 0 of
``ser_gpio``: the sensor answers only once that pin is driven high, which the
sensor node can do through a reset or enable GPIO where its binding has one.

With no serializer or camera connected, the deserializer's pattern generator
streams 800x480 RGB888 instead, selected through its ``VIDEO_CID_TEST_PATTERN``
control (gradient or checkerboard). An application overlay then disables
``max96717``, ``ser_i2c`` and ``ser_gpio``, enables ``csi_interface`` and points
the ``zephyr,camera`` chosen node at ``csi_capture_port``, which a sensor shield
would otherwise do. The serializer has the same control for its own pattern
generator, which sends a pattern over the link without a camera.

The capture sample has a configuration for a Raspberry Pi Camera Module 2 on
the serializer board, on the :zephyr:board:`stm32n6570_dk`:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/video/capture
   :board: stm32n6570_dk/stm32n657xx/fsbl
   :shield: raspberry_pi_camera_module_2,adi_gmsl2_camera_bridge
   :gen-args: -DEXTRA_CONF_FILE=boards/adi_gmsl2_camera_bridge_1x_imx219.conf -DEXTRA_DTC_OVERLAY_FILE=boards/adi_gmsl2_camera_bridge_1x_imx219.overlay
   :goals: build

References
**********

- `MAX96724 product page <https://www.analog.com/en/products/max96724.html>`_
- `MAX96717 product page <https://www.analog.com/en/products/max96717.html>`_
