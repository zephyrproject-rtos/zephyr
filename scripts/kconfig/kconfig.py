#!/usr/bin/env python3
# Modified from: https://github.com/ulfalizer/Kconfiglib/blob/master/examples/merge_config.py
from kconfiglib import Kconfig, Symbol, BOOL, STRING, TRISTATE, TRI_TO_STR
import sys

if len(sys.argv) < 5:
    print('usage: {} Kconfig dotconfig autoconf conf1 [conf2 ...]'
          .format(sys.argv[0]))
    sys.exit(1)

print("Parsing Kconfig tree in {}".format(sys.argv[1]))
kconf = Kconfig(sys.argv[1])

# Enable warnings for assignments to undefined symbols
kconf.enable_undef_warnings()

# This script uses alldefconfig as the base. Other starting states could be set
# up here as well. The approach in examples/allnoconfig_simpler.py could
# provide an allnoconfig starting state for example.

print("Using {} as base".format(sys.argv[4]))
for config in sys.argv[5:]:
    print("Merging {}".format(config))
# Create a merged configuration by loading the fragments with replace=False
for config in sys.argv[4:]:
    kconf.load_config(config, replace=False)


# Print warnings for symbols whose actual value doesn't match the assigned
# value

def name_and_loc(sym):
    # Helper for printing symbol names and Kconfig file location(s) in warnings

    if not sym.nodes:
        return sym.name + " (undefined)"

    return "{} (defined at {})".format(
        sym.name,
        ", ".join("{}:{}".format(node.filename, node.linenr)
                  for node in sym.nodes))

for sym in kconf.defined_syms:
    # Was the symbol assigned to?
    if sym.user_value is not None:
        # Tristate values are represented as 0, 1, 2. Having them as
        # "n", "m", "y" is more convenient here, so convert.
        if sym.type in (BOOL, TRISTATE):
            user_value = TRI_TO_STR[sym.user_value]
        else:
            user_value = sym.user_value
        if user_value != sym.str_value:
            print('warning: {} was assigned the value "{}" but got the '
                  'value "{}" -- check dependencies'.format(
                      name_and_loc(sym), user_value, sym.str_value
                  ),
                  file=sys.stderr)


# Turn the warning for malformed .config lines into an error
for warning in kconf.warnings:
    if "ignoring malformed line" in warning:
        print("Aborting due to malformed configuration settings",
              file=sys.stderr)
        sys.exit(1)


# Write the merged configuration
kconf.write_config(sys.argv[2])

# Write the C header
kconf.write_autoconf(sys.argv[3])
