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
under :file:`/boards/shields`:

.. code-block:: none

   boards/shields/<shield>
   ├── <shield>.overlay
   └── dts_fixup.h

These files provides shield configuration as follows:

* **<shield>.overlay**: This file provides a shield description in device tree
  format that is merged with the board's device tree information before
  compilation.

* **dts_fixup.h**: This is a fixup file to bind board components definitions with
  application in a generic fashion to enable shield compatibility across boards

Board compatibility
*******************

Hardware shield-to-board compatibility depends on the use of well-known
connectors used on popular boards (such as Arduino and 96boards).  For
software compatibility, boards must also provide a configuration matching
their supported connectors.

This should be done at two different level:

* Pinmux: Connector pins should be correctly configured to match shield pins

* Device tree: A board device tree file should define a node alias for each
  connector interface.  For example, for Arduino I2C:

.. code-block:: none

        #define arduino_i2c i2c1

        aliases {
                arduino,i2c = &i2c1;
        };

Note: With support of dtc v1.4.2, above will be replaced with the recently
introduced overriding node element:

.. code-block:: none

        arduino_i2c:i2c1{};


Shield activation
*****************

Activate support for a shield by adding the matching -DSHIELD arg to CMake
command

  .. zephyr-app-commands::
     :zephyr-app: your_app
     :shield: x_nucleo_iks01a1
     :goals: build


Alternatively, it could be set by default in a project's CMakeLists.txt:

.. code-block:: none

	set(SHIELD x_nucleo_iks01a1)
