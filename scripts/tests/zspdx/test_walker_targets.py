#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0
"""Tests for the files the walker collects from the CMake codemodel's targets.

The codemodel is built by hand, standing in for the CMake file-API reply of a build.
"""

import pytest
from conftest import NAMESPACE
from zspdx.cmakefileapi import (
    Codemodel,
    Config,
    ConfigTarget,
    Target,
    TargetDependency,
    TargetSource,
    TargetType,
)
from zspdx.model import ComponentPurpose, RelationshipType, SBOMComponent
from zspdx.walker import SOURCES_COMMENT, Walker, WalkerConfig


def utility_target(name, sources):
    return ConfigTarget(
        name=name,
        id=f"{name}::@0",
        target=Target(name=name, id=f"{name}::@0", type=TargetType.UTILITY, sources=sources),
    )


@pytest.fixture
def tree(tmp_path):
    """A source tree with a module holding a blob, and a build directory."""
    module_dir = tmp_path / "modules" / "mymodule"
    blob = module_dir / "zephyr" / "blobs" / "firmware.bin"
    blob.parent.mkdir(parents=True)
    blob.write_bytes(b"\x00\x01")

    build_dir = tmp_path / "build"
    (build_dir / "app").mkdir(parents=True)
    (build_dir / "app" / "libapp.a").write_bytes(b"!<arch>\n")
    generated = build_dir / "app" / "firmware.bin.inc"
    generated.write_text("0x00, 0x01,\n")
    rule = build_dir / "CMakeFiles" / "app_firmware.bin.inc.rule"
    rule.parent.mkdir()
    rule.write_text("")

    return {
        "source_dir": str(tmp_path / "app"),
        "build_dir": str(build_dir),
        "module_dir": str(module_dir),
        "blob": str(blob),
        "generated": str(generated),
        "rule": str(rule),
    }


@pytest.fixture
def walk(tree):
    """Factory walking the given targets and returning the resulting SBOM graph."""

    def _walk(cfg_targets):
        config = WalkerConfig()
        config.namespace_prefix = NAMESPACE
        config.build_dir = tree["build_dir"]
        walker = Walker(config)
        walker.setup_documents()

        module = SBOMComponent(
            name="mymodule-sources",
            purpose=ComponentPurpose.SOURCE,
            base_dir=tree["module_dir"],
            comment=SOURCES_COMMENT,
        )
        walker.sbom_graph.add_component(module, "zephyr")
        walker.component_zephyr_modules["mymodule"] = module

        walker.cm = Codemodel(
            paths_source=tree["source_dir"],
            paths_build=tree["build_dir"],
            configurations=[Config(config_targets=cfg_targets)],
        )
        walker.walk_targets()
        walker.walk_pending_sources()
        walker.walk_relationships()
        return walker.sbom_graph

    return _walk


def generator_target(tree):
    """The custom target generate_inc_file_for_target() creates for the blob."""
    return utility_target(
        "app_firmware.bin.inc",
        [
            TargetSource(path=tree["blob"]),
            TargetSource(path=tree["generated"], is_generated=True),
            TargetSource(path=tree["rule"], is_generated=True),
        ],
    )


def app_target(dependencies=()):
    return ConfigTarget(
        name="app",
        id="app::@0",
        target=Target(
            name="app",
            id="app::@0",
            type=TargetType.STATIC_LIBRARY,
            artifacts=["app/libapp.a"],
            dependencies=[TargetDependency(id=dep.id) for dep in dependencies],
        ),
    )


class TestUtilityDependencySources:
    """Tests for the inputs of UTILITY targets that an artifact-producing target depends on."""

    def test_embedded_blob_is_recorded_under_its_module(self, tree, walk):
        generator = generator_target(tree)
        graph = walk([app_target([generator]), generator])

        blob = graph.get_file(tree["blob"])
        assert blob is not None, "embedded blob missing from the SBOM"
        assert blob.component.name == "mymodule-sources"

        app = graph.get_component("app")
        assert any(
            r.relationship_type == RelationshipType.GENERATED_FROM and blob in r.to_elements
            for r in app.target_build_file.relationships
        ), "app build file is not GENERATED_FROM the embedded blob"

    def test_generated_sources_are_left_out(self, tree, walk):
        generator = generator_target(tree)
        graph = walk([app_target([generator]), generator])

        assert graph.get_file(tree["generated"]) is None
        assert graph.get_file(tree["rule"]) is None

    def test_utility_target_without_dependents_contributes_nothing(self, tree, walk):
        graph = walk([app_target(), generator_target(tree)])

        assert graph.get_file(tree["blob"]) is None
        assert graph.get_component("app_firmware.bin.inc") is None
