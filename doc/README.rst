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

The project's documentation currently comprises the following items:

* An Installation Guide for Linux host systems

* A set of Collaboration Guidelines for the project.

* Doxygen output from the code base for all APIs.

Installing the documentation processors
***************************************

Install the current version of ``Sphinx``, type:

.. code-block:: bash

   $ git clone https://github.com/sphinx-doc/sphinx.git sphinx

   $ cd sphinx

   $ sudo -E python setup.py install

   $ cd ..

   $ git clone https://github.com/michaeljones/breathe.git breathe

   $ cd breathe

   $ sudo -E python setup.py install

.. note::

   Make sure that ``Doxygen`` is installed in your system.
   The installation of Doxygen is beyond the scope of this document.

Running the Documentation Generators
************************************

Assuming that the Zephyr Project tree with the doc directory is in
``DIRECTORY``, type:

.. code-block:: bash

   $ cd DIRECTORY/doc

   $ make doxy html

Find the output in ``DIRECTORY/doc/_build/html/index.html``

Review the available formats with:

.. code-block:: bash

   $ make -C DIRECTORY/doc doxy html

If you want the LaTeX PDF output, you need to install all the Latex
packages first. That installation is beyond the scope of this document.

.. _ReST documentation: http://sphinx-doc.org/rest.html
