.. _inline:

Inline Markup
*************

Sphinx supports a large number of inline markup elements. The
documentation of the Zephyr OS only requires the use of the inline
markup found here. Additional inline markup can be used as long as it
is supported by Sphinx. Please refer to the `Sphinx Inline Markup`_
documentation for the full list of supported inline markup. Use the
following markup for every instance you encounter unless otherwise
specified.

* Use the 'abbreviation' markup to define an acronym or an initialism.
The abbreviation only needs to be added once per module. Ensure that
any acronym or initialism used in a module has been defined at least
once, otherwise the submission will not be accepted.

   :abbr:`API (Application Program Interface)`

   ``:abbr:`TIA (This Is an Abbreviation)```

* Use the 'command' markup only when the name of a specific command is
used as part of a paragraph for emphasis. Use the ``.. code-block::``
directive to supply full actionable commands as part as a series of
steps.

   :command:`make`

   ``:command:`command```

* Use the 'option' markup to emphasize the name of a command option
with or without its value. This markup is usually employed in
combination with the 'command' markup.

   :option:`-f`
   :option:`--all`
   :option:`-o output.xsl`
   The :command:`pandoc` command can be used without the :option:`-o`
   option, creating an output file with the same name as the source
   but a different extension.

   ``:option:`Option```

* Use the 'file' markup to emphasize a filename or directory. Do not
use the markup inside a code-block but use it inside all notices that
contain files or directories.

   :file:`collaboration.rst` :file:`doc/collaboration/figures`

   ``:file:`filename.ext` :file:`path/or/directory```

* Use the 'guilabel' markup to emphasize the elements in a graphic
user interface within a description. It replaces the use of quotes
when referring to windows' names, button labels, options or single
menu elements. Always follow the marked element with the appropriate
noun.

   In the :guilabel:`Tools` menu.
   Press the :guilabel:`OK` button.
   In the :guilabel:`Settings` window you find the :guilabel:`Hide
   Content` option.

   ``:guilabel:`UI-Label```

* Use the 'menuselection' markup to indicate the navigation through a
menu ending with a selection. Every 'menuselection' element can have
up to two menu steps before the selected item. If more than two steps
are required, it can be combined with a 'guilabel' or with another
'menuselection' element.

   :menuselection:`File --> Save As --> PDF`
   Go to :guilabel:`File` and select :menuselection:`Import --> Data
   Base --> MySQL`.
   Go to :menuselection:`Window --> View` and select :menuselection:`
   Perspective --> Other --> C++`

   ``:menuselection:`1stMenu --> 2ndMenu --> Selection```

* Use the 'makevar' markup to emphasize the name of a Makefile variable
. The markup can include only the name of the variable or the variable
plus its value.

   :makevar:`BSP`
   :makevar:`BSP=generic_pc`

   ``:makevar:`VARIABLE```

* Use the 'envvar' markup to emphasize the name of environment
variables. Just as with 'makevar', the markup can include only for the
name of the variable or the variable plus its value.

   :envvar:`ZEPHYR_BASE`
   :envvar:`QEMU_BIN_PATH=/usr/local/bin`

   ``:envvar:`ENVIRONMENT_VARIABLE```

.. _Sphinx Inline Markup:
   http://sphinx-doc.org/markup/inline.html#inline-markup