import os
import pytest

def pytest_addoption(parser):
    parser.addoption('--testdata')
    parser.addoption('--probe-port', help='Path to Probe device')
    parser.addoption('--probe', help='Name of probe')

def pytest_configure(config):
    # Retrieve the value of the command-line option.
    param_file = config.getoption("--testdata")
    # Set an environment variable so that it's accessible at module import time.
    os.environ["TESTDATA_FILE"] = param_file