# Copyright (c) 2020 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0

import os
import pytest
import time



def test_LightweightM2M_1_1_int_101(leshan):
    ''' LightweightM2M-1.1-int-101
    '''
    resp = leshan.get("/clients/ztest")
    assert resp != None

def test_LightweightM2M_1_1_int_102(leshan):
    ''' LightweightM2M-1.1-int-102
    '''
    start_time = time.time() * 1000
    resp = leshan.put("/clients/ztest/1/0/1", '{"id":1,"kind":"singleResource","value":"20","type":"integer"}')
    time.sleep(1)
    latest = leshan.get("/clients/ztest")
    assert latest["lastUpdate"] > start_time
    assert latest["lastUpdate"] <= time.time()*1000
    assert latest["lifetime"] == 20


if __name__ == "__main__":
    pytest.main()
