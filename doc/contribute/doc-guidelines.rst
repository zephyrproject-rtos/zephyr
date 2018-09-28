.. _doc_guidelines:

Documentation Guidelines for the Zephyr Project
###############################################

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

If you have a long bullet list of items, where each item is short,
you can indicate the list items should be rendered in multiple columns
with a special ``hlist`` directive::

   .. hlist::
      :columns: 3

      * A list of
      * short items
      * that should be
      * displayed
      * horizontally
      * so it doesn't
      * use up so much
      * space on
      * the page

This would be rendered as:

.. hlist::
   :columns: 3

   * A list of
   * short items
   * that should be
   * displayed
   * horizontally
   * so it doesn't
   * use up so much
   * space on
   * the page

Note the optional ``:columns:`` parameter (default is two columns), and
all the list items are indented by three spaces.

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

.. _internal-linking:

Internal Cross-Reference Linking
********************************

ReST links are only supported within the current file using the
notation::

   refer to the `internal-linking`_ page

which renders as,

   refer to the `internal-linking`_ page

Note the use of a trailing
underscore to indicate an outbound link. In this example, the label was
added immediately before a heading, so the text that's displayed is the
heading text itself.

With Sphinx however, we can create
link-references to any tagged text within the Zephyr Project documentation.

Target locations within documents are defined with a label directive:

   .. code-block:: rst

      .. _my label name:

Note the leading underscore indicating an inbound link.
The content immediately following
this label is the target for a ``:ref:`my label name```
reference from anywhere within the Zephyr documentation.
The label should be added immediately before a heading so there's a
natural phrase to show when referencing this label (e.g., the heading
text).

This is the same directive used to
define a label that's a reference to a URL::

   .. _Zephyr Wikipedia Page:
      https://en.wikipedia.org/wiki/Zephyr_(operating_system)


To enable easy cross-page linking within the site, each file should have
a reference label before its title so it can
be referenced from another file. These reference labels must be unique
across the whole site, so generic names such as "samples" should be
avoided.  For example the top of this document's.rst file is:


.. code-block:: rst

   .. _doc_guidelines:

   Documentation Guidelines for the Zephyr Project
   ###############################################


Other .rst documents can link to this document using the ``:ref:`doc_guidelines``` tag and
it will show up as :ref:`doc_guidelines`.  This type of internal cross reference works across
multiple files, and the link text is obtained from the document source so if the title changes,
the link text will update as well.


Non-ASCII Characters
********************

You can insert non-ASCII characters such as a Trademark symbol (|trade|),
by using the notation ``|trade|``.
Available replacement names are defined in an include file used during the Sphinx processing
of the reST files.  The names of these replacement characters are the same as used in HTML
entities used to insert characters in HTML, e.g., \&trade; and are defined in the
file ``sphinx_build/substitutions.txt`` as listed here:

.. literalinclude:: ../substitutions.txt
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

      struct _k_object {
         char *name;
         u8_t perms[CONFIG_MAX_THREAD_BYTES];
         u8_t type;
         u8_t flags;
         u32_t data;
      } __packed;

Note the blank line between the ``code-block`` directive and the first
line of the code-block body, and the body content is indented three
spaces (to the first non-white space of the directive name).

This would be rendered as:

   .. code-block:: c

      struct _k_object {
         char *name;
         u8_t perms[CONFIG_MAX_THREAD_BYTES];
         u8_t type;
         u8_t flags;
         u32_t data;
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

.. include:: ../extensions/zephyr/application.py
   :start-line: 10
   :start-after: '''
   :end-before: '''

For example, the ``.. zephyr-app-commands`` listed above would
render like this in the generated HTML output:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: qemu_x86
   :goals: build
