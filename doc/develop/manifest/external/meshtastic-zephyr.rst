.. _external_module_meshtastic_zephyr:

Meshtastic Zephyr stack
#######################

Introduction
************

`Meshtastic Zephyr`_ packages a Zephyr-native implementation of the `Meshtastic`_ mesh communication
stack. It is intended to run on Zephyr-supported boards with LoRa transceivers and is an alternative
to the original Meshtastic firmware.

The module provides APIs and optional shell commands for configuring the stack, sending and
receiving text messages, maintaining node information, and exposing PhoneAPI transports over
Bluetooth LE and serial. It also includes a Zephyr sample application that can be used as a starting
point for building Meshtastic-compatible devices.

Meshtastic Zephyr is licensed under the GPL-3.0 license, similar to the original Meshtastic
firmware.

Usage with Zephyr
*****************

To pull in Meshtastic Zephyr as a Zephyr module, either add it as a West project in the
:file:`west.yaml` file or pull it in by adding a submanifest (e.g.
``zephyr/submanifests/meshtastic-zephyr.yaml``) file with the following content and run
:command:`west update`:

.. code-block:: yaml

   manifest:
     projects:
       - name: meshtastic-zephyr
         url: https://github.com/kartben/meshtastic-zephyr.git
         revision: main
         path: modules/lib/meshtastic-zephyr # adjust the path as needed

The module requires the ``nanopb`` module to be present in the workspace, which is the case when
using Zephyr's default manifest.

After adding the module, build the sample application with:

.. code-block:: shell

   west build -b <board> modules/lib/meshtastic-zephyr/samples/meshtastic

When ``CONFIG_MESHTASTIC_SHELL`` is enabled, the module registers a ``meshtastic`` shell command
tree for inspecting node state, configuring channels, and sending messages or telemetry, depending
on the enabled Kconfig options.

References
**********

.. target-notes::

.. _Meshtastic Zephyr:
   https://github.com/kartben/meshtastic-zephyr

.. _Meshtastic:
   https://meshtastic.org/