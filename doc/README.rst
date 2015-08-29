Welcome to |codename|
#####################

.. This document is in Restructured Text Format.

   More information at `<http://sphinx-doc.org/rest.html>`_.
   This is a comment that won't show up in formatted output

Welcome to the |project|.

Thank you for your interest in the |project|. These instructions are
designed to walk you through generating the |project|'s documentation.


Documentation Notes
*******************

The project's documentation currently comprises the following items:

* An Installation Guide for Linux host systems

* A set of Collaboration Guidelines for the project.

* Raw Doxygen output from the code base.

Installing the documentation processors
***************************************

Install the current version of :program:`Sphinx`, type:

.. code-block:: bash

   $ git clone https://github.com/sphinx-doc/sphinx.git sphinx

   $ cd sphinx

   $ sudo -E python setup.py install

   $ cd ..

   $ git clone https://github.com/michaeljones/breathe.git breathe

   $ cd breathe

   $ sudo -E python setup.py install

.. note::

   Make sure that :program:`Doxygen` is installed in your system.
   The installation of Doxygen is beyond the scope of this document.

Running the documentation generators
************************************

Assuming that the |project| tree with the doc directory is in
:file:`DIRECTORY`, type:

.. code-block:: bash

   $ cd DIRECTORY/doc

   $ make doxy html

Find the output in :file:`DIRECTORY/doc/_build/html/index.html`

Review the available formats with:

.. code-block:: bash

   $ make -C DIRECTORY/doc doxy html

If you want the LaTeX PDF output, you need to install all the Latex
packages first. That installation is beyond the scope of this document.
