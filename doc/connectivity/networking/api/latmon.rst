.. _latmon

Latmon Network Service
######################

.. contents::
    :local:
    :depth: 2

Overview
********

Provides the functionality required for network-based latency monitoring, including socket management,
client-server communication, and data exchange with the Latmus service running on the System Under Test (SUT).

Contents
********

1. Overview
2. Features
3. Key Components
4. API Reference
5. Workflow
6. Example Usage

1. Overview
***********

The Latmon network service is responsible for establishing and managing network
communication between the Latmon application (running on a Zephyr-based board) and
the Latmus service (running on the SUT).

It uses TCP sockets for reliable communication and UDP sockets for broadcasting
the IP address of the Latmon device.


2. Features
***********

- **Socket Management**: Creates and manages TCP and UDP sockets for communication.
- **Client-Server Communication**: Handles incoming connections from the Latmus service.
- **Data Exchange**: Sends latency metrics and histogram data to the Latmus service.
- **IP Address Broadcasting**: Broadcasts the IP address of the Latmon device to facilitate discovery by the Latmus service.
- **Thread-Safe Design**: Uses Zephyr's kernel primitives (e.g., message queues and semaphores) for synchronization.


3. Key Components
*****************

3.1 **Socket Management**
The service provides functions to create, bind, and listen on sockets.
It also supports sending and receiving data over TCP and UDP.

3.2 **Message Queue**
A message queue (``latmon_msgq``) is used to pass messages between threads,
ensuring thread-safe communication.

3.3 **Synchronization**
Semaphores (``latmon_done`` and ``monitor_done``) are used to synchronize the monitoring
process and ensure proper cleanup.

3.4 **Thread Management**
The service uses Zephyr threads to handle incoming connections and manage the
monitoring process.


4. API Reference
****************

`int net_latmon_get_socket(void)`
---------------------------------
Creates and configures a TCP socket for the Latmon service.

- **Returns**: A valid socket descriptor on success, or exits the program on failure.


`int net_latmon_connect(int socket, struct in_addr *ip)`
--------------------------------------------------------
Waits for a connection from the Latmus service.

- **Parameters**:
  - `socket`: The socket descriptor to listen on.
  - `ip`:  The client's IP address.

- **Returns**: A valid client socket descriptor on success, or `-1` on failure.


`void net_latmon_start(int client, net_latmon_get_delta_t get_delta_f)`
-----------------------------------------------------------------------
Starts the latency monitoring process.

- **Parameters**:
  - `client`: The client socket descriptor.
  - `get_delta_f`: A callback function that calculates latency deltas.


`bool net_latmon_running(void)`
-------------------------------
Checks if the Latmon monitoring process is currently running.

- **Returns**: `true` if the monitoring process is running, `false` otherwise.


5. Workflow
***********

1. **Socket Creation**:
   - The ``net_latmon_get_socket()`` function is called to create and configure a TCP socket to communicate with the Latmus service.

2. **Connection Handling**:
   - The ``net_latmon_connect()`` function waits for a connection from the Latmus service.
   If no connection is received within the timeout period, the service broadcasts its IP address using UDP.

3. **Monitoring Start**:
   - Once a connection is established, the ``net_latmon_start()`` function is called to
   start the monitoring process. This function uses a callback to calculate latency deltas
   and sends the data to the Latmus service.

4. **Monitoring Status**:
   - The ``net_latmon_running()`` function can be used to check if the monitoring process is active.

5. **Thread Management**:
   - The service uses Zephyr threads to handle incoming connections and manage the monitoring process.

Enabling the Latmon Service
***************************

The following configuration option must me enabled in :file:`prj.conf` file.

- :kconfig:option:`CONFIG_NET_LATMON`


6. Example Usage
****************

### Setting Up the Latmon Service

.. code-block:: c

    #include <zephyr/net/latmon.h>
    #include <zephyr/net/socket.h>

    void main(void)
    {
	struct in_addr ip;
	int server_socket, client_socket;

	/* Create and configure the server socket */
	server_socket = net_latmon_get_socket();

	while (1) {
	    /* Wait for a connection from the Latmus service */
	    client_socket = net_latmon_connect(server_socket, &ip);
	    if (client_socket < 0) {
		continue;
	    }

	    /* Start the latency monitoring process */
	    net_latmon_start(client_socket, measure_latency_cycles);
	}
    }
