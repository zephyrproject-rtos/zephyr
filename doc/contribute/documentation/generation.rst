.. _zephyr_doc:

Documentation Generation
########################

These instructions will walk you through generating the Zephyr Project's
documentation on your local system using the same documentation sources
as we use to create the online documentation found at
https://docs.zephyrproject.org

.. _documentation-overview:

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
  https://docs.zephyrproject.org website. Most of the reStructuredText sources
  are found in the ``/doc`` directory, but others are stored within the
  code source tree near their specific component (such as ``/samples`` and
  ``/boards``)

* Doxygen-generated material used to create all API-specific documents
  also found at https://docs.zephyrproject.org

* Script-generated material for kernel configuration options based on Kconfig
  files found in the source code tree

.. graphviz::
   :caption: Schematic of the documentation build process

   digraph {
      rankdir=LR

      images [shape="rectangle" label=".png, .jpg\nimages"]
      rst [shape="rectangle" label="restructuredText\nfiles"]
      conf [shape="rectangle" label="conf.py\nconfiguration"]
      rtd [shape="rectangle" label="read-the-docs\ntheme"]
      header [shape="rectangle" label="c header\ncomments"]
      xml [shape="rectangle" label="XML"]
      html [shape="rectangle" label="HTML\nweb site"]
      sphinx[shape="ellipse" label="sphinx +\ndocutils"]
      images -> sphinx
      rst -> sphinx
      conf -> sphinx
      header -> doxygen
      doxygen -> xml
      xml-> sphinx
      rtd -> sphinx
      sphinx -> html
   }


The reStructuredText files are processed by the Sphinx documentation system,
and make use of the doxygen-generated API material.
Additional tools are required to generate the
documentation locally, as described in the following sections.

.. _documentation-processors:

Installing the documentation processors
***************************************

Our documentation processing has been tested to run with:

* Doxygen version 1.8.13
* Graphviz 2.43
* Latexmk version 4.56
* All Python dependencies listed in the repository file
  ``doc/requirements.txt``

In order to install the documentation tools, first install Zephyr as
described in :ref:`getting_started`. Then install additional tools
that are only required to generate the documentation,
as described below:

.. doc_processors_installation_start

.. tabs::

   .. group-tab:: Linux

      Common to all Linux installations, install the Python dependencies
      required to build the documentation:

      .. code-block:: console

         pip install -U -r ~/zephyrproject/zephyr/doc/requirements.txt

      On Ubuntu Linux:

      .. code-block:: console

         sudo apt-get install --no-install-recommends doxygen graphviz librsvg2-bin \
         texlive-latex-base texlive-latex-extra latexmk texlive-fonts-recommended imagemagick

      On Fedora Linux:

      .. code-block:: console

         sudo dnf install doxygen graphviz texlive-latex latexmk \
         texlive-collection-fontsrecommended librsvg2-tools ImageMagick

      On Clear Linux:

      .. code-block:: console

         sudo swupd bundle-add texlive graphviz ImageMagick

      On Arch Linux:

      .. code-block:: console

         sudo pacman -S graphviz doxygen librsvg texlive-core texlive-bin \
         texlive-latexextra texlive-fontsextra imagemagick

   .. group-tab:: macOS

      Install the Python dependencies required to build the documentation:

      .. code-block:: console

         pip install -U -r ~/zephyrproject/zephyr/doc/requirements.txt

      Use ``brew`` and ``tlmgr`` to install the tools:

      .. code-block:: console

         brew install doxygen graphviz mactex librsvg imagemagick
         tlmgr install latexmk
         tlmgr install collection-fontsrecommended

   .. group-tab:: Windows

      Install the Python dependencies required to build the documentation:

      .. code-block:: console

         pip install -U -r %HOMEPATH$\zephyrproject\zephyr\doc\requirements.txt

      Open a ``cmd.exe`` window as **Administrator** and run the following command:

      .. code-block:: console

         choco install doxygen.install graphviz strawberryperl miktex rsvg-convert imagemagick

      .. note::
         On Windows, the Sphinx executable ``sphinx-build.exe`` is placed in
         the ``Scripts`` folder of your Python installation path.
         Depending on how you have installed Python, you might need to
         add this folder to your ``PATH`` environment variable. Follow
         the instructions in `Windows Python Path`_ to add those if needed.

.. doc_processors_installation_end

Documentation presentation theme
********************************

Sphinx supports easy customization of the generated documentation
appearance through the use of themes. Replace the theme files and do
another ``make html`` and the output layout and style is changed.
The ``read-the-docs`` theme is installed as part of the
:ref:`install_py_requirements` step you took in the getting started
guide.

Running the documentation processors
************************************

The ``/doc`` directory in your cloned copy of the Zephyr project git
repo has all the .rst source files, extra tools, and Makefile for
generating a local copy of the Zephyr project's technical documentation.
Assuming the local Zephyr project copy is in a folder ``zephyr`` in your home
folder, here are the commands to generate the html content locally:

.. code-block:: console

   # On Linux/macOS
   cd ~/zephyr/doc
   # On Windows
   cd %userprofile%\zephyr\doc

   # Use cmake to configure a Ninja-based build system:
   cmake -GNinja -B_build .

   # Enter the build directory
   cd _build

   # To generate HTML output, run ninja on the generated build system:
   ninja html
   # If you modify or add .rst files, run ninja again:
   ninja html

   # To generate PDF output, run ninja on the generated build system:
   ninja pdf

.. warning::

   The documentation build system creates copies in the build
   directory of every .rst file used to generate the documentation,
   along with dependencies referenced by those .rst files.

   This means that Sphinx warnings and errors refer to the **copies**,
   and **not the version-controlled original files in Zephyr**. Be
   careful to make sure you don't accidentally edit the copy of the
   file in an error message, as these changes will not be saved.

Depending on your development system, it will take up to 15 minutes to
collect and generate the HTML content.  When done, you can view the HTML
output with your browser started at ``doc/_build/html/index.html`` and
if generated, the PDF file is available at ``doc/_build/latex/zephyr.pdf``.

If you want to build the documentation from scratch just delete the contents
of the build folder and run ``cmake`` and then ``ninja`` again.

.. note::

   If you add or remove a file from the documentation, you need to re-run CMake.

On Unix platforms a convenience :zephyr_file:`doc/Makefile` can be used to
build the documentation directly from there:

.. code-block:: console

   cd ~/zephyr/doc

   # To generate HTML output
   make html

   # To generate PDF output
   make pdf

Developer-mode Document Building
********************************

When making and testing major changes to the documentation, we provide an option
to temporarily stub-out the auto-generated Devicetree bindings documentation so
the doc build process runs faster.

To enable this mode, set the following option when invoking cmake::

   -DDT_TURBO_MODE=1

or invoke make with the following target::

   cd ~/zephyr

   # To generate HTML output without detailed Kconfig
   make html-fast

Viewing generated documentation locally
***************************************

The generated HTML documentation can be hosted locally with python for viewing
with a web browser:

.. code-block:: console

   $ python3 -m http.server -d _build/html

.. note::

   WSL2 users may need to explicitly bind the address to ``127.0.0.1`` in order
   to be accessible from the host machine:

   .. code-block:: console

      $ python3 -m http.server -d _build/html --bind 127.0.0.1

Alternatively, the documentation can be built with the ``make html-live``
(or ``make html-live-fast``) command, which will build the documentation, host
it locally, and watch the documentation directory for changes. When changes are
observed, it will automatically rebuild the documentation and refresh the hosted
files.

Linking external Doxygen projects against Zephyr
************************************************

External projects that build upon Zephyr functionality and wish to refer to
Zephyr documentation in Doxygen (through the use of @ref), can utilize the
tag file exported at `zephyr.tag <../../doxygen/html/zephyr.tag>`_

Once downloaded, the tag file can be used in a custom ``doxyfile.in`` as follows::

   TAGFILES = "/path/to/zephyr.tag=https://docs.zephyrproject.org/latest/doxygen/html/"

For additional information refer to `Doxygen External Documentation`_.


.. _reStructuredText: http://sphinx-doc.org/rest.html
.. _Sphinx: http://sphinx-doc.org/
.. _Windows Python Path: https://docs.python.org/3/using/windows.html#finding-the-python-executable
.. _Doxygen External Documentation: https://www.doxygen.nl/manual/external.html
