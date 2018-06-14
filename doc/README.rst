.. _zephyr_doc:

Zephyr documentation Generation
###############################

These instructions will walk you through generating the Zephyr Project's
documentation on your local system using the same documentation sources
as we use to create the online documentation found at
http://docs.zephyrproject.org

Documentation overview
**********************

Zephyr Project content is written using the reStructuredText markup
language (.rst file extension) with Sphinx extensions, and processed
using Sphinx to create a formatted stand-alone website. Developers can
view this content either in its raw form as .rst markup files, or you
can generate the HTML content and view it with a web browser directly on
your workstation. This same .rst content is also fed into the Zephyr
Project's public website documentation area (with a different theme
applied).

You can read details about `reStructuredText`_, and `Sphinx`_ from
their respective websites.

The project's documentation contains the following items:

* ReStructuredText source files used to generate documentation found at the
  http://docs.zephyrproject.org website. Most of the reStructuredText sources
  are found in the ``/doc`` directory, but others are stored within the
  code source tree near their specific component (such as ``/samples`` and
  ``/boards``)

* Doxygen-generated material used to create all API-specific documents
  also found at http://docs.zephyrproject.org

* Script-generated material for kernel configuration options based on Kconfig
  files found in the source code tree

The reStructuredText files are processed by the Sphinx documentation system,
and make use of the breathe extension for including the doxygen-generated API
material.  Additional tools are required to generate the
documentation locally, as described in the following sections.

Installing the documentation processors
***************************************

Our documentation processing has been tested to run with:

* Doxygen version 1.8.13
* Sphinx version 1.7.5
* Breathe version 4.9.1
* docutils version 0.14
* sphinx_rtd_theme version 0.4.0

Begin by cloning a copy of the git repository for the Zephyr project and
setting up your development environment as described in :ref:`getting_started`
or specifically for Ubuntu in :ref:`installation_linux`.

Other than ``doxygen``, the documentation tools should be installed
using ``pip3`` (as documented in the development environment set up
instructions).

The documentation generation tools are included in the set of tools
expected for the Zephyr build environment and so are included in
``requirements.txt``

Documentation presentation theme
********************************

Sphinx supports easy customization of the generated documentation
appearance through the use of themes.  Replace the theme files and do
another ``make htmldocs`` and the output layout and style is changed.
The ``read-the-docs`` theme is installed as part of the
``requirements.txt`` list above, and will be used if it's available, for
local doc generation.


Running the documentation processors
************************************

The ``/doc`` directory in your cloned copy of the Zephyr project git
repo has all the .rst source files, extra tools, and Makefile for
generating a local copy of the Zephyr project's technical documentation.
Assuming the local Zephyr project copy is ``~/zephyr``, here are the
commands to generate the html content locally:

.. code-block:: bash

   $ cd ~/zephyr
   $ source zephyr-env.sh
   $ make htmldocs

Depending on your development system, it will take about 15 minutes to
collect and generate the HTML content.  When done, you can view the HTML
output with your browser started at ``~/zephyr/doc/_build/html/index.html``

Filtering expected warnings
***************************

Alas, there are some known issues with the doxygen/Sphinx/Breathe
processing that generates warnings for some constructs, in particular
around unnamed structures in nested unions or structs.
While these issues are being considered for fixing in
Sphinx/Breathe, we've added a post-processing filter on the output of
the documentation build process to check for "expected" messages from the
generation process output.

The output from the Sphinx build is processed by the python script
``scripts/filter-known-issues.py`` together with a set of filter
configuration files in the ``.known-issues/doc`` folder.  (This
filtering is done as part of the ``doc/Makefile``.)

If you're contributing components included in the Zephyr API
documentation and run across these warnings, you can include filtering
them out as "expected" warnings by adding a conf file to the
``.known-issues/doc`` folder, following the example of other conf files
found there.

.. _reStructuredText: http://sphinx-doc.org/rest.html
.. _Sphinx: http://sphinx-doc.org/
