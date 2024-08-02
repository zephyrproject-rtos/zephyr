.. _networking_api:

Networking APIs
###############

Zephyr provides support for the standard BSD socket APIs (defined in
:zephyr_file:`include/zephyr/net/socket.h`) for the applications to
use. See :ref:`BSD socket API <bsd_sockets_interface>` for more details.

Apart of the standard API, Zephyr provides a set of custom networking APIs and
libraries for the application to use. See the list below for details.

.. note::
   The legacy connectivity API in :zephyr_file:`include/zephyr/net/net_context.h`
   should not be used by applications.

.. toctree::
   :maxdepth: 2

   apis.rst
   buf_mgmt.rst
   net_tech.rst
   protocols.rst
   system_mgmt.rst
   tsn.rst
   zperf.rst
