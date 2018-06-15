#!/usr/bin/env python3

# Prints a message giving the locations of all references to undefined symbols
# within the Kconfig files. Prints nothing if there are no undefined
# references.
#
# The top-level Kconfig file is assumed to be called 'Kconfig'.

import sys

import kconfiglib


def report():
    # Returns a message (string) giving the locations of all references to
    # undefined Kconfig symbols within the Kconfig files, or the empty string
    # if there are no references to undefined symbols.
    #
    # Note: This function is called directly during CI.

    kconf = kconfiglib.Kconfig()

    undef_msg = ""

    for name, sym in kconf.syms.items():
        # - sym.nodes empty means the symbol is undefined (has no definition
        #   locations)
        #
        # - Due to Kconfig internals, numbers show up as undefined Kconfig
        #   symbols, but shouldn't be flagged
        #
        # - The MODULES symbol always exists but is unused in Zephyr
        if not sym.nodes and not is_num(name) and name != "MODULES":
            undef_msg += ref_locations_str(sym)

    if undef_msg:
        return "Error: Found references to undefined Kconfig symbols:\n" \
               + undef_msg

    return ""


def is_num(name):
    # Heuristic to see if a symbol name looks like a number. Internally,
    # everything is a symbol, only undefined symbols (which numbers usually
    # are) get their name as their value.

    try:
        int(name)
        return True
    except ValueError:
        # Require hex constants to be prefixed with 0x in Kconfig files, so
        # that we can tell that e.g. F00 is an undefined symbol reference.
        if not name.startswith(("0x", "0X")):
            return False

        try:
            int(name, 16)
            return True
        except ValueError:
            return False


def ref_locations_str(sym):
    # Prints all locations where sym is referenced, along with the Kconfig
    # definitions of the referencing items

    msg = "\n{}\n{}".format(sym.name, len(sym.name)*"=")

    def search(node):
        nonlocal msg

        while node:
            if sym in node.referenced():
                msg += "\n\n- Referenced at {}:{}:\n\n{}" \
                       .format(node.filename, node.linenr, node)

            if node.list:
                search(node.list)

            node = node.next

    search(sym.kconfig.top_node)
    return msg


if __name__ == "__main__":
    sys.stdout.write(report())
