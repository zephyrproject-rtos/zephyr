.. _style:

Zephyr Project Documentation Style Guide
########################################

.. _styleIntro:

Introduction
************
This is the Zephyr Project Documentation Style Guide, a document
produced and maintained by the |project|. This chapter provides
overview information about the scope and purpose of this manual, along
with related resources.

Scope
=====
This guide addresses general documentation style and usage issues. It
includes the following:

:ref:`styleIntro`

Explains the scope, purpose, methodology of the document.


:ref:`generalWriting`

Covers the basic issues of writing styles, including general writing
guidelines and specific guidelines for step-by-step procedures.

:ref:`detailed`

Offers specific examples of all guidelines included in the
:ref:`generalWriting`. Additionally, it contains the guidelines for
module-based writing.

:ref:`words`

All the spelling, usage, grammar, mechanics, punctuation,
capitalization, and various rules about writing technical documents.

:ref:`ReST`

Specifically addresses writing standards and syntax with regard to
Restructuredtext.

This reference manual is written in American English.

Related Reference Guidelines

These related guidelines include information that may prove useful to
developers:

* :ref:`Gerrit`
* :ref:`code`

This style guide applies to the following technical documents:

* Commit messages.

* Technical presentations.

* Stand alone documents in Restructuredtext.

* All in-code comments.

* Release notes.


Purpose
=======
This manual provides general information, guidelines, procedures, and
instructions for the documentation work of the |project|. In addition,
this manual includes a style guide that details document conventions.

Consistent style is important for readers and writers. For readers,
consistency promotes clarity and confidence. For example, using an
abbreviation or term consistently prevents confusion and ambiguity. In
addition, producing information that has a clean and consistent style
tends to give readers more confidence in the information.

For writers, consistency is part of good publishing. Generally, the
poorer the style, the poorer the reader's opinion of the document,
writer, and publisher.

Audience
========
This style guide has two audiences:

* Primary audience: Technical writers that wish to create technical
  documentation for the |project| (writers).
* Secondary audience: Developers that wish to document their code and
  features (authors).

Where the guideline makes a distinction for the two audiences, we will
refer to these groups as writers and authors.

.. note::
	As secondary audience, developers are not expected to master the style
   and writing guidelines int his document; it is available to them as a
   reference.

Methodology
===========

The :ref:`style` contains exceptions to the other style guides. It also
contains additional material not found in those sources.

To research a style question, look in the :ref:`style` first. If the
question is not answered there, send your question to the
`mailing list`_. For hyphenation, spelling, or terminology usage
questions look in the Merriam-Webster's Collegiate Dictionary.

.. _mailing list: mailto:foss-rtos-collab@lists.01.org

If the question is answered in the existing style guide or dictionary,
the solution is implemented and enforced as described.


References
==========

In creating and refining the policies in this document, we consulted the
following sources for guidance:

* The Chicago Manual of Style (15th edition), The University of
  Chicago Press;
* Merriam-Webster Dictionary;
* Microsoft Manual of Style for Technical Publications, Microsoft
  Press;
* Microsoft Press Computer Dictionary, Microsoft Press; and
* Read Me First!, Oracle Technical Publications.

These sources do not always concur on questions of style and usage; nor
do we always agree with these sources. In areas where there is
disagreement, the decisions made may be explained within the respective
section.

The :ref:`style` takes precedence over all other style guides in all
cases. In cases where the guide does not address the issue at hand the
issue must be reported to the `mailing list`_.

Use Merriam-Webster's Collegiate Dictionary to determine correct
spelling, hyphenation, and usage.

.. _writing:

Style Guidelines
****************

This section addresses writing styles. It provides a rules for writing
clear, concise and consistent content. It is written using
Restructuredtext and contains all the use cases for the markup.

.. toctree::
	:maxdepth: 3

   writing.rst
   detailed.rst
   words.rst
   rest.rst