.. _contribute_to_zephyr:

Contributing to Zephyr
######################

Contributions from the community are the backbone of the project. Whether it is by submitting code,
improving documentation, or proposing new features, your efforts are highly appreciated. This page
lists useful resources and guidelines to help you in your contribution journey.

General Guidelines
==================

.. toctree::
   :maxdepth: 1
   :hidden:

   guidelines.rst
   contributor_expectations.rst
   reviewer_expectations.rst
   coding_guidelines/index.rst
   style/index.rst
   proposals_and_rfcs.rst
   modifying_contributions.rst


:ref:`contribute_guidelines`
   Learn about the overall process and guidelines for contributing to the Zephyr project.

   This page is a mandatory read for first-time contributors as it contains important information on
   how to ensure your contribution can be considered for inclusion in the project and potentially
   merged.

:ref:`contributor-expectations`
   This document is another mandatory read that describes the expected behavior of *all*
   contributors to the project.

:ref:`reviewer-expectations`
   This document is another mandatory read that describes the expected behavior when revieweing
   contributions to the project.

:ref:`coding_guidelines`
   Code contributions are expected to follow a set of coding guidelines to ensure consistency and
   readability across the code base.

:ref:`coding_style`
   Code contributions are expected to follow a set of style guidelines to ensure consistency and
   readability across the code base.

:ref:`rfcs`
   Learn when and how to submit RFCs (Request for Comments) for new features and changes to the
   project.

:ref:`modifying_contributions`
   Guidelines for modifying contributions made by other developers and how to deal with stale pull
   requests.

Documentation
=============

The Zephyr project thrives on good documentation. Whether it is as part of a code contribution or
as a standalone effort, contributing documentation is particularly valuable to the project.

.. toctree::
   :maxdepth: 1
   :hidden:

   documentation/guidelines.rst
   documentation/generation.rst

:ref:`doc_guidelines`
   This page provides some simple guidelines for writing documentation using the reSTructuredText
   (reST) markup language and Sphinx documentation generator.

:ref:`zephyr_doc`
   As you write documentation, it can be helpful to see how it will look when rendered.

   This page describes how to build the Zephyr documentation locally.


Dealing with external components
================================

.. toctree::
   :maxdepth: 1
   :hidden:

   external.rst
   bin_blobs.rst

:ref:`external-contributions`
   Basic functionality or features that would make useful addition to Zephyr might be readily
   available in other open source projects, and it is recommended and encouraged to reuse such code.
   This page describes in more details when and how to import external source code into Zephyr.

:ref:`external-tooling`
   Similarly, external tooling used during compilation, code analysis, testing or simulation, can be
   beneficial and is covered in this section.

:ref:`bin-blobs`
   As some functionality might only be made available with the help of executable code distributed
   in binary form, this page describes the process and guidelines for :ref:`contributing binary
   blobs <blobs-process>` to the project.

Need help along the way?
========================

If you have questions related to the contribution process, the Zephyr community is here to help.
You may join our Discord_ channel or use the `Developer Mailing List`_.


.. _Discord: https://chat.zephyrproject.org
.. _Developer Mailing List: https://lists.zephyrproject.org/g/devel
