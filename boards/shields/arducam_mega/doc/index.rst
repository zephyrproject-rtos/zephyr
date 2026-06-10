.. _arducam_mega:

ArduCam Mega SPI Camera Shield
###############################

Overview
********

The ArduCam Mega SPI shield provides the Arducam Mega image sensor over a standard
SPI interface. Four shield variants are available:

- ``arducam_mega`` - generic variant; requires a board-specific overlay
- ``arducam_mega_arduino`` - for boards exposing an Arduino-compatible SPI header
- ``arducam_mega_mikrobus`` - for boards exposing a MikroBus connector
- ``arducam_mega_xiao`` - for boards exposing a Seeed XIAO connector

Requirements
************

This shield requires a board that provides:

- An SPI bus
- A chip select GPIO

Each connector-specific variant relies on a standard SPI alias defined by the board
(``arduino_spi``, ``mikrobus_spi``, or ``xiao_spi``), which is assigned to the
``arducam_mega_spi`` alias used by the shared ``arducam_mega.overlay``. The base
``arducam_mega`` variant requires a board-specific overlay to define the
``arducam_mega_spi`` alias and CS pin manually.

Board Overlays
==============

The connector-specific variants assign the appropriate board SPI bus to the
``arducam_mega_spi`` alias and then include the shared ``arducam_mega.overlay``,
which registers the device node and sets ``zephyr,camera``.

**arducam_mega_arduino** — uses the ``arduino_spi`` alias:

.. code-block:: dts

   arducam_mega_spi: &arduino_spi {};
   #include "arducam_mega.overlay"

**arducam_mega_mikrobus** — uses the ``mikrobus_spi`` alias:

.. code-block:: dts

   arducam_mega_spi: &mikrobus_spi {};
   #include "arducam_mega.overlay"

**arducam_mega_xiao** — uses the ``xiao_spi`` alias:

.. code-block:: dts

   arducam_mega_spi: &xiao_spi {};
   #include "arducam_mega.overlay"

**arducam_mega** — generic variant; the shield overlay is intentionally empty.
Create ``boards/<board_name>.overlay`` inside the shield directory, replacing
``<board_spi_bus>``, ``<gpio_port>``, and ``<pin>`` with values appropriate for
your board:

.. code-block:: dts

   arducam_mega_spi: &<board_spi_bus> {
       cs-gpios = <&<gpio_port> <pin> GPIO_ACTIVE_LOW>;
   };
   #include "arducam_mega.overlay"

The shared ``arducam_mega.overlay`` included by all variants contains:

.. code-block:: dts

   / {
       chosen {
           zephyr,camera = &arducam_mega;
       };
   };

   &arducam_mega_spi {
       arducam_mega: arducam-mega@0 {
           compatible = "arducam,mega";
           reg = <0>;
           spi-max-frequency = <4000000>;
       };
   };

Programming
***********

Select the shield variant that matches your board's connector and pass it to
``west build``. For example, using the Arduino variant on the ``nucleo_h563zi``:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/tcpserversink
   :board: nucleo_h563zi
   :shield: arducam_mega_arduino
   :gen-args: -DCONFIG_NET_PKT_RX_COUNT=10 -DCONFIG_NET_PKT_TX_COUNT=10 -DCONFIG_NET_BUF_RX_COUNT=20 -DCONFIG_NET_BUF_TX_COUNT=20 -DCONFIG_NET_MAX_CONTEXTS=10 -DCONFIG_VIDEO_BUFFER_POOL_HEAP_SIZE=131072 -DCONFIG_VIDEO_FRAME_WIDTH=96 -DCONFIG_VIDEO_FRAME_HEIGHT=96 -DCONFIG_VIDEO_PIXEL_FORMAT=\"JPEG\"
   :goals: build flash

References
**********

.. target-notes::

.. _Arducam Mega:
   https://blog.arducam.com/camera-for-any-microcontroller/
