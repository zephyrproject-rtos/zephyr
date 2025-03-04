# Copyright (c) 2024 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.dts utils."""

# Relax pylint a bit for unit tests.
# pylint: disable=missing-function-docstring

from pathlib import Path

import pytest

from dtsh.utils import CMakeCache, YAMLFile, YAMLFilesystem, PropertyLineage

from .dtsh_uthelpers import DTShTests


def test_cmakecache_init() -> None:
    with DTShTests.from_res():
        assert 4 == len(CMakeCache("CMakeCache.txt"))

    # Opening a non existing file should not fault.
    assert CMakeCache.open("notafile") is None

    # Opening an invalid CMakeCache file will not fault,
    # and answer an empty cache instead (lines do not match expected format).
    with DTShTests.from_res():
        cache = CMakeCache.open("zephyr.dts")
        assert cache is not None
        assert 0 == len(cache)


def test_cmakecache_getstr() -> None:
    with DTShTests.from_res():
        cache = CMakeCache("CMakeCache.txt")
    assert "foobar" == cache.getstr("DTSH_TEST_STRING")
    assert cache.getstr("NOT_AN_ENTRY") is None


def test_cmakecache_getstrs() -> None:
    with DTShTests.from_res():
        cache = CMakeCache("CMakeCache.txt")
    assert ["foo", "bar"] == cache.getstrs("DTSH_TEST_STRING_LIST")
    assert [] == cache.getstrs("NOT_AN_ENTRY")


def test_cmakecache_getbool() -> None:
    with DTShTests.from_res():
        cmake_cache = CMakeCache("CMakeCache.txt")
    assert cmake_cache.getbool("DTSH_TEST_BOOL_TRUE")
    assert not cmake_cache.getbool("DTSH_TEST_BOOL_FALSE")
    assert not cmake_cache.getbool("NOT_AN_ENTRY")


def test_yamlfs_find_path() -> None:
    with DTShTests.from_res():
        yamlfs = YAMLFilesystem(["yaml"])

    assert yamlfs.find_path("notafile") is None

    for name, path in [
        ("power.yaml", DTShTests.get_resource_path("yaml", "power.yaml")),
        (
            "i2c-device.yaml",
            DTShTests.get_resource_path("yaml", "i2c-device.yaml"),
        ),
        (
            "sensor-device.yaml",
            DTShTests.get_resource_path("yaml", "sensor-device.yaml"),
        ),
    ]:
        assert path == yamlfs.find_path(name)


def test_yamlfs_find_file() -> None:
    with DTShTests.from_res():
        yamlfs = YAMLFilesystem(["yaml"])

    fyaml = yamlfs.find_file("i2c-device.yaml")
    assert fyaml
    assert DTShTests.get_resource_path("yaml", "i2c-device.yaml") == str(
        fyaml.path.absolute()
    )

    # Opening a non existing file should not fault.
    assert yamlfs.find_file("notafile") is None


def test_yamlfs_name2path() -> None:
    with DTShTests.from_res():
        yamlfs = YAMLFilesystem(["yaml"])

    assert 7 == len(yamlfs.name2path)
    assert (
        DTShTests.get_resource_path("yaml", "i2c-device.yaml")
        == yamlfs.name2path["i2c-device.yaml"]
    )

    with pytest.raises(KeyError):
        _ = yamlfs.name2path["notafile"]


def test_dtshdts_yamlfs_find_path() -> None:
    with DTShTests.from_res():
        yamlfs = YAMLFilesystem(["yaml"])

    assert DTShTests.get_resource_path(
        "yaml", "sensor-device.yaml"
    ) == yamlfs.find_path("sensor-device.yaml")


def test_dtshdts_yamlfs_find_file() -> None:
    with DTShTests.from_res():
        yamlfs = YAMLFilesystem(["yaml"])

    yaml = yamlfs.find_file("sensor-device.yaml")
    assert yaml
    assert DTShTests.get_resource_path("yaml", "sensor-device.yaml") == str(
        yaml.path.absolute()
    )


def test_yamlfile() -> None:
    yaml = YAMLFile(DTShTests.get_resource_path("yaml", "i2c-device.yaml"))

    # Lazy initialzation.
    assert yaml._content is None
    assert yaml._raw is None
    assert not yaml._depth2included
    assert yaml.content.startswith("# Copyright (c) 2017, Linaro Limited")
    assert yaml.content.endswith("i2c bus")
    assert "i2c" == yaml.raw["on-bus"]
    assert ["base.yaml", "power.yaml"] == yaml.includes

    # Fail-safe
    yaml = YAMLFile("notafile")
    assert "" == yaml.content
    assert {} == yaml.raw
    assert not yaml.includes


def test_included_at_depth() -> None:
    with DTShTests.from_res():
        fyaml = YAMLFile(Path("yaml") / "included_at_depth.yaml")
        assert fyaml.raw

    assert ["inc1.yaml", "inc2.yaml", "inc3.yaml"] == fyaml.includes
    assert ["inc1.yaml"] == [inc.name for inc in fyaml.includes_at_depth(0)]
    assert ["inc2.yaml"] == [inc.name for inc in fyaml.includes_at_depth(1)]
    assert ["inc3.yaml"] == [inc.name for inc in fyaml.includes_at_depth(2)]


def test_find_property() -> None:
    with DTShTests.from_res():
        yamlfs = YAMLFilesystem(["yaml"])
    fyaml_base = yamlfs.find_file("included_at_depth.yaml")
    assert fyaml_base

    # Depth-0.
    assert not yamlfs.find_property("inc1_depth0_p1", fyaml_base, 1)
    assert not yamlfs.find_property("inc1_depth0_p1", fyaml_base, 2)
    # From inc1.yaml.
    fyaml = yamlfs.find_property("inc1_depth0_p1", fyaml_base, 0)
    assert fyaml
    assert "inc1.yaml" == fyaml.path.name
    # Last modified in included_at_depth.yaml.
    fyaml = yamlfs.find_property("inc1_depth0_p2", fyaml_base, 0)
    assert fyaml
    assert "included_at_depth.yaml" == fyaml.path.name
    assert not yamlfs.find_property("inc1_depth0_p2", fyaml_base, 1)

    # Child-bindings.
    assert not yamlfs.find_property("inc2_depth0_p1", fyaml_base, 0)
    assert not yamlfs.find_property("inc2_depth1_p1", fyaml_base, 1)
    assert yamlfs.find_property("inc1_depth1_p1", fyaml_base, 1)
    # From inc2.yaml.
    fyaml = yamlfs.find_property("inc2_depth0_p1", fyaml_base, 1)
    assert fyaml
    assert "inc2.yaml" == fyaml.path.name
    # Last modified in included_at_depth.yaml.
    fyaml = yamlfs.find_property("inc2_depth0_p2", fyaml_base, 1)
    assert fyaml
    assert "included_at_depth.yaml" == fyaml.path.name

    # Grandchild-bindings.
    assert not yamlfs.find_property("inc3_depth0_p1", fyaml_base, 0)
    assert not yamlfs.find_property("inc3_depth1_p1", fyaml_base, 1)
    assert not yamlfs.find_property("inc3_depth2_p1", fyaml_base, 2)
    # From inc3.yaml.
    fyaml = yamlfs.find_property("inc3_depth0_p1", fyaml_base, 2)
    assert fyaml
    assert "inc3.yaml" == fyaml.path.name
    # Last modified in included_at_depth.yaml.
    fyaml = yamlfs.find_property("inc3_depth0_p2", fyaml_base, 2)
    assert fyaml
    assert "included_at_depth.yaml" == fyaml.path.name


def test_backtrack_property_depth0() -> None:
    with DTShTests.from_res():
        yamlfs = YAMLFilesystem(["yaml"])
    fyaml_base = yamlfs.find_file("included_at_depth.yaml")
    assert fyaml_base

    backtrack = PropertyLineage()
    # Included from inc1.yaml.
    yamlfs.backtrack_property(backtrack, "inc1_depth0_p1", fyaml_base, 0)
    assert backtrack.fyaml_last
    assert "inc1.yaml" == backtrack.fyaml_last.path.name
    assert backtrack.fyaml_spec
    assert "inc1.yaml" == backtrack.fyaml_spec.path.name
    # Included from inc1.yaml, last modified in included_at_depth.yaml.
    backtrack = PropertyLineage()
    yamlfs.backtrack_property(backtrack, "inc1_depth0_p2", fyaml_base, 0)
    assert backtrack.fyaml_last
    assert "included_at_depth.yaml" == backtrack.fyaml_last.path.name
    assert backtrack.fyaml_spec
    assert "inc1.yaml" == backtrack.fyaml_spec.path.name
    # Included from inc1.yaml, does not have a description.
    backtrack = PropertyLineage()
    yamlfs.backtrack_property(backtrack, "inc1_depth0_p3", fyaml_base, 0)
    assert backtrack.fyaml_last
    assert "inc1.yaml" == backtrack.fyaml_last.path.name
    assert not backtrack.fyaml_spec

    # Undefined at other levels.
    backtrack = PropertyLineage()
    yamlfs.backtrack_property(backtrack, "inc1_depth1_p1", fyaml_base, 0)
    assert not backtrack.fyaml_last
    assert not backtrack.fyaml_spec
    backtrack = PropertyLineage()
    yamlfs.backtrack_property(backtrack, "inc1_depth0_p1", fyaml_base, 1)
    assert not backtrack.fyaml_last
    assert not backtrack.fyaml_spec
    backtrack = PropertyLineage()
    yamlfs.backtrack_property(backtrack, "inc1_depth0_p1", fyaml_base, 2)
    assert not backtrack.fyaml_last
    assert not backtrack.fyaml_spec


def test_backtrack_property_child_binding() -> None:
    with DTShTests.from_res():
        yamlfs = YAMLFilesystem(["yaml"])
    fyaml_base = yamlfs.find_file("included_at_depth.yaml")
    assert fyaml_base

    # Included from inc1.yaml.
    backtrack = PropertyLineage()
    yamlfs.backtrack_property(backtrack, "inc1_depth1_p1", fyaml_base, 1)
    assert backtrack.fyaml_last
    assert "inc1.yaml" == backtrack.fyaml_last.path.name
    assert backtrack.fyaml_spec
    assert "inc1.yaml" == backtrack.fyaml_spec.path.name
    # Included from inc2.yaml.
    backtrack = PropertyLineage()
    yamlfs.backtrack_property(backtrack, "inc2_depth0_p1", fyaml_base, 1)
    assert backtrack.fyaml_last
    assert "inc2.yaml" == backtrack.fyaml_last.path.name
    assert backtrack.fyaml_spec
    assert "inc2.yaml" == backtrack.fyaml_spec.path.name
    # Included from inc2.yaml, last modified in included_at_depth.yaml.
    backtrack = PropertyLineage()
    yamlfs.backtrack_property(backtrack, "inc2_depth0_p2", fyaml_base, 1)
    assert backtrack.fyaml_last
    assert "included_at_depth.yaml" == backtrack.fyaml_last.path.name
    assert backtrack.fyaml_spec
    assert "inc2.yaml" == backtrack.fyaml_spec.path.name
    # Included from inc2.yaml, does not have a description.
    backtrack = PropertyLineage()
    yamlfs.backtrack_property(backtrack, "inc2_depth0_p3", fyaml_base, 1)
    assert backtrack.fyaml_last
    assert "inc2.yaml" == backtrack.fyaml_last.path.name
    assert not backtrack.fyaml_spec

    # Undefined at other levels.
    backtrack = PropertyLineage()
    yamlfs.backtrack_property(backtrack, "inc2_depth0_p1", fyaml_base, 0)
    assert not backtrack.fyaml_last
    assert not backtrack.fyaml_spec
    backtrack = PropertyLineage()
    yamlfs.backtrack_property(backtrack, "inc2_depth1_p1", fyaml_base, 1)
    assert not backtrack.fyaml_last
    assert not backtrack.fyaml_spec


def test_backtrack_property_grandchild_binding() -> None:
    with DTShTests.from_res():
        yamlfs = YAMLFilesystem(["yaml"])
    fyaml_base = yamlfs.find_file("included_at_depth.yaml")
    assert fyaml_base

    # Included from inc1.yaml.
    backtrack = PropertyLineage()
    yamlfs.backtrack_property(backtrack, "inc1_depth2_p1", fyaml_base, 2)
    assert backtrack.fyaml_last
    assert "inc1.yaml" == backtrack.fyaml_last.path.name
    assert backtrack.fyaml_spec
    assert "inc1.yaml" == backtrack.fyaml_spec.path.name
    # Included from inc2.yaml.
    backtrack = PropertyLineage()
    yamlfs.backtrack_property(backtrack, "inc2_depth1_p1", fyaml_base, 2)
    assert backtrack.fyaml_last
    assert "inc2.yaml" == backtrack.fyaml_last.path.name
    assert backtrack.fyaml_spec
    assert "inc2.yaml" == backtrack.fyaml_spec.path.name

    # Included from inc3.yaml.
    backtrack = PropertyLineage()
    yamlfs.backtrack_property(backtrack, "inc3_depth0_p1", fyaml_base, 2)
    assert backtrack.fyaml_last
    assert "inc3.yaml" == backtrack.fyaml_last.path.name
    assert backtrack.fyaml_spec
    assert "inc3.yaml" == backtrack.fyaml_spec.path.name
    # Included from inc3.yaml, last modified in included_at_depth.yaml.
    backtrack = PropertyLineage()
    yamlfs.backtrack_property(backtrack, "inc3_depth0_p2", fyaml_base, 2)
    assert backtrack.fyaml_last
    assert "included_at_depth.yaml" == backtrack.fyaml_last.path.name
    assert backtrack.fyaml_spec
    assert "inc3.yaml" == backtrack.fyaml_spec.path.name
    # Included from inc3.yaml, does not have a description.
    backtrack = PropertyLineage()
    yamlfs.backtrack_property(backtrack, "inc3_depth0_p3", fyaml_base, 2)
    assert backtrack.fyaml_last
    assert "inc3.yaml" == backtrack.fyaml_last.path.name
    assert not backtrack.fyaml_spec
