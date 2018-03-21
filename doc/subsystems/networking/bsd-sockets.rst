.. _bsd_sockets_api:

BSD Sockets compatible API
##########################

Zephyr offers an implementation of a subset of the BSD Sockets API (a part
of the POSIX standard). This API allows to reuse existing programming experience
and port existing simple networking applications to Zephyr.

Here are the key requirements and concepts which governed BSD Sockets
compatible API implementation for Zephyr:

* Should have minimal overhead, similar to the requirement for other
  Zephyr subsystems.
* Should be implemented on top of
  :ref:`native networking API <networking_api_usage>` to keep modular
  design.
* Should be namespaced by default, to avoid name conflicts with well-known
  names like ``close()``, which may be part of libc or other POSIX
  compatibility libraries. If enabled, should also expose native POSIX
  names.

BSD Sockets compatible API is enabled using :option:`CONFIG_NET_SOCKETS`
config option and implements the following operations: ``socket()``, ``close()``,
``recv()``, ``recvfrom()``, ``send()``, ``sendto()``, ``connect()``, ``bind()``,
``listen()``, ``fcntl()`` (to set non-blocking mode), ``poll()``.

Based on the namespacing requirements above, these operations are by
default exposed as functions with ``zsock_`` prefix, e.g.
:c:func:`zsock_socket()` and :c:func:`zsock_close()`. If the config option
:option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined, all the functions
will be also exposed as aliases without the prefix. This includes the
functions like ``close()`` and ``fcntl()`` (which may conflict with
functions in libc or other libraries, for example, with the filesystem
libraries).

The native BSD Sockets API uses file descriptors to represent sockets. File descriptors
are small integers, consecutively assigned from zero. Internally, there is usually a table
mapping file descriptors to internal object pointers. For memory efficiency reasons, the
Zephyr BSD Sockets compatible API is devoid of such a table. Instead, ``net_context``
pointers, cast to an int, are used to represent sockets. Thus, socket identifiers aren't
really small integers, so the ``select()`` operation is not available, as it depends on the
"small int" property of file descriptors. Instead of using ``select()`` then, use the ``poll()``
operation, which is generally more efficient.

The BSD Sockets API (and the POSIX API in general) also treat negative file
descriptors values in a special way (such values usually mean an
error). As the Zephyr API uses a pointer value cast to an int for file descriptors, it means
that the pointer should not have the highest bit set, in other words,
pointers should not refer to the second (highest) part of the address space.
For many CPU architectures and SoCs Zephyr supports, user RAM is
located in the lower half, so the above condition is satisfied. If
you face an issue with some SoC because of this, please report it to the Zephyr bug
tracker or mailing list. The decision to use pointers to represent
sockets might be reworked in the future.

The final entailment of the design requirements above is that the Zephyr
API aggressively employs the short-read/short-write property of the POSIX API
whenever possible (to minimize complexity and overheads). POSIX allows
for calls like ``recv()`` and ``send()`` to actually process (receive
or send) less data than requested by the user (on STREAM type sockets).
For example, a call ``recv(sock, 1000, 0)`` may return 100,
meaning that only 100 bytes were read (short read), and the application
needs to retry call(s) to read the remaining 900 bytes.
