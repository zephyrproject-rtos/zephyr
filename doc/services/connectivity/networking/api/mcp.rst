.. _mcp_server_interface:

MCP Server
##########

.. contents::
    :local:
    :depth: 2

Overview
********

The `Model Context Protocol`_ (MCP) is an open standard for connecting AI
applications to external data sources and tools. It defines a JSON-RPC-based
communication protocol between MCP clients (AI agents) and MCP servers
(capability providers).

The Zephyr MCP Server library implements the server role of the MCP specification
(version 2025-11-25). It allows networked Zephyr devices to expose tools that AI
agents can discover and invoke over HTTP. The library builds on existing Zephyr
subsystems: the HTTP server library for transport and the JSON library for
serialization.

.. note::

   This library is marked :ref:`experimental <api_lifecycle_experimental>`.
   Only the tool service with text responses is implemented. Additional features,
   like more content types, session management, SSE streaming, authorization and
   other services are planned for future releases.

.. _Model Context Protocol: https://modelcontextprotocol.io/specification/2025-11-25

Architecture
************

.. graphviz::
   :caption: MCP Server layered architecture
   :alt: MCP Server layered architecture diagram

   digraph mcp_arch {
       rankdir=TB;
       node [shape=box, style=filled, fillcolor="#e8e8e8", fontname="sans-serif"];
       edge [arrowsize=0.8];

       app [label="Application\n(tool callbacks)", fillcolor="#cce5ff"];
       core [label="MCP Server Core\n(protocol, registries, workers)"];
       json [label="JSON Processing\n(Zephyr JSON library)"];
       transport [label="Transport Layer\n(HTTP / mock)"];

       app -> core [label="register/respond"];
       core -> json [label="serialize/parse"];
       core -> transport [label="send/receive"];
   }

Application
   Registers tools and implements their callbacks. Tool execution results are
   submitted back to the core via :c:func:`mcp_server_submit_tool_message`.

MCP Server Core
   Implements the MCP protocol state machines. Manages client connections, a
   tool registry, execution tracking, and a configurable worker thread pool.
   A health monitor thread enforces timeouts and triggers cancellations.

JSON Processing
   Serializes outgoing responses and deserializes incoming JSON-RPC requests
   using the Zephyr JSON library.

Transport Layer
   Abstracts the network protocol. The included HTTP transport uses the Zephyr
   HTTP server library. A mock transport is available for unit testing.

HTTP Transport and Asynchronous Responses
=========================================

Zephyr's HTTP server runs in a single thread and processes one request at a
time across all connections. A resource callback must return before the server
can handle the next request from any client. Because tool execution may take
longer than acceptable for blocking the entire server, the MCP HTTP transport
implements a polling-to-SSE fallback:

1. On a POST request, the transport polls internally for a tool response up to
   :kconfig:option:`CONFIG_MCP_HTTP_TIMEOUT_MS` (checked every
   :kconfig:option:`CONFIG_MCP_HTTP_POLL_INTERVAL_MS`).
2. If a response is ready within this window, it is returned directly as
   ``application/json``.
3. If the timeout expires, the transport switches to SSE mode: it returns a
   ``text/event-stream`` response containing only an event ID (no data).
   This signals the client that the response is pending and it should start
   polling with periodic GET requests (interval controlled by
   :kconfig:option:`CONFIG_MCP_HTTP_SSE_RETRY_MS`).
4. The client then issues the periodic GET requests with a ``Last-Event-Id``
   header. If a result is ready, the server sends the response and ends
   the SSE stream. If the result is not ready, the server sends a retry
   response again.

.. note::

   This is not full SSE streaming. The mechanism only delivers deferred
   responses for requests that could not complete within the initial HTTP
   timeout. Server-initiated notifications and streaming tool output are
   not supported in this phase.

Configuration
*************

Enable the library with :kconfig:option:`CONFIG_MCP_SERVER`. The transport is
selected through :kconfig:option:`CONFIG_MCP_TRANSPORT_HTTP` (default) or
:kconfig:option:`CONFIG_MCP_TRANSPORT_MOCK` (testing only).

Minimal ``prj.conf``:

.. code-block:: kconfig

   CONFIG_NETWORKING=y
   CONFIG_NET_TCP=y
   CONFIG_HTTP_SERVER=y
   CONFIG_MCP_SERVER=y
   CONFIG_MCP_TRANSPORT_HTTP=y

Key configuration groups:

Scalability
   :kconfig:option:`CONFIG_MCP_MAX_CLIENTS`,
   :kconfig:option:`CONFIG_MCP_MAX_CLIENT_REQUESTS`,
   :kconfig:option:`CONFIG_MCP_MAX_TOOLS`,
   :kconfig:option:`CONFIG_MCP_REQUEST_WORKERS`

Timeouts
   :kconfig:option:`CONFIG_MCP_TOOL_EXEC_TIMEOUT_MS`,
   :kconfig:option:`CONFIG_MCP_TOOL_IDLE_TIMEOUT_MS`,
   :kconfig:option:`CONFIG_MCP_TOOL_CANCEL_TIMEOUT_MS`,
   :kconfig:option:`CONFIG_MCP_CLIENT_TIMEOUT_MS`
   :kconfig:option:`CONFIG_MCP_HTTP_TIMEOUT_MS`

Memory allocation
   :kconfig:option:`CONFIG_MCP_ALLOC_SLAB` (default) provides deterministic,
   fragmentation-free allocation using pre-allocated slabs.
   :kconfig:option:`CONFIG_MCP_ALLOC_HEAP` uses ``k_malloc``/``k_free`` for
   on-demand allocation at the cost of possible fragmentation.

Usage
*****

Server Setup
============

.. code-block:: c

   #include <zephyr/net/mcp/mcp_server.h>
   #include <zephyr/net/mcp/mcp_server_http.h>

   static mcp_server_ctx_t server;

   int main(void)
   {
       server = mcp_server_init();
       mcp_server_http_init(server);

       /* Register tools here */

       mcp_server_start(server);
       mcp_server_http_start(server);
       return 0;
   }

Tool Registration
=================

.. code-block:: c

   static int my_tool_cb(enum mcp_tool_event_type event,
                         const char *arguments,
                         const char *execution_token)
   {
       if (event == MCP_TOOL_CANCEL_REQUEST) {
           struct mcp_tool_message ack = {
               .type = MCP_USR_TOOL_CANCEL_ACK,
           };

           mcp_server_submit_tool_message(server, &ack, execution_token);

           /* Handle cancellation here */
       }

       struct mcp_tool_message resp = {
           .type = MCP_USR_TOOL_RESPONSE,
           .data = "Tool execution result",
           .length = strlen("Tool execution result"),
           .is_error = false,
       };
       return mcp_server_submit_tool_message(server, &resp, execution_token);
   }

   static const struct mcp_tool_record my_tool = {
       .metadata = {
           .name = "my_tool",
           .input_schema = "{\"type\":\"object\",\"properties\":{}}",
       },
       .callback = my_tool_cb,
   };

   mcp_server_add_tool(server, &my_tool);

The ``.data`` field accepts a plain text string. The server wraps it into an
MCP-compliant ``"text"`` content item automatically. Maximum length is
:kconfig:option:`CONFIG_MCP_TOOL_RESULT_MAX_LEN`.

Tool Callback Patterns
======================

Blocking
   Short-running tools execute directly in the worker thread and call
   :c:func:`mcp_server_submit_tool_message` before returning. The worker
   stack size is :kconfig:option:`CONFIG_MCP_REQUEST_WORKER_STACK_SIZE`.

Asynchronous
   Long-running tools should spawn a dedicated thread, return immediately
   from the callback, and submit the response later using the provided
   execution token. Periodic pings (``MCP_USR_TOOL_PING``) prevent the
   health monitor from cancelling idle executions.

Cancellation
   When the health monitor or a client requests cancellation, the callback
   is invoked with ``MCP_TOOL_CANCEL_REQUEST``. The tool should stop work
   and submit ``MCP_USR_TOOL_CANCEL_ACK``.

Tool Removal
============

Tools can be removed at runtime with :c:func:`mcp_server_remove_tool`. The
call returns ``-EBUSY`` if the tool is currently executing; retry later.

Limitations
***********

The following MCP features are not yet implemented:

- Resources, prompts, sampling, roots, and session management
- Server-initiated notifications and streaming tool output
- Image and embedded-resource content types (only ``"text"`` is supported)
- Full SSE transport (only deferred response delivery is supported)

Testing
*******

Unit tests are available under :zephyr_file:`tests/net/lib/mcp/`. They use
the mock transport (:kconfig:option:`CONFIG_MCP_TRANSPORT_MOCK`) to exercise
protocol logic without a network stack.

Sample
******

See :zephyr:code-sample:`mcp-server-hello-world` for a working example that
registers multiple tools including GPIO-based LED control.

API Reference
*************

.. doxygengroup:: mcp_server
