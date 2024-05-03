Zephyr HTTP Server
==================

Overview
--------

This sample application demonstrates the use of the ``http_server`` library.
This library provides high-level functions to simplify and abstract server implementation.
The server supports the HTTP/1.1 protocol which can also be upgraded to HTTP/2,
it also support native HTTP/2 protocol without upgrading.

Requirement
-----------

`QEMU Networking <https://docs.zephyrproject.org/latest/connectivity/networking/qemu_setup.html#networking-with-qemu>`_

Building and running the server
-------------------------------

To build and run the application:

.. code-block:: bash

   $ west build -p auto -b <board_to_use> -t run samples/net/sockets/http_server

When the server is up, we can make requests to the server using either HTTP/1.1 or
HTTP/2 protocol from the host machine.

**With HTTP/1.1:**

- Using a browser: ``http://192.0.2.1/``
- Using curl: ``curl -v --compressed http://192.0.2.1/``
- Using ab (Apache Bench): ``ab -n10 http://192.0.2.1/``

**With HTTP/2:**

- Using nghttp client: ``nghttp -v --no-dep http://192.0.2.1/``
- Using curl: ``curl --http2 -v --compressed http://192.0.2.1/``
- Using h2load: ``h2load -n10 http://192.0.2.1/``

Server Customization
---------------------

The server sample contains several parameters that can be customized based on
the requirements. These are the configurable parameters:

- ``CONFIG_NET_SAMPLE_HTTP_SERVER_SERVICE_PORT``: Configures the service port.

- ``CONFIG_HTTP_SERVER_MAX_CLIENTS``: Defines the maximum number of HTTP/2
  clients that the server can handle simultaneously.

- ``CONFIG_HTTP_SERVER_MAX_STREAMS``: Specifies the maximum number of HTTP/2
  streams that can be established per client.

- ``CONFIG_HTTP_SERVER_CLIENT_BUFFER_SIZE``: Defines the buffer size allocated
  for each client. This limits the maximum length of an individual HTTP header
  supported.

- ``CONFIG_HTTP_SERVER_MAX_URL_LENGTH``: Specifies the maximum length of an HTTP
  URL that the server can process.

- ``CONFIG_NET_SAMPLE_WEBSOCKET_SERVICE``: Enables Websocket service endpoint.
  This allows a Websocket client to connect to ``/`` endpoint, all the data that
  the client sends is echoed back.

To customize these options, we can run ``west build -t menuconfig``, which provides
us with an interactive configuration interface. Then we could navigate from the top-level
menu to: ``-> Subsystems and OS Services -> Networking -> Network Protocols``.

Websocket Connectivity
----------------------

You can use a simple Websocket client application like this to test the Websocket
connectivity.

.. code-block:: python

   import websocket

   websocket.enableTrace(True)
   ws = websocket.WebSocket()
   ws.connect("ws://192.0.2.1/")
   ws.send("Hello, Server")
   print(ws.recv())
   while True:
     line = input("> ")
     if line == "quit":
       break
     ws.send(line)
     print(ws.recv())
   ws.close()


Performance Analysis
--------------------

CPU Usage Profiling
*******************

We can use ``perf`` to collect statistics about the CPU usage of our server
running in native_sim board with the ``stat`` command:

.. code-block:: bash

   $ sudo perf stat -p <pid_of_server>

``perf stat`` will then start monitoring our server. We can let it run while
sending requests to our server. Once we've collected enough data, we can
stop ``perf stat``, which will print a summary of the performance statistics.

Hotspot Analysis
****************

``perf record`` and ``perf report`` can be used together to identify the
functions in our code that consume the most CPU time:

.. code-block:: bash

   $ sudo perf record -g -p <pid_of_server> -o perf.data

After running our server under load (For example, using ApacheBench tool),
we can stop the recording and analyze the data using:

.. code-block:: bash

   $ sudo perf report -i perf.data

After generating a file named ``perf.data`` which contains the profiling data,
we can visualize it using ``FlameGraph`` tool. It's particularly useful for
identifying the most expensive code-paths and inspect where our application is
spending the most time.

To do this, we need to convert the ``perf.data`` to a format that ``FlameGraph``
can understand:

.. code-block:: bash

   $ sudo perf script | ~/FlameGraph/stackcollapse-perf.pl > out.perf-folded

And, then, generate the ``FlameGraph``:

.. code-block:: bash

   $ ~/FlameGraph/flamegraph.pl out.perf-folded > flamegraph.svg

We can view flamegraph.svg using a web browser.
