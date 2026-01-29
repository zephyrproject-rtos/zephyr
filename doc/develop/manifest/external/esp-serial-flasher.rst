.. _external_module_esp_serial_flasher:

ESP Serial Flasher
##################

Introduction
************

Short intro into the module, how it relates to Zephyr, and the license/terms of use.

Usage with Zephyr
*****************

To pull in ESP Serial Flasher as a Zephyr module, either add it as a West project in the ``west.yaml``
file or pull it in by adding a submanifest (e.g. ``zephyr/submanifests/esf.yaml``) file
with the following content and run ``west update``:

.. code-block:: yaml
   manifest:
   projects:
     - name: esp-serial-flasher
       url: https://github.com/espressif/esp-serial-flasher
       revision: master
       path: modules/esp-serial-flasher  # adjust the path as needed

.. code-block:: c

   #include <esp-serial-flasher/esp_loader.h>


Reference
*********

External references and links.
