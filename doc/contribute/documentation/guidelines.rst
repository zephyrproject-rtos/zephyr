.. _doc_guidelines:

Documentation Guidelines
########################

.. note::

   For instructions on building the documentation, see :ref:`zephyr_doc`.

Zephyr Project content is written using the `reStructuredText`_ markup
language (.rst file extension) with Sphinx extensions, and processed
using Sphinx to create a formatted standalone website.  Developers can
view this content either in its raw form as .rst markup files, or (with
Sphinx installed) they can build the documentation using the Makefile
on Linux systems, or make.bat on Windows, to
generate the HTML content. The HTML content can then be viewed using a
web browser. This same .rst content is also fed into the
`Zephyr documentation`_ website (with a different theme applied).

You can read details about `reStructuredText`_
and about `Sphinx extensions`_ from their respective websites.

.. _Sphinx extensions: http://www.sphinx-doc.org/en/stable/contents.html
.. _reStructuredText: http://docutils.sourceforge.net/docs/ref/rst/restructuredtext.html
.. _Sphinx Inline Markup:  http://sphinx-doc.org/markup/inline.html#inline-markup
.. _Zephyr documentation:  https://docs.zephyrproject.org

This document provides a quick reference for commonly used reST and
Sphinx-defined directives and roles used to create the documentation
you're reading.

Headings
********

While reST allows use of both and overline and matching underline to
indicate a heading, we only use an underline indicator for headings.

* Document title (h1) use "#" for the underline character
* First section heading level (h2) use "*"
* Second section heading level (h3) use "="
* Third section heading level (h4) use "-"

The heading underline must be at least as long as the title it's under.

For example::

   This is a title heading
   #######################

   some content goes here

   First section heading
   *********************


Content Highlighting
********************

Some common reST inline markup samples:

* one asterisk: ``*text*`` for emphasis (*italics*),
* two asterisks: ``**text**`` for strong emphasis (**boldface**), and
* two backquotes: ````text```` for ``inline code`` samples.

If asterisks or backquotes appear in running text and could be confused with
inline markup delimiters, you can eliminate the confusion by adding a
backslash (``\``) before it.

Lists
*****

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
******************

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
******

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

File names and Commands
***********************

Sphinx extends reST by supporting additional inline markup elements (called
"roles") used to tag text with special
meanings and allow style output formatting. (You can refer to the `Sphinx Inline Markup`_
documentation for the full list).

For example, there are roles for marking :file:`filenames`
(``:file:`name```) and command names such as :command:`make`
(``:command:`make```).  You can also use the \`\`inline code\`\`
markup (double backticks) to indicate a ``filename``.

For references to files that are in the Zephyr GitHub tree, a special
role can be used that creates a hyperlink to that file.  For example a
reference to the reST file used to create this document can be generated
using ``:zephyr_file:`doc/contribute/documentation/index.rst```  that will
show up as :zephyr_file:`doc/contribute/documentation/index.rst`, a link to
the "blob" file in the github repo.  There's also a
``:zephyr_raw:`doc/guides/documentation/index.rst``` role that will
link to the "raw" content,
:zephyr_raw:`doc/contribute/documentation/index.rst`. (You can click on
these links to see the difference.)

.. _internal-linking:

Internal Cross-Reference Linking
********************************

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


External Cross-Reference Linking
********************************

With Sphinx's help, we can create
link-references to any tagged text within the Zephyr Project documentation.

Target locations in a document are defined with a label directive:

   .. code-block:: rst

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
avoided.  For example the top of this document's .rst file is:


.. code-block:: rst

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


\`any\` links
*************

Within the Zephyr project, we've defined the default role to be "any",
meaning if you just write a phrase in back-ticks, e.g.,
```doc_guidelines```, Sphinx will search through all domains looking for
something called doc_guidelines to link to. In this case it will find
the label at the top of this document, and link to `doc_guidelines`.
This can be useful for linking to doxygen-generated links for function
names and such, but will cause a warning such as::

   WARNING: 'any' reference target not found: doc_giudelines

if you misspelled ```doc_guidelines``` as ```doc_giudelines```.

Non-ASCII Characters
********************

You can insert non-ASCII characters such as a Trademark symbol (|trade|),
by using the notation ``|trade|``.
Available replacement names are defined in an include file used during the Sphinx processing
of the reST files.  The names of these replacement characters are the same as used in HTML
entities used to insert characters in HTML, e.g., \&trade; and are defined in the
file ``sphinx_build/substitutions.txt`` as listed here:

.. literalinclude:: ../../substitutions.txt
   :language: rst

We've kept the substitutions list small but others can be added as
needed by submitting a change to the ``substitutions.txt`` file.

Code and Command Examples
*************************

Use the reST ``code-block`` directive to create a highlighted block of
fixed-width text, typically used for showing formatted code or console
commands and output.  Smart syntax highlighting is also supported (using the
Pygments package). You can also directly specify the highlighting language.
For example::

   .. code-block:: c

      struct z_object {
         char *name;
         uint8_t perms[CONFIG_MAX_THREAD_BYTES];
         uint8_t type;
         uint8_t flags;
         uint32_t data;
      } __packed;

Note the blank line between the ``code-block`` directive and the first
line of the code-block body, and the body content is indented three
spaces (to the first non-white space of the directive name).

This would be rendered as:

   .. code-block:: c

      struct z_object {
         char *name;
         uint8_t perms[CONFIG_MAX_THREAD_BYTES];
         uint8_t type;
         uint8_t flags;
         uint32_t data;
      } __packed;


You can specify other languages for the ``code-block`` directive,
including ``c``, ``python``, and ``rst``, and also ``console``,
``bash``, or ``shell``. If you want no syntax highlighting, use the
language ``none``,  for example::

   .. code-block:: none

      This would be a block of text styled with a background
      and box, but with no syntax highlighting.

Would display as:

   .. code-block:: none

      This would be a block of text styled with a background
      and box, but with no syntax highlighting.

There's a shorthand for writing code blocks too: end the introductory
paragraph with a double colon (``::``) and indent the code block content
by three spaces.  On output, only one colon will be shown.  The
highlighting package makes a best guess at the type of content in the
block and highlighting purposes.

Images
******

Images are included in documentation by using an image directive::

   .. image:: ../../images/doc-gen-flow.png
      :align: center
      :alt: alt text for the image

or if you'd like to add an image caption, use::

    .. figure:: ../../images/doc-gen-flow.png
       :alt: image description

       Caption for the figure

The file name specified is relative to the document source file,
and we recommend putting images into an ``images`` folder where the document
source is found.  The usual image formats handled by a web browser are
supported: JPEG, PNG, GIF, and SVG.  Keep the image size only as large
as needed, generally at least 500 px wide but no more than 1000 px, and
no more than 250 KB unless a particularly large image is needed for
clarity.

Tabs, spaces, and indenting
***************************

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

Keep the line length for documentation less than 80 characters to make
it easier for reviewing in GitHub. Long lines because of URL references
are an allowed exception.

zephyr-app-commands Directive
*****************************

.. include:: ../../_extensions/zephyr/application.py
   :start-line: 10
   :start-after: '''
   :end-before: '''

For example, the ``.. zephyr-app-commands`` listed above would
render like this in the generated HTML output:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: qemu_x86
   :goals: build

Alternative Tabbed Content
**************************

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
<https://github.com/djungelorm/sphinx-tabs>`_ extension that's included
in the Zephyr setup.  Within a tab, you can have most any content *other
than a heading* (code-blocks, ordered and unordered lists, pictures,
paragraphs, and such).  You can read more about sphinx-tabs from the
link above.

Instruction Steps
*****************

Also introduced in the :ref:`getting_started` is a style that makes it
easy to create tutorial guides with clearly identified steps. Add
the ``.. rst-class:: numbered-step`` directive immediately before a
second-level heading (by project convention, a heading underlined with
asterisks ``******``, and it will be displayed as a numbered step,
sequentially numbered within the document.  For example::

   .. rst-class:: numbered-step

   Put your right hand in
   **********************

.. rst-class:: numbered-step

Put your right hand in
**********************

See the :zephyr_raw:`doc/develop/getting_started/index.rst` source file and
compare with the :ref:`getting_started` to see a full example.  As implemented,
only one set of numbered steps is intended per document.

For instructions on building the documentation, see :ref:`zephyr_doc`.
