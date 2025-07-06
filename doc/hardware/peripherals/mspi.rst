.. _mspi_api:

Multi-bit SPI Bus
#################

The MSPI (multi-bit SPI) is provided as a generic API to accommodate
advanced SPI peripherals and devices that typically require command,
address and data phases, and multiple signal lines during these phases.
While the API supports advanced features such as :term:`XIP` and scrambling,
it is also compatible with generic SPI.

.. contents::
    :local:
    :depth: 2

.. _mspi-controller-api:

MSPI Controller API
*******************

Zephyr's MSPI controller API may be used when a multi-bit SPI controller
is present. E.g. Ambiq MSPI, QSPI, OSPI, Flexspi, etc.
The API supports single to hex SDR/DDR IO with variable latency and advanced
features such as :term:`XIP` and scrambling. Applicable devices include but
not limited to high-speed, high density flash/psram memory devices, displays
and sensors.

The MSPI interface contains controller drivers that are SoC platform specific
and implement the MSPI APIs, and device drivers that reference these APIs.
The relationship between the controller and device drivers is many-to-many to
allow for easy switching between platforms.

Here is a list of generic steps for initializing the MSPI controller and the
MSPI bus inside the device driver initialization function:

#. Initialize the data structure of the MSPI controller driver instance.
   The usual device defining macros such as :c:macro:`DEVICE_DT_INST_DEFINE`
   can be used, and the initialization function, config and data provided
   as a parameter to the macro.

#. Initialize the hardware, including but not limited to:

   * Check :c:struct:`mspi_cfg` against hardware's own capabilities to prevent
     incorrect usages.

   * Setup default pinmux.

   * Setup the clock for the controller.

   * Power on the hardware.

   * Configure the hardware using :c:struct:`mspi_cfg` and possibly more
     platform specific settings.

   * Usually, the :c:struct:`mspi_cfg` is filled from device tree and contains
     static, boot time parameters. However, if needed, one can use :c:func:`mspi_config`
     to re-initialize the hardware with new parameters during runtime.

   * Release any lock if applicable.

#. Perform device driver initialization. As usually, :c:macro:`DEVICE_DT_INST_DEFINE`
   can be used. Inside device driver initialization function, perform the following
   required steps.

   #. Call :c:func:`mspi_dev_config` with device specific hardware settings obtained
      from device datasheets.

      * The :c:struct:`mspi_dev_cfg` should be filled by device tree and helper macro
        :c:macro:`MSPI_DEVICE_CONFIG_DT` can be used.

      * The controller driver should then validate the members of :c:struct:`mspi_dev_cfg`
        to prevent incorrect usage.

      * The controller driver should implement a mutex to protect from accidental access.

      * The controller driver may also switch between different devices based on
        :c:struct:`mspi_dev_id`.

   #. Call API for additional setups if supported by hardware

      * :c:func:`mspi_xip_config` for :term:`XIP` feature

      * :c:func:`mspi_scramble_config` for scrambling feature

      * :c:func:`mspi_timing_config` for platform specific timing setup.

   #. Register any callback with :c:func:`mspi_register_callback` if needed.

   #. Release the controller mutex lock.

Transceive
==========
The transceive request is of type :c:struct:`mspi_xfer` which allows dynamic change to
the transfer related settings once the mode of operation is determined and configured
by :c:func:`mspi_dev_config`.

The API also supports bulk transfers with different starting addresses and sizes with
:c:struct:`mspi_xfer_packet`. However, it is up to the controller implementation
whether to support scatter IO and callback management. The controller can determine
which user callback to trigger based on :c:enum:`mspi_bus_event_cb_mask` upon completion
of each async/sync transfer if the callback had been registered using
:c:func:`mspi_register_callback`. Or not to trigger any callback at all with
:c:enum:`MSPI_BUS_NO_CB` even if the callbacks are already registered.
In which case that a controller supports hardware command queue, user could take full
advantage of the hardware performance if scatter IO and callback management are supported
by the driver implementation.

Device Tree
===========

Here is an example for defining an MSPI controller in device tree:
The mspi controller's bindings should reference mspi-controller.yaml as one of the base.

.. code-block:: devicetree

   mspi0: mspi@400 {
            status = "okay";
            compatible = "zephyr,mspi-emul-controller";

            reg = < 0x400 0x4 >;
            #address-cells = < 0x1 >;
            #size-cells = < 0x0 >;

            clock-frequency = < 0x17d7840 >;
            op-mode = "MSPI_CONTROLLER";
            duplex = "MSPI_HALF_DUPLEX";
            ce-gpios = < &gpio0 0x5 0x1 >, < &gpio0 0x12 0x1 >;
            dqs-support;

            pinctrl-0 = < &pinmux-mspi0 >;
            pinctrl-names = "default";
   };

Here is an example for defining an MSPI device in device tree:
The mspi device's bindings should reference mspi-device.yaml as one of the base.

.. code-block:: devicetree

   &mspi0 {

            mspi_dev0: mspi_dev0@0 {
                     status = "okay";
                     compatible = "zephyr,mspi-emul-device";

                     reg = < 0x0 >;
                     size = < 0x10000 >;

                     mspi-max-frequency = < 0x2dc6c00 >;
                     mspi-io-mode = "MSPI_IO_MODE_QUAD";
                     mspi-data-rate = "MSPI_DATA_RATE_SINGLE";
                     mspi-hardware-ce-num = < 0x0 >;
                     read-instruction = < 0xb >;
                     write-instruction = < 0x2 >;
                     instruction-length = "INSTR_1_BYTE";
                     address-length = "ADDR_4_BYTE";
                     rx-dummy = < 0x8 >;
                     tx-dummy = < 0x0 >;
                     xip-config = < 0x0 0x0 0x0 0x0 >;
                     ce-break-config = < 0x0 0x0 >;
            };

   };

User should specify target operating parameters in the DTS such as ``mspi-max-frequency``,
``mspi-io-mode`` and ``mspi-data-rate`` even though they may subject to change during runtime.
It should represent the typical configuration of the device during normal operations.

Multi Peripheral
================
With :c:struct:`mspi_dev_id` defined as collection of the device index and CE GPIO from
device tree, the API supports multiple devices on the same controller instance.
The controller driver implementation may or may not support device switching,
which can be performed either by software or by hardware. If the switching is handled
by software, it should be performed in :c:func:`mspi_dev_config` call.

The device driver should record the current operating conditions of the device to support
software controlled device switching by saving and updating :c:struct:`mspi_dev_cfg` and
other relevant mspi struct or private data structures. In particular, :c:struct:`mspi_dev_id`
which contains the identity of the device needs to be used for every API call.


Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_MSPI`
* :kconfig:option:`CONFIG_MSPI_ASYNC`
* :kconfig:option:`CONFIG_MSPI_PERIPHERAL`
* :kconfig:option:`CONFIG_MSPI_XIP`
* :kconfig:option:`CONFIG_MSPI_SCRAMBLE`
* :kconfig:option:`CONFIG_MSPI_TIMING`
* :kconfig:option:`CONFIG_MSPI_INIT_PRIORITY`
* :kconfig:option:`CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE`

API Reference
*************

.. doxygengroup:: mspi_interface
