.. _external_module_requests:

requests
########

Introduction
************

The `requests`_ project provides an easy-to-use interface for performing common
HTTP(S) operations such as GET, POST, PUT and DELETE using Zephyr's networking stack.
It also includes built-in shell commands to interact with HTTP(S) endpoints directly
from the Zephyr shell.

.. code-block:: shell

   uart:~$ requests
   requests - HTTP requests commands
   Subcommands:
     get     : Perform HTTP GET request
               Usage: get <url>
     post    : Perform HTTP POST request
               Usage: post <url> <body>
     put     : Perform HTTP PUT request
               Usage: put <url> <body>
     delete  : Perform HTTP DELETE request
               Usage: delete <url>
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

Refer to the ``requests`` headers for API details.

References
**********

.. target-notes::

.. _requests: https://github.com/walidbadar/requests
