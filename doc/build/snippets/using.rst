.. _using-snippets:

Using Snippets
##############

.. tip::

   See :ref:`built-in-snippets` for a list of snippets that are provided by
   Zephyr.

Snippets have names. You use snippets by giving their names to the build
system.

With west build
***************

To use a snippet named ``foo`` when building an application ``app``:

.. code-block:: console

   west build -S foo app

To use multiple snippets:

.. code-block:: console

   west build -S snippet1 -S snippet2 [...] app

With cmake
**********

If you are running CMake directly instead of using ``west build``, use the
``SNIPPET`` variable. This is a whitespace- or semicolon-separated list of
snippet names you want to use. For example:

.. code-block:: console

   cmake -Sapp -Bbuild -DSNIPPET="snippet1;snippet2" [...]
   cmake --build build

From application
****************

An application can request snippets directly from ``CMakeLists.txt`` with
the ``SNIPPET_APP`` variable. These snippets are merged with those provided
from the command line (``west build`` or ``cmake -DSNIPPET=...``).

.. code-block:: cmake

   set(SNIPPET_APP "snippet3;snippet4")
