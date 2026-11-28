.. _net_url_interface:

URL Components
##############

.. contents::
    :local:
    :depth: 2

Overview
********

:c:func:`net_url_parse` splits a URL into its scheme, user information, host, port, path,
query and fragment. The components are offsets into the parsed string, so no copy is made
and the caller decides which components it needs and how much memory to give them.

:c:func:`net_url_field_copy` copies one component into a NUL-terminated buffer, and
:c:func:`net_url_target_copy` copies the request target of an HTTP request, the path and
the query. :c:func:`net_url_scheme_is` compares the scheme without regard to case.

The helpers are enabled with :kconfig:option:`CONFIG_NET_URL`, which selects the parser
they are built on.

API Reference
*************

.. doxygengroup:: net_url
