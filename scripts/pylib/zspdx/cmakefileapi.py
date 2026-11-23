# Copyright (c) 2020 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum


@dataclass
class Codemodel:
    """Root of the CMake file-API codemodel-v2 reply.

    Attributes:
        paths_source: Absolute path to the top-level CMake source directory.
        paths_build: Absolute path to the top-level CMake build directory.
        configurations: Parsed build configurations, one per generator configuration.
    """

    paths_source: str = ""
    paths_build: str = ""
    configurations: list[Config] = field(default_factory=list)

    def __repr__(self):
        return f"Codemodel: source {self.paths_source}, build {self.paths_build}"


@dataclass
class Config:
    """A member of the codemodel configurations array.

    Attributes:
        name: Configuration name (e.g. ``"Debug"``), or empty for single-config generators.
        directories: Directories that make up this configuration.
        projects: Projects that make up this configuration.
        config_targets: Targets that make up this configuration.
    """

    name: str = ""
    directories: list[ConfigDir] = field(default_factory=list)
    projects: list[ConfigProject] = field(default_factory=list)
    config_targets: list[ConfigTarget] = field(default_factory=list)

    def __repr__(self):
        if self.name == "":
            return "Config: [no name]"
        else:
            return f"Config: {self.name}"


@dataclass
class ConfigDir:
    """A member of the configuration.directories array.

    Attributes:
        source: Absolute path to the directory's source tree.
        build: Absolute path to the directory's build tree.
        parent_index: Index of the parent directory in the configuration, or -1 if none.
        child_indexes: Indexes of child directories in the configuration.
        project_index: Index of the owning project in the configuration.
        target_indexes: Indexes of targets defined in this directory.
        minimum_cmake_version: Minimum CMake version required by this directory, if specified.
        has_install_rule: Whether this directory (or a subdirectory) has an install rule.
        parent: Parent directory, resolved from parent_index after loading.
        children: Child directories, resolved from child_indexes after loading.
        project: Owning project, resolved from project_index after loading.
        targets: Targets defined in this directory, resolved from target_indexes after loading.
    """

    source: str = ""
    build: str = ""
    parent_index: int = -1
    child_indexes: list[int] = field(default_factory=list)
    project_index: int = -1
    target_indexes: list[int] = field(default_factory=list)
    minimum_cmake_version: str = ""
    has_install_rule: bool = False

    parent: ConfigDir | None = None
    children: list[ConfigDir] = field(default_factory=list)
    project: ConfigProject | None = None
    targets: list[ConfigTarget] = field(default_factory=list)

    def __repr__(self):
        return f"ConfigDir: source {self.source}, build {self.build}"


@dataclass
class ConfigProject:
    """A member of the configuration.projects array.

    Attributes:
        name: Project name.
        parent_index: Index of the parent project in the configuration, or -1 if none.
        child_indexes: Indexes of child projects in the configuration.
        directory_indexes: Indexes of directories that belong to this project.
        target_indexes: Indexes of targets that belong to this project.
        parent: Parent project, resolved from parent_index after loading.
        children: Child projects, resolved from child_indexes after loading.
        directories: Directories belonging to this project, resolved from directory_indexes.
        targets: Targets belonging to this project, resolved from target_indexes after loading.
    """

    name: str = ""
    parent_index: int = -1
    child_indexes: list[int] = field(default_factory=list)
    directory_indexes: list[int] = field(default_factory=list)
    target_indexes: list[int] = field(default_factory=list)

    parent: ConfigProject | None = None
    children: list[ConfigProject] = field(default_factory=list)
    directories: list[ConfigDir] = field(default_factory=list)
    targets: list[ConfigTarget] = field(default_factory=list)

    def __repr__(self):
        return f"ConfigProject: {self.name}"


@dataclass
class ConfigTarget:
    """A member of the configuration.config_targets array.

    Attributes:
        name: Target name.
        id: Target's unique CMake file-API ID.
        directory_index: Index of the directory that defines this target.
        project_index: Index of the project that owns this target.
        json_file: Path to the target's own JSON reply file, relative to the reply directory.
        target: Full target data, loaded from json_file.
        directory: Owning directory, resolved from directory_index after loading.
        project: Owning project, resolved from project_index after loading.
    """

    name: str = ""
    id: str = ""
    directory_index: int = -1
    project_index: int = -1
    json_file: str = ""

    target: Target | None = None

    directory: ConfigDir | None = None
    project: ConfigProject | None = None

    def __repr__(self):
        return f"ConfigTarget: {self.name}"


class TargetType(Enum):
    """The available values for Target.type."""

    UNKNOWN = 0
    EXECUTABLE = 1
    STATIC_LIBRARY = 2
    SHARED_LIBRARY = 3
    MODULE_LIBRARY = 4
    OBJECT_LIBRARY = 5
    UTILITY = 6


@dataclass
class TargetInstallDestination:
    """A member of the target.install_destinations array.

    Attributes:
        path: Absolute install destination path.
        backtrace: Index into the target's backtrace graph, or -1 if none.
    """

    path: str = ""
    backtrace: int = -1

    def __repr__(self):
        return f"TargetInstallDestination: {self.path}"


@dataclass
class TargetCommandFragment:
    """A member of the target.link_command_fragments and archive_command_fragments arrays.

    Attributes:
        fragment: Command-line fragment text.
        role: Role of the fragment (e.g. ``"flags"``, ``"libraries"``).
    """

    fragment: str = ""
    role: str = ""

    def __repr__(self):
        return f"TargetCommandFragment: {self.fragment}"


@dataclass
class TargetDependency:
    """A member of the target.dependencies array.

    Attributes:
        id: CMake file-API ID of the target this target depends on.
        backtrace: Index into the target's backtrace graph, or -1 if none.
    """

    id: str = ""
    backtrace: int = -1

    def __repr__(self):
        return f"TargetDependency: {self.id}"


@dataclass
class TargetSource:
    """A member of the target.sources array.

    Attributes:
        path: Source file path, relative to the top-level source directory.
        compile_group_index: Index of this source's compile group, or -1 if not compiled.
        source_group_index: Index of this source's source group, or -1 if none.
        is_generated: Whether this is a generated (not authored) source file.
        backtrace: Index into the target's backtrace graph, or -1 if none.
        compile_group: Compile group, resolved from compile_group_index after loading.
        source_group: Source group, resolved from source_group_index after loading.
    """

    path: str = ""
    compile_group_index: int = -1
    source_group_index: int = -1
    is_generated: bool = False
    backtrace: int = -1

    compile_group: TargetCompileGroup | None = None
    source_group: TargetSourceGroup | None = None

    def __repr__(self):
        return f"TargetSource: {self.path}"


@dataclass
class TargetSourceGroup:
    """A member of the target.source_groups array.

    Attributes:
        name: Source group name.
        source_indexes: Indexes of the sources in this group.
        sources: Sources in this group, resolved from source_indexes after loading.
    """

    name: str = ""
    source_indexes: list[int] = field(default_factory=list)

    sources: list[TargetSource] = field(default_factory=list)

    def __repr__(self):
        return f"TargetSourceGroup: {self.name}"


@dataclass
class TargetCompileGroupInclude:
    """A member of the target.compile_groups.includes array.

    Attributes:
        path: Include directory path.
        is_system: Whether this is a system include directory.
        backtrace: Index into the target's backtrace graph, or -1 if none.
    """

    path: str = ""
    is_system: bool = False
    backtrace: int = -1

    def __repr__(self):
        return f"TargetCompileGroupInclude: {self.path}"


@dataclass
class TargetCompileGroupPrecompileHeader:
    """A member of the target.compile_groups.precompile_headers array.

    Attributes:
        header: Precompiled header path.
        backtrace: Index into the target's backtrace graph, or -1 if none.
    """

    header: str = ""
    backtrace: int = -1

    def __repr__(self):
        return f"TargetCompileGroupPrecompileHeader: {self.header}"


@dataclass
class TargetCompileGroupDefine:
    """A member of the target.compile_groups.defines array.

    Attributes:
        define: Preprocessor define (e.g. ``"NDEBUG"`` or ``"FOO=1"``).
        backtrace: Index into the target's backtrace graph, or -1 if none.
    """

    define: str = ""
    backtrace: int = -1

    def __repr__(self):
        return f"TargetCompileGroupDefine: {self.define}"


@dataclass
class TargetCompileGroup:
    """A member of the target.compile_groups array.

    Attributes:
        source_indexes: Indexes of the sources compiled with this group's settings.
        language: Source language compiled by this group (e.g. ``"C"``, ``"CXX"``).
        compile_command_fragments: Compiler command-line fragments.
        includes: Include directories used by this group.
        precompile_headers: Precompiled headers used by this group.
        defines: Preprocessor defines used by this group.
        sysroot: Sysroot path passed to the compiler, if any.
        sources: Sources compiled with this group's settings, resolved from source_indexes.
    """

    source_indexes: list[int] = field(default_factory=list)
    language: str = ""
    compile_command_fragments: list[str] = field(default_factory=list)
    includes: list[TargetCompileGroupInclude] = field(default_factory=list)
    precompile_headers: list[TargetCompileGroupPrecompileHeader] = field(default_factory=list)
    defines: list[TargetCompileGroupDefine] = field(default_factory=list)
    sysroot: str = ""

    sources: list[TargetSource] = field(default_factory=list)

    def __repr__(self):
        return f"TargetCompileGroup: {self.sources}"


@dataclass
class TargetBacktraceGraphNode:
    """A member of the target.backtrace_graph_nodes array.

    Attributes:
        file: Index into the target's backtrace_graph_files array.
        line: Line number in the file, or -1 if unknown.
        command: Index into the target's backtrace_graph_commands array, or -1 if none.
        parent: Index of the parent node in the backtrace graph, or -1 if none.
    """

    file: int = -1
    line: int = -1
    command: int = -1
    parent: int = -1

    def __repr__(self):
        return f"TargetBacktraceGraphNode: {self.command}"


@dataclass
class Target:
    """Actual data in config.target.target, loaded from config.target.json_file.

    Attributes:
        name: Target name.
        id: Target's unique CMake file-API ID.
        type: Kind of target (executable, library, utility, etc.).
        backtrace: Index into backtrace_graph_nodes, or -1 if none.
        folder: IDE folder the target is organized under, if any.
        paths_source: Absolute path to the target's source directory.
        paths_build: Absolute path to the target's build directory.
        name_on_disk: Name of the target's primary artifact on disk.
        artifacts: Paths to the target's build artifacts.
        is_generator_provided: Whether the target was added by CMake itself, not the project.
        install_prefix: Install prefix, set only if the target has an install rule.
        install_destinations: Install destinations, set only if the target has an install rule.
        link_language: Link language, set only for executables and shared libraries.
        link_command_fragments: Link command fragments, set only for executables and shared
            libraries.
        link_lto: Whether link-time optimization is enabled, set only for executables and
            shared libraries.
        link_sysroot: Link-time sysroot path, set only for executables and shared libraries.
        archive_command_fragments: Archive command fragments, set only for static libraries.
        archive_lto: Whether link-time optimization is enabled, set only for static libraries.
        dependencies: Targets this target depends on, set only if there are any.
        sources: Target's source files.
        source_groups: Source groups, set only if sources are grouped by source_group() or by
            default.
        compile_groups: Compile groups, set only if the target has sources that compile.
        backtrace_graph_nodes: Nodes of the target's backtrace graph.
        backtrace_graph_commands: Commands referenced from the backtrace graph.
        backtrace_graph_files: Files referenced from the backtrace graph.
    """

    name: str = ""
    id: str = ""
    type: TargetType = TargetType.UNKNOWN
    backtrace: int = -1
    folder: str = ""
    paths_source: str = ""
    paths_build: str = ""
    name_on_disk: str = ""
    artifacts: list[str] = field(default_factory=list)
    is_generator_provided: bool = False

    install_prefix: str = ""
    install_destinations: list[TargetInstallDestination] = field(default_factory=list)

    link_language: str = ""
    link_command_fragments: list[TargetCommandFragment] = field(default_factory=list)
    link_lto: bool = False
    link_sysroot: str = ""

    archive_command_fragments: list[TargetCommandFragment] = field(default_factory=list)
    archive_lto: bool = False

    dependencies: list[TargetDependency] = field(default_factory=list)

    sources: list[TargetSource] = field(default_factory=list)

    source_groups: list[TargetSourceGroup] = field(default_factory=list)

    compile_groups: list[TargetCompileGroup] = field(default_factory=list)

    backtrace_graph_nodes: list[TargetBacktraceGraphNode] = field(default_factory=list)
    backtrace_graph_commands: list[str] = field(default_factory=list)
    backtrace_graph_files: list[str] = field(default_factory=list)

    def __repr__(self):
        return f"Target: {self.name}"


@dataclass
class CMakeInfo:
    """Generator and version from the "cmake" object of the file-based API index reply.

    Attributes:
        generator_name: CMake generator name (e.g. ``"Ninja"``).
        generator_multi_config: Whether the generator supports multiple configurations.
        cmake_path: Absolute path to the cmake executable.
        version_major: CMake major version.
        version_minor: CMake minor version.
        version_patch: CMake patch version.
        version_string: Full CMake version string.
    """

    generator_name: str = ""
    generator_multi_config: bool = False
    cmake_path: str = ""
    version_major: int = 0
    version_minor: int = 0
    version_patch: int = 0
    version_string: str = ""

    def __repr__(self):
        return f"CMakeInfo: version {self.version_string}, generator {self.generator_name}"


@dataclass
class ToolchainCompiler:
    """Compiler information for a single language from the toolchains-v1 reply.

    Attributes:
        path: Absolute path to the compiler executable.
        id: Compiler identifier (e.g. ``"GNU"``, ``"Clang"``).
        version: Compiler version string.
        target: Compiler target triple, if known.
    """

    path: str = ""
    id: str = ""
    version: str = ""
    target: str = ""

    def __repr__(self):
        return f"ToolchainCompiler: {self.id} {self.version} at {self.path}"


@dataclass
class Toolchain:
    """A member of the toolchains-v1 reply toolchains array.

    Attributes:
        language: Source language this toolchain compiles (e.g. ``"C"``, ``"CXX"``).
        compiler: Compiler used for this language, if known.
        source_file_extensions: File extensions associated with this language.
    """

    language: str = ""
    compiler: ToolchainCompiler | None = None
    source_file_extensions: list[str] = field(default_factory=list)

    def __repr__(self):
        return f"Toolchain: {self.language}"


@dataclass
class Toolchains:
    """Collection of toolchains keyed by language, from the toolchains-v1 reply.

    Attributes:
        by_language: Toolchains keyed by source language.
    """

    by_language: dict[str, Toolchain] = field(default_factory=dict)

    def get_compiler_path(self, language):
        """Get the compiler path for a given language, or "" if unknown."""
        tc = self.by_language.get(language)
        return tc.compiler.path if tc and tc.compiler else ""

    def get_compiler_version(self, language):
        """Get the compiler version for a given language, or "" if unknown."""
        tc = self.by_language.get(language)
        return tc.compiler.version if tc and tc.compiler else ""

    def get_compiler_id(self, language):
        """Get the compiler ID for a given language, or "" if unknown."""
        tc = self.by_language.get(language)
        return tc.compiler.id if tc and tc.compiler else ""

    def __repr__(self):
        return f"Toolchains: {list(self.by_language.keys())}"
