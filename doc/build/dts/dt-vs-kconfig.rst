.. _dt_vs_kconfig:

Devicetree versus Kconfig
#########################

Along with devicetree, Zephyr also uses the Kconfig language to configure the
source code. Whether to use devicetree or Kconfig for a particular purpose can
sometimes be confusing. This section should help you decide which one to use.

In short:

* Use devicetree to describe **hardware** and its **boot-time configuration**.
  Examples include peripherals on a board, boot-time clock frequencies,
  interrupt lines, etc.
* Use Kconfig to configure **software support** to build into the final
  image. Examples include whether to add networking support, which drivers are
  needed by the application, etc.

In other words, devicetree mainly deals with hardware, and Kconfig with
software.

For example, consider a board containing a SoC with 2 UART, or serial port,
instances.

* The fact that the board has this UART **hardware** is described with two UART
  nodes in the devicetree. These provide the UART type (via the ``compatible``
  property) and certain settings such as the address range of the hardware
  peripheral registers in memory (via the ``reg`` property).
* Additionally, the UART **boot-time configuration** is also described with
  devicetree. This could include configuration such as the RX IRQ line's
  priority and the UART baud rate. These may be modifiable at runtime, but
  their boot-time configuration is described in devicetree.
* From the application code, both UART are interchangeable and using one or the
  other has no impact, it is only a matter of board design. It is pure
  **hardware configuration** and should be done with devicetree.
* Whether or not to include **software support** for UART in the build is
  controlled via Kconfig. Applications which do not need to use the UARTs can
  remove the driver source code from the build using Kconfig, even though the
  board's devicetree still includes UART nodes.

As another example, consider a device with a 2.4GHz, multi-protocol radio
supporting both the Bluetooth Low Energy and 802.15.4 wireless technologies.

* Devicetree should be used to describe the presence of the radio **hardware**,
  what driver or drivers it's compatible with, etc.
* **Boot-time configuration** for the radio, such as TX power in dBm, should
  also be specified using devicetree.
* On the application, using one or the other protocol will not get the same code
  into play, but the same hardware will be used. This is relevant to **software
  configuration**.
* Kconfig should determine which **software features** should be built for the
  radio, such as selecting a BLE or 802.15.4 protocol stack.

As another example, Kconfig options that formerly enabled a particular
instance of a driver (that is itself enabled by Kconfig) have been
removed.  The devices are selected individually using devicetree's
:ref:`status <dt-important-props>` keyword on the corresponding hardware
instance.

There are **exceptions** to these rules:

* Because Kconfig is unable to flexibly control some instance-specific driver
  configuration parameters, such as the size of an internal buffer, these
  options may be defined in devicetree.  However, to make clear that they are
  specific to Zephyr drivers and not hardware description or configuration these
  properties should be prefixed with ``zephyr,``,
  e.g. ``zephyr,random-mac-address`` in the common Ethernet devicetree
  properties.
* Devicetree's ``chosen`` keyword, which allows the user to select a specific
  instance of a hardware device to be used for a particular purpose. An example
  of this is selecting a particular UART for use as the system's console.

Automatic Kconfig symbols from devicetree
*****************************************

During the devicetree processing step, CMake runs
:zephyr_file:`scripts/dts/gen_driver_kconfig_dts.py`, which scans all
:ref:`DTS root <dts_root>` directories, including :zephyr_file:`dts/bindings` and
writes ``Kconfig.dts`` into the build's ``KCONFIG_BINARY_DIR`` (for example
``<build>/zephyr`` or ``<build>/<image>/zephyr`` in case of :ref:`sysbuild`).
For each ``compatible = "vendor,chip"`` that appears in a binding, the generated
file contains:

.. code-block:: kconfig

   DT_COMPAT_VENDOR_CHIP := vendor,chip

   config DT_HAS_VENDOR_CHIP_ENABLED
           def_bool $(dt_compat_enabled,$(DT_COMPAT_VENDOR_CHIP))

The assignment keeps the literal ``compatible`` string available to the
preprocessor functions described in :ref:`kconfig-functions`, so that Kconfig
files can call helpers such as ``$(dt_compat_on_bus,$(DT_COMPAT_<compatible>),i2c)``.
Characters like ``-``, ``,`` and ``@`` are converted to underscores when the
symbol names are created.

The hidden boolean symbol becomes ``CONFIG_DT_HAS_<compatible>_ENABLED`` after
Kconfig runs. Its value tracks whether the current devicetree contains at least
one node with that ``compatible`` whose :ref:`status <dt-important-props>` is
``okay``. Symbols for enabling drivers should almost certainly be ``default y``
and have ``depends on CONFIG_DT_HAS_<compatible>_ENABLED``, for example:

.. code-block:: kconfig

   config SENSOR_VENDOR_CHIP
           bool "Vendor Chip sensor"
           default y
           depends on DT_HAS_VENDOR_CHIP_ENABLED

Because these symbols are generated automatically, adding a new binding with
a ``compatible`` property is all that is required to make the corresponding
``DT_HAS_<compatible>_ENABLED`` and ``DT_COMPAT_<compatible>`` constructs
available to Kconfig.
