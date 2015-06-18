.. _rest_tables:

Tables
******

Tables must only be used for information that is either too numerous or too
related for a list to be appropriate. The general rule of thumb is that the
minimal size of a table is a 2x2 not counting the table header. The |project|
's use of ReST makes table size much more important. Tables are difficult to
do properly in ReST following the line length restrictions. If you plan on
adding content in form of a table consider transforming it into a list
before you embark on creating a table. Follow these general guidelines:

* Use tables sparingly.

* Keep the 72-78 characters line length in mind.

* Indent the contents correctly. This allows the content to be read even if
it is not rendered.

* Only create a table if the body of the table is larger than 6, that means
either 2x3 or 3x2.



ReST supports two types of tables: simple and grid tables. Single tables are
only suited for very short content since they only allow cells with a single
line. Grid tables support multi-lined cells and offer many layout options.

Use simple tables only to create a matrix of terms, for example:

============ =========== ===========
Number       Name        Color
============ =========== ===========
1            Door        Brown
2            Floors      Gray
3            Ceilings    Blue
============ =========== ===========

Use the following template to create a simple table.

.. code-block:: rst

   ======== ========
   heading1 heading2
   ======== ========
   row1x1   row2x1
   row1x2   row2x2
   row1x3   row2x3
   ======== ========

The template renders as:

======== ========
heading1 heading2
======== ========
row1x1   row2x1
row1x2   row2x2
row1x3   row2x3
======== ========

The use of '=' between the headings and the rows in simple tables tags the
content of the headings as the table header. Do not add emphasis to the
contents of the table header using \*\*.

Use grid tables for all other scenarios involving tables. For example:

+-----------------+------------------------+--------------+------------+
| Name            | Purpose                | Known        | References |
| (or brand name) |                        | Applications |            |
+=================+========================+==============+============+
| Super Glue      | Glues things together  | Small car    | Quick Fix, |
|                 | with extra strength.   | repairs.     |  2010.     |
+-----------------+------------------------+--------------+------------+
| Masking Tape    | Stops paint from       | Painting     | Master     |
|                 | covering a surface     | walls.       | Painter,   |
|                 | allowing for sharp     |              | 2007.      |
|                 | edges.                 |              |            |
+-----------------+------------------------+--------------+------------+

This template can help you create grid tables:

.. code-block:: rst

   +------------------------+------------+----------+----------+
   | Header row, column 1   | Header 2   | Header 3 | Header 4 |
   | (header rows optional) |            |          |          |
   +========================+============+==========+==========+
   | body row 1, column 1   | column 2   | column 3 | column 4 |
   +------------------------+------------+----------+----------+
   | body row 2             | ...        | ...      |          |
   +------------------------+------------+----------+----------+

The use of '=' in grid tables is used to separate the table header from the
table body. Do not add emphasis to the contents of the table header using
\*\*.