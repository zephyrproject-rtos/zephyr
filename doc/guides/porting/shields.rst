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
  format that is merged with the board's devicetree information before
  compilation.

* **Kconfig.shield**: This file defines shield Kconfig symbols that will be
  used for default shield configuration. To ease use with applications,
  the default shield configuration here should be consistent with those in
  the :ref:`default_board_configuration`.

* **Kconfig.defconfig**: This file defines the default shield configuration. It
  is made to be consistent with the :ref:`default_board_configuration`. Hence,
  shield configuration should be done by keeping in mind that features
  activation is application responsibility.

Board compatibility
*******************

Hardware shield-to-board compatibility depends on the use of well-known
connectors used on popular boards (such as Arduino and 96boards).  For
software compatibility, boards must also provide a configuration matching
their supported connectors.

This should be done at two different level:

* Pinmux: Connector pins should be correctly configured to match shield pins

* Devicetree: A board :ref:`device-tree` file should define a node alias for
  each connector interface. For example, for Arduino I2C:

.. code-block:: none

        #define arduino_i2c i2c1

        aliases {
                arduino,i2c = &i2c1;
        };

Note: With support of dtc v1.4.2, above will be replaced with the recently
introduced overriding node element:

.. code-block:: none

        arduino_i2c:i2c1{};

Board specific shield configuration
-----------------------------------

If modifications are needed to fit a shield to a particular board, you can
override a shield description for a specific board by adding board-overriding
files to a shield, as follows:

.. code-block:: none

   boards/shields/<shield>
   └── <boards>
       ├── board.overlay
       └── board.defconfig


Shield activation
*****************

Activate support for one or more shields by adding the matching -DSHIELD arg to
CMake command

  .. zephyr-app-commands::
     :zephyr-app: your_app
     :shield: "x_nucleo_idb05a1 x_nucleo_iks01a1"
     :goals: build


Alternatively, it could be set by default in a project's CMakeLists.txt:

.. code-block:: none

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
   └── <boards>
       └── <shield_v2>
           ├── board.overlay
           └── board.defconfig
