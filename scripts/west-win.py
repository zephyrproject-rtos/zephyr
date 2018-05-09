# Windows-specific launcher alias for west (west wind?).

import os
import sys

zephyr_base = os.environ['ZEPHYR_BASE']
sys.path.append(os.path.join(zephyr_base, 'scripts', 'meta'))

from west.main import main  # noqa E402 (silence flake8 warning)

main(sys.argv[1:])
