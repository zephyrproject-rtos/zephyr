import pytest
import os

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")

@pytest.fixture(name='test_data')
def _test_data():
    """ Pytest fixture to set the path of test_data"""
    data = ZEPHYR_BASE + "/scripts/tests/sanitycheck/test_data/"
    return data
