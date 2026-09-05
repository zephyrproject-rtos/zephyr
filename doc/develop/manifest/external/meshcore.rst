.. _external_module_meshcore:

MeshCore
########

Introduction
************

MeshCore is a lightweight, portable C++ library that enables multi-hop packet routing for
embedded projects using LoRa and other packet radios. It lets low-power devices form
decentralized mesh networks for remote communication without relying on internet or cellular
infrastructure, making it well suited to off-grid, emergency-response, outdoor, and IoT/sensor
use cases.

The library implements multi-hop routing with configurable hop limits, a self-healing and
fully decentralized network topology that requires no central server, and low power consumption
suitable for battery- or solar-powered nodes. MeshCore ships several ready-to-use application
roles, including Companion Radio, KISS Modem, Repeater, Room Server, Secure Chat, and Sensor.

For Zephyr-based applications, MeshCore provides a portable networking layer that can be built
on top of Zephyr's LoRa and radio drivers, allowing mesh connectivity to be added to a Zephyr
firmware image without pulling in a full networking stack.

MeshCore supports a range of LoRa-based boards. For the list of supported hardware and more
information, see `MeshCore GitHub`_.

MeshCore is licensed under the MIT License.

Usage with Zephyr
*****************

To pull in MeshCore as a Zephyr module, either add it as a West project in the ``west.yml``
file or pull it in by adding a submanifest (e.g. ``zephyr/submanifests/meshcore.yaml``) with the
following content and run ``west update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: meshcore
         url: https://github.com/meshcore-dev/MeshCore
         revision: bbb58cceb852a9190b46d5f6984e8e5140e6991e
         path: modules/lib/meshcore # adjust path or leave blank for west workspace root directory

Once the module is present in the workspace, ``west update`` checks it out at the pinned
revision. Pin ``revision`` to a specific commit for reproducible builds, or set it to ``main``
to track the latest upstream changes.

Reference
*********

- `MeshCore GitHub`_
- `Zephyr Modules Documentation`_

.. target-notes::

.. _MeshCore GitHub:
   https://github.com/meshcore-dev/MeshCore

.. _Zephyr Modules Documentation:
   https://docs.zephyrproject.org/latest/develop/modules.html
