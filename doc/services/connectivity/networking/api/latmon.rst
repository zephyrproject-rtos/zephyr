.. _latmon:

Latmon Network Service
######################

.. contents::
    :local:
    :depth: 2

Overview
********

Provides the functionality required for network-based latency monitoring, including socket
management, client-server communication, and data exchange with the Latmus service running
on the System Under Test (SUT).

The Latmon network service is responsible for establishing and managing network
communication between the Latmon application (running on a Zephyr-based board) and
the Latmus service (running on the SUT).

It uses TCP sockets for reliable communication and UDP sockets for broadcasting
the IP address of the Latmon device.

API Reference
*************

.. doxygengroup:: latmon

Features
********

- **Socket Management**: Creates and manages TCP and UDP sockets for communication.
- **Client-Server Communication**: Handles incoming connections from the Latmus service.
- **Data Exchange**: Sends latency metrics and histogram data to the Latmus service.
- **IP Address Broadcasting**: Broadcasts the IP address of the Latmon device to facilitate
  discovery by the Latmus service.
- **Thread-Safe Design**: Uses Zephyr's kernel primitives (e.g., message queues and semaphores) for
  synchronization.

Workflow
********

Socket Creation
===============

The :c:func:`net_latmon_get_socket()` function is called to create and configure a TCP socket to
communicate with the Latmus service. A connection address can be specified as a parameter to
bind the socket to a specific interface and port.

Connection Handling
===================

The :c:func:`net_latmon_connect()` function waits for a connection from the Latmus service.
If no connection is received within the timeout period, the service broadcasts its IP address
using UDP and returns ``-EAGAIN``.
If the broadcast request cannot be sent, the function returns ``-1``, and the client should quit.

Monitoring Start
================

Once a connection is established, the :c:func:`net_latmon_start()` function is called to
start the monitoring process. This function uses a callback to calculate latency deltas
and sends the data to the Latmus service.

Monitoring Status
=================

The :c:func:`net_latmon_running()` function can be used to check if the monitoring process is active.

Thread Management
=================

The service uses Zephyr threads to handle incoming connections and manage the monitoring
process.

Enabling the Latmon Service
***************************

The following configuration option must be enabled in the :file:`prj.conf` file.

- :kconfig:option:`CONFIG_NET_LATMON`

The following options may be configured to customize the Latmon service:

- :kconfig:option:`CONFIG_NET_LATMON_PORT` - Port number for the Latmon service.
- :kconfig:option:`CONFIG_NET_LATMON_XFER_THREAD_STACK_SIZE`
- :kconfig:option:`CONFIG_NET_LATMON_XFER_THREAD_PRIORITY`
- :kconfig:option:`CONFIG_NET_LATMON_THREAD_STACK_SIZE`
- :kconfig:option:`CONFIG_NET_LATMON_THREAD_PRIORITY`
- :kconfig:option:`CONFIG_NET_LATMON_MONITOR_THREAD_STACK_SIZE`
- :kconfig:option:`CONFIG_NET_LATMON_MONITOR_THREAD_PRIORITY`

Example Usage
*************

.. code-block:: c

    #include <zephyr/net/latmon.h>
    #include <zephyr/net/socket.h>

    void main(void)
    {
        struct in_addr ip;
        int server_socket, client_socket;

        /* Create and configure the server socket */
        server_socket = net_latmon_get_socket(NULL);

        while (1) {
            /* Wait for a connection from the Latmus service */
            client_socket = net_latmon_connect(server_socket, &ip);
            if (client_socket < 0) {
                if (client_socket == -EAGAIN) {
                    continue;
                }
                goto out;
            }

            /* Start the latency monitoring process */
            net_latmon_start(client_socket, measure_latency_cycles);
        }
    out:
        close(server_socket);
    }
