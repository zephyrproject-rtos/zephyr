.. _troubleshooting:

Troubleshooting obscure warnings from Doxygen / Sphinx
######################################################

Some code construct are problematic and
push the limits of the documentation scanners. Here are some common
issues and the solutions to them:

WARNING: Error when parsing function declaration.
*************************************************

See :ref:`functions <functions>` and :ref:`function
typedefs <function_definitions>`.

WARNING: Invalid definition: Expected end of definition. [error at 12] ... SOMETYPE.__unnamed__
***************************************************************************************************

This might be one of:

- :ref:`Unnamed structs <unnamed_structs>`

- :ref:`Non-anonymous unnamed structs <unnamed_structs_var>`


WARNING: documented symbol `XYZ` was not declared or defined
************************************************************

This happens when there is a documentation block for a function with
*@fn* (or *@struct*, *@var*, etc ... ) but then the parser can't find
the actual C definition/declaration of that function.

Check for mispellings in the documentation block vs the defines, and
if the parser is being told to look for the C code where the
definition/declaration is supposed to be (:file:`doc/doxygen.config`).



WARNING: unknown option: SOMECONFIGOPTION (or something else in caps)
*********************************************************************

This comes when using:

.. code-block:: rest

   ... when you enable :option:`SOMECONFIGOPTION`, the system...

to document a config option that is declared with:

.. code-block:: rest

   .. option:: CONFIG_SOMECONFIGOPTION

The fix is to refer to ``CONFIG_SOMECONFIGOPTION``:

.. code-block:: rest

   ... when you enable :option:`CONFIG_SOMECONFIGOPTION`, the system...


If you want to document a configuration parameter, you have to declare
it first :literal:`.. config: NAME` and then refer to it. But if you
won't declare it, just name it as:

.. code-block:: rest

   ... when you enable ``NAME``, the system ...

WARNING: unknown option: SOMETHING=y
====================================

Someone is trying to document a config option when set to *yes* versus
*no* or similar. Usually looks like:

.. code-block:: rest

  ... when :option:`CONFIG_SOMETHING=y`, then the system will bla...

change to:

.. code-block:: rest

  ... when :option:`CONFIG_SOMETHING`\=y, then the system will bla...


WARNING: undefined label: config_something (if the link has no caption the label must precede a section header)
***************************************************************************************************************

``CONFIG_SOMETHING`` is not defined in any :literal:`.. option::
CONFIG_SOMETHING` block, which means it probably doesn't exist in any
``KConfig`` file. Verify if it is a valid config option.

...doc/reference/kconfig/CONFIG_SOMETHING.rst:NN: WARNING: Definition list ends without a blank line; unexpected unindent
*************************************************************************************************************************

This usually originates from the help text in a Kconfig option which
is not laid out properly.

For example::

  config  FAULT_DUMP
          int
          prompt "Fault dump level"
          default 2
          range 0 2
          help
            Different levels for display information when a fault occurs.

            2: The default. Display specific and verbose information. Consumes
               the most memory (long strings).
            1: Display general and short information. Consumes less memory
               (short strings).
            0: Off.

The ReST parser will be confused by the lack of blank lines between
the ``2``, ``1`` and ``0`` items, so help him by adding bullets and
spacing the lines::

  config  FAULT_DUMP
          int
          prompt "Fault dump level"
          default 2
          range 0 2
          help
          Different levels for display information when a fault occurs.

          - 2: The default. Display specific and verbose
               information. Consumes the most memory (long strings).

          - 1: Display general and short information. Consumes less
               memory (short strings).

          - 0: Off.


WARNING: Unparseable C++ cross-reference: u'struct somestruct'
**************************************************************

Usually followed by::

  Invalid definition: Expected identifier in nested name, got keyword: struct [error at 6]
    struct somestruct
      ------^

this probably means someone is trying to refer to a C symbol as C++;
look for:

.. code-block:: rest

  ...use the datatype :cpp:type:`struct somestruct` for doing...

and replace with:

.. code-block:: rest

  ...use the datatype :c:type:`struct somestruct` for doing...

FILE.rst:: WARNING: document isn't included in any toctree
**********************************************************

This usually happens when you include a file inside another instead of
sorting them with a TOC tree:

 - double check: is this really necessary?
 - add :literal:`:orphan:` as the very first line of the file to get
   rid of this warning.

I have a set of functions with the same parameters and I am too lazy to type
****************************************************************************

Use *@copydetails*:

.. code-block:: c

   /**
    * @copydetails FUNCTION_1
    *
    * This does the same as FUNCTION_1 but also sommersaults.
    */


The API documentation is missing a term or link to it when I use @ref term.
****************************************************************************

When you use an :literal:`@ref term` in your doxygen comment, there must be
a corresponding definition of :literal:`term` somewhere in the system.  If
there isn't, then there can't be a link to that term's defintion.  Make sure
you've spelled :literal:`term` correctly and there is whitespace after the
term. Using :literal:`@ref term.` (at the end of a sentence for example) won't
work so add a space like this: :literal:`@ref term .`
