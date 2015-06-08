.. _modular:

Modular Writing
###############

In a modular approach to writing content is organized into a series of
specific modules.

Contrasted with linear book-oriented or narrative writing, module- based
writing results in discrete chunks of content that can be rearranged
and reused in multiple documents without author intervention or
rewriting.

Modular writing doesn't include any content format formating. The format
of the output is handled by a style sheet, which is applied when the
content is published. The |project| uses Sphinx to build the
documentation. Sphinx gathers the content from all modules and produces
HTML output that allows the use of CSS stylesheets for format.

Some advantages of modular writing:

* Write once and only once.
* Reuse sections for multiple products, different audiences.
* Restructure content quickly.
* Revising a few sections is faster than revising a whole document.
* No need to format the content.

Some terms used in modular writing:

Rules: Must do these steps.

Requirements: Must use these materials.

Guidelines: Specific things to do.

The |project| strives for full modularization of content. The
modularization is still a work in progress and new content must be
structured following this guidelines.

.. note::
   The guidelines presented in :ref:`generalWriting` still apply to
   modular writing, except where specifically noted to the contrary.

Module Types
************

There are four module types in modular writing: overview, concept, task,
and reference. Each type answers a different question and contains
distinct kinds of information. They are organized to help users find
the information they need.

Overview
========
Main theme or goal. Overview modules set the tone and expectation for
the modules that follow:

* Present the main theme of the module group.

* Be brief. An overview module is the container for the module group,
  not a detailed explanation.

* Focus on the overall goal of the information, so the users know why
  it is important for them and what they will accomplish.

The |project|'s documentation uses overview modules as container
modules. Container modules include a toctree directive that includes
either other container modules or concept, task and reference modules.
For example in a System Requirements overview module:

.. code-block:: rst

   .. toctree::
      :maxdepth: 2

      requirements_packages.rst
      requirements_install.rst
      requirements_limitations.rst

Concept
=======

Basic information and definitions. Effective concept modules are easy to
read and easy to scan. Use these tips to write useful conceptual
information:

• Stick to one idea per module.

• Focus on the goal for the users. Present conceptual information that
  supports the tasks that accomplish the goal.

• Organize information into small chunks, and label the chunks for
  scannability.

• Use bulleted lists and tables for at-a-glance access to the
  information.

• Avoid referring to the module itself, as in This module describes....

• Don't refer to other modules, as in As you learned in the previous
  chapter....

• Don't force users to read a dense paragraph only to learn they
  didn't need to read it.

Task
====
How to accomplish the goal. Follow these guidelines:

• Begin each step with the goal, followed by the navigation, then the
  instruction. For example: "To install the package, go to the home
  directory and type:"

• Write individual steps as separate, **numbered** entries. You can
  combine up to three short steps if they occur in the same part of the
  interface. Be mindful of the correct syntax for interfaces. For
  example: "Select :menuselection:`Tools --> Options`, and click the :
  guilabel:`Edit` tab."

• Limit a task to seven to ten steps if possible. Seven is ideal; ten
  is acceptable.

• Break up a long task into a series of shorter tasks, collectively
  referred to as a process, that one must perform in sequence to
  achieve the goal.

.. note::
   In a process, don't call a task a step. Reserve the word step for a
   single meaningful action within a task.

• Use a table for optional actions under a step.

• Use the same verb for the same action.

• Don't put explanatory information within the task or the step; put
  it in a note or a paragraph after the applicable step or after the
  task . If the information is lengthy, put it in the associated
  concept module or in a container module for a group of tasks.

• Verify that each step tells users to perform an action. Don't force
  users to understand or interpret a concept while they are performing
  the step.

Reference
=========

Background information. Follow these rules when writing reference
modules, including modules for page-level help and command-line options:

• For page-level reference modules, present options and their
  definitions in the order they appear in the interface.

* Write the definition of an option as a sentence fragment, beginning
  with a third-person present-tense verb, as in Specifies...,
  Defines..., Creates....

* Use "See :ref:`cross-references`" for cross-references to supporting
  tasks and concepts.

Module Structure
****************

Each of the four different module types consists of three (sometimes
four) items, in this order:

1. Title: Well-written module titles help users find information at a
glance.

2. Short description: The short description answers the question "Why
should I read this?"

3. Body: The body of a module is the actual content, and it includes
section headings, paragraphs, tables, and lists. Use tables, lists, and
figures to chunk related information. The body contains the answers to
the questions "What" and "How" depending on the module type.

4. Considerations (optional): Considerations add information to some
modules.

Example:

.. _moduleExample:

.. code-block:: rst

   Title
   #####

   These tips apply to all four module types.

   Sections
   ********
   Do this...

   Level 2 Sections
   ================

   Level 3 Sections
   ----------------

Do this...

* Include one idea per module.

* Keep module titles brief and focused.

* Put the most important and distinctive information at the beginning
  of the title rather than the end.

* Create titles as sentence fragments.

* Use plural nouns in titles, Deploying New Drivers, unless you are
  referring to a single item, Submitting a Change.

* If you can take a position, use qualitative words; benefits,
  advantages, limitations; especially to show the benefit to the user.


 * Use "vs.", not "versus", in titles, Daily vs. Weekly Backups.

* Review and rewrite the module title after you've written the content.

Don't do this...

* Don't begin a module title with Understanding or About or How to.
* Don't begin a title with an article: a, an, the. Using plural nouns
  is one way to avoid using articles.

Module Titles
=============

Titles of overview modules: A noun, noun phrase, or present participial
form of a verb (-ing). Provides a sense of process and continuity.
Examples:

* System Requirements

* Managing Your Network

Titles of concept modules: A descriptive noun phrase or verb phrase. The
title must answer the question "What about it?" Examples:

* Considerations When Planning a System

* Advantages of Widget Security

* Limitations of Edit mode

Titles of task modules: An imperative (command) phrase. Describes the
task, not the function to be used. Examples:

* Configure General Server Settings

* Import File Formats

Titles of reference modules: The name of a reference object. Provides
quick access to facts needed to understand a concept or complete a
task. Examples:

* Assembly Code Options

* Default Values Provided by the Platform

Short Descriptions
==================

The short description is the first paragraph of a module — a brief
statement of the module's theme that helps readers find the information
they are looking for. An effective short description is:

* Short: Provides just enough information—in one to three complete
  sentences or roughly 25 to 35 words to let users know whether to read
  more.

* Informative: Provides enough detail for advanced users to get what
  they need and move on.

* Pertinent: Doesn't include background information; instead
  summarizes the purpose of the module.

Converting Conventional writing to Module-Based writing
*******************************************************

If you are converting a conventionally written document to module- based
content, you will have to rearrange and rewrite some of the content.
Here are some guidelines and some things to look for:

* Limit headings to three levels. If you are working with an older
  document, you might have to convert fourth and lower levels into
  tables or lists. Lists are excellent for splitting information up.
  See :ref:`lists` for guidelines on creating lists.

* Divide large concepts into smaller sections. Sections are useful
  when the information in each section isn't long enough to be in its
  own module or when the info would not make sense in its own module.
  If the content has many sections, think of ways to split it into
  separate modules.

* Make sure each section has an introductory paragraph. For section
  heads that have no introductory paragraph, add one or promote the
  next section up a level.

* Keep cross-references generic. Convert explicit table and figure
  cross-references within the same module to generic references, such
  as "the following table". Relocate these figures/tables closer to the
  referring text.

* Avoid "glue text". Glue text is a word or phrase that links a
  module/section to a previous or next module/section, as if the
  content were linear or sequential. Module-based writing is not linear.

* Use generic terms. Instead of a specific product name, use CPU or
  processor or PCH when possible, especially in images. This will help
  repurpose the content for other products and cases.

* Rewrite content for reusability. Search for words like chapter and
  page and replace with module, section, or some term that does not
  chain the content to the book- or page-based metaphor. When
  necessary, rewrite content so that it may be reused in other
  documents.