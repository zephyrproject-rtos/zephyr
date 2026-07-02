# Copyright (c) 2020 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

from enum import Enum


class Codemodel:
    def __init__(self):
        super().__init__()

        self.paths_source = ""
        self.paths_build = ""
        self.configurations = []

    def __repr__(self):
        return f"Codemodel: source {self.paths_source}, build {self.paths_build}"


# A member of the codemodel configurations array
class Config:
    def __init__(self):
        super().__init__()

        self.name = ""
        self.directories = []
        self.projects = []
        self.config_targets = []

    def __repr__(self):
        if self.name == "":
            return "Config: [no name]"
        else:
            return f"Config: {self.name}"


# A member of the configuration.directories array
class ConfigDir:
    def __init__(self):
        super().__init__()

        self.source = ""
        self.build = ""
        self.parent_index = -1
        self.child_indexes = []
        self.project_index = -1
        self.target_indexes = []
        self.minimum_cmake_version = ""
        self.has_install_rule = False

        # actual items, calculated from indices after loading
        self.parent = None
        self.children = []
        self.project = None
        self.targets = []

    def __repr__(self):
        return f"ConfigDir: source {self.source}, build {self.build}"


# A member of the configuration.projects array
class ConfigProject:
    def __init__(self):
        super().__init__()

        self.name = ""
        self.parent_index = -1
        self.child_indexes = []
        self.directory_indexes = []
        self.target_indexes = []

        # actual items, calculated from indices after loading
        self.parent = None
        self.children = []
        self.directories = []
        self.targets = []

    def __repr__(self):
        return f"ConfigProject: {self.name}"


# A member of the configuration.config_targets array
class ConfigTarget:
    def __init__(self):
        super().__init__()

        self.name = ""
        self.id = ""
        self.directory_index = -1
        self.project_index = -1
        self.json_file = ""

        # actual target data, loaded from self.json_file
        self.target = None

        # actual items, calculated from indices after loading
        self.directory = None
        self.project = None

    def __repr__(self):
        return f"ConfigTarget: {self.name}"


# The available values for Target.type
class TargetType(Enum):
    UNKNOWN = 0
    EXECUTABLE = 1
    STATIC_LIBRARY = 2
    SHARED_LIBRARY = 3
    MODULE_LIBRARY = 4
    OBJECT_LIBRARY = 5
    UTILITY = 6


# A member of the target.install_destinations array
class TargetInstallDestination:
    def __init__(self):
        super().__init__()

        self.path = ""
        self.backtrace = -1

    def __repr__(self):
        return f"TargetInstallDestination: {self.path}"


# A member of the target.link_command_fragments and
# archive_command_fragments array
class TargetCommandFragment:
    def __init__(self):
        super().__init__()

        self.fragment = ""
        self.role = ""

    def __repr__(self):
        return f"TargetCommandFragment: {self.fragment}"


# A member of the target.dependencies array
class TargetDependency:
    def __init__(self):
        super().__init__()

        self.id = ""
        self.backtrace = -1

    def __repr__(self):
        return f"TargetDependency: {self.id}"


# A member of the target.sources array
class TargetSource:
    def __init__(self):
        super().__init__()

        self.path = ""
        self.compile_group_index = -1
        self.source_group_index = -1
        self.is_generated = False
        self.backtrace = -1

        # actual items, calculated from indices after loading
        self.compile_group = None
        self.source_group = None

    def __repr__(self):
        return f"TargetSource: {self.path}"


# A member of the target.source_groups array
class TargetSourceGroup:
    def __init__(self):
        super().__init__()

        self.name = ""
        self.source_indexes = []

        # actual items, calculated from indices after loading
        self.sources = []

    def __repr__(self):
        return f"TargetSourceGroup: {self.name}"


# A member of the target.compile_groups.includes array
class TargetCompileGroupInclude:
    def __init__(self):
        super().__init__()

        self.path = ""
        self.is_system = False
        self.backtrace = -1

    def __repr__(self):
        return f"TargetCompileGroupInclude: {self.path}"


# A member of the target.compile_groups.precompile_headers array
class TargetCompileGroupPrecompileHeader:
    def __init__(self):
        super().__init__()

        self.header = ""
        self.backtrace = -1

    def __repr__(self):
        return f"TargetCompileGroupPrecompileHeader: {self.header}"


# A member of the target.compile_groups.defines array
class TargetCompileGroupDefine:
    def __init__(self):
        super().__init__()

        self.define = ""
        self.backtrace = -1

    def __repr__(self):
        return f"TargetCompileGroupDefine: {self.define}"


# A member of the target.compile_groups array
class TargetCompileGroup:
    def __init__(self):
        super().__init__()

        self.source_indexes = []
        self.language = ""
        self.compile_command_fragments = []
        self.includes = []
        self.precompile_headers = []
        self.defines = []
        self.sysroot = ""

        # actual items, calculated from indices after loading
        self.sources = []

    def __repr__(self):
        return f"TargetCompileGroup: {self.sources}"


# A member of the target.backtrace_graph_nodes array
class TargetBacktraceGraphNode:
    def __init__(self):
        super().__init__()

        self.file = -1
        self.line = -1
        self.command = -1
        self.parent = -1

    def __repr__(self):
        return f"TargetBacktraceGraphNode: {self.command}"


# Actual data in config.target.target, loaded from
# config.target.json_file
class Target:
    def __init__(self):
        super().__init__()

        self.name = ""
        self.id = ""
        self.type = TargetType.UNKNOWN
        self.backtrace = -1
        self.folder = ""
        self.paths_source = ""
        self.paths_build = ""
        self.name_on_disk = ""
        self.artifacts = []
        self.is_generator_provided = False

        # only if install rule is present
        self.install_prefix = ""
        self.install_destinations = []

        # only for executables and shared library targets that link into
        # a runtime binary
        self.link_language = ""
        self.link_command_fragments = []
        self.link_lto = False
        self.link_sysroot = ""

        # only for static library targets
        self.archive_command_fragments = []
        self.archive_lto = False

        # only if the target depends on other targets
        self.dependencies = []

        # corresponds to target's source files
        self.sources = []

        # only if sources are grouped together by source_group() or by default
        self.source_groups = []

        # only if target has sources that compile
        self.compile_groups = []

        # graph of backtraces referenced from elsewhere
        self.backtrace_graph_nodes = []
        self.backtrace_graph_commands = []
        self.backtrace_graph_files = []

    def __repr__(self):
        return f"Target: {self.name}"
