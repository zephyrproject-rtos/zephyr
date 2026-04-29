.. _mcp_server:

MCP Server
##########

.. contents::
     :local:
     :depth: 2

Overview
********

The Model Context Protocol (MCP) is an open-source standard for connecting
AI applications to external systems, as defined by https://modelcontextprotocol.io.

MCP enables AI applications like Claude or ChatGPT to connect to:

- **Data sources** (e.g., local files, databases)
- **Tools** (e.g., search engines, calculators, device controls)
- **Workflows** (e.g., specialized prompts)

This allows AI agents to access key information and perform tasks on IoT devices.

The Zephyr MCP Server library brings MCP server functionality to MCU-based devices,
enabling them to expose tools and services to AI agents over standard network protocols.
This opens the door to new AI-driven IoT systems where embedded devices can be
directly controlled and queried by agentic AI models.

Current Implementation Status
==============================

**Phase 1 (Current Release)** includes:

- MCP Protocol Specification version 2025-11-25 compliance (core features)
- HTTP streaming transport with partial SSE support (as workaround for async limitations)
- Full tool support (registration, execution, cancellation)
- Mandatory MCP protocol methods (initialize, ping, tools/list, tools/call)
- Health monitoring with configurable timeouts
- Client lifecycle management
- Execution context tracking with token-based system

**Future Phases** will include:

- MCP Authorization and enhanced security (TLS/DTLS, client authentication)
- Full Server-Sent Events (SSE) support for streaming responses
- Additional MCP services (Resources, Prompts, Sampling, Tasks)
- MCP Session handling
- Additional transport protocols

.. note::
    The current implementation uses SSE as a workaround for HTTP server async limitations.
    Event IDs are tracked at the transport layer but not exposed to tools. SSE is not
    yet used for server-initiated notifications or streaming tool responses.

Architecture
************

The MCP server follows a layered architecture design with clear separation of concerns:

.. code-block:: none

    ┌─────────────────────────────────────────────────┐
    │          Application Layer                      │
    │  - Implements tool functionality                │
    │  - Registers/unregisters tools                  │
    │  - Handles tool callbacks (execution/cancel)    │
    │  - Application-specific business logic          │
    └──────────────────┬──────────────────────────────┘
                       │
    ┌──────────────────▼──────────────────────────────┐
    │          MCP Server Core                        │
    │  - Protocol specification implementation        │
    │  - Client Registry (lifecycle tracking)         │
    │  - Tool Registry (registered tools)             │
    │  - Execution Registry (active executions)       │
    │  - Worker thread pool management                │
    │  - Health monitoring (timeouts/cancellation)    │
    └──────────────────┬──────────────────────────────┘
                       │
    ┌──────────────────▼──────────────────────────────┐
    │          JSON Processing Layer                  │
    │  - Uses Zephyr JSON library                     │
    │  - Request deserialization                      │
    │  - Response/notification serialization          │
    └──────────────────┬──────────────────────────────┘
                       │
    ┌──────────────────▼──────────────────────────────┐
    │          Transport Layer                        │
    │  - Transport abstraction (operation structs)    │
    │  - HTTP transport implementation (included)     │
    │  - Response queueing with event ID ordering     │
    │  - Reference counting for safe cleanup          │
    └─────────────────────────────────────────────────┘

Core Components
===============

MCP Server Core
---------------

The server core (``mcp_server.c``) implements the MCP protocol and manages:

- **Client Registry**: Tracks connected clients and their lifecycle states
- **Tool Registry**: Stores registered tools and metadata
- **Execution Registry**: Monitors active tool executions
- **Worker Thread Pool**: Configurable number of threads for async request processing
  (see :kconfig:option:`CONFIG_MCP_REQUEST_WORKERS`)
- **Health Monitor**: Single thread that detects and handles:

  - Execution timeouts (:kconfig:option:`CONFIG_MCP_TOOL_EXEC_TIMEOUT_MS`)
  - Idle executions (:kconfig:option:`CONFIG_MCP_TOOL_IDLE_TIMEOUT_MS`)
  - Client timeouts (:kconfig:option:`CONFIG_MCP_CLIENT_TIMEOUT_MS`)
  - Cancellation acknowledgment timeouts (:kconfig:option:`CONFIG_MCP_TOOL_CANCEL_TIMEOUT_MS`)
  - Automatically cancels stale executions
  - Sends cancellation notifications to clients

Transport Layer
---------------

The HTTP transport layer (``mcp_server_http.c``) abstracts the underlying protocol through
operation structs:

- **HTTP Transport**: Built on Zephyr HTTP server library
- **Partial SSE Support**: Event ID tracking for request ordering (workaround)
- **Response Queueing**: Min-heap based ordering by event ID
- **Reference Counting**: Ensures safe client cleanup during disconnection
- **Extensible**: New transports can be added without modifying core

JSON Processing
---------------

The JSON layer (``mcp_json.c``) uses Zephyr's JSON library to:

- Deserialize incoming requests
- Serialize responses and notifications
- Support all MCP message types (requests, responses, notifications, errors)
- Handle optional fields based on configuration

Memory Management
-----------------

The MCP library provides configurable memory allocation with two strategies:

**Memory Slabs (Default)**

Selected via :kconfig:option:`CONFIG_MCP_ALLOC_SLAB`:

- Deterministic O(1) allocation time
- Zero fragmentation
- Pre-allocated at startup
- Three size tiers for different allocation needs

**Heap Allocation**

Selected via :kconfig:option:`CONFIG_MCP_ALLOC_HEAP`:

- On-demand allocation via :c:func:`k_malloc` / :c:func:`k_free`
- Lower idle memory usage
- Risk of fragmentation over time

Memory Slab Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~

When using memory slabs, three tiers handle different allocation sizes:

.. list-table:: Memory Slab Tiers
   :header-rows: 1
   :widths: 15 25 25 35

   * - Tier
     - Size Config
     - Count Config
     - Use Case
   * - Small
     - :kconfig:option:`CONFIG_MCP_SLAB_SMALL_SIZE`
     - :kconfig:option:`CONFIG_MCP_SLAB_SMALL_COUNT`
     - Error responses, notification parameters
   * - Medium
     - :kconfig:option:`CONFIG_MCP_SLAB_MEDIUM_SIZE`
     - :kconfig:option:`CONFIG_MCP_SLAB_MEDIUM_COUNT`
     - Parsed messages, response structures
   * - Large
     - :kconfig:option:`CONFIG_MCP_MAX_MESSAGE_SIZE`
     - :kconfig:option:`CONFIG_MCP_SLAB_LARGE_COUNT`
     - JSON buffers, tools list responses

**Default Memory Usage (Slabs):**

.. list-table::
   :header-rows: 1
   :widths: 20 20 20 20

   * - Slab
     - Block Size
     - Count
     - Total
   * - Small
     - 256 B
     - 10
     - 2.5 KB
   * - Medium
     - 1024 B
     - 20
     - 20 KB
   * - Large
     - 2048 B
     - 22
     - 44 KB
   * - **Total**
     -
     -
     - **~66.5 KB**

**Allocation Behavior:**

When a slab tier is exhausted, allocation automatically falls back to the next larger tier:

.. code-block:: none

    Request for 200 bytes:
    1. Try small slab (256 B blocks) → if full, continue
    2. Try medium slab (1024 B blocks) → if full, continue
    3. Try large slab (2048 B blocks) → if full, return NULL

This fallback ensures graceful handling of temporary memory pressure.

**Sizing Recommendations:**

For the large slab (JSON buffers), the recommended count is:

.. code-block:: none

    MCP_SLAB_LARGE_COUNT >= (MCP_MAX_CLIENTS × MCP_MAX_CLIENT_REQUESTS)
                          + MCP_REQUEST_WORKERS
                          + margin

    Default: (3 × 5) + 2 + 5 = 22

This accounts for:

- Responses queued in the transport layer
- Workers actively building responses
- Health monitor cancel notifications and error responses

For the medium slab (parsed messages and response structures):

.. code-block:: none

    MCP_SLAB_MEDIUM_COUNT >= (MCP_MAX_CLIENTS × MCP_MAX_CLIENT_REQUESTS)
                           + margin

    Default: (3 × 5) + 5 = 20

**Comparison:**

.. list-table::
   :header-rows: 1
   :widths: 25 25 25 25

   * - Approach
     - Idle Memory
     - Peak Memory
     - Fragmentation
   * - Slabs (default)
     - ~66.5 KB
     - ~66.5 KB
     - None
   * - Heap
     - ~0
     - ~66.5 KB
     - Possible

Custom Memory Allocators
~~~~~~~~~~~~~~~~~~~~~~~~

Both allocation methods use weak symbols that applications can override:

.. code-block:: c

    void *mcp_alloc(size_t size)
    {
        return my_custom_alloc(size);
    }

    void mcp_free(void *ptr)
    {
        my_custom_free(ptr);
    }

This allows integration with custom memory pools, RTOS-specific allocators,
or debugging wrappers.

.. note::
    When overriding allocators, ensure thread-safety as allocations occur
    from multiple worker threads and the health monitor thread concurrently.

Thread Model
============

The MCP server uses a multi-threaded architecture for concurrent request handling:

Main Application Thread
-----------------------

- Server initialization (:c:func:`mcp_server_init`)
- Transport initialization (:c:func:`mcp_server_http_init`)
- Tool registration/removal
- Server startup (:c:func:`mcp_server_start`)

Transport Threads
-----------------

Transport-specific (for HTTP):

- HTTP server listener threads (Zephyr HTTP server library)
- Receive incoming HTTP requests
- Call ``mcp_server_handle_request()`` to pass data to MCP core
- Queue responses with event ID ordering

Worker Threads
--------------

Configurable pool via :kconfig:option:`CONFIG_MCP_REQUEST_WORKERS`:

- Priority: ``K_PRIO_COOP(7)``
- Stack size: :kconfig:option:`CONFIG_MCP_REQUEST_WORKER_STACK_SIZE`
- Pull requests from shared message queue
- Process MCP protocol requests:

  - ``initialize``: Client initialization handshake
  - ``ping``: Heartbeat responses
  - ``tools/list``: Return available tools
  - ``tools/call``: Execute tool callbacks
  - ``notifications/initialized``: Client state transition
  - ``notifications/cancelled``: Handle cancellation requests

- Execute tool callbacks directly or allow tools to spawn their own threads

Request Queue
-------------

- Depth: :kconfig:option:`CONFIG_MCP_MAX_CLIENTS` * :kconfig:option:`CONFIG_MCP_MAX_CLIENT_REQUESTS`
- Messages include client reference, transport message ID, and parsed data
- FIFO ordering

Health Monitor Thread
---------------------

Single dedicated thread:

- Priority: ``K_PRIO_PREEMPT(6)``
- Stack size: :kconfig:option:`CONFIG_MCP_HEALTH_MONITOR_STACK_SIZE`
- Check interval: :kconfig:option:`CONFIG_MCP_HEALTH_CHECK_INTERVAL_MS`
- Monitors all execution contexts and client contexts
- Sends cancellation notifications on timeout
- Invokes tool cancellation callbacks
- Cleans up idle clients

Tool Callback Execution Context
--------------------------------

Tool callbacks execute in one of two modes:

1. **Blocking Mode**: Short-running tools execute directly in worker thread context

    - Blocks the worker thread until completion
    - Suitable for fast operations
    - Minimizes thread overhead

2. **Async Mode**: Long-running tools spawn their own thread

    - Return immediately from callback
    - Recommended for long-running operations
    - Worker thread returns to pool

.. warning::
    Tool callbacks block MCP worker threads. For operations that take significant time,
    spawn your own thread to avoid degrading server responsiveness. Ensure your thread
    has adequate stack size for the tool's requirements.

.. note::
    The worker thread stack size (:kconfig:option:`CONFIG_MCP_REQUEST_WORKER_STACK_SIZE`) must be
    sufficient for tool callbacks that execute in blocking mode. Increase this value
    if your tools require more stack space, or use async mode with custom thread pools.

Client Lifecycle
================

Clients progress through well-defined states managed by the MCP server core.
Each client is stored in a fixed-size pool (:kconfig:option:`CONFIG_MCP_MAX_CLIENTS`).

Lifecycle States
----------------

Clients transition through the following states:

.. code-block:: none

    DEINITIALIZED ──┐
           ▲        │
           │        │ (allocate on first request)
           │        ▼
           │       NEW
           │        │
           │        │ (initialize request/response)
           │        ▼
           │   INITIALIZING
           │        │
           │        │ (initialized notification)
           │        ▼
           │   INITIALIZED
           │        │
           │        │ (timeout/disconnect)
           │        ▼
           │   DEINITIALIZING
           │        │
           │        │ (refcount reaches 0)
           └────────┘

1. **DEINITIALIZED**: No client allocated in this slot

   - Initial state for all registry entries
   - Entry is available for new client allocation

2. **NEW**: Client allocated, waiting for initialization

   - Created when first request arrives (transport binding established)
   - Only accepts ``initialize`` request
   - Rejects all other requests with ``-EACCES``

3. **INITIALIZING**: Initialize response sent to client

   - Server has sent initialization response with capabilities
   - Waiting for client to confirm with ``initialized`` notification
   - Intermediate state to prevent race conditions

4. **INITIALIZED**: Client confirmed initialization

   - Ready for all MCP operations (ping, tools/list, tools/call)
   - Can have up to :kconfig:option:`CONFIG_MCP_MAX_CLIENT_REQUESTS` concurrent requests
   - Tracks active executions in ``active_requests[]`` array

5. **DEINITIALIZING**: Client in the clean-up process

   - Client is disconnected and not intended for further usage
   - Once all active references are closed, client gets cleaned up

State Transitions
-----------------

Valid transitions:

- ``NEW → INITIALIZING``: On ``initialize`` request received and response sent
- ``INITIALIZING → INITIALIZED``: On ``initialized`` notification received
- ``[Any State] → DEINITIALIZED``: On client timeout or disconnect

Invalid transitions result in ``-EPERM`` or ``-EACCES`` errors.

Reference Counting
------------------

Each client context uses atomic reference counting for safe cleanup:

- **Initial reference**: Created when client is allocated (refcount = 1)
- **Additional references**: Acquired when:

  - Message is queued for worker thread processing
  - Execution context is created for tool call
  - Health monitor is checking client state

- **Reference release**: When message processing completes or execution finishes
- **Cleanup**: When refcount reaches 0, client is fully removed and slot is freed

This mechanism ensures:

- No use-after-free when clients disconnect during request processing
- Transport can safely queue responses until client is fully cleaned up
- Worker threads can safely access client context during async operations

.. important::
   The MCP core marks clients as ``DEINITIALIZING`` immediately on disconnect/timeout,
   preventing new references. Actual cleanup happens when the last reference is released.

Active Request Tracking
-----------------------

Each client tracks active tool executions:

- Array: ``active_requests[CONFIG_MCP_MAX_CLIENT_REQUESTS]``
- Pointers to execution contexts in the execution registry
- Incremented on ``tools/call`` request
- Decremented when tool submits response or acknowledges cancellation
- Used to enforce per-client concurrency limits

Client Health Monitoring
-------------------------

The health monitor thread checks each client:

- **Last message timestamp**: Updated on any request/response
- **Idle timeout**: :kconfig:option:`CONFIG_MCP_CLIENT_TIMEOUT_MS`
- **Action on timeout**: Client marked as disconnected, all executions cancelled

Tool Execution Flow
===================

Each tool execution is tracked through its lifecycle with a unique execution token.

Execution States
----------------

Tool executions progress through these states:

.. code-block:: none

    FREE ────┐
      ▲      │
      │      │ (tools/call request)
      │      ▼
      │   ACTIVE ◄──────┐
      │      │          │ (ping messages)
      │      │          │
      │      ├──────────┘
      │      │
      │      ├───► CANCELED ──────┐
      │      │    (timeout or     │
      │      │     client request)│
      │      │                    │
      │      │ (tool response)    │ (cancel ack)
      │      ▼                    ▼
      └─── FINISHED ◄─────────────┘

1. **FREE**: Execution slot available

   - Default state in execution registry
   - Can be allocated for new tool call

2. **ACTIVE**: Tool execution in progress

   - Created when ``tools/call`` request is processed
   - Execution token assigned
   - Tool callback is invoked
   - Monitored for execution timeout and idle timeout

3. **CANCELED**: Cancellation requested

   - Triggered by client ``cancelled`` notification OR health monitor timeout
   - Tool receives :c:enumerator:`MCP_TOOL_CANCEL_REQUEST` event
   - Cancellation timestamp recorded
   - Monitored for cancellation acknowledgment timeout

4. **FINISHED**: Execution completed

   - Tool submitted response (:c:enumerator:`MCP_USR_TOOL_RESPONSE`)
   - OR tool acknowledged cancellation (:c:enumerator:`MCP_USR_TOOL_CANCEL_ACK`)
   - Execution context cleaned up and slot freed

Complete Execution Flow
-----------------------

Normal execution path:

1. **Client Request Arrival**:

   - Transport layer receives HTTP request with JSON-RPC message
   - Transport validates headers, creates ``mcp_transport_message``
   - Calls ``mcp_server_handle_request()``
   - Waits for up to :kconfig:option:`CONFIG_MCP_HTTP_TIMEOUT_MS` ms for a tool execution to
     complete and after timeout switches communication to SSE and expects client to send GET
     requests periodically until a response is ready

2. **Request Parsing and Validation**:

   - JSON message parsed into ``struct mcp_message``
   - Method extracted and validated
   - Client context looked up by transport binding
   - Client state validated (must be ``INITIALIZED`` for tool calls)

3. **Request Queueing**:

   - For ``tools/call``: Message queued for worker thread
   - Client reference count incremented
   - ``initialize`` requests handled immediately (special case)

4. **Worker Processing**:

   - Worker thread dequeues request from message queue
   - For ``tools/call``:

     - Tool looked up in tool registry by name
     - Tool refcount incremented (prevents removal during execution)
     - Execution context created with unique token
     - Client's ``active_request_count`` incremented
     - Execution context added to client's ``active_requests[]`` array

5. **Tool Callback Invocation**:

   - Worker calls ``tool->callback(MCP_TOOL_CALL_REQUEST, params, token)``
   - Tool can either:

     - **Block**: Process and call :c:func:`mcp_server_submit_tool_message` before returning
     - **Async**: Spawn thread, return immediately, thread calls :c:func:`mcp_server_submit_tool_message` later

6. **Tool Response Submission**:

   - Tool calls :c:func:`mcp_server_submit_tool_message` with :c:enumerator:`MCP_USR_TOOL_RESPONSE`
   - Execution context looked up by token
   - If execution was canceled, response is dropped with warning
   - Response serialized to JSON
   - Sent via transport layer's ``send()`` operation

7. **Cleanup**:

   - Execution state set to ``FINISHED``
   - Removed from client's ``active_requests[]`` array
   - Client's ``active_request_count`` decremented
   - Tool refcount decremented
   - Execution context freed and returned to pool
   - Client reference released

Cancellation Flow
-----------------

Cancellation can be triggered by:

1. **Client Request**: ``cancelled`` notification with request ID
2. **Execution Timeout**: Health monitor detects :kconfig:option:`CONFIG_MCP_TOOL_EXEC_TIMEOUT_MS` exceeded
3. **Idle Timeout**: Health monitor detects :kconfig:option:`CONFIG_MCP_TOOL_IDLE_TIMEOUT_MS` exceeded

Cancellation process:

1. Execution state changed to ``CANCELED``
2. Cancellation timestamp recorded
3. Tool callback invoked with :c:enumerator:`MCP_TOOL_CANCEL_REQUEST` event
4. Tool should:

   - Stop processing gracefully
   - Submit :c:enumerator:`MCP_USR_TOOL_CANCEL_ACK` message
   - Clean up any resources

5. If tool doesn't acknowledge within :kconfig:option:`CONFIG_MCP_TOOL_CANCEL_TIMEOUT_MS`:

   - Health monitor logs error
   - Execution remains in ``CANCELED`` state
   - Cleanup happens when tool finally responds or server restarts

Execution Token System
----------------------

Each tool execution receives a UUID-based token for tracking:

  - Correlates responses with original requests
  - Allows tools to submit responses from any thread
  - Enables execution state queries
  - Prevents response spoofing from malicious tools

Execution Monitoring
--------------------

The health monitor thread continuously checks all active executions:

**Execution Timeout Check**:

.. code-block:: c

  if (uptime - start_timestamp > CONFIG_MCP_TOOL_EXEC_TIMEOUT_MS) {
      /* Send cancellation notification to client */
      /* Invoke tool's cancel callback */
      /* Transition to CANCELED state */
  }

  if (uptime - last_message_timestamp > CONFIG_MCP_TOOL_IDLE_TIMEOUT_MS) {
      /* Send cancellation notification to client */
      /* Invoke tool's cancel callback */
      /* Transition to CANCELED state */
  }

Tools can prevent idle timeout by sending ping messages:

.. code-block:: c

   struct mcp_tool_message ping = {
      .type = MCP_USR_TOOL_PING,
      .data = NULL,
      .length = 0
   };

   mcp_server_submit_tool_message(server, &ping, execution_token);

.. note::
  Ping messages update last_message_timestamp but do not send any data to the client.
  They are purely for keeping the execution alive during long-running operations.

Usage Guide
***********

Server Initialization
=====================

Initialize and start the MCP server:

.. code-block:: c

    #include <zephyr/net/mcp/mcp_server.h>
    #include <zephyr/net/mcp/mcp_server_http.h>

    mcp_server_ctx_t server;

    int main(void)
    {
        int ret;

        /* Initialize MCP server core */
        server = mcp_server_init();
        if (server == NULL) {
            return -ENOMEM;
        }

        /* Initialize HTTP transport layer */
        ret = mcp_server_http_init(server);
        if (ret != 0) {
            return ret;
        }

        /* Register tools (see Tool Registration section) */
        ret = mcp_server_add_tool(server, &my_tool);
        if (ret != 0) {
            return ret;
        }

        /* Start worker threads and health monitor */
        ret = mcp_server_start(server);
        if (ret != 0) {
            return ret;
        }

        /* Start HTTP listener */
        ret = mcp_server_http_start(server);
        if (ret != 0) {
            return ret;
        }

        return 0;
    }

Tool Registration
=================

Define a tool and register it with the server:

.. code-block:: c

    static int my_tool_callback(enum mcp_tool_event_type event,
                                const char *params,
                                const char *execution_token)
    {
        if (event == MCP_TOOL_CANCEL_REQUEST) {
            /* Handle cancellation */
            struct mcp_tool_message ack = {
                  .type = MCP_USR_TOOL_CANCEL_ACK,
                  .data = NULL,
                  .length = 0
            };
            mcp_server_submit_tool_message(server, &ack, execution_token);
            return 0;
        }

        /* Process tool request and generate response */
        char *result = "{\"type\":\"text\",\"text\":\"Result data\"}";
        struct mcp_tool_message response = {
            .type = MCP_USR_TOOL_RESPONSE,
            .data = result,
            .length = strlen(result),
            .is_error = false
        };

        return mcp_server_submit_tool_message(server, &response, execution_token);
    }

    static const struct mcp_tool_record my_tool = {
        .metadata = {
            .name = "my_tool",
            .input_schema = "{\"type\":\"object\",\"properties\":{}}",
    #ifdef CONFIG_MCP_TOOL_DESC
            .description = "Tool description for AI agents",
    #endif
        },
        .callback = my_tool_callback
    };

    /* Register the tool */
    ret = mcp_server_add_tool(server, &my_tool);

.. note::
    Tool callbacks are executed in worker thread context. For long-running
    operations, spawn a separate thread to avoid blocking workers.

Tool Removal
============

Remove a tool dynamically:

.. code-block:: c

    int ret = mcp_server_remove_tool(server, "my_tool");
    if (ret == -EBUSY) {
        /* Tool is currently executing, retry later */
    } else if (ret == -ENOENT) {
        /* Tool not found */
    }

.. warning::
    Tools cannot be removed while actively executing. Check for ``-EBUSY`` and retry.

Submitting Tool Responses
==========================

Tools must submit exactly one response per execution:

**Success Response:**

.. code-block:: c

    struct mcp_tool_message msg = {
        .type = MCP_USR_TOOL_RESPONSE,
        .data = "{\"type\":\"text\",\"text\":\"Success\"}",
        .length = strlen("{\"type\":\"text\",\"text\":\"Success\"}"),
        .is_error = false
    };
    mcp_server_submit_tool_message(server, &msg, execution_token);

**Error Response:**

.. code-block:: c

    struct mcp_tool_message msg = {
        .type = MCP_USR_TOOL_RESPONSE,
        .data = "{\"type\":\"text\",\"text\":\"Error occurred\"}",
        .length = strlen("{\"type\":\"text\",\"text\":\"Error occurred\"}"),
        .is_error = true
    };
    mcp_server_submit_tool_message(server, &msg, execution_token);

**Cancellation Acknowledgment:**

.. code-block:: c

    struct mcp_tool_message ack = {
         .type = MCP_USR_TOOL_CANCEL_ACK,
         .data = NULL,
         .length = 0
    };
    mcp_server_submit_tool_message(server, &ack, execution_token);

Long-Running Operations
=======================

For operations exceeding :kconfig:option:`CONFIG_MCP_TOOL_IDLE_TIMEOUT_MS`, send periodic pings:

.. code-block:: c

    /* During long operation */
    struct mcp_tool_message ping = {
			.type = MCP_USR_TOOL_PING,
			.data = NULL,
			.length = 0
    };
    mcp_server_submit_tool_message(server, &ping, execution_token);

.. tip::
    Ping messages update the idle timer without sending data to the client.
    They prevent automatic cancellation during long computations.

Checking Execution State
=========================

Query whether an execution has been cancelled:

.. code-block:: c

    bool is_canceled;
    int ret;

    ret = mcp_server_is_execution_canceled(server, execution_token, &is_canceled);
    if (ret == 0 && is_canceled) {
        /* Stop processing and cleanup */
    }

.. note::
    Prefer handling :c:enumerator:`MCP_TOOL_CANCEL_REQUEST` events over polling this API.
    Events are more efficient and don't require mutex operations.

Response Format
===============

Tool responses must provide plain text data that the MCP server will
wrap in the appropriate content structure.

**Important:** The ``.data`` field in :c:struct:`mcp_tool_message` should contain
the actual response text, NOT a JSON-encoded content object. The server
automatically wraps your text in the MCP content format.

**Correct Usage:**

.. code-block:: c

   static int my_tool_callback(enum mcp_tool_event_type event,
                               const char *arguments,
                               const char *execution_token)
   {
       /* Your tool logic here */

       /* Provide plain text response */
       const char *result_text = "Temperature is 23.5°C";

       struct mcp_tool_message response = {
           .type = MCP_USR_TOOL_RESPONSE,
           .data = result_text,
           .length = strlen(result_text),
           .is_error = false
       };

       return mcp_server_submit_tool_message(server, &response, execution_token);
   }

**What the Server Sends to Client:**

The server automatically wraps your text in the MCP content structure:

.. code-block:: json

   {
       "jsonrpc": "2.0",
       "id": 1,
       "result": {
           "content": [
               {
                   "type": "text",
                   "text": "Temperature is 23.5°C"
               }
           ],
           "isError": false
       }
   }

**Returning Structured Data:**

If you need to return structured data (JSON), encode it as a string:

.. code-block:: c

   static int sensor_tool_callback(enum mcp_tool_event_type event,
                                   const char *arguments,
                                   const char *execution_token)
   {
       char response_buffer[256];

       /* Build JSON response as a string */
       snprintf(response_buffer, sizeof(response_buffer),
                "{\"temperature\":23.5,\"humidity\":45.2,\"unit\":\"C\"}");

       struct mcp_tool_message response = {
           .type = MCP_USR_TOOL_RESPONSE,
           .data = response_buffer,
           .length = strlen(response_buffer),
           .is_error = false
       };

       return mcp_server_submit_tool_message(server, &response, execution_token);
   }

**Client Will Receive:**

.. code-block:: json

   {
       "jsonrpc": "2.0",
       "id": 1,
       "result": {
           "content": [
               {
                   "type": "text",
                   "text": "{\"temperature\":23.5,\"humidity\":45.2,\"unit\":\"C\"}"
               }
           ],
           "isError": false
       }
   }

The AI agent can then parse the JSON string from the ``text`` field.

**Error Responses:**

To return an error, set the ``.is_error`` flag:

.. code-block:: c

   struct mcp_tool_message error_response = {
       .type = MCP_USR_TOOL_RESPONSE,
       .data = "Sensor not responding",
       .length = strlen("Sensor not responding"),
       .is_error = true
   };

   mcp_server_submit_tool_message(server, &error_response, execution_token);

**Client Receives:**

.. code-block:: json

   {
       "jsonrpc": "2.0",
       "id": 1,
       "result": {
           "content": [
               {
                   "type": "text",
                   "text": "Sensor not responding"
               }
           ],
           "isError": true
       }
   }

**Maximum Response Length:**

Response text is limited by :kconfig:option:`CONFIG_MCP_TOOL_RESULT_MAX_LEN`.

For larger responses, increase this configuration option in your ``prj.conf``.

Custom Memory Allocators
========================

Override default allocator by redefining weak symbols:

.. code-block:: c

    void *mcp_alloc(size_t size)
    {
        return my_custom_alloc(size);
    }

    void mcp_free(void *ptr)
    {
        my_custom_free(ptr);
    }

Sample Application
******************

For application examples see:

:zephyr:code-sample:`mcp-server-hello-world`

See Also
********

.. doxygengroup:: mcp_server

- :ref:`networking_api` - Zephyr networking APIs
- https://modelcontextprotocol.io - MCP specification
