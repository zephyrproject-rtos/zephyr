# Enable debug support in mdb
# Dwarf version 2 can be recognized by mdb
# The default dwarf version in gdb is not recognized by mdb
zephyr_cc_option(-g3 -gdwarf-2)

# Without this (poorly named) option, compiler may generate undefined
# references to abort().
# See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63691
zephyr_cc_option(-fno-delete-null-pointer-checks)

zephyr_cc_option_ifdef (CONFIG_LTO         -flto)

add_subdirectory(core)
