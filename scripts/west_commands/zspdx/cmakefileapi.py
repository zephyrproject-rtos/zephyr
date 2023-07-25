# Copyright (c) 2020 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

from enum import Enum

class Codemodel:

    def __init__(self):
        super(Codemodel, self).__init__()

        self.paths_source = ""
        self.paths_build = ""
        self.configurations = []

    def __repr__(self):
        return f"Codemodel: source {self.paths_source}, build {self.paths_build}"

# A member of the codemodel configurations array
class Config:

    def __init__(self):
        super(Config, self).__init__()

        self.name = ""
        self.directories = []
        self.projects = []
        self.configTargets = []

    def __repr__(self):
        if self.name == "":
            return f"Config: [no name]"
        else:
            return f"Config: {self.name}"

# A member of the configuration.directories array
class ConfigDir:

    def __init__(self):
        super(ConfigDir, self).__init__()

        self.source = ""
        self.build = ""
        self.parentIndex = -1
        self.childIndexes = []
        self.projectIndex = -1
        self.targetIndexes = []
        self.minimumCMakeVersion = ""
        self.hasInstallRule = False

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
        super(ConfigProject, self).__init__()

        self.name = ""
        self.parentIndex = -1
        self.childIndexes = []
        self.directoryIndexes = []
        self.targetIndexes = []

        # actual items, calculated from indices after loading
        self.parent = None
        self.children = []
        self.directories = []
        self.targets = []

    def __repr__(self):
        return f"ConfigProject: {self.name}"

# A member of the configuration.configTargets array
class ConfigTarget:

    def __init__(self):
        super(ConfigTarget, self).__init__()

        self.name = ""
        self.id = ""
        self.directoryIndex = -1
        self.projectIndex = -1
        self.jsonFile = ""

        # actual target data, loaded from self.jsonFile
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
        super(TargetInstallDestination, self).__init__()

        self.path = ""
        self.backtrace = -1

    def __repr__(self):
        return f"TargetInstallDestination: {self.path}"

# A member of the target.link_commandFragments and
# archive_commandFragments array
class TargetCommandFragment:

    def __init__(self):
        super(TargetCommandFragment, self).__init__()

        self.fragment = ""
        self.role = ""

    def __repr__(self):
        return f"TargetCommandFragment: {self.fragment}"

# A member of the target.dependencies array
class TargetDependency:

    def __init__(self):
        super(TargetDependency, self).__init__()

        self.id = ""
        self.backtrace = -1

    def __repr__(self):
        return f"TargetDependency: {self.id}"

# A member of the target.sources array
class TargetSource:

    def __init__(self):
        super(TargetSource, self).__init__()

        self.path = ""
        self.compileGroupIndex = -1
        self.sourceGroupIndex = -1
        self.isGenerated = False
        self.backtrace = -1

        # actual items, calculated from indices after loading
        self.compileGroup = None
        self.sourceGroup = None

    def __repr__(self):
        return f"TargetSource: {self.path}"

# A member of the target.sourceGroups array
class TargetSourceGroup:

    def __init__(self):
        super(TargetSourceGroup, self).__init__()

        self.name = ""
        self.sourceIndexes = []

        # actual items, calculated from indices after loading
        self.sources = []

    def __repr__(self):
        return f"TargetSourceGroup: {self.name}"

# A member of the target.compileGroups.includes array
class TargetCompileGroupInclude:

    def __init__(self):
        super(TargetCompileGroupInclude, self).__init__()

        self.path = ""
        self.isSystem = False
        self.backtrace = -1

    def __repr__(self):
        return f"TargetCompileGroupInclude: {self.path}"

# A member of the target.compileGroups.precompileHeaders array
class TargetCompileGroupPrecompileHeader:

    def __init__(self):
        super(TargetCompileGroupPrecompileHeader, self).__init__()

        self.header = ""
        self.backtrace = -1

    def __repr__(self):
        return f"TargetCompileGroupPrecompileHeader: {self.header}"

# A member of the target.compileGroups.defines array
class TargetCompileGroupDefine:

    def __init__(self):
        super(TargetCompileGroupDefine, self).__init__()

        self.define = ""
        self.backtrace = -1

    def __repr__(self):
        return f"TargetCompileGroupDefine: {self.define}"

# A member of the target.compileGroups array
class TargetCompileGroup:

    def __init__(self):
        super(TargetCompileGroup, self).__init__()

        self.sourceIndexes = []
        self.language = ""
        self.compileCommandFragments = []
        self.includes = []
        self.precompileHeaders = []
        self.defines = []
        self.sysroot = ""

        # actual items, calculated from indices after loading
        self.sources = []

    def __repr__(self):
        return f"TargetCompileGroup: {self.sources}"

# A member of the target.backtraceGraph_nodes array
class TargetBacktraceGraphNode:

    def __init__(self):
        super(TargetBacktraceGraphNode, self).__init__()

        self.file = -1
        self.line = -1
        self.command = -1
        self.parent = -1

    def __repr__(self):
        return f"TargetBacktraceGraphNode: {self.command}"

# Actual data in config.target.target, loaded from
# config.target.jsonFile
class Target:

    def __init__(self):
        super(Target, self).__init__()

        self.name = ""
        self.id = ""
        self.type = TargetType.UNKNOWN
        self.backtrace = -1
        self.folder = ""
        self.paths_source = ""
        self.paths_build = ""
        self.nameOnDisk = ""
        self.artifacts = []
        self.isGeneratorProvided = False

        # only if install rule is present
        self.install_prefix = ""
        self.install_destinations = []

        # only for executables and shared library targets that link into
        # a runtime binary
        self.link_language = ""
        self.link_commandFragments = []
        self.link_lto = False
        self.link_sysroot = ""

        # only for static library targets
        self.archive_commandFragments = []
        self.archive_lto = False

        # only if the target depends on other targets
        self.dependencies = []

        # corresponds to target's source files
        self.sources = []

        # only if sources are grouped together by source_group() or by default
        self.sourceGroups = []

        # only if target has sources that compile
        self.compileGroups = []

        # graph of backtraces referenced from elsewhere
        self.backtraceGraph_nodes = []
        self.backtraceGraph_commands = []
        self.backtraceGraph_files = []

    def __repr__(self):
        return f"Target: {self.name}"
