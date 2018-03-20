#!/usr/bin/env python3
# Modified from: https://github.com/ulfalizer/Kconfiglib/blob/master/examples/merge_config.py
from kconfiglib import Kconfig, Symbol, BOOL, STRING, TRISTATE, TRI_TO_STR
import sys
import locale

if len(sys.argv) < 5:
    print('usage: {} Kconfig dotconfig autoconf conf1 [conf2 ...]'
          .format(sys.argv[0]))
    sys.exit(1)

# Decode Kconfig sources as UTF-8 instead of decoding them according
# to the system locale (which might be ascii-only).
#
# Usually this has no effect because the norm is to have a system
# locale that covers the UTF-8 character set. Also, it is rare (but
# not forbidden) for Kconfig sources to have non-ascii characters. But
# using UTF-8 in Kconfig comments (for instance '# Author: Sebastian
# Bøe') is supported, and this will cause a crash when a user has
# configured his system ($ export LANG=C) to decode as ascii.
#
# See https://github.com/ulfalizer/Kconfiglib/pull/41 for a detailed
# discussion about explicitly decoding as UTF-8.
locale.setlocale(locale.LC_CTYPE, "C.UTF-8")

print("Parsing Kconfig tree in {}".format(sys.argv[1]))
kconf = Kconfig(sys.argv[1])

# Enable warnings for assignments to undefined symbols
kconf.enable_undef_warnings()

# (This script uses alldefconfig as the base. Other starting states could be
# set up here as well. The approach in examples/allnoconfig_simpler.py could
# provide an allnoconfig starting state for example.)

print("Using {} as base".format(sys.argv[4]))
for config in sys.argv[5:]:
    print("Merging {}".format(config))
# Create a merged configuration by loading the fragments with replace=False
for config in sys.argv[4:]:
    kconf.load_config(config, replace=False)

# Write the merged configuration
kconf.write_config(sys.argv[2])

# Output autoconf
kconf.write_autoconf(sys.argv[3])

# Print warnings for symbols whose actual value doesn't match the assigned
# value
for sym in kconf.defined_syms:
    # Was the symbol assigned to?
    #print('name: {} value: {}'.format(sym.name, sym.str_value))
    if sym.user_value is not None:
        # Tristate values are represented as 0, 1, 2. Having them as
        # "n", "m", "y" is more convenient here, so convert.
        if sym.type in (BOOL, TRISTATE):
            user_value = TRI_TO_STR[sym.user_value]
        else:
            user_value = sym.user_value
        if user_value != sym.str_value:
            print('warning: {} was assigned the value "{}" but got the '
                  'value "{}" -- check dependencies'
                  .format(sym.name, user_value, sym.str_value))

