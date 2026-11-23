# Copyright (c) 2020 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

import json
import logging
import os

from . import cmakefileapi

_logger = logging.getLogger(__name__)


def parse_reply(reply_index_path):
    reply_dir, _ = os.path.split(reply_index_path)

    # first we need to find the codemodel reply file
    try:
        with open(reply_index_path) as index_file:
            js = json.load(index_file)

            # get reply object
            reply_dict = js.get("reply", {})
            if reply_dict == {}:
                _logger.error('no "reply" field found in index file')
                return None
            # get codemodel object
            cm_dict = reply_dict.get("codemodel-v2", {})
            if cm_dict == {}:
                _logger.error('no "codemodel-v2" field found in "reply" object in index file')
                return None
            # and get codemodel filename
            json_file = cm_dict.get("jsonFile", "")
            if json_file == "":
                _logger.error('no "jsonFile" field found in "codemodel-v2" object in index file')
                return None

            return parse_codemodel(reply_dir, json_file)

    except OSError:
        _logger.exception("Error loading %s", reply_index_path)
        return None
    except json.decoder.JSONDecodeError:
        _logger.exception("Error parsing JSON in %s", reply_index_path)
        return None


def parse_toolchains_and_info(reply_index_path):
    """Parse the CMake info and toolchains-v1 reply from the file-API index.

    Returns a ``(CMakeInfo, Toolchains)`` tuple, each falling back to an empty object when its data
    is missing or unparsable.
    """
    cmake_info = cmakefileapi.CMakeInfo()
    toolchains = cmakefileapi.Toolchains()

    reply_dir, _ = os.path.split(reply_index_path)
    try:
        with open(reply_index_path) as index_file:
            js = json.load(index_file)
    except OSError:
        _logger.exception("Error loading %s", reply_index_path)
        return cmake_info, toolchains
    except json.decoder.JSONDecodeError:
        _logger.exception("Error parsing JSON in %s", reply_index_path)
        return cmake_info, toolchains

    cmake_info = parse_cmake_info(js)

    reply_dict = js.get("reply", {})
    toolchains_dict = reply_dict.get("toolchains-v1", {})
    toolchains_json_file = toolchains_dict.get("jsonFile", "")
    if toolchains_json_file:
        toolchains = parse_toolchains(reply_dir, toolchains_json_file)
    else:
        _logger.debug('no "toolchains-v1" reply found in CMake file API index')

    return cmake_info, toolchains


def parse_cmake_info(js):
    """Parse the ``cmake`` object (generator and version) from the index file."""
    cmake_info = cmakefileapi.CMakeInfo()

    cmake_dict = js.get("cmake", {})
    if cmake_dict:
        generator_dict = cmake_dict.get("generator", {})
        cmake_info.generator_name = generator_dict.get("name", "")
        cmake_info.generator_multi_config = generator_dict.get("multiConfig", False)

        paths_dict = cmake_dict.get("paths", {})
        cmake_info.cmake_path = paths_dict.get("cmake", "")

        version_dict = cmake_dict.get("version", {})
        cmake_info.version_major = version_dict.get("major", 0)
        cmake_info.version_minor = version_dict.get("minor", 0)
        cmake_info.version_patch = version_dict.get("patch", 0)
        cmake_info.version_string = version_dict.get("string", "")

    return cmake_info


def parse_toolchains(reply_dir, toolchains_file):
    """Parse a toolchains-v1 reply file into a ``Toolchains`` object."""
    toolchains_path = os.path.join(reply_dir, toolchains_file)
    toolchains = cmakefileapi.Toolchains()

    try:
        with open(toolchains_path) as f:
            js = json.load(f)
    except OSError:
        _logger.exception("Error loading %s", toolchains_path)
        return toolchains
    except json.decoder.JSONDecodeError:
        _logger.exception("Error parsing JSON in %s", toolchains_path)
        return toolchains

    kind = js.get("kind", "")
    if kind != "toolchains":
        _logger.warning('Expected "kind":"toolchains" in %s, got %s', toolchains_path, kind)

    for tc_dict in js.get("toolchains", []):
        tc = cmakefileapi.Toolchain()
        tc.language = tc_dict.get("language", "")
        tc.source_file_extensions = tc_dict.get("sourceFileExtensions", [])

        compiler_dict = tc_dict.get("compiler", {})
        if compiler_dict:
            compiler = cmakefileapi.ToolchainCompiler()
            compiler.path = compiler_dict.get("path", "")
            compiler.id = compiler_dict.get("id", "")
            compiler.version = compiler_dict.get("version", "")
            compiler.target = compiler_dict.get("target", "")
            tc.compiler = compiler

        if tc.language:
            toolchains.by_language[tc.language] = tc

    _logger.debug("parsed %d toolchains from CMake file API", len(toolchains.by_language))
    return toolchains


def parse_codemodel(reply_dir, codemodel_file):
    codemodel_path = os.path.join(reply_dir, codemodel_file)

    try:
        with open(codemodel_path) as cm_file:
            js = json.load(cm_file)

            cm = cmakefileapi.Codemodel()

            # for correctness, check kind and version
            kind = js.get("kind", "")
            if kind != "codemodel":
                _logger.error(
                    'Error loading CMake API reply: expected "kind":"codemodel" in %s, got %s',
                    codemodel_path,
                    kind,
                )
                return None
            version = js.get("version", {})
            version_major = version.get("major", -1)
            if version_major != 2:
                if version_major == -1:
                    _logger.error(
                        "Error loading CMake API reply: expected major version 2 in %s, "
                        "no version found",
                        codemodel_path,
                    )
                    return None
                _logger.error(
                    "Error loading CMake API reply: expected major version 2 in %s, got %d",
                    codemodel_path,
                    version_major,
                )
                return None

            # get paths
            paths_dict = js.get("paths", {})
            cm.paths_source = paths_dict.get("source", "")
            cm.paths_build = paths_dict.get("build", "")

            # get configurations
            configs_arr = js.get("configurations", [])
            for cfg_dict in configs_arr:
                cfg = parse_config(cfg_dict, reply_dir)
                if cfg:
                    cm.configurations.append(cfg)

            # and after parsing is done, link all the indices
            link_codemodel(cm)

            return cm

    except OSError:
        _logger.exception("Error loading %s", codemodel_path)
        return None
    except json.decoder.JSONDecodeError:
        _logger.exception("Error parsing JSON in %s", codemodel_path)
        return None


def parse_config(cfg_dict, reply_dir):
    cfg = cmakefileapi.Config()
    cfg.name = cfg_dict.get("name", "")

    # parse and add each directory
    dirs_arr = cfg_dict.get("directories", [])
    for dir_dict in dirs_arr:
        if dir_dict != {}:
            cfgdir = cmakefileapi.ConfigDir()
            cfgdir.source = dir_dict.get("source", "")
            cfgdir.build = dir_dict.get("build", "")
            cfgdir.parent_index = dir_dict.get("parentIndex", -1)
            cfgdir.child_indexes = dir_dict.get("childIndexes", [])
            cfgdir.project_index = dir_dict.get("projectIndex", -1)
            cfgdir.target_indexes = dir_dict.get("targetIndexes", [])
            min_cmake_ver_dict = dir_dict.get("minimumCMakeVersion", {})
            cfgdir.minimum_cmake_version = min_cmake_ver_dict.get("string", "")
            cfgdir.has_install_rule = dir_dict.get("hasInstallRule", False)
            cfg.directories.append(cfgdir)

    # parse and add each project
    projects_arr = cfg_dict.get("projects", [])
    for prj_dict in projects_arr:
        if prj_dict != {}:
            prj = cmakefileapi.ConfigProject()
            prj.name = prj_dict.get("name", "")
            prj.parent_index = prj_dict.get("parentIndex", -1)
            prj.child_indexes = prj_dict.get("childIndexes", [])
            prj.directory_indexes = prj_dict.get("directoryIndexes", [])
            prj.target_indexes = prj_dict.get("targetIndexes", [])
            cfg.projects.append(prj)

    # parse and add each target
    cfg_targets_arr = cfg_dict.get("targets", [])
    for cfg_target_dict in cfg_targets_arr:
        if cfg_target_dict != {}:
            cfg_target = cmakefileapi.ConfigTarget()
            cfg_target.name = cfg_target_dict.get("name", "")
            cfg_target.id = cfg_target_dict.get("id", "")
            cfg_target.directory_index = cfg_target_dict.get("directoryIndex", -1)
            cfg_target.project_index = cfg_target_dict.get("projectIndex", -1)
            cfg_target.json_file = cfg_target_dict.get("jsonFile", "")

            if cfg_target.json_file != "":
                cfg_target.target = parse_target(os.path.join(reply_dir, cfg_target.json_file))
            else:
                cfg_target.target = None

            cfg.config_targets.append(cfg_target)

    return cfg


def parse_target(target_path):
    try:
        with open(target_path) as target_file:
            js = json.load(target_file)

            target = cmakefileapi.Target()

            target.name = js.get("name", "")
            target.id = js.get("id", "")
            target.type = parse_target_type(js.get("type", "UNKNOWN"))
            target.backtrace = js.get("backtrace", -1)
            target.folder = js.get("folder", "")

            # get paths
            paths_dict = js.get("paths", {})
            target.paths_source = paths_dict.get("source", "")
            target.paths_build = paths_dict.get("build", "")

            target.name_on_disk = js.get("nameOnDisk", "")

            # parse artifacts if present
            artifacts_arr = js.get("artifacts", [])
            target.artifacts = []
            for artifact_dict in artifacts_arr:
                artifact_path = artifact_dict.get("path", "")
                if artifact_path != "":
                    target.artifacts.append(artifact_path)

            target.is_generator_provided = js.get("isGeneratorProvided", False)

            # call separate functions to parse subsections
            parse_target_install(target, js)
            parse_target_link(target, js)
            parse_target_archive(target, js)
            parse_target_dependencies(target, js)
            parse_target_sources(target, js)
            parse_target_source_groups(target, js)
            parse_target_compile_groups(target, js)
            parse_target_backtrace_graph(target, js)

            return target

    except OSError:
        _logger.exception("Error loading %s", target_path)
        return None
    except json.decoder.JSONDecodeError:
        _logger.exception("Error parsing JSON in %s", target_path)
        return None


def parse_target_type(target_type):
    return {
        "EXECUTABLE": cmakefileapi.TargetType.EXECUTABLE,
        "STATIC_LIBRARY": cmakefileapi.TargetType.STATIC_LIBRARY,
        "SHARED_LIBRARY": cmakefileapi.TargetType.SHARED_LIBRARY,
        "MODULE_LIBRARY": cmakefileapi.TargetType.MODULE_LIBRARY,
        "OBJECT_LIBRARY": cmakefileapi.TargetType.OBJECT_LIBRARY,
        "UTILITY": cmakefileapi.TargetType.UTILITY,
    }.get(target_type, cmakefileapi.TargetType.UNKNOWN)


def parse_target_install(target, js):
    install_dict = js.get("install", {})
    if install_dict == {}:
        return
    prefix_dict = install_dict.get("prefix", {})
    target.install_prefix = prefix_dict.get("path", "")

    destinations_arr = install_dict.get("destinations", [])
    for destination_dict in destinations_arr:
        dest = cmakefileapi.TargetInstallDestination()
        dest.path = destination_dict.get("path", "")
        dest.backtrace = destination_dict.get("backtrace", -1)
        target.install_destinations.append(dest)


def parse_target_link(target, js):
    link_dict = js.get("link", {})
    if link_dict == {}:
        return
    target.link_language = link_dict.get("language", {})
    target.link_lto = link_dict.get("lto", False)
    sysroot_dict = link_dict.get("sysroot", {})
    target.link_sysroot = sysroot_dict.get("path", "")

    fragments_arr = link_dict.get("commandFragments", [])
    for fragment_dict in fragments_arr:
        fragment = cmakefileapi.TargetCommandFragment()
        fragment.fragment = fragment_dict.get("fragment", "")
        fragment.role = fragment_dict.get("role", "")
        target.link_command_fragments.append(fragment)


def parse_target_archive(target, js):
    archive_dict = js.get("archive", {})
    if archive_dict == {}:
        return
    target.archive_lto = archive_dict.get("lto", False)

    fragments_arr = archive_dict.get("commandFragments", [])
    for fragment_dict in fragments_arr:
        fragment = cmakefileapi.TargetCommandFragment()
        fragment.fragment = fragment_dict.get("fragment", "")
        fragment.role = fragment_dict.get("role", "")
        target.archive_command_fragments.append(fragment)


def parse_target_dependencies(target, js):
    dependencies_arr = js.get("dependencies", [])
    for dependency_dict in dependencies_arr:
        dep = cmakefileapi.TargetDependency()
        dep.id = dependency_dict.get("id", "")
        dep.backtrace = dependency_dict.get("backtrace", -1)
        target.dependencies.append(dep)


def parse_target_sources(target, js):
    sources_arr = js.get("sources", [])
    for source_dict in sources_arr:
        src = cmakefileapi.TargetSource()
        src.path = source_dict.get("path", "")
        src.compile_group_index = source_dict.get("compileGroupIndex", -1)
        src.source_group_index = source_dict.get("sourceGroupIndex", -1)
        src.is_generated = source_dict.get("isGenerated", False)
        src.backtrace = source_dict.get("backtrace", -1)
        target.sources.append(src)


def parse_target_source_groups(target, js):
    source_groups_arr = js.get("sourceGroups", [])
    for source_group_dict in source_groups_arr:
        srcgrp = cmakefileapi.TargetSourceGroup()
        srcgrp.name = source_group_dict.get("name", "")
        srcgrp.source_indexes = source_group_dict.get("sourceIndexes", [])
        target.source_groups.append(srcgrp)


def parse_target_compile_groups(target, js):
    compile_groups_arr = js.get("compileGroups", [])
    for compile_group_dict in compile_groups_arr:
        cmpgrp = cmakefileapi.TargetCompileGroup()
        cmpgrp.source_indexes = compile_group_dict.get("sourceIndexes", [])
        cmpgrp.language = compile_group_dict.get("language", "")
        cmpgrp.sysroot = compile_group_dict.get("sysroot", "")

        command_fragments_arr = compile_group_dict.get("compileCommandFragments", [])
        for command_fragment_dict in command_fragments_arr:
            fragment = command_fragment_dict.get("fragment", "")
            if fragment != "":
                cmpgrp.compile_command_fragments.append(fragment)

        includes_arr = compile_group_dict.get("includes", [])
        for include_dict in includes_arr:
            grp_include = cmakefileapi.TargetCompileGroupInclude()
            grp_include.path = include_dict.get("path", "")
            grp_include.is_system = include_dict.get("isSystem", False)
            grp_include.backtrace = include_dict.get("backtrace", -1)
            cmpgrp.includes.append(grp_include)

        precompile_headers_arr = compile_group_dict.get("precompileHeaders", [])
        for precompile_header_dict in precompile_headers_arr:
            grp_header = cmakefileapi.TargetCompileGroupPrecompileHeader()
            grp_header.header = precompile_header_dict.get("header", "")
            grp_header.backtrace = precompile_header_dict.get("backtrace", -1)
            cmpgrp.precompile_headers.append(grp_header)

        defines_arr = compile_group_dict.get("defines", [])
        for define_dict in defines_arr:
            grp_define = cmakefileapi.TargetCompileGroupDefine()
            grp_define.define = define_dict.get("define", "")
            grp_define.backtrace = define_dict.get("backtrace", -1)
            cmpgrp.defines.append(grp_define)

        target.compile_groups.append(cmpgrp)


def parse_target_backtrace_graph(target, js):
    backtrace_graph_dict = js.get("backtraceGraph", {})
    if backtrace_graph_dict == {}:
        return
    target.backtrace_graph_commands = backtrace_graph_dict.get("commands", [])
    target.backtrace_graph_files = backtrace_graph_dict.get("files", [])

    nodes_arr = backtrace_graph_dict.get("nodes", [])
    for node_dict in nodes_arr:
        node = cmakefileapi.TargetBacktraceGraphNode()
        node.file = node_dict.get("file", -1)
        node.line = node_dict.get("line", -1)
        node.command = node_dict.get("command", -1)
        node.parent = node_dict.get("parent", -1)
        target.backtrace_graph_nodes.append(node)


# Create direct pointers for all Configs in Codemodel
# takes: Codemodel
def link_codemodel(cm):
    for cfg in cm.configurations:
        link_config(cfg)


# Create direct pointers for all contents of Config
# takes: Config
def link_config(cfg):
    for cfg_dir in cfg.directories:
        link_config_dir(cfg, cfg_dir)
    for cfg_prj in cfg.projects:
        link_config_project(cfg, cfg_prj)
    for cfg_target in cfg.config_targets:
        link_config_target(cfg, cfg_target)


# Create direct pointers for ConfigDir indices
# takes: Config and ConfigDir
def link_config_dir(cfg, cfg_dir):
    if cfg_dir.parent_index == -1:
        cfg_dir.parent = None
    else:
        cfg_dir.parent = cfg.directories[cfg_dir.parent_index]

    if cfg_dir.project_index == -1:
        cfg_dir.project = None
    else:
        cfg_dir.project = cfg.projects[cfg_dir.project_index]

    cfg_dir.children = []
    for child_index in cfg_dir.child_indexes:
        cfg_dir.children.append(cfg.directories[child_index])

    cfg_dir.targets = []
    for target_index in cfg_dir.target_indexes:
        cfg_dir.targets.append(cfg.config_targets[target_index])


# Create direct pointers for ConfigProject indices
# takes: Config and ConfigProject
def link_config_project(cfg, cfg_prj):
    if cfg_prj.parent_index == -1:
        cfg_prj.parent = None
    else:
        cfg_prj.parent = cfg.projects[cfg_prj.parent_index]

    cfg_prj.children = []
    for child_index in cfg_prj.child_indexes:
        cfg_prj.children.append(cfg.projects[child_index])

    cfg_prj.directories = []
    for dir_index in cfg_prj.directory_indexes:
        cfg_prj.directories.append(cfg.directories[dir_index])

    cfg_prj.targets = []
    for target_index in cfg_prj.target_indexes:
        cfg_prj.targets.append(cfg.config_targets[target_index])


# Create direct pointers for ConfigTarget indices
# takes: Config and ConfigTarget
def link_config_target(cfg, cfg_target):
    if cfg_target.directory_index == -1:
        cfg_target.directory = None
    else:
        cfg_target.directory = cfg.directories[cfg_target.directory_index]

    if cfg_target.project_index == -1:
        cfg_target.project = None
    else:
        cfg_target.project = cfg.projects[cfg_target.project_index]

    # and link target's sources and source groups
    for ts in cfg_target.target.sources:
        link_target_source(cfg_target.target, ts)
    for tsg in cfg_target.target.source_groups:
        link_target_source_group(cfg_target.target, tsg)
    for tcg in cfg_target.target.compile_groups:
        link_target_compile_group(cfg_target.target, tcg)


# Create direct pointers for TargetSource indices
# takes: Target and TargetSource
def link_target_source(target, target_src):
    if target_src.compile_group_index == -1:
        target_src.compile_group = None
    else:
        target_src.compile_group = target.compile_groups[target_src.compile_group_index]

    if target_src.source_group_index == -1:
        target_src.source_group = None
    else:
        target_src.source_group = target.source_groups[target_src.source_group_index]


# Create direct pointers for TargetSourceGroup indices
# takes: Target and TargetSourceGroup
def link_target_source_group(target, target_src_grp):
    target_src_grp.sources = []
    for src_index in target_src_grp.source_indexes:
        target_src_grp.sources.append(target.sources[src_index])


# Create direct pointers for TargetCompileGroup indices
# takes: Target and TargetCompileGroup
def link_target_compile_group(target, target_cmp_grp):
    target_cmp_grp.sources = []
    for src_index in target_cmp_grp.source_indexes:
        target_cmp_grp.sources.append(target.sources[src_index])
