.. zephyr:code-sample:: mcp-server-hello-world
   :name: MCP Server Hello World
   :relevant-api: mcp_server

   Demonstrates a basic MCP (Model Context Protocol) server with HTTP transport.

Overview
********

This sample demonstrates how to create a Model Context Protocol (MCP) server
using Zephyr's MCP subsystem with HTTP transport. The server exposes three
tools that can be invoked by MCP clients:

- **hello_world**: Returns a greeting message after a simulated 12-second workload
- **goodbye_world**: Returns a farewell message immediately
- **led_control**: Controls an LED on the board (on/off/toggle)

The sample showcases key MCP server features including:

- Tool registration with input schemas
- Tool callback handling
- Long-running tool execution with progress pings (using the MCP core worker thread)
- Hint on how to check for cancellations

Requirements
************

- A board with networking support
- A board with an LED connected to the ``led0`` alias (optional, for LED control)

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
accessible under the url ``http://mcp-hello-world.local:8080/mcp``.

Sample Output
*************

.. code-block:: console

   Initializing...
   LED initialized successfully
   Registering Tool #1: Hello world!...
   Tool #1 registered.
   Registering Tool #2: Goodbye world!...
   Tool #2 registered.
   Registering Tool #3: LED Control...
   Tool #3 registered.
   Starting...
   MCP Server running...

When tools are invoked, you will see output similar to:

.. code-block:: console

   Hello World tool executed with arguments: {"message":"test"}, token: <uuid>
   LED control tool executed with arguments: {"action":"on"}, token: <uuid>
