# to import this in BabbleSimBuild or BabbleSimRun, add there:
# from test_utils.BabbleSimError import BabbleSimError

import logging

LOGGER_NAME = f"bsim_plugin.{__name__.split('.')[-1]}"
logger = logging.getLogger(LOGGER_NAME)

class BabbleSimError(Exception):
    pass
