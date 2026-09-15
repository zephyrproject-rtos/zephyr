# Board alias file for in-tree aliases.
#
# This file allows boards to have an alias that can be used to allow a secondary name.
# For example `plank/foo` (alias) -> `plank/bar` (real board target)
# Defined using the form: `set(<alias_board>/<alias_qualifier>_BOARD_ALIAS <board>/<qualifier>)`
# Example:
# set(plank/foo_BOARD_ALIAS plank/bar)
set(myra_sip_baseboard/myra_BOARD_ALIAS myra_sip_baseboard/stm32g491xx)
