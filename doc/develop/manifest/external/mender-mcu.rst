.. _external_module_mender_mcu:

mender-mcu
##########

Introduction
************

`mender-mcu`_ enables robust firmware updates on resource-constrained devices by integrating with Zephyr. It
implements an Update Module interface, which allows a module to define the specifics of how to handle an update.
mender-mcu provides a default Update Module that integrates with MCUboot to provide A/B updates. This allows
microcontroller units (MCUs) to perform atomic, fail-safe OTA updates with automatic rollback on failure, similar
to Mender's updates for Linux devices.

The client communicates with the Mender server to report device inventory and identity, checks for
available updates, downloads new firmware, and coordinates with the Update Module to install updates safely.
Mender server is available as an open-source version licensed under the Apache-2.0 license, and an enterprise
version licensed under a commercial license, available through commercial plans for on-prem hosting and hosted
Mender, a fully managed service.

mender-mcu is licensed under the Apache-2.0 license.

Requirements
************

* cJSON for JSON parsing

Usage with Zephyr
*****************

To pull in mender-mcu as a Zephyr :ref:`module <modules>`, either add it as a West project in the ``west.yaml``
file or pull it in by adding a submanifest (e.g. ``zephyr/submanifests/mender-mcu.yaml``) file
with the following content and run ``west update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: mender-mcu
         url: https://github.com/mendersoftware/mender-mcu
         revision: main
         path: modules/mender-mcu # adjust the path as needed

For more detailed instructions and API documentation, refer to the `mender-mcu documentation`_.
The `Zephyr reference project`_ provides an example reference integration.

References
**********

.. target-notes::

.. _mender-mcu:
   https://github.com/mendersoftware/mender-mcu

.. _mender-mcu documentation:
   https://docs.mender.io/operating-system-updates-zephyr

.. _Zephyr reference project:
   https://github.com/mendersoftware/mender-mcu-integration
