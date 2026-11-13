Overview

Sample which enters ship mode using a regulator parent after a boot delay. The boot delay helps
avoid deadlocking devices which are not debuggable after they enter ship mode. They become hard
or impossible to recover if they enter ship mode to quickly after boot/reset.

Sample Output

*** Delaying boot by 2000ms... ***
regulator@120000 entering ship mode
