.. _shields:

Shields
#######

Shields, also known as "add-on" or "daughter boards", attach to a board
to extend its features and services for easier and modularized prototyping.
In Zephyr, the shield feature provides Zephyr-formatted shield
descriptions for easier compatibility with applications.

Shield porting and configuration
********************************

Shield configuration files are available in the board directory
under :zephyr_file:`/boards/shields`:

.. code-block:: none

   boards/shields/<shield>
   ├── <shield>.overlay
   ├── Kconfig.shield
   └── Kconfig.defconfig

These files provides shield configuration as follows:

* **<shield>.overlay**: This file provides a shield description in devicetree
  format that is merged with the board's :ref:`devicetree <dt-guide>`
  before compilation.

* **Kconfig.shield**: This file defines shield Kconfig symbols that will be
  used for default shield configuration. To ease use with applications,
  the default shield configuration here should be consistent with those in
  the :ref:`default_board_configuration`.

* **Kconfig.defconfig**: This file defines the default shield configuration. It
  is made to be consistent with the :ref:`default_board_configuration`. Hence,
  shield configuration should be done by keeping in mind that features
  activation is application responsibility.

Besides, in order to avoid name conflicts with devices that may be defined at
board level, it is advised, specifically for shields devicetree descriptions,
to provide a device nodelabel is the form <device>_<shield>, for instance:

.. code-block:: devicetree

        sdhc_myshield: sdhc@1 {
                reg = <1>;
                ...
        };

Adding Source Code
******************

It is possible to add source code to shields, as a way to meet configuration
requirements that are specific to the shield (e.g: initialization routines,
timing constraints, etc), in order to enable it for proper operation with the
different Zephyr components.

.. note::

   Source code in shields shall not be used for purposes other than the
   one described above. Generic functionalities that could be reused among
   shields (and/or targets) shall not be captured here.

To effectively incorporate source code: add a :file:`CMakeLists.txt` file, as
well as the corresponding source files (referenced in CMake similar to other
areas of Zephyr, e.g: boards).

Board compatibility
*******************

Hardware shield-to-board compatibility depends on the use of well-known
connectors used on popular boards (such as Arduino and 96boards).  For
software compatibility, boards must also provide a configuration matching
their supported connectors.

This should be done at two different level:

* Pinmux: Connector pins should be correctly configured to match shield pins

* Devicetree: A board :ref:`devicetree <dt-guide>` file,
  :file:`BOARD.dts` should define an alternate nodelabel for each connector interface.
  For example, for Arduino I2C:

.. code-block:: devicetree

        arduino_i2c: &i2c1 {};

Board specific shield configuration
-----------------------------------

If modifications are needed to fit a shield to a particular board or board
revision, you can override a shield description for a specific board by adding
board or board revision overriding files to a shield, as follows:

.. code-block:: none

   boards/shields/<shield>
   └── boards
       ├── <board>_<revision>.overlay
       ├── <board>.overlay
       ├── <board>.defconfig
       ├── <board>_<revision>.conf
       └── <board>.conf


Shield activation
*****************

Activate support for one or more shields by adding the matching ``--shield`` arguments
to the west command:

  .. zephyr-app-commands::
     :zephyr-app: your_app
     :shield: x_nucleo_idb05a1,x_nucleo_iks01a1
     :goals: build


Alternatively, it could be set by default in a project's CMakeLists.txt:

.. code-block:: cmake

	set(SHIELD x_nucleo_iks01a1)

Shield variants
***************

Some shields may support several variants or revisions. In that case, it is
possible to provide multiple version of the shields description:

.. code-block:: none

   boards/shields/<shield>
   ├── <shield_v1>.overlay
   ├── <shield_v1>.defconfig
   ├── <shield_v2>.overlay
   └── <shield_v2>.defconfig

In this case, a shield-particular revision name can be used:

  .. zephyr-app-commands::
     :zephyr-app: your_app
     :shield: shield_v2
     :goals: build

You can also provide a board-specific configuration to a specific shield
revision:

.. code-block:: none

   boards/shields/<shield>
   ├── <shield_v1>.overlay
   ├── <shield_v1>.defconfig
   ├── <shield_v2>.overlay
   ├── <shield_v2>.defconfig
   └── boards
       └── <shield_v2>
           ├── <board>.overlay
           └── <board>.defconfig

GPIO nexus nodes
****************

GPIOs accessed by the shield peripherals must be identified using the
shield GPIO abstraction, for example from the ``arduino-header-r3``
compatible.  Boards that provide the header must map the header pins
to SOC-specific pins.  This is accomplished by including a `nexus
node`_ that looks like the following into the board devicetree file:

.. _nexus node:
    https://github.com/devicetree-org/devicetree-specification/blob/4b1dac80eaca45b4babf5299452a951008a5d864/source/devicetree-basics.rst#nexus-nodes-and-specifier-mapping

.. code-block:: devicetree

    arduino_header: connector {
            compatible = "arduino-header-r3";
            #gpio-cells = <2>;
            gpio-map-mask = <0xffffffff 0xffffffc0>;
            gpio-map-pass-thru = <0 0x3f>;
            gpio-map = <0 0 &gpioa 0 0>,    /* A0 */
                       <1 0 &gpioa 1 0>,    /* A1 */
                       <2 0 &gpioa 4 0>,    /* A2 */
                       <3 0 &gpiob 0 0>,    /* A3 */
                       <4 0 &gpioc 1 0>,    /* A4 */
                       <5 0 &gpioc 0 0>,    /* A5 */
                       <6 0 &gpioa 3 0>,    /* D0 */
                       <7 0 &gpioa 2 0>,    /* D1 */
                       <8 0 &gpioa 10 0>,   /* D2 */
                       <9 0 &gpiob 3 0>,    /* D3 */
                       <10 0 &gpiob 5 0>,   /* D4 */
                       <11 0 &gpiob 4 0>,   /* D5 */
                       <12 0 &gpiob 10 0>,  /* D6 */
                       <13 0 &gpioa 8 0>,   /* D7 */
                       <14 0 &gpioa 9 0>,   /* D8 */
                       <15 0 &gpioc 7 0>,   /* D9 */
                       <16 0 &gpiob 6 0>,   /* D10 */
                       <17 0 &gpioa 7 0>,   /* D11 */
                       <18 0 &gpioa 6 0>,   /* D12 */
                       <19 0 &gpioa 5 0>,   /* D13 */
                       <20 0 &gpiob 9 0>,   /* D14 */
                       <21 0 &gpiob 8 0>;   /* D15 */
    };

This specifies how Arduino pin references like ``<&arduino_header 11
0>`` are converted to SOC gpio pin references like ``<&gpiob 4 0>``.

In Zephyr GPIO specifiers generally have two parameters (indicated by
``#gpio-cells = <2>``): the pin number and a set of flags.  The low 6
bits of the flags correspond to features that can be configured in
devicetree.  In some cases it's necessary to use a non-zero flag value
to tell the driver how a particular pin behaves, as with:

.. code-block:: devicetree

    drdy-gpios = <&arduino_header 11 GPIO_ACTIVE_LOW>;

After preprocessing this becomes ``<&arduino_header 11 1>``.  Normally
the presence of such a flag would cause the map lookup to fail,
because there is no map entry with a non-zero flags value.  The
``gpio-map-mask`` property specifies that, for lookup, all bits of the
pin and all but the low 6 bits of the flags are used to identify the
specifier.  Then the ``gpio-map-pass-thru`` specifies that the low 6
bits of the flags are copied over, so the SOC GPIO reference becomes
``<&gpiob 4 1>`` as intended.

See `nexus node`_ for more information about this capability.
