.. _setting_up:

Setting Up for Zephyr Development
#################################

Setting up your development environment for Zephyr is done in two steps:

1. Downloading the code
2. Set up the development environment

Download the Code
*****************

The code is hosted at the Linux Foundation with a Gerrit backend that supports
anonymous cloning via GIT.

Checking Out the Source Code
============================

#. Clone the repository:

    .. code-block:: console

       $ git clone https://gerrit.zephyrproject.org/r/zephyr zephyr-project

You have successfully checked out a copy of the source code to your local
machine.

.. note::
   Once you're ready to start contributing, follow the steps to make yourself
   a Linux Foundation account at :ref:`gerrit_accounts`.

.. important::
   Linux users need to download the Zephyr SDK even after successfully
   cloning the source code. The SDK contains packages that are not part of
   the Zephyr Project. See :ref:`zephyr_sdk` for details.

Set Up the Development Environment
**********************************

The Zephyr project supports these operating systems:

* Linux
* Mac OS

Follow the steps appropriate for your development system's
:abbr:`OS (Operating System)`.

Use the following procedures to create a new development environment. Given
that the file hierarchy might change from one release to another, these
instructions could not work properly in an existing development environment.

Perform the steps in the procedures in the order they appear.

.. toctree::
   :maxdepth: 2

   installation_linux.rst
   installation_mac.rst
   installation_win.rst

.. _Linux Foundation ID website: https://identity.linuxfoundation.org

.. _Gerrit: https://gerrit.zephyrproject.org/
