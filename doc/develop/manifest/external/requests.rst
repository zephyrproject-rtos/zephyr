.. _external_module_requests:

requests
########

Introduction
************

The `requests`_ project provides a library that offers a simple interface to perform
common HTTP operations such as GET, POST, PUT and DELETE over Zephyr’s networking stack.
It also includes built-in shell commands to interact with HTTP endpoints directly from
the Zephyr shell.

.. code-block:: shell

   uart:~$ requests
   requests - HTTP requests commands
   Subcommands:
     get     : Perform HTTP GET request: requests get <url>
     post    : Perform HTTP POST request: requests post <url> <body>
     put     : Perform HTTP PUT request: requests put <url> <body>
     delete  : Perform HTTP DELETE request: requests delete <url>
   uart:~$

Usage with Zephyr
*****************

To pull in requests as a Zephyr module, either add it as a West project in the :file:`west.yaml`
file or pull it in by adding a submanifest (e.g. ``zephyr/submanifests/requests.yaml``) file
with the following content and run :command:`west update`:

.. code-block:: yaml

   manifest:
     projects:
       - name: requests
         url: https://github.com/walidbadar/requests.git
         revision: main
         path: modules/lib/requests # adjust the path as needed

Once you've added requests to your project, run ``west update``.

Refer to the ``requests`` headers for API details.

References
**********

.. target-notes::

.. _requests: https://github.com/walidbadar/requests
