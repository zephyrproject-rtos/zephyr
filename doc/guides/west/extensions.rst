.. _west-extensions:

Extensions
##########

West is "pluggable": you can add your own commands to west without editing its
source code. These are called **west extension commands**, or just "extensions"
for short. Extensions show up in the ``west --help`` output in a special
section for the project which defines them. This page provides general
information on west extension commands, and has a tutorial for writing your
own.

Some commands you can run when using west with Zephyr, like the ones used to
:ref:`build, flash, and debug <west-build-flash-debug>` and the
:ref:`ones described here <west-zephyr-ext-cmds>` , are extensions. That's why
help for them shows up like this in ``west --help``:

.. code-block:: none

   commands from project at "zephyr":
     completion:           display shell completion scripts
     boards:               display information about supported boards
     build:                compile a Zephyr application
     sign:                 sign a Zephyr binary for bootloader chain-loading
     flash:                flash and run a binary on a board
     debug:                flash and interactively debug a Zephyr application
     debugserver:          connect to board and launch a debug server
     attach:               interactively debug a board

See :file:`zephyr/scripts/west-commands.yml` and the
:file:`zephyr/scripts/west_commands` directory for the implementation details.

Disabling Extension Commands
****************************

To disable support for extension commands, set the ``commands.allow_extensions``
:ref:`configuration <west-config>` option to ``false``. To set this
globally for whenever you run west, use:

.. code-block:: console

   west config --global commands.allow_extensions false

If you want to, you can then re-enable them in a particular installation with:

.. code-block:: console

   west config --local commands.allow_extensions true

Note that the files containing extension commands are not imported by west
unless the commands are explicitly run. See below for details.

Adding a West Extension
***********************

There are three steps to adding your own extension:

#. Write the code implementing the command.
#. Add information about it to a :file:`west-commands.yml` file.
#. Make sure the :file:`west-commands.yml` file is referenced in the
   :term:`west manifest`.

Note that west ignores extension commands whose names are the same as a
built-in command.

Step 1: Implement Your Command
==============================

Create a Python file to contain your command implementation (see the "Meta >
Requires" information on the `west PyPI page`_ for details on the currently
supported versions of Python). You can put it in anywhere in any project
tracked by your :term:`west manifest`, or the manifest repository itself.
This file must contain a subclass of the ``west.commands.WestCommand`` class;
this class will be instantiated and used when your extension is run.

Here is a basic skeleton you can use to get started. It contains a subclass of
``WestCommand``, with implementations for all the abstract methods. For more
details on the west APIs you can use, see :ref:`west-apis`.

.. code-block:: py

   '''my_west_extension.py

   Basic example of a west extension.'''

   from textwrap import dedent            # just for nicer code indentation

   from west.commands import WestCommand  # your extension must subclass this
   from west import log                   # use this for user output

   class MyCommand(WestCommand):

       def __init__(self):
           super().__init__(
               'my-command-name',  # gets stored as self.name
               'one-line help for what my-command-name does',  # self.help
               # self.description:
               dedent('''
               A multi-line description of my-command.

               You can split this up into multiple paragraphs and they'll get
               reflowed for you. You can also pass
               formatter_class=argparse.RawDescriptionHelpFormatter when calling
               parser_adder.add_parser() below if you want to keep your line
               endings.'''))

       def do_add_parser(self, parser_adder):
           # This is a bit of boilerplate, which allows you full control over the
           # type of argparse handling you want. The "parser_adder" argument is
           # the return value of an argparse.ArgumentParser.add_subparsers() call.
           parser = parser_adder.add_parser(self.name,
                                            help=self.help,
                                            description=self.description)

           # Add some example options using the standard argparse module API.
           parser.add_argument('-o', '--optional', help='an optional argument')
           parser.add_argument('required', help='a required argument')

           return parser           # gets stored as self.parser

       def do_run(self, args, unknown_args):
           # This gets called when the user runs the command, e.g.:
           #
           #   $ west my-command-name -o FOO BAR
           #   --optional is FOO
           #   required is BAR
           log.inf('--optional is', args.optional)
           log.inf('required is', args.required)

You can ignore the second argument to ``do_run()`` (``unknown_args`` above), as
``WestCommand`` will reject unknown arguments by default. If you want to be
passed a list of unknown arguments instead, add ``accepts_unknown_args=True``
to the ``super().__init__()`` arguments.

Step 2: Add or Update Your :file:`west-commands.yml`
====================================================

You now need to add a :file:`west-commands.yml` file to your project which
describes your extension to west.

Here is an example for the above class definition, assuming it's in
:file:`my_west_extension.py` at the project root directory:

.. code-block:: yaml

   west-commands:
     - file: my_west_extension.py
       commands:
         - name: my-command-name
           class: MyCommand
           help: one-line help for what my-command-name does

The top level of this YAML file is a map with a ``west-commands`` key.  The
key's value is a sequence of "command descriptors".  Each command descriptor
gives the location of a file implementing west extensions, along with the names
of those extensions, and optionally the names of the classes which define them
(if not given, the ``class`` value defaults to the same thing as ``name``).

Some information in this file is redundant with definitions in the Python code.
This is because west won't import :file:`my_west_extension.py` until the user
runs ``west my-command-name``, since:

- It allows users to run ``west update`` with a manifest from an untrusted
  source, then use other west commands without your code being imported along
  the way. Since importing a Python module is shell-equivalent, this provides
  some peace of mind.

- It's a small optimization, since your code will only be imported if it is
  needed.

So, unless your command is explicitly run, west will just load the
:file:`west-commands.yml` file to get the basic information it needs to display
information about your extension to the user in ``west --help`` output, etc.

If you have multiple extensions, or want to split your extensions across
multiple files, your :file:`west-commands.yml` will look something like this:

.. code-block:: yaml

   west-commands:
     - file: my_west_extension.py
       commands:
         - name: my-command-name
           class: MyCommand
           help: one-line help for what my-command-name does
     - file: another_file.py
       commands:
         - name: command2
           help: another cool west extension
         - name: a-third-command
           class: ThirdCommand
           help: a third command in the same file as command2

Above:

- :file:`my_west_extension.py` defines extension ``my-command-name``
  with class ``MyCommand``
- :file:`another_file.py` defines two extensions:

  #. ``command2`` with class ``command2``
  #. ``a-third-command`` with class ``ThirdCommand``

See the file :file:`west-commands-schema.yml` in the `west repository`_ for a
schema describing the contents of a :file:`west-comands.yml`.

Step 3: Update Your Manifest
============================

Finally, you need to specify the location of the :file:`west-commands.yml` you
just edited in your west manifest. If your extension is in a project, add it
like this:

.. code-block:: yaml

   manifest:
      # [... other contents ...]

      projects:
        - name: your-project
          west-commands: path/to/west-commands.yml
        # [... other projects ...]

Where :file:`path/to/west-commands.yml` is relative to the root of the project.
Note that the name :file:`west-commands.yml`, while encouraged, is just a
convention; you can name the file something else if you need to.

Alternatively, if your extension is in the manifest repository, just do the
same thing in the manifest's ``self`` section, like this:

.. code-block:: yaml

   manifest:
     # [... other contents ...]

     self:
       west-commands: path/to/west-commands.yml

That's it; you can now run ``west my-command-name``. Your command's name, help,
and the project which contains its code will now also show up in the ``west
--help`` output.  If you share the updated repositories with others, they'll be
able to use it, too.

.. _west PyPI page:
   https://pypi.org/project/west/

.. _west repository:
   https://github.com/zephyrproject-rtos/west/
