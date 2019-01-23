This directory contains implementations for west commands which are
tightly coupled to the zephyr tree. Currently, those are the build,
flash, and debug commands.

Before adding more here, consider whether you might want to put new
extensions in upstream west. For example, any commands which operate
on the multi-repo need to be in upstream west, not here. Try to limit
what goes in here to just those files that change along with Zephyr
itself.

Thanks!
