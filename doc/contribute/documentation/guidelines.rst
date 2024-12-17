.. _doc_guidelines:

Documentation Guidelines
########################

.. highlight:: rst

.. note::

   For instructions on building the documentation, see :ref:`zephyr_doc`.

Zephyr Project content is written using the `reStructuredText`_ markup
language (.rst file extension) with Sphinx extensions, and processed
using Sphinx to create a formatted standalone website.  Developers can
view this content either in its raw form as .rst markup files, or (with
Sphinx installed) they can :ref:`build the documentation <zephyr_doc>` locally
to generate the documentation in HTML or PDF format. The HTML content can
then be viewed using a web browser. This same .rst content is served by the
`Zephyr documentation`_ website.

You can read details about `reStructuredText`_
and about `Sphinx extensions`_ from their respective websites.

.. _Sphinx extensions: http://www.sphinx-doc.org/en/stable/contents.html
.. _reStructuredText: http://docutils.sourceforge.net/docs/ref/rst/restructuredtext.html
.. _Sphinx Inline Markup:  http://sphinx-doc.org/markup/inline.html#inline-markup
.. _Zephyr documentation:  https://docs.zephyrproject.org

This document provides a quick reference for commonly used reST and
Sphinx-defined directives and roles used to create the documentation
you're reading.

Content Structure
*****************

Tabs, spaces, and indenting
===========================

Indenting is significant in reST file content, and using spaces is
preferred.  Extra indenting can (unintentionally) change the way content
is rendered too.  For lists and directives, indent the content text to
the first non-white space in the preceding line.  For example::

   * List item that spans multiple lines of text
     showing where to indent the continuation line.

   1. And for numbered list items, the continuation
      line should align with the text of the line above.

   .. code-block::

      The text within a directive block should align with the
      first character of the directive name.

Refer to the Zephyr :ref:`coding_style` for additional requirements.

Headings
========

While reST allows use of both an overline and matching underline to
indicate a heading, we only use an underline indicator for headings.

* Document title (h1) use ``#`` for the underline character
* First section heading level (h2) use ``*``
* Second section heading level (h3) use ``=``
* Third section heading level (h4) use ``-``

The heading underline must be the same length as the heading text.

For example::

   This is a title heading
   #######################

   some content goes here

   First section heading
   *********************


Lists
=====

For bullet lists, place an asterisk (``*``) or hyphen (``-``) at
the start of a paragraph and indent continuation lines with two
spaces.

The first item in a list (or sublist) must have a blank line before it
and should be indented at the same level as the preceding paragraph
(and not indented itself).

For numbered lists
start with a 1. or a. for example, and continue with autonumbering by
using a ``#`` sign.  Indent continuation lines with three spaces::

   * This is a bulleted list.
   * It has two items, the second
     item and has more than one line of reST text.  Additional lines
     are indented to the first character of the
     text of the bullet list.

   1. This is a new numbered list. If the wasn't a blank line before it,
      it would be a continuation of the previous list (or paragraph).
   #. It has two items too.

   a. This is a numbered list using alphabetic list headings
   #. It has three items (and uses autonumbering for the rest of the list)
   #. Here's the third item

   #. This is an autonumbered list (default is to use numbers starting
      with 1).

      #. This is a second-level list under the first item (also
         autonumbered).  Notice the indenting.
      #. And a second item in the nested list.
   #. And a second item back in the containing list.  No blank line
      needed, but it wouldn't hurt for readability.

Definition lists (with a term and its definition) are a convenient way
to document a word or phrase with an explanation.  For example this reST
content::

   The Makefile has targets that include:

   html
      Build the HTML output for the project

   clean
      Remove all generated output, restoring the folders to a
      clean state.

Would be rendered as:

   The Makefile has targets that include:

   html
      Build the HTML output for the project

   clean
      Remove all generated output, restoring the folders to a
      clean state.

Multi-column lists
==================

If you have a long bullet list of items, where each item is short, you
can indicate the list items should be rendered in multiple columns with
a special ``.. rst-class:: rst-columns`` directive.  The directive will
apply to the next non-comment element (e.g., paragraph), or to content
indented under the directive. For example, this unordered list::

   .. rst-class:: rst-columns

   * A list of
   * short items
   * that should be
   * displayed
   * horizontally
   * so it doesn't
   * use up so much
   * space on
   * the page

would be rendered as:

.. rst-class:: rst-columns

   * A list of
   * short items
   * that should be
   * displayed
   * horizontally
   * so it doesn't
   * use up so much
   * space on
   * the page

A maximum of three columns will be displayed, and change based on the
available width of the display window, reducing to one column on narrow
(phone) screens if necessary.  We've deprecated use of the ``hlist``
directive because it misbehaves on smaller screens.

Tables
======

There are a few ways to create tables, each with their limitations or
quirks.  `Grid tables
<http://docutils.sourceforge.net/docs/ref/rst/restructuredtext.html#grid-tables>`_
offer the most capability for defining merged rows and columns, but are
hard to maintain::

   +------------------------+------------+----------+----------+
   | Header row, column 1   | Header 2   | Header 3 | Header 4 |
   | (header rows optional) |            |          |          |
   +========================+============+==========+==========+
   | body row 1, column 1   | column 2   | column 3 | column 4 |
   +------------------------+------------+----------+----------+
   | body row 2             | ...        | ...      | you can  |
   +------------------------+------------+----------+ easily   +
   | body row 3 with a two column span   | ...      | span     |
   +------------------------+------------+----------+ rows     +
   | body row 4             | ...        | ...      | too      |
   +------------------------+------------+----------+----------+

This example would render as:

+------------------------+------------+----------+----------+
| Header row, column 1   | Header 2   | Header 3 | Header 4 |
| (header rows optional) |            |          |          |
+========================+============+==========+==========+
| body row 1, column 1   | column 2   | column 3 | column 4 |
+------------------------+------------+----------+----------+
| body row 2             | ...        | ...      | you can  |
+------------------------+------------+----------+ easily   +
| body row 3 with a two column span   | ...      | span     |
+------------------------+------------+----------+ rows     +
| body row 4             | ...        | ...      | too      |
+------------------------+------------+----------+----------+

`List tables
<http://docutils.sourceforge.net/docs/ref/rst/directives.html#list-table>`_
are much easier to maintain, but don't support row or column spans::

   .. list-table:: Table title
      :widths: 15 20 40
      :header-rows: 1

      * - Heading 1
        - Heading 2
        - Heading 3
      * - body row 1, column 1
        - body row 1, column 2
        - body row 1, column 3
      * - body row 2, column 1
        - body row 2, column 2
        - body row 2, column 3

This example would render as:

.. list-table:: Table title
   :widths: 15 20 40
   :header-rows: 1

   * - Heading 1
     - Heading 2
     - Heading 3
   * - body row 1, column 1
     - body row 1, column 2
     - body row 1, column 3
   * - body row 2, column 1
     - body row 2, column 2
     - body row 2, column 3

The ``:widths:`` parameter lets you define relative column widths.  The
default is equal column widths. If you have a three-column table and you
want the first column to be half as wide as the other two equal-width
columns, you can specify ``:widths: 1 2 2``.  If you'd like the browser
to set the column widths automatically based on the column contents, you
can use ``:widths: auto``.

Tabbed Content
==============

As introduced in the :ref:`getting_started`, you can provide alternative
content to the reader via a tabbed interface. When the reader clicks on
a tab, the content for that tab is displayed, for example::

   .. tabs::

      .. tab:: Apples

         Apples are green, or sometimes red.

      .. tab:: Pears

         Pears are green.

      .. tab:: Oranges

         Oranges are orange.

will display as:

.. tabs::

   .. tab:: Apples

      Apples are green, or sometimes red.

   .. tab:: Pears

      Pears are green.

   .. tab:: Oranges

      Oranges are orange.

Tabs can also be grouped, so that changing the current tab in one area
changes all tabs with the same name throughout the page.  For example:

.. tabs::

   .. group-tab:: Linux

      Linux Line 1

   .. group-tab:: macOS

      macOS Line 1

   .. group-tab:: Windows

      Windows Line 1

.. tabs::

   .. group-tab:: Linux

      Linux Line 2

   .. group-tab:: macOS

      macOS Line 2

   .. group-tab:: Windows

      Windows Line 2

In this latter case, we're using ``.. group-tab::`` instead of simply
``.. tab::``.  Under the hood, we're using the `sphinx-tabs
<https://github.com/executablebooks/sphinx-tabs>`_ extension that's included
in the Zephyr setup.  Within a tab, you can have most any content *other
than a heading* (code-blocks, ordered and unordered lists, pictures,
paragraphs, and such).  You can read more about sphinx-tabs from the
link above.


Text Formatting
***************

ReSTructuredText supports a variety of text formatting options. This section provides a quick
reference for some of the most commonly used text formatting options in Zephyr documentation. For an
exhaustive list, refer to the `reStructuredText Quick Reference`_,
`reStructuredText Interpreted Text Roles`_ as well as the `additional roles provided by Sphinx`_.

.. _reStructuredText Quick Reference: http://docutils.sourceforge.io/docs/user/rst/quickref.html
.. _reStructuredText Interpreted Text Roles: https://docutils.sourceforge.io/docs/ref/rst/roles.html
.. _additional roles provided by Sphinx: https://www.sphinx-doc.org/en/master/usage/restructuredtext/roles.html

Content Highlighting
====================

Some common reST inline markup samples:

* one asterisk: ``*text*`` for emphasis (*italics*),
* two asterisks: ``**text**`` for strong emphasis (**boldface**), and
* two backquotes: ````text```` for ``inline code`` samples.

If asterisks or backquotes appear in running text and could be confused with
inline markup delimiters, you can eliminate the confusion by adding a
backslash (``\``) before it.

File Names and Commands
=======================

Sphinx extends reST by supporting additional inline markup elements (called
"roles") used to tag text with special
meanings and allow style output formatting. (You can refer to the `Sphinx Inline Markup`_
documentation for the full list).

While double quotes can be used for rendering text as "code", you are encouraged to use the
following roles for marking up file names, command names, and other "special" text.

* :rst:role:`file` for file names, e.g., ``:file:`CMakeLists.txt``` will render as
  :file:`CMakeLists.txt`

  .. note::

     In case you want to indicate a "variable" file path, you may use curly braces to enclose the
     variable part of the path, e.g., ``:file:`{boardname}_defconfig``` will render as
     :file:`{boardname}_defconfig`.

* :rst:role:`command` for command names, e.g., ``:command:`make``` will render as :command:`make`

* :rst:role:`envvar` for environment variables, e.g., ``:envvar:`ZEPHYR_BASE``` will render as
  :envvar:`ZEPHYR_BASE`

For creating references to files that are hosted in the Zephyr organization on GitHub, refer to
:ref:`linking_to_zephyr_files` section below.

User Interaction
================

When documenting user interactions, such as key combinations or GUI interactions, use the following
roles to highlight the commands in a meaningful way:

* :rst:role:`kbd` for keyboard input, e.g., ``:kbd:`Ctrl-C``` will render as :kbd:`Ctrl-C`

* :rst:role:`menuselection` for menu selections, e.g., ``:menuselection:`File --> Open``` will render
  as :menuselection:`File --> Open`

* :rst:role:`guilabel` for GUI labels, e.g., ``:guilabel:`Cancel``` will render as :guilabel:`Cancel`

Mathematical Formulas
=====================

You can include mathematical formulas using either the :rst:role:`math` role or :rst:dir:`math`
directive. The directive provides more flexibility in case you have a more complex formula.

The input language for mathematics is LaTeX markup. Example::

   The answer to life, the universe, and everything is :math:`30 + 2^2 + \sqrt{64} = 42`.

This would render as:

   The answer to life, the universe, and everything is :math:`30 + 2^2 + \sqrt{64} = 42`.

Non-ASCII Characters
====================

You can insert non-ASCII characters such as a Trademark symbol (|trade|),
by using the notation ``|trade|``.
Available replacement names are defined in an include file used during the Sphinx processing
of the reST files.  The names of these replacement characters are the same as used in HTML
entities used to insert characters in HTML, e.g., ``\&trade;`` and are defined in the
file :zephyr_file:`doc/substitutions.txt` as listed below:

.. literalinclude:: ../../substitutions.txt
   :language: rst

We've kept the substitutions list small but others can be added as
needed by submitting a change to the :zephyr_file:`doc/substitutions.txt` file.

Code Blocks and Command Examples
================================

Use the reST :rst:dir:`code-block` directive to create a highlighted block of
fixed-width text, typically used for showing formatted code or console
commands and output.  Smart syntax highlighting is also supported (using the
Pygments package). You can also directly specify the highlighting language.
For example::

   .. code-block:: c

      struct k_object {
         char *name;
         uint8_t perms[CONFIG_MAX_THREAD_BYTES];
         uint8_t type;
         uint8_t flags;
         uint32_t data;
      } __packed;

Note the blank line between the :rst:dir:`code-block` directive and the first
line of the code-block body, and the body content is indented three
spaces (to the first non-white space of the directive name).

This would be rendered as:

   .. code-block:: c

      struct k_object {
         char *name;
         uint8_t perms[CONFIG_MAX_THREAD_BYTES];
         uint8_t type;
         uint8_t flags;
         uint32_t data;
      } __packed;


Other languages are of course supported (see `languages supported by Pygments`_), and in particular,
you are encouraged to make use of the following when appropriate:

.. _`languages supported by Pygments`: http://pygments.org/languages/

* ``c`` for C code
* ``cpp`` for C++ code
* ``python`` for Python code
* ``console`` for console output, i.e. interactive shell sessions where commands are prefixed by a
  prompt (ex. ``$`` for Linux, or ``uart:~$`` for Zephyr's shell), and where the output is also
  shown. The commands will be highlighted, and the output will not. What's more, copying code block
  using the "copy" button will automatically copy just the commands, excluding the prompt and the
  outputs of the commands.
* ``shell`` or ``bash`` for shell commands. Both languages get highlighted the same but you may use
  ``bash`` for conveying that the commands are bash-specific, and ``shell`` for generic shell
  commands.

  .. note::

     Do not use ``bash`` or ``shell`` if your code block includes a prompt, use ``console`` instead.

     Reciprocally, do not use ``console`` if your code block does not include a prompt and is not
     showcasing an interactive session with command(s) and their output.

     .. list-table:: When to use ``bash``/``shell`` vs. ``console``
        :class: wrap-normal
        :header-rows: 1
        :widths: 20,40,40

        * - Use case
          - ``code-block`` snippet
          - Expected output

        * - One or several commands, no output

          - .. code-block:: rst

               .. code-block:: shell

                  echo "Hello World!"

          - .. code-block:: shell

               echo "Hello World!"

        * - An interactive shell session with command(s) and their output

          - .. code-block:: rst

               .. code-block:: console

                  $ echo "Hello World!"
                  Hello World!

          - .. code-block:: console

               $ echo "Hello World!"
               Hello World!

        * - An interactive Zephyr shell session, with commands and their outputs

          - .. code-block:: rst

               .. code-block:: console

                  uart:~$ version
                  Zephyr version 3.5.99
                  uart:~$ kernel uptime
                  Uptime: 20970 ms

          - .. code-block:: console

               uart:~$ version
               Zephyr version 3.5.99
               uart:~$ kernel uptime
               Uptime: 20970 ms

* ``bat`` for Windows batch files
* ``cfg`` for config files with "KEY=value" entries (ex. Kconfig ``.conf`` files)
* ``cmake`` for CMake
* ``devicetree`` for Devicetree
* ``kconfig`` for Kconfig
* ``yaml`` for YAML
* ``rst`` for reStructuredText

When no language is specified, the language is set to ``none`` and the code block is not
highlighted. You may also use ``none`` explicitly to achieve the same result; for example::

   .. code-block:: none

      This would be a block of text styled with a background
      and box, but with no syntax highlighting.

Would display as:

   .. code-block:: none

      This would be a block of text styled with a background
      and box, but with no syntax highlighting.

There's a shorthand for writing code blocks too: end the introductory paragraph with a double colon
(``::``) and indent the code block content that follows it by three spaces.  On output, only one
colon will be shown.  The code block will have no highlighting (i.e. ``none``). You may however use
the :rst:dir:`highlight` directive to customize the default language used in your document (see for
example how this is done at the beginning of this very document).


Links and Cross-References
**************************

.. _internal-linking:

Cross-referencing internal content
==================================

Traditional ReST links are only supported within the current file using the
notation::

   Refer to the `internal-linking`_ page

which renders as,

   Refer to the `internal-linking`_ page

Note the use of a trailing
underscore to indicate an outbound link. In this example, the label was
added immediately before a heading, so the text that's displayed is the
heading text itself. You can change the text that's displayed as the
link writing this as::

   Refer to the `show this text instead <internal-linking_>`_ page

which renders as,

   Refer to the `show this text instead <internal-linking_>`_ page


Cross-referencing external content
==================================

With Sphinx's help, we can create
link-references to any tagged text within the Zephyr Project documentation.

Target locations in a document are defined with a label directive::

      .. _my label name:

      Heading
      =======

Note the leading underscore indicating an inbound link.
The content immediately following
this label must be a heading, and is the target for a ``:ref:`my label name```
reference from anywhere within the Zephyr documentation.
The heading text is shown when referencing this label.
You can also change the text that's displayed for this link, such as::

   :ref:`some other text <my label name>`


To enable easy cross-page linking within the site, each file should have
a reference label before its title so it can
be referenced from another file. These reference labels must be unique
across the whole site, so generic names such as "samples" should be
avoided.  For example the top of this document's .rst file is::

   .. _doc_guidelines:

   Documentation Guidelines for the Zephyr Project
   ###############################################


Other .rst documents can link to this document using the ``:ref:`doc_guidelines``` tag and
it will show up as :ref:`doc_guidelines`.  This type of internal cross reference works across
multiple files, and the link text is obtained from the document source so if the title changes,
the link text will update as well.

You can also define links to any URL and then reference it in your document.
For example, with this label definition in the document::

   .. _Zephyr Wikipedia Page:
      https://en.wikipedia.org/wiki/Zephyr_(operating_system)

you can reference it with::

   Read the `Zephyr Wikipedia Page`_ for more information about the
   project.

.. tip::

   When a document contains many external links, it can be useful to list them in a single
   "References" section at the end of the document. This can be done using the
   :rst:dir:`target-notes` directive. Example::

      References
      ==========

      .. target-notes::

      .. _external_link1: https://example.com
      .. _external_link2: https://example.org

Cross-referencing C documentation
=================================

.. rst:role:: c:member
              c:data
              c:var
              c:func
              c:macro
              c:struct
              c:union
              c:enum
              c:enumerator
              c:type

   You may use these roles to cross-reference the Doxygen documentation of C functions, macros,
   types, etc.

   They are rendered in the HTML output as links to the corresponding Doxygen documentation for the
   item. For example::

      Check out :c:function:`gpio_pin_configure` for more information.

   Will render as:

      Check out :c:func:`gpio_pin_configure` for more information.

   You may provide a custom link text, similar to the built-in :rst:role:`ref` role.

Visual Elements
***************

.. _doc_images:

Images
======

Images are included in the documentation by using an :rst:dir:`image` directive::

   .. image:: ../../images/doc-gen-flow.png
      :align: center
      :alt: alt text for the image

or if you'd like to add an image caption, use::

    .. figure:: ../../images/doc-gen-flow.png
       :alt: image description

       Caption for the figure

The file name specified is relative to the document source file,
and we recommend putting images into an ``images`` folder where the document
source is found.

The usual image formats handled by a web browser are supported: WebP, PNG, GIF,
JPEG, and SVG.

Keep the image size only as large as needed, generally at least 500 px wide but
no more than 1000 px, and no more than 100 KB unless a particularly large image
is needed for clarity.

Recommended image formats based on content
------------------------------------------

* **Screenshots**: WebP or PNG.
* **Diagrams**: Consider using Graphviz for simple diagrams (see
  `dedicated section <graphviz_diagrams>`_ below. If using an external tool, SVG is preferred.
* **Photos** (ex. boards): WebP. Use transparency if possible/available.


.. _graphviz_diagrams:

Graphviz
========

`Graphviz`_ is a tool for creating diagrams specified in a simple text language. As it's important
to allow for diagrams used in the documentation to be easily maintained, we encourage the use of
Graphviz for creating diagrams. Graphviz is particularly well suited for creating state diagrams, flow
charts, and other types of diagrams that can be expressed as a graph.

To include a Graphviz diagram in a document, use the :rst:dir:`graphviz` directive. For example::

   .. graphviz::

      digraph G {
         rankdir=LR;
         A -> B;
         B -> C;
         C -> D;
      }

Would render as:

   .. graphviz::

      digraph G {
         rankdir=LR;
         A -> B;
         B -> C;
         C -> D;
      }

Please refer to the `Graphviz documentation`_ for more information on how to create diagrams using
Graphviz's DOT language.

.. _Graphviz: https://graphviz.org
.. _Graphviz documentation: https://graphviz.org/documentation


Custom Sphinx Roles and Directives
**********************************

The Zephyr documentation uses custom Sphinx roles and directives to provide additional functionality
and to make it easier to write and maintain consistent documentation.

Application build commands
==========================

.. rst:directive:: .. zephyr-app-commands::

   Generate consistent documentation of the shell commands needed to manage (build, flash, etc.) an
   application

   For example, to generate commands to build ``samples/hello_world`` for ``qemu_x86`` use::

      .. zephyr-app-commands::
         :zephyr-app: samples/hello_world
         :board: qemu_x86
         :goals: build

   This will render as:

      .. zephyr-app-commands::
         :zephyr-app: samples/hello_world
         :board: qemu_x86
         :goals: build

   .. rubric::  Options

   .. rst:directive:option:: tool
      :type: string

      Which tool to use. Valid options are currently ``cmake``, ``west`` and ``all``.
      The default is ``west``.

   .. rst:directive:option:: app
      :type: string

      Path to the application to build.

   .. rst:directive:option:: zephyr-app
      :type: string

      Path to the application to build, this is an app present in the upstream zephyr repository.
      Mutually exclusive with ``:app:``.

   .. rst:directive:option:: cd-into
      :type: no value

      If set, build instructions are given from within the ``:app:`` folder, instead of outside of
      it.

   .. rst:directive:option:: generator
      :type: string

      Which build system to generate.

      Valid options are currently ``ninja`` and ``make``. The default is ``ninja``. This option is
      not case sensitive.

   .. rst:directive:option:: host-os

      Which host OS the instructions are for.

      Valid options are ``unix``, ``win`` and ``all``. The default is ``all``.

   .. rst:directive:option:: board
      :type: string

      If set, build commands will target the given board.

   .. rst:directive:option:: shield
      :type: string

      If set, build commands will target the given shield.

      Multiple shields can be provided in a comma separated list.

   .. rst:directive:option:: conf

      If set, build commands will use the given configuration file(s).

      If multiple configuration files are provided, enclose the space-separated list of files with
      double quotes, e.g., `"a.conf b.conf"`.

   .. rst:directive:option:: gen-args
      :type: string

      If set, indicates additional arguments to the CMake invocation.

   .. rst:directive:option:: build-args
      :type: string

      If set, indicates additional arguments to the build invocation.

   .. rst:directive:option:: west-args
      :type: string

      If set, additional arguments to the west invocation (ignored for ``:tool: cmake``).

   .. rst:directive:option:: flash-args
      :type: string

      If set, additional arguments to the flash invocation.

   .. rst:directive:option:: snippets
      :type: string

      If set, indicates the application should be compiled with the listed snippets.

      Multiple snippets can be provided in a comma separated list.

   .. rst:directive:option:: build-dir
      :type: string

      If set, the application build directory will *APPEND* this relative, Unix-separated, path to
      the standard build directory. This is mostly useful for distinguishing builds for one
      application within a single page.

   .. rst:directive:option:: build-dir-fmt
      :type: string

      If set, assume that `west config build.dir-fmt`` has been set to this path.

      Exclusive with ``:build-dir:`` and depends on ``:tool: west``.

   .. rst:directive:option:: goals
      :type: string

      A whitespace-separated list of what to do with the app (any of ``build``, ``flash``,
      ``debug``, ``debugserver``, ``run``).

      Commands to accomplish these tasks will be generated in the right order.

   .. rst:directive:option:: maybe-skip-config
      :type: no value

      If set, this indicates the reader may have already created a build directory and changed
      there, and will tweak the text to note that doing so again is not necessary.

   .. rst:directive:option:: compact
      :type: no value

      If set, the generated output is a single code block with no additional comment lines.


.. _linking_to_zephyr_files:

Cross-referencing files in the Zephyr tree
==========================================

Special roles are available to reference files in the Zephyr tree. For example, referencing this
very file can be done using the :rst:role:`zephyr_file` role, like this::

   Check out :zephyr_file:`doc/contribute/documentation/guidelines.rst` for more information.

This would render as:

   Check out :zephyr_file:`doc/contribute/documentation/guidelines.rst` for more information.

You may use the :rst:role:`zephyr_raw` role instead if you want to reference the "raw" content.

.. rst:role:: zephyr_file

   This role is used to reference a file in the Zephyr tree. For example::

      Check out :zephyr_file:`doc/contribute/documentation/guidelines.rst` for more information.

   Will render as:

      Check out :zephyr_file:`doc/contribute/documentation/guidelines.rst` for more information.

.. rst:role:: zephyr_raw

   This role is used to reference the raw content of a file in the Zephyr tree. For example::

      Check out :zephyr_raw:`doc/contribute/documentation/guidelines.rst` for more information.

   Will render as:

      Check out :zephyr_raw:`doc/contribute/documentation/guidelines.rst` for more information.

.. rst:role:: module_file

   This role is used to reference a module in the Zephyr tree. For example::

         Check out :module_file:`hal_stm32:CMakeLists.txt` for more information.

   Will render as:

         Check out :module_file:`hal_stm32:CMakeLists.txt` for more information.


Cross-referencing GitHub issues and pull requests
=================================================

.. rst:role:: github

   This role is used to reference a GitHub issue or pull request.

   For example, to reference issue #1234::

      Check out :github:`1234` for more background about this known issue.

   This will render as:

      Check out :github:`1234` for more background about this known issue.

Doxygen API documentation
=========================

.. app.add_directive("doxygengroup", DoxygenGroupDirective)
.. app.add_role_to_domain("c", "group", CXRefRole())

.. rst:directive:: .. doxygengroup:: name

   This directive is used to output a short description of a Doxygen group and a link to the
   corresponding Doxygen-generated documentation.

   All the code samples (declared using the :rst:dir:`zephyr:code-sample` directive) indicating the
   group as relevant will automatically be list and referenced in the rendered output.

   For example::

      .. doxygengroup:: can_interface

   Will render as:

      .. doxygengroup:: can_interface


   .. rubric:: Options

   .. rst:directive:option:: project
      :type: project name (optional)

      Associated Doxygen project. This can be useful when multiple Doxygen
      projects are configured.

.. rst:role:: c:group

   This role is used to reference a Doxygen group in the Zephyr tree. In the HTML documentation,
   they are rendered as links to the corresponding Doxygen-generated documentation for the group.
   For example::

      Check out :c:group:`gpio_interface` for more information.

   Will render as:

      Check out :c:group:`gpio_interface` for more information.

   You may provide a custom link text, similar to the built-in :rst:role:`ref` role.


Kconfig options
===============

If you want to reference a Kconfig option from a document, you can use the
:rst:role:`kconfig:option` role and provide the name of the option you want to reference. The role
will automatically generate a link to the documentation of the Kconfig option when building HTML
output.

Make sure to use the full name of the Kconfig option, including the ``CONFIG_`` prefix.

.. rst:role:: kconfig:option

   This role is used to reference a Kconfig option in the Zephyr tree. For example::

      Check out :kconfig:option:`CONFIG_GPIO` for more information.

   Will render as:

      Check out :kconfig:option:`CONFIG_GPIO` for more information.

Devicetree bindings
===================

If you want to reference a Devicetree binding from a document, you can use the
:rst:role:`dtcompatible` role and provide the compatible string of the binding you want to
reference. The role will automatically generate a link to the documentation of the binding when
building HTML output.

.. rst:role:: dtcompatible

   This role can be used inline to make a reference to the generated documentation for the
   Devicetree compatible given as argument.

   There may be more than one page for a single compatible. For example, that happens if a binding
   behaves differently depending on the bus the node is on. If that occurs, the reference points at
   a "disambiguation" page which links out to all the possibilities, similarly to how Wikipedia
   disambiguation pages work. Example::

      Check out :dtcompatible:`zephyr,input-longpress` for more information.

   Will render as:

      Check out :dtcompatible:`zephyr,input-longpress` for more information.

Code samples
============

.. rst:directive:: .. zephyr:code-sample:: id

   This directive is used to describe a code sample, including which noteworthy APIs it may be
   exercising.

   For example::

      .. zephyr:code-sample:: blinky
         :name: Blinky
         :relevant-api: gpio_interface

         Blink an LED forever using the GPIO API.

   The content of the directive is used as the description of the code sample.

   .. rubric:: Options

   .. rst:directive:option:: name
      :type: text

      Indicates the human-readable short name of the sample.

   .. rst:directive:option:: relevant-api
      :type: text

      Optional space-separated list of Doxygen group names that correspond to the APIs exercised
      by the code sample.

.. rst:role:: zephyr:code-sample

   This role is used to reference a code sample described using :rst:dir:`zephyr:code-sample`.

   For example::

      Check out :zephyr:code-sample:`blinky` for more information.

   Will render as:

      Check out :zephyr:code-sample:`blinky` for more information.

   This can be used exactly like the built-in :rst:role:`ref` role, i.e. you may provide a custom
   link text. For example::

      Check out :zephyr:code-sample:`blinky code sample <blinky>` for more information.

   Will render as:

      Check out :zephyr:code-sample:`blinky code sample <blinky>` for more information.

.. rst:directive:: .. zephyr:code-sample-category:: id

   This directive is used to define a category for grouping code samples.

   For example::

      .. zephyr:code-sample-category:: gpio
         :name: GPIO
         :show-listing:

         Samples related to the GPIO subsystem.

   The contents of the directive is used as the description of the category. It can contain any
   valid reStructuredText content.

   .. rubric:: Options

   .. rst:directive:option:: name
      :type: text

      Indicates the human-readable name of the category.

   .. rst:directive:option:: show-listing
      :type: flag

      If set, a listing of code samples in the category will be shown. The listing is automatically
      generated based on all code samples found in the subdirectories of the current document.

   .. rst:directive:option:: glob
      :type: text

      A glob pattern to match the files to include in the listing. The default is `*/*` but it can
      be overridden e.g. when samples may be found in directories not sitting directly under the
      category directory.

.. rst:role:: zephyr:code-sample-category

   This role is used to reference a code sample category described using
   :rst:dir:`zephyr:code-sample-category`.

   For example::

      Check out :zephyr:code-sample-category:`cloud` samples for more information.

   Will render as:

      Check out :zephyr:code-sample-category:`cloud` samples for more information.

.. rst:directive:: .. zephyr:code-sample-listing::

   This directive is used to show a listing of all code samples found in one or more categories.

   For example::

      .. zephyr:code-sample-listing::
         :categories: cloud

   Will render as:

      .. zephyr:code-sample-listing::
         :categories: cloud

   .. rubric:: Options

   .. rst:directive:option:: categories
      :type: text

      A space-separated list of category IDs for which to show the listing.

   .. rst:directive:option:: live-search
      :type: flag

      A flag to include a search box right above the listing. The search box allows users to filter
      the listing by code sample name/description, which can be useful for categories with a large
      number of samples. This option is only available in the HTML builder.

Boards
======

.. rst:directive:: .. zephyr:board:: name

   This directive is used at the beginning of a document to indicate it is the main documentation
   page for a board whose name is given as the directive argument.

   For example::

      .. zephyr:board:: wio_terminal

   The metadata for the board is read from various config files and used to automatically populate
   some sections of the board documentation. A board documentation page that uses this directive
   can be linked to using the :rst:role:`zephyr:board` role.

.. rst:role:: zephyr:board

   This role is used to reference a board documented using :rst:dir:`zephyr:board`.

   For example::

      Check out :zephyr:board:`wio_terminal` for more information.

   Will render as:

      Check out :zephyr:board:`wio_terminal` for more information.

.. rst:directive:: .. zephyr:board-catalog::

   This directive is used to generate a catalog of Zephyr-supported boards that can be used to
   quickly browse the list of all supported boards and filter them according to various criteria.


References
**********

.. target-notes::
