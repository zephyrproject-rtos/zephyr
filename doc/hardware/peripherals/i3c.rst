.. _i3c_api:

I3C
###

I3C (Improved Inter-Integrated Circuit) is a two-signal shared
peripheral interface bus.  Devices on the bus can operate in
two roles: as a "controller" that initiates transactions and
controls the clock, or as a "target" that responds to transaction
commands.

Currently, the API is based on `I3C Specification`_ version 1.1.1.

.. contents::
    :local:
    :depth: 2

.. _i3c-controller-api:

I3C Controller API
******************

Zephyr's I3C controller API is used when an I3C controller controls
the bus, in particularly the start and stop conditions and the clock.
This is the most common mode, used to interact with I3C target
devices such as sensors.

Due to the nature of the I3C, there are devices on the bus where
they may not have addresses when powered on. Therefore, an additional
dynamic address assignment needs to be carried out by the I3C
controller. Because of this, the controller needs to maintain
separate structures to keep track of device status. This can be done
at build time, for example, by creating arrays of device descriptors
for both I3C and I\ :sup:`2`\ C devices:

.. code-block:: c

   static struct i3c_device_desc i3c_device_array[] = I3C_DEVICE_ARRAY_DT_INST(inst);
   static struct i3c_i2c_device_desc i2c_device_array[] = I3C_I2C_DEVICE_ARRAY_DT_INST(inst);

The macros :c:macro:`I3C_DEVICE_ARRAY_DT_INST` and
:c:macro:`I3C_I2C_DEVICE_ARRAY_DT_INST` are helper macros to aid in
create arrays of device descriptors corresponding to the devicetree
nodes under the I3C controller.

Here is a list of generic steps for initializing the I3C
controller and the I3C bus inside the device driver
initialization function:

#. Initialize the data structure of the I3C controller device
   driver instance. The usual device defining macros such as
   :c:macro:`DEVICE_DT_INST_DEFINE` can be used, and the initialization
   function provided as a parameter to the macro.

   * The :c:struct:`i3c_addr_slots` and :c:struct:`i3c_dev_list` are
     structures to aid in address assignments and device list management.
     If this is being used, this struct needs to be initialized by calling
     :c:func:`i3c_addr_slots_init`. These two structures can also be used
     with various helper functions.

   * Initialize the device descriptors if needed by the controller
     driver.

#. Initialize the hardware, including but not limited to:

   * Setup pin mux and directions.

   * Setup the clock for the controller.

   * Power on the hardware.

   * Configure the hardware (e.g. SCL clock frequency).

#. Perform bus initialization. There is a generic helper function,
   :c:func:`i3c_bus_init`, which performs the following steps.
   This function can be used if the controller does not require
   any special handling during bus initialization.

   #. Do ``RSTDAA`` to reset dynamic addresses of connected devices.
      If any connected devices have already been assigned an address,
      the bookkeeping data structures do not have records of these,
      for example, at power-on. So it is a good idea to reset and
      assign them new addresses.

   #. Do ``DISEC`` to disable any events from devices.

   #. Do ``SETDASA`` to use static addresses as dynamic address
      if so desired.

      * ``SETAASA`` may not be supported for all connected devices
        to assign static addresses as dynamic addresses.

      * BCR and DCR need to be obtained separately to populate
        the relevant fields in the I3C target device descriptor
        struct.

   #. Do ``ENTDAA`` to start dynamic address assignment, if there are
      still devices without addresses.

      * If there is a device waiting for address, it will send
        its Provisioned ID, BCR, and DCR back. Match the received
        Provisioned ID to the list of registered I3C devices.

        * If there is a match, assign an address (either from
          the stated static address if ``SETDASA`` has not been
          done, or use a free address).

          * Also, set the BCR and DCR fields in the device descriptor
            struct.

        * If there is no match, depending on policy, it can be
          assigned a free address, or the device driver can stop
          the assignment process and errors out.

          * Note that the I3C API requires device descriptor to
            function. A device without a device descriptor cannot be
            accessed through the API.

      * This step can be skipped if there is no connected devices
        requiring DAA.

   #. These are optional but highly recommended:

      * Do ``GETMRL`` and ``GETMWL`` to get maximum read/write
        length.

      * Do ``GETMXDS`` to get maximum read/write speed and maximum
        read turnaround time.

      * The helper function, :c:func:`i3c_bus_init`, would retrieve
        basic device information such as BCR, DCR, MRL and MWL.

   #. Do ``ENEC`` to re-enable events from devices.

      * The helper function, :c:func:`i3c_bus_init`, only re-enables
        hot-join events. IBI event should only be enabled when
        enabling IBI of a device.

In-Band Interrupt (IBI)
=======================

If a target device can generate In-Band Interrupt (IBI),
the controller needs to be made aware of it.

* :c:func:`i3c_ibi_enable` to enable IBI of a target device.

  * Some controller hardware have IBI slots which need to be
    programmed so that the controller can recognize incoming IBIs
    from a particular target device.

    * If the hardware has IBI slots, :c:func:`i3c_ibi_enable`
      needs to program those IBI slots.

    * Note that there are usually limited IBI slots on
      the controller so this operation may fail.

  * The implementation in driver should also send the ``ENEC`` command
    to enable interrupt of this target device.

* :c:func:`i3c_ibi_disable` to disable IBI of a target device.

  * If controller hardware makes use of IBI slots, this will remove
    description of the target device from the slots.

  * The implementation in driver should also send the ``DISEC`` command
    to disable interrupt of this target device.

Device Tree
===========

Here is an example for defining a I3C controller in device tree:

.. code-block:: devicetree

   i3c0: i3c@10000 {
           compatible = "vendor,i3c";

           #address-cells = < 0x3 >;
           #size-cells = < 0x0 >;

           reg = < 0x10000 0x1000 >;
           interrupts = < 0x1F 0x0 >;

           pinctrl-0 = < &pinmux-i3c >;
           pinctrl-names = "default";

           i2c-scl-hz = < 400000 >;

           i3c-scl-hz = < 12000000 >;

           status = "okay";

           i3c-dev0: i3c-dev0@420000ABCD12345678 {
                   compatible = "vendor,i3c-dev";

                   reg = < 0x42 0xABCD 0x12345678 >;

                   status = "okay";
           };

           i2c-dev0: i2c-dev0@380000000000000050 {
                   compatible = "vendor-i2c-dev";

                   reg = < 0x38 0x0 0x50 >;

                   status = "okay";
           };
   };

I3C Devices
-----------

For I3C devices, the ``reg`` property has 3 elements:

* The first one is the static address of the device.

  * Can be zero if static address is not used. Address will be
    assigned during DAA (Dynamic Address Assignment).

  * If non-zero and property ``assigned-address`` is not set,
    this will be the address of the device after SETDASA
    (Set Dynamic Address from Static Address) is issued.

* Second element is the upper 16-bit of the Provisioned ID (PID)
  which contains the manufacturer ID left-shifted by 1. This is
  the bits 33-47 (zero-based) of the 48-bit Provisioned ID.

* Third element contains the lower 32-bit of the Provisioned ID
  which is a combination of the part ID (left-shifted by 16,
  bits 16-31 of the PID) and the instance ID (left-shifted by 12,
  bits 12-15 of the PID).

Note that the unit-address (the part after ``@``) must match
the ``reg`` property fully where each element is treated as
32-bit integer, combining to form a 96-bit integer. This is
required for properly generating device tree macros.

I\ :sup:`2`\ C Devices
----------------------

For I\ :sup:`2`\ C devices where the device driver has support for
working under I3C bus, the device node can be described as
a child of the I3C controller. If the device driver is written to
only work with I\ :sup:`2`\ C controllers, define the node under
the I\ :sup:`2`\ C virtual controller as described below.
Otherwise, the ``reg`` property, similar to I3C devices,
has 3 elements:

* The first one is the static address of the device. This must be
  a valid address as I\ :sup:`2`\ C devices do not support
  dynamic address assignment.

* Second element is always zero.

  * This is used by various helper macros to determine whether
    the device tree entry corresponds to a I\ :sup:`2`\ C device.

* Third element is the LVR (Legacy Virtual Register):

  * bit[31:8] are unused.

  * bit[7:5] are the I\ :sup:`2`\ C device index:

    * Index ``0``

      * I3C device has a 50 ns spike filter where it is not
        affected by high frequency on SCL.

    * Index ``1``

      * I\ :sup:`2`\ C device does not have a 50 ns spike filter but
        can work with high frequency on SCL.

    * Index ``2``

      * I3C device does not have a 50 ns spike filter and
        cannot work with high frequency on SCL.

  * bit[4] is the I\ :sup:`2`\ C mode indicator:

    * ``0`` is FM+ mode.

    * ``1`` is FM mode.

Similar to I3C devices, the unit-address must match the ``reg``
property fully where each element is treated as 32-bit integer,
combining to form a 96-bit integer.

Device Drivers for I3C Devices
==============================

All of the transfer functions of I3C controller API require
the use of device descriptors, :c:struct:`i3c_device_desc`.
This struct contains runtime information about a I3C device,
such as, its dynamic address, BCR, DCR, MRL and MWL. Therefore,
the device driver of a I3C device should grab a pointer to
this device descriptor from the controller using
:c:func:`i3c_device_find`. This function takes an ID parameter
of type :c:struct:`i3c_device_id` for matching. The returned
pointer can then be used in subsequent API calls to
the controller.

I\ :sup:`2`\ C Devices under I3C Bus
====================================

Since I3C is backware compatible with I\ :sup:`2`\ C, the I3C controller
API can accommodate I2C API calls without modifications if the controller
device driver implements the I2C API. This has the advantage of using
existing I2C devices without any modifications to their device drivers.
However, since the I3C controller API works on device descriptors,
any calls to I2C API will need to look up the corresponding device
descriptor from the I2C device address. This adds a bit of processing
cost to any I2C API calls.

On the other hand, a device driver can be extended to utilize native
I2C device support via the I3C controller API. During device
initialization, :c:func:`i3c_i2c_device_find` needs to be called to
retrieve the pointer to the device descriptor. This pointer can be used
in subsequent API calls.

Note that, with either methods mentioned above, the devicetree node of
the I2C device must be declared according to I3C standard:

The I\ :sup:`2`\ C virtual controller device driver provides a way to
interface I\ :sup:`2`\ C devices on the I3C bus where the associated
device drivers can be used as-is without modifications. This requires
adding an intermediate node in the device tree:

.. code-block:: devicetree

   i3c0: i3c@10000 {
           <... I3C controller related properties ...>
           <... Nodes of I3C devices, if any ...>

           i2c-dev0: i2c-dev0@420000000000000050 {
                   compatible = "vendor-i2c-dev";

                   reg = < 0x42 0x0 0x50 >;

                   status = "okay";
           };
   };

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_I3C`
* :kconfig:option:`CONFIG_I3C_USE_GROUP_ADDR`
* :kconfig:option:`CONFIG_I3C_USE_IBI`
* :kconfig:option:`CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE`
* :kconfig:option:`CONFIG_I3C_CONTROLLER_INIT_PRIORITY`

API Reference
*************

.. doxygengroup:: i3c_interface
.. doxygengroup:: i3c_ccc
.. doxygengroup:: i3c_addresses

.. _I3C Specification: https://www.mipi.org/specifications/i3c-sensor-specification
