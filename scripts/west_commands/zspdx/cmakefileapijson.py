# Copyright (c) 2020 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

import json
import os

from west import log

import zspdx.cmakefileapi

def parseReply(replyIndexPath):
    replyDir, _ = os.path.split(replyIndexPath)

    # first we need to find the codemodel reply file
    try:
        with open(replyIndexPath, 'r') as indexFile:
            js = json.load(indexFile)

            # get reply object
            reply_dict = js.get("reply", {})
            if reply_dict == {}:
                log.err(f"no \"reply\" field found in index file")
                return None
            # get codemodel object
            cm_dict = reply_dict.get("codemodel-v2", {})
            if cm_dict == {}:
                log.err(f"no \"codemodel-v2\" field found in \"reply\" object in index file")
                return None
            # and get codemodel filename
            jsonFile = cm_dict.get("jsonFile", "")
            if jsonFile == "":
                log.err(f"no \"jsonFile\" field found in \"codemodel-v2\" object in index file")
                return None

            return parseCodemodel(replyDir, jsonFile)

    except OSError as e:
        log.err(f"Error loading {replyIndexPath}: {str(e)}")
        return None
    except json.decoder.JSONDecodeError as e:
        log.err(f"Error parsing JSON in {replyIndexPath}: {str(e)}")
        return None

def parseCodemodel(replyDir, codemodelFile):
    codemodelPath = os.path.join(replyDir, codemodelFile)

    try:
        with open(codemodelPath, 'r') as cmFile:
            js = json.load(cmFile)

            cm = zspdx.cmakefileapi.Codemodel()

            # for correctness, check kind and version
            kind = js.get("kind", "")
            if kind != "codemodel":
                log.err(f"Error loading CMake API reply: expected \"kind\":\"codemodel\" in {codemodelPath}, got {kind}")
                return None
            version = js.get("version", {})
            versionMajor = version.get("major", -1)
            if versionMajor != 2:
                if versionMajor == -1:
                    log.err(f"Error loading CMake API reply: expected major version 2 in {codemodelPath}, no version found")
                    return None
                log.err(f"Error loading CMake API reply: expected major version 2 in {codemodelPath}, got {versionMajor}")
                return None

            # get paths
            paths_dict = js.get("paths", {})
            cm.paths_source = paths_dict.get("source", "")
            cm.paths_build = paths_dict.get("build", "")

            # get configurations
            configs_arr = js.get("configurations", [])
            for cfg_dict in configs_arr:
                cfg = parseConfig(cfg_dict, replyDir)
                if cfg:
                    cm.configurations.append(cfg)

            # and after parsing is done, link all the indices
            linkCodemodel(cm)

            return cm

    except OSError as e:
        log.err(f"Error loading {codemodelPath}: {str(e)}")
        return None
    except json.decoder.JSONDecodeError as e:
        log.err(f"Error parsing JSON in {codemodelPath}: {str(e)}")
        return None

def parseConfig(cfg_dict, replyDir):
    cfg = zspdx.cmakefileapi.Config()
    cfg.name = cfg_dict.get("name", "")

    # parse and add each directory
    dirs_arr = cfg_dict.get("directories", [])
    for dir_dict in dirs_arr:
        if dir_dict != {}:
            cfgdir = zspdx.cmakefileapi.ConfigDir()
            cfgdir.source = dir_dict.get("source", "")
            cfgdir.build = dir_dict.get("build", "")
            cfgdir.parentIndex = dir_dict.get("parentIndex", -1)
            cfgdir.childIndexes = dir_dict.get("childIndexes", [])
            cfgdir.projectIndex = dir_dict.get("projectIndex", -1)
            cfgdir.targetIndexes = dir_dict.get("targetIndexes", [])
            minCMakeVer_dict = dir_dict.get("minimumCMakeVersion", {})
            cfgdir.minimumCMakeVersion = minCMakeVer_dict.get("string", "")
            cfgdir.hasInstallRule = dir_dict.get("hasInstallRule", False)
            cfg.directories.append(cfgdir)

    # parse and add each project
    projects_arr = cfg_dict.get("projects", [])
    for prj_dict in projects_arr:
        if prj_dict != {}:
            prj = zspdx.cmakefileapi.ConfigProject()
            prj.name = prj_dict.get("name", "")
            prj.parentIndex = prj_dict.get("parentIndex", -1)
            prj.childIndexes = prj_dict.get("childIndexes", [])
            prj.directoryIndexes = prj_dict.get("directoryIndexes", [])
            prj.targetIndexes = prj_dict.get("targetIndexes", [])
            cfg.projects.append(prj)

    # parse and add each target
    cfgTargets_arr = cfg_dict.get("targets", [])
    for cfgTarget_dict in cfgTargets_arr:
        if cfgTarget_dict != {}:
            cfgTarget = zspdx.cmakefileapi.ConfigTarget()
            cfgTarget.name = cfgTarget_dict.get("name", "")
            cfgTarget.id = cfgTarget_dict.get("id", "")
            cfgTarget.directoryIndex = cfgTarget_dict.get("directoryIndex", -1)
            cfgTarget.projectIndex = cfgTarget_dict.get("projectIndex", -1)
            cfgTarget.jsonFile = cfgTarget_dict.get("jsonFile", "")

            if cfgTarget.jsonFile != "":
                cfgTarget.target = parseTarget(os.path.join(replyDir, cfgTarget.jsonFile))
            else:
                cfgTarget.target = None

            cfg.configTargets.append(cfgTarget)

    return cfg

def parseTarget(targetPath):
    try:
        with open(targetPath, 'r') as targetFile:
            js = json.load(targetFile)

            target = zspdx.cmakefileapi.Target()

            target.name = js.get("name", "")
            target.id = js.get("id", "")
            target.type = parseTargetType(js.get("type", "UNKNOWN"))
            target.backtrace = js.get("backtrace", -1)
            target.folder = js.get("folder", "")

            # get paths
            paths_dict = js.get("paths", {})
            target.paths_source = paths_dict.get("source", "")
            target.paths_build = paths_dict.get("build", "")

            target.nameOnDisk = js.get("nameOnDisk", "")

            # parse artifacts if present
            artifacts_arr = js.get("artifacts", [])
            target.artifacts = []
            for artifact_dict in artifacts_arr:
                artifact_path = artifact_dict.get("path", "")
                if artifact_path != "":
                    target.artifacts.append(artifact_path)

            target.isGeneratorProvided = js.get("isGeneratorProvided", False)

            # call separate functions to parse subsections
            parseTargetInstall(target, js)
            parseTargetLink(target, js)
            parseTargetArchive(target, js)
            parseTargetDependencies(target, js)
            parseTargetSources(target, js)
            parseTargetSourceGroups(target, js)
            parseTargetCompileGroups(target, js)
            parseTargetBacktraceGraph(target, js)

            return target

    except OSError as e:
        log.err(f"Error loading {targetPath}: {str(e)}")
        return None
    except json.decoder.JSONDecodeError as e:
        log.err(f"Error parsing JSON in {targetPath}: {str(e)}")
        return None

def parseTargetType(targetType):
    if targetType == "EXECUTABLE":
        return zspdx.cmakefileapi.TargetType.EXECUTABLE
    elif targetType == "STATIC_LIBRARY":
        return zspdx.cmakefileapi.TargetType.STATIC_LIBRARY
    elif targetType == "SHARED_LIBRARY":
        return zspdx.cmakefileapi.TargetType.SHARED_LIBRARY
    elif targetType == "MODULE_LIBRARY":
        return zspdx.cmakefileapi.TargetType.MODULE_LIBRARY
    elif targetType == "OBJECT_LIBRARY":
        return zspdx.cmakefileapi.TargetType.OBJECT_LIBRARY
    elif targetType == "UTILITY":
        return zspdx.cmakefileapi.TargetType.UTILITY
    else:
        return zspdx.cmakefileapi.TargetType.UNKNOWN

def parseTargetInstall(target, js):
    install_dict = js.get("install", {})
    if install_dict == {}:
        return
    prefix_dict = install_dict.get("prefix", {})
    target.install_prefix = prefix_dict.get("path", "")

    destinations_arr = install_dict.get("destinations", [])
    for destination_dict in destinations_arr:
        dest = zspdx.cmakefileapi.TargetInstallDestination()
        dest.path = destination_dict.get("path", "")
        dest.backtrace = destination_dict.get("backtrace", -1)
        target.install_destinations.append(dest)

def parseTargetLink(target, js):
    link_dict = js.get("link", {})
    if link_dict == {}:
        return
    target.link_language = link_dict.get("language", {})
    target.link_lto = link_dict.get("lto", False)
    sysroot_dict = link_dict.get("sysroot", {})
    target.link_sysroot = sysroot_dict.get("path", "")

    fragments_arr = link_dict.get("commandFragments", [])
    for fragment_dict in fragments_arr:
        fragment = zspdx.cmakefileapi.TargetCommandFragment()
        fragment.fragment = fragment_dict.get("fragment", "")
        fragment.role = fragment_dict.get("role", "")
        target.link_commandFragments.append(fragment)

def parseTargetArchive(target, js):
    archive_dict = js.get("archive", {})
    if archive_dict == {}:
        return
    target.archive_lto = archive_dict.get("lto", False)

    fragments_arr = archive_dict.get("commandFragments", [])
    for fragment_dict in fragments_arr:
        fragment = zspdx.cmakefileapi.TargetCommandFragment()
        fragment.fragment = fragment_dict.get("fragment", "")
        fragment.role = fragment_dict.get("role", "")
        target.archive_commandFragments.append(fragment)

def parseTargetDependencies(target, js):
    dependencies_arr = js.get("dependencies", [])
    for dependency_dict in dependencies_arr:
        dep = zspdx.cmakefileapi.TargetDependency()
        dep.id = dependency_dict.get("id", "")
        dep.backtrace = dependency_dict.get("backtrace", -1)
        target.dependencies.append(dep)

def parseTargetSources(target, js):
    sources_arr = js.get("sources", [])
    for source_dict in sources_arr:
        src = zspdx.cmakefileapi.TargetSource()
        src.path = source_dict.get("path", "")
        src.compileGroupIndex = source_dict.get("compileGroupIndex", -1)
        src.sourceGroupIndex = source_dict.get("sourceGroupIndex", -1)
        src.isGenerated = source_dict.get("isGenerated", False)
        src.backtrace = source_dict.get("backtrace", -1)
        target.sources.append(src)

def parseTargetSourceGroups(target, js):
    sourceGroups_arr = js.get("sourceGroups", [])
    for sourceGroup_dict in sourceGroups_arr:
        srcgrp = zspdx.cmakefileapi.TargetSourceGroup()
        srcgrp.name = sourceGroup_dict.get("name", "")
        srcgrp.sourceIndexes = sourceGroup_dict.get("sourceIndexes", [])
        target.sourceGroups.append(srcgrp)

def parseTargetCompileGroups(target, js):
    compileGroups_arr = js.get("compileGroups", [])
    for compileGroup_dict in compileGroups_arr:
        cmpgrp = zspdx.cmakefileapi.TargetCompileGroup()
        cmpgrp.sourceIndexes = compileGroup_dict.get("sourceIndexes", [])
        cmpgrp.language = compileGroup_dict.get("language", "")
        cmpgrp.sysroot = compileGroup_dict.get("sysroot", "")

        commandFragments_arr = compileGroup_dict.get("compileCommandFragments", [])
        for commandFragment_dict in commandFragments_arr:
            fragment = commandFragment_dict.get("fragment", "")
            if fragment != "":
                cmpgrp.compileCommandFragments.append(fragment)

        includes_arr = compileGroup_dict.get("includes", [])
        for include_dict in includes_arr:
            grpInclude = zspdx.cmakefileapi.TargetCompileGroupInclude()
            grpInclude.path = include_dict.get("path", "")
            grpInclude.isSystem = include_dict.get("isSystem", False)
            grpInclude.backtrace = include_dict.get("backtrace", -1)
            cmpgrp.includes.append(grpInclude)

        precompileHeaders_arr = compileGroup_dict.get("precompileHeaders", [])
        for precompileHeader_dict in precompileHeaders_arr:
            grpHeader = zspdx.cmakefileapi.TargetCompileGroupPrecompileHeader()
            grpHeader.header = precompileHeader_dict.get("header", "")
            grpHeader.backtrace = precompileHeader_dict.get("backtrace", -1)
            cmpgrp.precompileHeaders.append(grpHeader)

        defines_arr = compileGroup_dict.get("defines", [])
        for define_dict in defines_arr:
            grpDefine = zspdx.cmakefileapi.TargetCompileGroupDefine()
            grpDefine.define = define_dict.get("define", "")
            grpDefine.backtrace = define_dict.get("backtrace", -1)
            cmpgrp.defines.append(grpDefine)

        target.compileGroups.append(cmpgrp)

def parseTargetBacktraceGraph(target, js):
    backtraceGraph_dict = js.get("backtraceGraph", {})
    if backtraceGraph_dict == {}:
        return
    target.backtraceGraph_commands = backtraceGraph_dict.get("commands", [])
    target.backtraceGraph_files = backtraceGraph_dict.get("files", [])

    nodes_arr = backtraceGraph_dict.get("nodes", [])
    for node_dict in nodes_arr:
        node = zspdx.cmakefileapi.TargetBacktraceGraphNode()
        node.file = node_dict.get("file", -1)
        node.line = node_dict.get("line", -1)
        node.command = node_dict.get("command", -1)
        node.parent = node_dict.get("parent", -1)
        target.backtraceGraph_nodes.append(node)

# Create direct pointers for all Configs in Codemodel
# takes: Codemodel
def linkCodemodel(cm):
    for cfg in cm.configurations:
        linkConfig(cfg)

# Create direct pointers for all contents of Config
# takes: Config
def linkConfig(cfg):
    for cfgDir in cfg.directories:
        linkConfigDir(cfg, cfgDir)
    for cfgPrj in cfg.projects:
        linkConfigProject(cfg, cfgPrj)
    for cfgTarget in cfg.configTargets:
        linkConfigTarget(cfg, cfgTarget)

# Create direct pointers for ConfigDir indices
# takes: Config and ConfigDir
def linkConfigDir(cfg, cfgDir):
    if cfgDir.parentIndex == -1:
        cfgDir.parent = None
    else:
        cfgDir.parent = cfg.directories[cfgDir.parentIndex]

    if cfgDir.projectIndex == -1:
        cfgDir.project = None
    else:
        cfgDir.project = cfg.projects[cfgDir.projectIndex]

    cfgDir.children = []
    for childIndex in cfgDir.childIndexes:
        cfgDir.children.append(cfg.directories[childIndex])

    cfgDir.targets = []
    for targetIndex in cfgDir.targetIndexes:
        cfgDir.targets.append(cfg.configTargets[targetIndex])

# Create direct pointers for ConfigProject indices
# takes: Config and ConfigProject
def linkConfigProject(cfg, cfgPrj):
    if cfgPrj.parentIndex == -1:
        cfgPrj.parent = None
    else:
        cfgPrj.parent = cfg.projects[cfgPrj.parentIndex]

    cfgPrj.children = []
    for childIndex in cfgPrj.childIndexes:
        cfgPrj.children.append(cfg.projects[childIndex])

    cfgPrj.directories = []
    for dirIndex in cfgPrj.directoryIndexes:
        cfgPrj.directories.append(cfg.directories[dirIndex])

    cfgPrj.targets = []
    for targetIndex in cfgPrj.targetIndexes:
        cfgPrj.targets.append(cfg.configTargets[targetIndex])

# Create direct pointers for ConfigTarget indices
# takes: Config and ConfigTarget
def linkConfigTarget(cfg, cfgTarget):
    if cfgTarget.directoryIndex == -1:
        cfgTarget.directory = None
    else:
        cfgTarget.directory = cfg.directories[cfgTarget.directoryIndex]

    if cfgTarget.projectIndex == -1:
        cfgTarget.project = None
    else:
        cfgTarget.project = cfg.projects[cfgTarget.projectIndex]

    # and link target's sources and source groups
    for ts in cfgTarget.target.sources:
        linkTargetSource(cfgTarget.target, ts)
    for tsg in cfgTarget.target.sourceGroups:
        linkTargetSourceGroup(cfgTarget.target, tsg)
    for tcg in cfgTarget.target.compileGroups:
        linkTargetCompileGroup(cfgTarget.target, tcg)

# Create direct pointers for TargetSource indices
# takes: Target and TargetSource
def linkTargetSource(target, targetSrc):
    if targetSrc.compileGroupIndex == -1:
        targetSrc.compileGroup = None
    else:
        targetSrc.compileGroup = target.compileGroups[targetSrc.compileGroupIndex]

    if targetSrc.sourceGroupIndex == -1:
        targetSrc.sourceGroup = None
    else:
        targetSrc.sourceGroup = target.sourceGroups[targetSrc.sourceGroupIndex]

# Create direct pointers for TargetSourceGroup indices
# takes: Target and TargetSourceGroup
def linkTargetSourceGroup(target, targetSrcGrp):
    targetSrcGrp.sources = []
    for srcIndex in targetSrcGrp.sourceIndexes:
        targetSrcGrp.sources.append(target.sources[srcIndex])

# Create direct pointers for TargetCompileGroup indices
# takes: Target and TargetCompileGroup
def linkTargetCompileGroup(target, targetCmpGrp):
    targetCmpGrp.sources = []
    for srcIndex in targetCmpGrp.sourceIndexes:
        targetCmpGrp.sources.append(target.sources[srcIndex])
