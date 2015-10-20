.. _code_examples:

Code Examples
*************

Collaborating to the Zephyr Kernel is all about code. Therefore, your
documentation must include code examples. The code examples can be
written directly in the documentation or included from a source file.
Use these guidelines to insert code blocks to your documentation:

* Include code examples from a source file . Only write the code
  example directly into the documentation if the example is less than 10
  lines long or not part of the code base of the Zephyr Kernel.

* Use the ``:lineos:`` option of the directives to add line numbers to
  your example if it is larger than 12 lines.

* Specify the programing language of your example. Not only will it
  add syntax highlighting but it also allows the reader to identify code
  efficiently. Use bash for console commands, asm for assembly code and
  c for C code.

* Treat all console commands that a user must use as code examples.

Examples
--------

This is a code example included from a file in another directory. Only
certain lines of the source file are included. Those lines of code are
renumbered and the 7th, 11th and 13th lines are highlighted.

.. code-block:: rst

   .. literalinclude:: ../code/doxygen/hello_commented.c
      :language: c
      :lines: 97-110
      :emphasize-lines: 7, 11, 13
      :linenos:

Renders as:

.. literalinclude:: ../code/doxygen/hello_commented.c
   :language: c
   :lines: 97-110
   :emphasize-lines: 7, 11, 13
   :linenos:

This is an example of a series of console commands. Line numbering is
not required. It is only required to specify that these are commands
by using bash as a programing language.

.. code-block:: rst

   .. code-block:: bash

      $ mkdir ${HOME}/x86-build

      $ mkdir ${HOME}/arm-build

      $ mkdir ${HOME}/cross-src

Renders as:

.. code-block:: bash

   $ mkdir ${HOME}/x86-build

   $ mkdir ${HOME}/arm-build

   $ mkdir ${HOME}/cross-src

Finally, this is a code example that is not part of the Zephyr Kernel's
code base. It is not even valid code but it is used to illustrate a
concept.

.. code-block:: rest

   .. code-block:: c

      static NANO_CPU_INT_STUB_DECL (deviceStub);

      void deviceDriver (void)

      {

      .
      .
      .

      nanoCpuIntConnect (deviceIRQ, devicePrio, deviceIntHandler,
      deviceStub);

      .
      .
      .

      }

Renders as:

.. code-block:: c

   static NANO_CPU_INT_STUB_DECL (deviceStub);

   void deviceDriver (void)

   {

   .
   .
   .

   nanoCpuIntConnect (deviceIRQ, devicePrio, deviceIntHandler,
   deviceStub);

   .
   .
   .

   }


Templates
---------

We have included templates for a basic ``.. code-block::`` directive
and for a ``.. literalinclude::`` directive.

Use ``code-block`` for console commands, brief examples and examples
outside the code base of the Zephyr Kernel.

.. code-block:: rst

   .. code-block:: language

      source

Use ``litteralinclude`` to insert code from a source file. Keep in
mind that you can include the entire contents of the file or just
specific lines.

.. code-block:: rst

   .. literalinclude:: ../path/to/file/file_name.c
      :language: c
      :lines: 5-30, 32, 70-100
      :emphasize-lines: 3
      :linenos:

.. caution::
   The ``:emphasize-lines:`` option uses the line numbering provided
   by ``:lineos:``. The emphasized line in the template will be the
   third one of the example but the eighth one of the source file.
