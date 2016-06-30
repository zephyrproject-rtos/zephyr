.. _structs:

Structure Documentation
#######################

Structures, or structs for short, require very little documentation,
and it's best to document them wherever they are defined.
Structs require only a brief description detailing why they are needed.
Each variable that composes a struct needs a comment contained within
the correct syntax. A fully simplified syntax is therefore appropriate.

.. note::

   Follow the same rules as struct when documenting an enum.
   Use the simplified syntax to add the brief description.

Structure Comments Template
***************************
Structs have a simplified template:

.. literalinclude:: ex_struct_pre.c
   :language: c
   :lines: 1-11
   :emphasize-lines: 8
   :linenos:

Doxygen does not require any commands to recognize the different comments.
It does, however, require that line 8 be left blank.

.. _unnamed_structs:

Unnamed structures or unions
****************************

The *Sphinx* parser seems to get confused by unnamed structures, used
specially in nested unions / structs:

.. code-block:: c

  struct sensor_value {
          enum sensor_value_type type;
          union {
                  struct {
                          int32_t val1;
                          int32_t val2;
                  };
                  double dval;
          };
  };

This will likely generate an error such as::

  doc/api/io_interfaces.rst:14: WARNING: Invalid definition: Expected identifier in nested name. [error at 0]

    ^
  doc/api/io_interfaces.rst:14: WARNING: Invalid definition: Expected identifier in nested name. [error at 0]

    ^
  doc/api/io_interfaces.rst:14: WARNING: Invalid definition: Expected end of definition. [error at 12]
    sensor_value.__unnamed__
    ------------^

There is no really good workaround we can use, other than live with
the warning and ignore it. As well, because of this, the documentation
of the members doesn't really work yet.

The issue reported to developers in
https://github.com/sphinx-doc/sphinx/issues/2683.

When running into this issue, the member documentation has to be done
with *@param* indicators, otherwise they won't be extracted:

.. code-block:: c

  /**
   * @brief UART device configuration.
   *
   * @param port Base port number
   * @param base Memory mapped base address
   * @param regs Register address
   * @param sys_clk_freq System clock frequency in Hz
   */
  struct uart_device_config {
        union {
                uint32_t port;
                uint8_t *base;
                uint32_t regs;
        };

        uint32_t sys_clk_freq;
  ...

.. _unnamed_structs_var:

Unnamed structures or unions which declare a variable
-----------------------------------------------------

A special case of this is non-anynoymous unnamed structs/unions, where
a variable is defined. In this case, the workaround is much more
simple. Consider:

.. code-block:: c

   union dev_config {
           uint32_t raw;
	   struct {
                   uint32_t        use_10_bit_addr : 1;
		   uint32_t        speed : 3;
		   uint32_t        is_master_device : 1;
		   uint32_t        is_slave_read : 1;
		   uint32_t        reserved : 26;
	   } bits;
   };

This will likely generate an error such as::

  doc/api/io_interfaces.rst:28: WARNING: Invalid definition: Expected identifier in nested name. [error at 19]
  struct dev_config::@61  dev_config::bits
  -------------------^

The simple solution is to name that unnamed struct, with an internal
name (such as *__bits*):

.. code-block:: c

   union dev_config {
           uint32_t raw;
	   struct __bits {
                   uint32_t        use_10_bit_addr : 1;
		   uint32_t        speed : 3;
		   uint32_t        is_master_device : 1;
		   uint32_t        is_slave_read : 1;
		   uint32_t        reserved : 26;
	   } bits;
   };


Structure Documentation Example
*******************************

Correct:

.. literalinclude:: irq_test_common_commented.h
   :language: c
   :lines: 102-110
   :emphasize-lines: 6
   :linenos:

Make sure to start every comment with
:literal:`/**` and end it with :literal:`*/`. Every comment must start
with a capital letter and end with a period.

Doxygen requires that line 6 is left blank. Ensure a blank line
separates each group of variable and comment.

Incorrect:


.. literalinclude:: irq_test_common_wrong.h
   :language: c
   :lines: 1-7
   :emphasize-lines: 2
   :linenos:

The struct has no documentation. Developers that want to expand its
functionality have no way of understanding why the struct is defined
this way, nor what its components are.
