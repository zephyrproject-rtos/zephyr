# Prints a tree of all items in the configuration
# vim: tabstop=4 shiftwidth=4 expandtab

import kconfiglib
import sys

# Integers representing symbol types
UNKNOWN, BOOL, TRISTATE, STRING, HEX, INT = range(6)

# Strings to use for types
TYPENAME = {UNKNOWN: "unknown", BOOL: "bool", TRISTATE: "tristate",
            STRING: "string", HEX: "hex", INT: "int"}


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
            #print_with_indent("config {0}".format(item.get_name()), indent)

            var = "CONFIG_%s" %item.get_name()
            f.write("     kconfig/%s.rst\n" %var)
            config = open("%s/%s.rst" % (outdir, var), "w")
            config.write("\n.. _CONFIG_%s:\n" %item.get_name())
            config.write("\n%s\n" %var)
            config.write("%s\n\n" %(len("%s" %var) * '#' ))
            if text:
                config.write("\n%s\n\n" %text)
            else:
                config.write("\nThe configuration item %s:\n\n" %var)
            config.write(item.rest())

            config.close()
        elif item.is_menu():
            #print_with_indent('menu "{0}"'.format(item.get_title()), indent)
            print_items(item.get_items(), outdir, indent + 2)
        elif item.is_choice():
            #print_with_indent('choice', indent)
            print_items(item.get_items(), outdir, indent + 2)
        elif item.is_comment():
            pass
            #print_with_indent('comment "{0}"'.format(item.get_text()), indent)


f = open("%s/index.rst" % (sys.argv[2]), "w")
f.write(".. toctree::\n     :maxdepth: 2\n\n")
conf = kconfiglib.Config(sys.argv[1])
print_items(conf.get_top_level_items(), sys.argv[2],  0)
f.close()
