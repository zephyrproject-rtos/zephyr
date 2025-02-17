# Copyright: (c)  2024, Intel Corporation


def pytest_addoption(parser):
    parser.addoption('--testdata')
    parser.addoption('--pm_probe_port', help='Path to Probe device')
    parser.addoption('--pm_probe', help='Name of probe')
