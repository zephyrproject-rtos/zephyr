.. _external_module_xedge:

Xedge
#####

Introduction
************

`Xedge`_ is a secure, embedded web and IoT edge framework designed for
resource-constrained devices and RTOS environments. It is built on the
Barracuda App Server technology and provides a high-level, Lua-based
application environment for developing secure, network-connected devices.

Xedge is licensed under the GPLv2 license with a commercial license option.

Usage with Zephyr
*****************

The Xedge framework is a Zephyr :ref:`module <modules>` that enables developers to implement
web-based management interfaces, REST APIs, and secure IoT services directly on embedded
hardware.

To pull in Xedge as a Zephyr module, add a submanifest (e.g.
``zephyr/submanifests/xedge.yaml``) file with the following content and run ``west update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: xedge
         url: https://github.com/RealTimeLogic/Xedge4Zephyr.git
         revision: main
         path: modules/Xedge4Zephyr

For detailed build instructions, supported features, and examples, please refer to the
`Xedge for Zephyr GitHub Repository`_.

References
**********

.. target-notes::

.. _Xedge:
.. _Xedge Introduction:
   https://realtimelogic.com/products/xedge/

.. _Xedge for Zephyr GitHub Repository:
   https://github.com/RealTimeLogic/Xedge4Zephyr

.. _Barracuda App Server:
   https://realtimelogic.com/products/barracuda-application-server/
