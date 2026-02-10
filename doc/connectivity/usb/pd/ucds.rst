.. _usbc_api:

USB-C device stack
##################

The USB-C device stack is a hardware independent interface between a
Type-C Port Controller (TCPC) and customer applications. It is a port of
the Google ChromeOS Type-C Port Manager (TCPM) stack.
It provides the following functionalities:

* Uses the APIs provided by the Type-C Port Controller drivers to interact with
  the Type-C Port Controller.
* Provides a programming interface that's used by a customer applications.
  The APIs is described in
  :zephyr_file:`include/zephyr/usb_c/usbc.h`

Configuration options
**********************************************************

The USB-C device stack supports implementation of Sink only, Source only,
and Dual Role Power (DRP) devices.

- :kconfig:option:`CONFIG_USBC_CSM_SINK_ONLY`: Sink USB-C Connection State Machine
- :kconfig:option:`CONFIG_USBC_CSM_SOURCE_ONLY`: Source USBC Connection State Machine
- :kconfig:option:`CONFIG_USBC_CSM_DRP`: Dual Role Power (DRP) USB-C Connection State Machine

:zephyr:code-sample-category:`List<usbc>` of samples for different purposes.

Implementing a Sink Type-C and Power Delivery USB-C device
**********************************************************

The configuration of a USB-C Device is done in the stack layer and devicetree.

The following devicetree, structures and callbacks need to be defined:

* Devicetree usb-c-connector node referencing a TCPC
* Devicetree vbus node referencing a VBUS measurement device
* User defined structure that encapsulates application specific data
* Policy callbacks

For example, for the Sample USB-C Sink application:

Each Physical Type-C port is represented in the devicetree by a usb-c-connector
compatible node:

.. literalinclude:: ../../../../samples/subsys/usb_c/sink/boards/b_g474e_dpow1.overlay
   :language: dts
   :start-after: usbc.rst usbc-port start
   :end-before: usbc.rst usbc-port end
   :linenos:

VBUS is measured by a device that's referenced in the devicetree by a
usb-c-vbus-adc compatible node:

.. literalinclude:: ../../../../samples/subsys/usb_c/sink/boards/b_g474e_dpow1.overlay
   :language: dts
   :start-after: usbc.rst vbus-voltage-divider-adc start
   :end-before: usbc.rst vbus-voltage-divider-adc end
   :linenos:


A user defined structure is defined and later registered with the subsystem and can
be accessed from callback through an API:

.. literalinclude:: ../../../../samples/subsys/usb_c/sink/src/main.c
   :language: c
   :start-after: usbc.rst port data object start
   :end-before: usbc.rst port data object end
   :linenos:

These callbacks are used by the subsystem to set or get application specific data:

.. literalinclude:: ../../../../samples/subsys/usb_c/sink/src/main.c
   :language: c
   :start-after: usbc.rst callbacks start
   :end-before: usbc.rst callbacks end
   :linenos:

This callback is used by the subsystem to query if a certain action can be taken:

.. literalinclude:: ../../../../samples/subsys/usb_c/sink/src/main.c
   :language: c
   :start-after: usbc.rst check start
   :end-before: usbc.rst check end
   :linenos:

This callback is used by the subsystem to notify the application of an event:

.. literalinclude:: ../../../../samples/subsys/usb_c/sink/src/main.c
   :language: c
   :start-after: usbc.rst notify start
   :end-before: usbc.rst notify end
   :linenos:

Registering the callbacks:

.. literalinclude:: ../../../../samples/subsys/usb_c/sink/src/main.c
   :language: c
   :start-after: usbc.rst register start
   :end-before: usbc.rst register end
   :linenos:

Register the user defined structure:

.. literalinclude:: ../../../../samples/subsys/usb_c/sink/src/main.c
   :language: c
   :start-after: usbc.rst user data start
   :end-before: usbc.rst user data end
   :linenos:

Start the USB-C subsystem:

.. literalinclude:: ../../../../samples/subsys/usb_c/sink/src/main.c
   :language: c
   :start-after: usbc.rst usbc start
   :end-before: usbc.rst usbc end
   :linenos:

Implementing a Source Type-C and Power Delivery USB-C device
************************************************************

The configuration of a USB-C Device is done in the stack layer and devicetree.

Define the following devicetree, structures and callbacks:

* Devicetree ``usb-c-connector`` node referencing a TCPC
* Devicetree ``vbus`` node referencing a VBUS measurement device
* Devicetree ``pwrctrl`` node for VBUS and VCONN power control
* User defined structure that encapsulates application specific data
* Policy callbacks

For example, for the Sample USB-C Source application:

Each Physical Type-C port is represented in the devicetree by a ``usb-c-connector``
compatible node:

.. literalinclude:: ../../../../samples/subsys/usb_c/source/boards/stm32g081b_eval.overlay
   :language: dts
   :start-after: usbc.rst usbc-port start
   :end-before: usbc.rst usbc-port end
   :linenos:

VBUS is measured by a device that's referenced in the devicetree by a
``usb-c-vbus-adc`` compatible node:

.. literalinclude:: ../../../../samples/subsys/usb_c/source/boards/stm32g081b_eval.overlay
   :language: dts
   :start-after: usbc.rst vbus-voltage-divider-adc start
   :end-before: usbc.rst vbus-voltage-divider-adc end
   :linenos:

Power control for VBUS and VCONN can be managed by a device referenced in the
devicetree by a ``zephyr,usb-c-pwrctrl`` compatible node:

.. literalinclude:: ../../../../samples/subsys/usb_c/source/boards/stm32g081b_eval.overlay
   :language: dts
   :start-after: usbc.rst pwrctrl start
   :end-before: usbc.rst pwrctrl end
   :linenos:

A user defined structure is defined and later registered with the subsystem and can
be accessed from callback through an API:

.. literalinclude:: ../../../../samples/subsys/usb_c/source/src/main.c
   :language: c
   :start-after: usbc.rst port data object start
   :end-before: usbc.rst port data object end
   :linenos:

These callbacks are used by the subsystem to set or get application specific data:

.. literalinclude:: ../../../../samples/subsys/usb_c/source/src/main.c
   :language: c
   :start-after: usbc.rst callbacks start
   :end-before: usbc.rst callbacks end
   :linenos:

This callback is used by the subsystem to query if a certain action can be taken:

.. literalinclude:: ../../../../samples/subsys/usb_c/source/src/main.c
   :language: c
   :start-after: usbc.rst check start
   :end-before: usbc.rst check end
   :linenos:

This callback is used by the subsystem to notify the application of an event:

.. literalinclude:: ../../../../samples/subsys/usb_c/source/src/main.c
   :language: c
   :start-after: usbc.rst notify start
   :end-before: usbc.rst notify end
   :linenos:

Registering the callbacks:

.. literalinclude:: ../../../../samples/subsys/usb_c/source/src/main.c
   :language: c
   :start-after: usbc.rst register start
   :end-before: usbc.rst register end
   :linenos:

Register the user defined structure:

.. literalinclude:: ../../../../samples/subsys/usb_c/source/src/main.c
   :language: c
   :start-after: usbc.rst user data start
   :end-before: usbc.rst user data end
   :linenos:

Start the USB-C subsystem:

.. literalinclude:: ../../../../samples/subsys/usb_c/source/src/main.c
   :language: c
   :start-after: usbc.rst usbc start
   :end-before: usbc.rst usbc end
   :linenos:

Implementing a Dual Role Power (DRP) USB-C device
**************************************************

DRP devices can operate as both Source and Sink, automatically negotiating the
appropriate role with the port partner. When unattached, the device toggles
between Source (Rp) and Sink (Rd) CC line advertisements to detect and connect
with any partner type. Once an attach is detected, the device enters the
appropriate attached state (Attached.SRC or Attached.SNK) and starts the
corresponding Policy Engine state machine (PE_SRC or PE_SNK) to negotiate
power delivery.

Configuration is similar to Source and Sink devices, with the following key
differences:

* Set ``power-role = "dual"`` in the devicetree ``usb-c-connector`` node
* Implement callbacks for both Source and Sink operations

The DRP toggle behavior can be configured via Kconfig:

- :kconfig:option:`CONFIG_USBC_DRP_PERIOD_MS`: Toggle period (50-100ms, default 75ms)
- :kconfig:option:`CONFIG_USBC_DRP_DUTY_CYCLE`: Percentage of time as Source (30-70%, default 50%)

See :zephyr:code-sample:`usb-c-drp` for a complete example.

API reference
*************

.. doxygengroup:: _usbc_device_api

SINK callback reference
***********************

.. doxygengroup:: sink_callbacks

SOURCE callback reference
*************************

.. doxygengroup:: source_callbacks
