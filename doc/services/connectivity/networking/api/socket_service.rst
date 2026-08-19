.. _socket_service_interface:

Socket Services
###############

.. contents::
    :local:
    :depth: 2

Overview
********

The socket service API can be used to install a handler that is called if there
is data received on a socket. The API helps to avoid creating a dedicated thread
for each TCP or UDP service that the application provides. Instead one thread
is created that serves data to multiple listening sockets which saves memory
as only one thread needs to be created in the system in this case.

See :zephyr:code-sample:`sockets-service-echo` sample application to learn how
to create a simple BSD socket based server application using the sockets service API.
The source code for this sample application can be found at:
:zephyr_file:`samples/net/sockets/echo_service`.

API Description
***************

Socket service API is enabled using :kconfig:option:`CONFIG_NET_SOCKETS_SERVICE`
config option and implements the following operations:

* :c:macro:`NET_SOCKET_SERVICE_SYNC_DEFINE`

  Define a network socket service. This socket service is created with extern
  scope so it can be used from multiple C source files.

* :c:macro:`NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC`

  Define a network socket service with static scope. This socket service can only
  be used within one C source file.

* :c:func:`net_socket_service_register`

  Register pollable sockets for this service. User must create the sockets
  before this call.

* :c:func:`net_socket_service_unregister`

  Remove pollable sockets for this service. User can close the sockets after
  this call.

* :c:type:`net_socket_service_handler_t`

  User specified callback which is called when data is received on the
  listening socket.

Application Overview
********************

If the socket service API is enabled, an application must create the service like
this:

.. code-block:: c

   #define MAX_BUF_LEN 1500
   #define MAX_SERVICES 1

   static void udp_service_handler(struct net_socket_service_event *pev)
   {
	struct pollfd *pfd = &pev->event;
	int client = pfd->fd;
	struct sockaddr_in6 addr;
	socklen_t addrlen = sizeof(addr);

	/* In this example we use one static buffer in order to avoid
	 * having a large stack.
	 */
	static char buf[MAX_BUF_LEN];

	len = recvfrom(client, buf, sizeof(buf), 0,
		       (struct sockaddr *)&addr, &addrlen);
	if (len <= 0) {
		/* Error */
		...
		return;
	}

	/* Do something with the received data. The pev variable contains
	 * user data that was stored in the socket service when it was
	 * registered.
	 */
   }

   NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(service_udp, udp_service_handler, MAX_SERVICES);

The application needs to create the sockets, then register them to the socket
service after which the socket service thread will start to call the callback
for any incoming data.

.. code-block:: c

   /* Create one or multiple sockets */

   struct pollfd sockfd_udp[1] = { 0 };
   int sock, ret;

   sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
   if (sock < 0) {
	LOG_ERR("socket: %d", -errno);
	return -errno;
   }

   /* Set possible socket options after creation */
   ...

   /* Then bind the socket to local address */
   if (bind(sock, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
	LOG_ERR("bind: %d", -errno);
	return -errno;
   }

   /* Set the polled sockets */
   sockfd_udp[0].fd = sock;
   sockfd_udp[0].events = POLLIN;

   /* Register UDP socket to service handler */
   ret = net_socket_service_register(&service_udp, sockfd_udp,
				     ARRAY_SIZE(sockfd_udp), NULL);
   if (ret < 0) {
	LOG_ERR("Cannot register socket service handler (%d)", ret);
	return ret;
   }

   /* Application logic happens here. When application is ready to
    * quit, one should unregister the socket service and close the
    * socket.
    */

   (void)net_socket_service_unregister(&service_udp);
   close(sock);

The TCP socket needs slightly different logic as we need to add any
accepted socket to the listening socket by calling the
:c:func:`net_socket_service_register` for the accepted socket.

.. code-block:: c

   struct sockaddr_in6 client_addr;
   socklen_t client_addr_len = sizeof(client_addr);
   struct pollfd sockfd_tcp[1] = { 0 };
   int client;

   /* TCP socket service is created similar way as the UDP one */
   sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
   if (sock < 0) {
	LOG_ERR("socket: %d", -errno);
	return -errno;
   }

   if (bind(sock, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
	LOG_ERR("bind: %d", -errno);
	return -errno;
   }

   if (listen(sock, 5) < 0) {
	LOG_ERR("listen: %d", -errno);
	return -errno;
   }

   while (1) {
	client = accept(tcp_sock, (struct sockaddr *)&client_addr,
			&client_addr_len);
	if (client < 0) {
		LOG_ERR("accept: %d", -errno);
		continue;
	}

	inet_ntop(client_addr.sin6_family, &client_addr.sin6_addr,
		  addr_str, sizeof(addr_str));
	LOG_INF("Connection from %s (%d)", addr_str, client);

	sockfd_tcp[0].fd = client;
	sockfd_tcp[0].events = POLLIN;

	/* Register all the sockets to service handler */
	ret = net_socket_service_register(&service_tcp, sockfd_tcp,
					  ARRAY_SIZE(sockfd_tcp), NULL);
	if (ret < 0) {
		LOG_ERR("Cannot register socket service handler (%d)", ret);
		break;
	}
   }

For any closed TCP client connection we need to remove the closed
socket from the polled socket list.

.. code-block:: c

   /* If the TCP socket is closed while reading the data in the handler,
    * mark it as non pollable.
    */
   if (sockfd_tcp[0].fd == client) {
	sockfd_tcp[0].fd = -1;

	/* Update the handler so that client connection is
	 * not monitored any more.
	 */
	 (void)net_socket_service_register(&service_tcp, sockfd_tcp,
					   ARRAY_SIZE(sockfd_tcp), NULL);
	 close(client);

	 LOG_INF("Connection from %s closed", addr_str);
   }

Please see a more complete example in the ``echo_service`` sample source
code at :zephyr_file:`samples/net/sockets/echo_service/src/main.c`.

API Reference
*************

.. doxygengroup:: bsd_socket_service
