#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

'''
This test file contains testcases for reporting functionality of Testsuite class of twister
'''
import sys
import os
import xml.etree.ElementTree as ET
import csv
from collections import defaultdict
from mock import MagicMock
import pytest

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/schemas/twister/"))


def test_discard_report(class_testsuite, platforms_list, all_testcases_dict, caplog, tmpdir):
    """ Testing discard_report function of Testsuite class in twister
    Test 1: Check if apply_filters function has been run before running
    discard_report
    Test 2: Test if the generated report is not empty
    Test 3: Test if the gerenrated report contains the expected columns"""
    class_testsuite.platforms = platforms_list
    class_testsuite.testcases = all_testcases_dict
    filename = tmpdir.mkdir("test_discard").join("discard_report.csv")
    with pytest.raises(SystemExit):
        class_testsuite.discard_report(filename)
    assert "apply_filters() hasn't been run!" in caplog.text

    kwargs = {"exclude_tag" : ['test_a'], "exclude_platform" : ['demo_board_1'],
              "platform" : ['demo_board_2']}
    class_testsuite.apply_filters(**kwargs)
    class_testsuite.discard_report(filename)
    assert os.stat(filename).st_size != 0
    with open(filename, "r") as file:
        csv_reader = csv.reader(file)
        assert set(['test', 'arch', 'platform', 'reason']) == set(list(csv_reader)[0])

def test_csv_report(class_testsuite, instances_fixture, tmpdir):
    """ Testing csv_report function of Testsuite class in twister
    Test 1: Assert the csv_report isnt empty after execution of csv_report function
    Test 2: Assert on the columns and values of the generated csv_report"""
    class_testsuite.instances = instances_fixture
    filename = tmpdir.mkdir("test_csv").join("twister_csv_report.csv")
    class_testsuite.csv_report(filename)
    assert os.path.exists(filename)
    assert os.stat(filename).st_size != 0

    mydict = {'test': [], 'arch' : [], 'platform' : [], 'status': [],
              'extra_args': [], 'handler': [], 'handler_time': [],
              'ram_size': [], 'rom_size': []}

    with open(filename, "r") as file:
        csv_reader = csv.reader(file)
        assert set(mydict.keys()) == set(list(csv_reader)[0])

    for instance in class_testsuite.instances.values():
        mydict["test"].append(instance.testcase.name)
        mydict["arch"].append(instance.platform.arch)
        mydict["platform"].append(instance.platform.name)
        instance_status = instance.status if instance.status is not None else ""
        mydict["status"].append(instance_status)
        args = " ".join(instance.testcase.extra_args)
        mydict["extra_args"].append(args)
        mydict["handler"].append(instance.platform.simulation)
        mydict["handler_time"].append(instance.metrics.get("handler_time", ""))
        mydict["ram_size"].append(instance.metrics.get("ram_size", '0'))
        mydict["rom_size"].append(instance.metrics.get("rom_size", '0'))

    dict_file = open(filename, "r")
    dict_reader = csv.DictReader(dict_file)
    columns = defaultdict(list)
    for row in dict_reader:
        for (key, value) in row.items():
            columns[key].append(value)
    for _, value in enumerate(mydict):
        assert columns[value] == mydict[value]
    dict_file.close()

def test_xunit_report(class_testsuite, test_data,
                      instances_fixture, platforms_list, all_testcases_dict):
    """ Testing xunit_report function of Testsuite class in twister
    Test 1: Assert twister.xml file exists after execution of xunit_report function
    Test 2: Assert on fails, passes, skips, errors values
    Test 3: Assert on the tree structure of twister.xml file"""
    class_testsuite.platforms = platforms_list
    class_testsuite.testcases = all_testcases_dict
    kwargs = {"exclude_tag" : ['test_a'], "exclude_platform" : ['demo_board_1'],
              "platform" : ['demo_board_2']}
    class_testsuite.apply_filters(**kwargs)
    class_testsuite.instances = instances_fixture
    inst1 = class_testsuite.instances.get("demo_board_2/scripts/tests/twister/test_data/testcases/tests/test_a/test_a.check_1")
    inst2 = class_testsuite.instances.get("demo_board_2/scripts/tests/twister/test_data/testcases/tests/test_a/test_a.check_2")
    inst1.status = "failed"
    inst2.status = "skipped"

    filename = test_data + "twister.xml"
    fails, passes, errors, skips = class_testsuite.xunit_report(filename)
    assert os.path.exists(filename)

    filesize = os.path.getsize(filename)
    assert filesize != 0

    tree = ET.parse(filename)
    assert int(tree.findall('testsuite')[0].attrib["skipped"]) == int(skips)
    assert int(tree.findall('testsuite')[0].attrib["failures"]) == int(fails)
    assert int(tree.findall('testsuite')[0].attrib["errors"]) == int(errors)
    assert int(tree.findall('testsuite')[0].attrib["tests"]) == int(passes+fails+skips+errors)

    for index in range(1, len(class_testsuite.instances)+1):
        # index=0 corresponds to 'properties'. Test cases start from index=1
        if len(list(tree.findall('testsuite')[0][index])) != 0:
            if tree.findall('testsuite')[0][index][0].attrib["type"] == "failure":
                assert tree.findall('testsuite')[0][index].attrib["name"] == \
                       (inst1.testcase.name)
            elif tree.findall('testsuite')[0][index][0].attrib["type"] == "skipped":
                assert tree.findall('testsuite')[0][index].attrib["name"] == \
                       (inst2.testcase.name)
    os.remove(filename)

def test_compare_metrics(class_testsuite, test_data, instances_fixture, caplog):
    """ Testing compare_metrics function of Testsuite class in twister
    Test 1: Error message is raised if file twister.csv file doesnt exist
    Test 2: Assert on compare_metrics results for expected values"""
    class_testsuite.instances = instances_fixture
    for instance in class_testsuite.instances.values():
        instance.metrics["ram_size"] = 5
        instance.metrics["rom_size"] = 9
    filename_not_exist = test_data + "twister_file_not_exist.csv"
    class_testsuite.compare_metrics(filename_not_exist)
    assert "Cannot compare metrics, " + filename_not_exist + " not found" in caplog.text

    filename = test_data + "twister.csv"
    results = class_testsuite.compare_metrics(filename)
    for instance in class_testsuite.instances.values():
        for res in results:
            assert res[0].platform.name == instance.platform.name
            if (res[0].platform.name == instance.platform.name) and \
               (res[0].testcase.name == instance.testcase.name):
                if res[1] == "ram_size":
                    assert res[2] == instance.metrics["ram_size"]
                elif res[1] == "rom_size":
                    assert res[2] == instance.metrics["rom_size"]

def test_target_report(class_testsuite, instances_fixture, tmpdir_factory):
    """ Testing target_report function of Testsuite class in twister
    Test: Assert xunit_report function is called from target_report function"""
    class_testsuite.instances = instances_fixture
    outdir = tmpdir_factory.mktemp("tmp")
    class_testsuite.xunit_report = MagicMock(side_effect=class_testsuite.xunit_report)
    class_testsuite.target_report(outdir, "abc", append=False)
    assert class_testsuite.instances
    class_testsuite.xunit_report.assert_called()
