.. _bluetooth_connection_mgmt:

Connection Management
#####################

The Zephyr Bluetooth stack uses an abstraction called :c:struct:`bt_conn`
to represent connections to other devices. The internals of this struct
are not exposed to the application, but a limited amount of information
(such as the remote address) can be acquired using the
:c:func:`bt_conn_get_info` API. Connection objects are reference
counted, and the application is expected to use the
:c:func:`bt_conn_ref` API whenever storing a connection pointer for a
longer period of time, since this ensures that the object remains valid
(even if the connection would get disconnected). Similarly the
:c:func:`bt_conn_unref` API is to be used when releasing a reference
to a connection.

An application may track connections by registering a
:c:struct:`bt_conn_cb` struct using the :c:func:`bt_conn_cb_register`
API.  This struct lets the application define callbacks for connection &
disconnection events, as well as other events related to a connection
such as a change in the security level or the connection parameters.
When acting as a central the application will also get hold of the
connection object through the return value of the
:c:func:`bt_conn_create_le` API.

API Reference
*************

.. doxygengroup:: bt_conn
   :project: Zephyr
   :members:
