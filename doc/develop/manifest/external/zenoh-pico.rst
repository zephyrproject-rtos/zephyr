.. _external_module_zenoh_pico:

zenoh-pico
##########

Introduction
************

`zenoh-pico`_ is the `Eclipse Zenoh`_ implementation that targets constrained devices, offering a
native C API. It provides Zero Overhead Pub/sub, Store/Query and Compute capabilities for embedded
systems and microcontrollers.

zenoh-pico unifies data in motion, data at rest, and computations while retaining time and space
efficiency well beyond mainstream stacks. It is fully compatible with the main Rust Zenoh
implementation, providing a lightweight implementation of most functionalities.

zenoh-pico is licensed under the Eclipse Public License 2.0 and Apache License 2.0.

Usage with Zephyr
*****************

The zenoh-pico repository is a Zephyr :ref:`module <modules>` which provides distributed
communication capabilities to Zephyr applications. It supports UDP (unicast and multicast), TCP
transport layers over IPv4, IPv6, and 6LoWPAN networks with WiFi, Ethernet, Thread, and Serial data
link layers.

To pull in zenoh-pico as a Zephyr module, either add it as a West project in the ``west.yaml``
file or pull it in by adding a submanifest (e.g. ``zephyr/submanifests/zenoh-pico.yaml``) file
with the following content and run ``west update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: zenoh-pico
         url: https://github.com/eclipse-zenoh/zenoh-pico.git
         revision: main
         path: modules/lib/zenoh-pico # adjust the path as needed

For more detailed instructions and API documentation, refer to the `zenoh-pico documentation`_ as
well as the provided `Zephyr examples`_.

References
**********

.. target-notes::

.. _zenoh-pico:
   https://github.com/eclipse-zenoh/zenoh-pico

.. _Eclipse Zenoh:
   https://zenoh.io

.. _zenoh-pico documentation:
   https://zenoh-pico.readthedocs.io/en/latest/

.. _Zephyr examples:
   https://github.com/eclipse-zenoh/zenoh-pico/tree/main/examples/zephyr
