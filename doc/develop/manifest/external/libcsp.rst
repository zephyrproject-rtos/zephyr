.. _external_module_libcsp:

libcsp (Cubesat Space Protocol)
###############################

Introduction
************

libcsp is an implementation of the CubeSat Space Protocol (CSP). It is a small protocol stack
written in C. CSP is designed to ease communication between distributed embedded systems in small
networks, such as CubeSats.  The design follows the TCP/IP model and includes a transport protocol,
a routing protocol, and several MAC-layer interfaces. The core of libcsp includes a router, a
connection-oriented socket API, and message and connection pools.

Zephyr is used in some Cubesats and they use libcsp to communicates with the other components in the
Cubesats.

libcsp is licensed under the MIT license.


Usage with Zephyr
*****************

To use libcsp with Zephyr, you first need to add the following snipet to your ``west.yaml``:

.. code-block:: yaml

   manifest:
     projects:
       - name: libcsp
         url: https://github.com/libcsp/libcsp
         revision: develop
         path: modules/lib/libcsp


And add this to your ``prj.conf``:

.. code-block:: cfg

     CONFIG_LIBCSP=y

Once you've added libcsp to your project, run ``west update``.

For more detailed instructions and API documentation, refer to the `libcsp documentation`_.


Reference
*********

.. target-notes::

.. _libcsp:
   https://github.com/libcsp/libcsp

.. _libcsp documentation:
   https://libcsp.github.io/libcsp/
