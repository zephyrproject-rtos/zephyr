# Prints a tree of all items in the configuration
# vim: tabstop=4 shiftwidth=4 expandtab

import kconfiglib
import sys

# Integers representing symbol types
UNKNOWN, BOOL, TRISTATE, STRING, HEX, INT = range(6)

# Strings to use for types
TYPENAME = {UNKNOWN: "unknown", BOOL: "bool", TRISTATE: "tristate",
            STRING: "string", HEX: "hex", INT: "int"}

done = []
itemIndex = {}

def print_with_indent(s, indent):
    print((" " * indent) + s)

def print_items(items, outdir, indent):
    for item in items:
        if item.is_symbol() or item.is_choice():
            text = item.get_help()
        elif item.is_menu():
            text = item.get_title()
        else:
            # Comment
            text = item.get_text()
        if item.is_symbol():
            var = "CONFIG_%s" %item.get_name()
            if not var in done:
                done.append(var)

                # Save up the config items in itemIndex, a dictionary indexed
                # by the item name with the value as the "prompt" (short
                # description) from the Kconfig file.  (We'll output them
                # later in alphabetic order

                if len(item.get_prompts()) > 0:
                    p = item.get_prompts()[0]
                else:
                    p = ""
                itemIndex[var] = "   * - :option:`%s`\n     - %s\n" % (var, p)

                # Create a details .rst document for each symbol discovered

                config = open("%s/%s.rst" % (outdir, var), "w")
                config.write(":orphan:\n\n")
                config.write(".. title:: %s\n\n" %item.get_name())
                config.write(".. option:: CONFIG_%s\n" %item.get_name())
                config.write(".. _CONFIG_%s:\n" %item.get_name())
                if text:
                    config.write("\n%s\n\n" %text)
                else:
                    config.write("\nThe configuration item %s:\n\n" %var)
                config.write("%s\n" %item.rest())

                config.close()
        elif item.is_menu():
            print_items(item.get_items(), outdir, indent + 2)
        elif item.is_choice():
            print_items(item.get_items(), outdir, indent + 2)
        elif item.is_comment():
            pass


f = open("%s/index.rst" % (sys.argv[2]), "w")
f.write(""".. _configuration:

Configuration Options Reference Guide
#####################################

Introduction
************

Kconfig files describe the configuration symbols supported in the build
system, the logical organization and structure that group the symbols in menus
and sub-menus, and the relationships between the different configuration
symbols that govern the valid configuration combinations.

The Kconfig files are distributed across the build directory tree. The files
are organized based on their common characteristics and on what new symbols
they add to the configuration menus.

The configuration options' information below is extracted directly from
:program:`Kconfig` using the :file:`~/doc/scripts/genrest/genrest.py` script.
Click on the option name in the table below for detailed information about
each option.


Supported Options
*****************

.. list-table:: Alphabetized Index of Configuration Options
   :header-rows: 1

   * - Kconfig Symbol
     - Description
""")
conf = kconfiglib.Config(sys.argv[1])
print_items(conf.get_top_level_items(), sys.argv[2],  0)

# print_items created separate .rst files for each configuration option as
# well as filling itemIndex with all these options (and their descriptions).
# Now we can print out the accumulated config symbols in alphabetic order.

for item in sorted(itemIndex):
    f.write(itemIndex[item])

f.close()
