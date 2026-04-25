.. _external_module_thrift:

Thrift
######

Introduction
************

`Apache Thrift`_ is an `IDL`_ specification, `RPC`_ framework, and
`code generator`_. It works across all major operating systems, supports over
27 programming languages, 7 protocols, and 6 low-level transports. Thrift was
originally developed at `Facebook in 2006`_ and then shared with the
`Apache Software Foundation`_. Thrift supports a rich set of types and data
structures, and abstracts away transport and protocol details, which lets
developers focus on application logic.

Usage with Zephyr
*****************

To pull in Thrift as a Zephyr module, either add it as a West project in the ``west.yaml``
file or pull it in by adding a submanifest (e.g. ``zephyr/submanifests/thrift.yaml``) file
with the following content and run ``west update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: Thrift
         path: modules/lib/thrift
         revision: zephyr
         url: https://github.com/zephyrproject-rtos/thrift

This module contains sample applications and tests under the ``zephyr/`` directory.

.. target-notes::

.. _Apache Thrift: https://github.com/apache/thrift
.. _IDL: https://en.wikipedia.org/wiki/Interface_description_language
.. _RPC: https://en.wikipedia.org/wiki/Remote_procedure_call
.. _code generator: https://en.wikipedia.org/wiki/Automatic_programming
.. _Facebook in 2006: https://thrift.apache.org/static/files/thrift-20070401.pdf
.. _Apache Software Foundation: https://www.apache.org
