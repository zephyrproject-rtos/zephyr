# Copyright (c) 2020 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0

import os
import pytest

@pytest.fixture(autouse=True)
def pytest_cmdopt_handle(cmdopt):
    ''' handle cmdopt '''
    print("handle cmdopt:")
    print(cmdopt)
    data_path = cmdopt
    os.environ['data_path'] = str(data_path)

def test_case(cmdopt):
    ''' test case '''
    if not os.path.exists(cmdopt):
        assert 0
    print("run test cases in:")
    print(cmdopt)

if __name__ == "__main__":
    pytest.main()
