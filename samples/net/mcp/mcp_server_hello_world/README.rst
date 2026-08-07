.. zephyr:code-sample:: mcp-server-hello-world
   :name: MCP Server Hello World
   :relevant-api: mcp_server

   This code sample shows two very basic tool definitions to introduce users to the Zephyr MCP
   Server library.

Overview
********

The sample defines two tools: an LED-control tool, which allows MCP clients to turn on, turn off
or toggle an on-board LED, and a delayed response tool, which triggers a delayed response mechanism
to show users how the server behaves in such scenarios.

Once the server connects to a network and receives an IP address, it becomes available either under
the assigned IP or through the mDNS address, like http://mcp-hello-world.local:8080/mcp.

Requirements
************

- A board with networking support
- A board with an LED connected to the ``led0`` alias (optional, for LED control)

Wiring
******

No special wiring requirements.

Building and Running
********************

Build and flash the sample as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/net/mcp/mcp_server_hello_world
   :board: <board_name>
   :goals: build flash
   :compact:

Replace ``<board_name>`` with your target board.

Testing
*******

Once the server is running, it listens for MCP client connections over HTTP.
You can connect using any MCP-compatible client and invoke the registered tools.
The sample is configured to use DHCP and mDNS by default, so the MCP server is
accessible under the URL ``http://mcp-hello-world.local:8080/mcp``.

Sample Output
*************

.. code-block:: console

   Initializing...
   LED initialized successfully
   Registering Tool #1: Delayed response tool...
   Tool #1 registered.
   Registering Tool #2: LED Control...
   Tool #2 registered.
   Starting...
   MCP Server running...

When tools are invoked, you will see output similar to:

.. code-block:: console

   Delayed response tool executed with arguments: {"message":"test"}, token: <uuid>
   LED control tool executed with arguments: {"action":"on"}, token: <uuid>

References
**********

- `Model Context Protocol Specification <https://modelcontextprotocol.io/specification>`_
