:orphan:

Welcome to Zephyr Kernel
########################

.. This document is in Restructured Text Format.
   Find more information regarding the ReST markup in the
   `ReST documentation`_.
   This is a comment that won't show up in formatted output

Welcome to the Zephyr Project.

Thank you for your interest in the Zephyr Project. These instructions are
designed to walk you through generating the Zephyr Project's documentation.

Documentation Notes
*******************

Zephyr Project content is written using the reStructuredText markup language
(.rst file extension) with Sphinx extensions, and processed using sphinx to
create a formatted stand-alone website. Developers can view this content either
in its raw form as .rst markup files, or you can generate the HTML content and view it
with a web browser directly on your workstations drive. This same .rst
content is also fed into the Zephyr Project's public website documentation area
(with a different theme applied).

You can read details about reStructuredText and about Sphinx extensions from
their respective websites.

The project's documentation currently comprises the following items:

* ReStructuredText source files used to generate documentation found at
  https://zephyrproject.org/doc website. Most of the reStructuredText sources
  are found in the ``/doc`` directory, but there are others stored within the
  code source tree near their specific component (such as ``/samples`` and
  ``/boards``)

* Doxygen-generated material used to create all API-specific documents
  also found at https://zephyrproject.org/doc

* Script-generated material for kernel configuration options based on kconfig
  files found in the source code tree

The reStructuredText files are processed by the Sphinx documentation system,
and make use of the breathe extension for including the doxygen-generated API
material.  Additional tools are required to generate the
documentation locally, as described in the following sections.

Installing the documentation processors
***************************************

Our documentation processing has been tested to run with:

* Doxygen version 1.8.10 (and 1.8.11)
* Sphinx version 1.4.4 (but not with 1.5.1)
* Breathe version 4.4.0
* docutils version 0.12 (0.13 has issues with Sphinx 1.4.4)

Begin by cloning a copy of the git repository for the zephyr project and
setting up your development environment as described in :ref:`getting_started`
or specifically for Ubuntu in :ref:`installation_linux`.  (Be sure to
export the environment variables ``ZEPHYR_GCC_VARIANT`` and
``ZEPHYR_SDK_INSTALL_DIR`` as documented there.)

Here are a set of commands to install the documentation generations tools on
Ubuntu:

.. code-block:: bash

   $ sudo -E apt-get install python-pip
   $ pip install --upgrade pip
   $ sudo -E apt-get install doxygen
   $ pip install sphinx==1.4.4
   $ sudo -HE pip install breathe
   $ sudo -HE pip install sphinx-rtd-theme

There is a known issue that causes docutils version 0.13 to fail with sphinx
1.4.4.  Verify the version of docutils using:

.. code-block:: bash

   $ pip show docutils

If this shows you've got version 0.13 of docutils installed, you can install
the working version of docutils with:

.. code-block:: bash

   $ sudo -HE  pip install docutils==0.12


Running the Documentation Generators
************************************

The ``/doc`` directory in your cloned copy of zephyr project git repo has all the
.rst source files, extra tools, and Makefile for generating a local copy of
the Zephyr project's technical documentation.  Assuming the local Zephyr
project copy is ``~/zephyr``, here are the commands to generate the html
content locally:

.. code-block:: bash

   $ cd ~/zephyr
   $ source zephyr-env.sh
   $ make htmldocs

The html output will be in ``~/zephyr/doc/_build/html/index.html``


.. _ReST documentation: http://sphinx-doc.org/rest.html
