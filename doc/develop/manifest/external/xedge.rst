.. _external_module_xedge:

Xedge
#####

Xedge is a secure, embedded web and IoT edge framework designed for
resource-constrained devices and RTOS environments. It is built on the
Barracuda App Server technology and provides a high-level, Lua-based
application environment for developing secure, network-connected
devices.

Xedge enables device manufacturers and embedded developers to
implement modern web-based management interfaces, REST APIs, and
secure IoT services directly on embedded hardware without requiring
external gateways, cloud dependencies, or heavyweight runtimes.

At its core, Xedge combines an embedded web server, application logic,
and security services into a single, tightly integrated runtime that
is optimized for long-lived, production-grade devices.

Key Features
************

* A flexible, Lua-based environment
* A full stack of industrial-strength protocols, including:

  * OPC UA
  * Modbus
  * MQTT
  * SMQ - pub/sub
  * WebSockets
  * HTTP / HTTPS
  * Embedded Web/App Server

Use Cases
*********

Xedge is commonly used in devices that require:

- Secure web-based configuration and diagnostics
- Local dashboards and control panels hosted directly on the device
- REST or WebSocket APIs for integration with external systems
- Secure edge connectivity without relying on cloud services
- Long lifecycle support with minimal maintenance overhead

Architecture Overview
*********************

Xedge runs directly on the target device and exposes application
functionality through standard web technologies. The application logic
is written in Lua and executed inside the embedded runtime, while
security-critical components such as TLS and cryptographic services
are implemented in native code for performance and robustness.

This architecture allows developers to separate application logic from
low-level system concerns, making Xedge well suited for teams with
mixed expertise levels.

Getting Started
***************

For build instructions, refer to the official `Xedge for Zephyr`_ repository.


.. _Xedge for Zephyr:
    https://github.com/RealTimeLogic/Xedge4Zephyr

Additional Resources
********************

- `Real Time Logic`_
- `Xedge Introduction`_
- `Barracuda App Server`_

.. target-notes::

.. _Real Time Logic:
    https://realtimelogic.com

.. _Xedge Introduction:
    https://realtimelogic.com/products/xedge/

.. _Barracuda App Server:
    https://realtimelogic.com/products/barracuda-application-server/
