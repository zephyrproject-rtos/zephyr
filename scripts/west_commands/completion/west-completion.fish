# Copyright (c) 2025 Nancy Sangani
# SPDX-License-Identifier: Apache-2.0
# check if we are currently in a west workspace
# this is used to filter which command to show
#
# return 0 if in west workspace
# return 1 else
function __zephyr_west_check_if_in_workspace
    west topdir &>/dev/null
    if test $status = 0
        return 0
    else
        return 1
    end
end

# exclude the caller if one of the arguments is present in the command line
#
# return 1 if one of the arguments is present in the command line
# return 0 else
function __zephyr_west_exclude
    set -l tokens (commandline -opc)

    for t in $tokens
        for a in $argv
            if test $t = $a
                return 1
            end
        end
    end

    return 0
end

# function used to have a maximum number of arguments
#
# argv[1] is the maximum number of arguments
# argv[n] are the arguments to count, if not specified will count all arguments after 'west <command>' on the command line
#
# return 1 if the command line contain more than $argv[1] element from $argv[n...]
# return 0 else
function __zephyr_west_max_args
    set -l tokens (commandline -opc)
    set -l argc (count $argv)
    set -l max $argv[1]
    set -l counter 0

    if test $argc -eq 1
        if test (math (count $tokens) - 2) -ge $max
            return 1
        else
            return 0
        end
    end

    for idx in (seq 2 $argc)
        if contains $argv[idx] $tokens
            set counter (math $counter + 1)
        end
    end

    if $counter -ge $max
        return 1
    end

    return 0
end

# alias of '__fish_complete_directories' but set the arguments to ''
function __zephyr_west_complete_directories
    __fish_complete_directories '' ''
end

# check if a given token is the last one in the command line
#
# return 0 if one of the given argument is the last token
# return 1 else
function __zephyr_west_is_last_token
    set -l tokens (commandline -opc)

    for token in $argv
        if string match -qr -- "$token*" "$tokens[-1]"
            return 0
        end
    end

    return 1
end

# function similar to '__fish_use_subcommand' but with special cases
function __zephyr_west_use_subcommand
    set -l tokens (commandline -opc)

    for idx in (seq 2 (count $tokens))
        switch $tokens[$idx]
            case '-*'
                continue
            case '*'
                if test $idx -ge 3
                    set -l prv_idx (math $idx - 1)
                    switch $tokens[$prv_idx]
                        # this option can be placed before subcommand and require a folder
                        # if we don't do that the folder will be catched as a subcommand and
                        # the subcommands will not be completed
                        case '-z' '--zephyr-base'
                            continue
                    end
                end
            end
        return 1
    end

    return 0
end

# function similar to '__fish_seen_subcommand_from' but with special cases
function __zephyr_west_seen_subcommand_from
    set -l tokens (commandline -opc)
    set -e tokens[1]

    # special case:
    # we don't want the command completion when doing `west help <cmd>`
    if contains -- "help" $tokens
        return 1
    end

    for token in $tokens
        if contains -- $token $argv
            return 0
        end
    end

    return 1
end

# return the list of projects
function __zephyr_west_complete_projects
    set -l tokens (commandline -opc)
    set -l zephyr_base ""
    set -l projects

    for idx in (seq 1 (count $tokens))
        if test \("$tokens[$idx]" = "-z"\) -o \("$tokens[$idx]" = "--zephyr-base"\)
            if set -q $tokens[(math $idx + 1)]
                set $zephyr_base $tokens (math $idx + 1)
                break
            end
        end
    end

    if test $zephyr_base != ""
        set projects (west "-z $zephyr_base" list --format="{name}")
    else
        set projects (west list --format="{name}")
    end

    printf "%s\n" $projects
end

# return the list of available west commands
function __zephyr_west_complete_help
    set -l builtin_cmds "init" "create a west repository" \
                        "update" "update projects described in west manifest" \
                        "list" "print information about projects" \
                        "manifest" "manage the west manifest" \
                        "compare" "compare project status against the manifest" \
                        "diff" '"git diff" for one or more projects' \
                        "status" '"git status" for one or more projects' \
                        "forall" "run a command in one or more local projects" \
                        "grep" "run grep or a grep-like tool in one or more local projects" \
                        "help" "get help for west or a command" \
                        "config" "get or set config file values" \
                        "topdir" "print the top level directory of the workspace"
    set -l nb_builtin_cmds (count $builtin_cmds)

    set -l ext_cmds "completion" "display shell completion scripts" \
                    "boards" "display information about supported boards" \
                    "shields" "display list of supported shields" \
                    "build" "compile a Zephyr application" \
                    "twister" "west twister wrapper" \
                    "sign" "sign a Zephyr binary for bootloader chain-loading" \
                    "flash" "flash and run a binary on a board" \
                    "debug" "flash and interactively debug a Zephyr application" \
                    "debugserver" "connect to board and launch a debug server" \
                    "attach" "interactively debug a board" \
                    "rtt" "open an rtt shell" \
                    "zephyr-export" "export Zephyr installation as a CMake config package" \
                    "spdx" "create SPDX bill of materials" \
                    "blobs" "work with binary blobs" \
                    "bindesc" "work with Binary Descriptors" \
                    "robot" "run RobotFramework test suites" \
                    "simulate" "simulate board" \
                    "sdk" "manage SDKs" \
                    "packages" "manage packages for Zephyr" \
                    "patch" "manage patches for Zephyr modules" \
                    "gtags" "create a GNU global tags file for the current workspace"
    set -l nb_ext_cmds (count $ext_cmds)

    if __zephyr_west_check_if_in_workspace
        for idx in (seq 1 2 $nb_ext_cmds)
            set -l desc_idx (math $idx + 1)
            printf "%s\n" $ext_cmds[$idx]\t"$ext_cmds[$desc_idx]"
        end
    end

    for idx in (seq 1 2 $nb_builtin_cmds)
        set -l desc_idx (math $idx + 1)
        printf "%s\n" $builtin_cmds[$idx]\t"$builtin_cmds[$desc_idx]"
    end
end

function __zephyr_west_topdir
    set -l cwd (pwd)
    set -l fallback 1

    if test (count $argv) -eq 2
        set cwd $argv[1]
        set fallback $argv[2]
    end

    set -l cwd_split (string split '/' $cwd)

    while true
        set -l tmp_path (path normalize /(string join "/" $cwd_split))

        if test -d $tmp_path/.west
            echo "$tmp_path"
            return
        end

        if test -z "$tmp_path" -o $tmp_path = "/"
            break
        end

        set -e cwd_split[-1]
        set tmp_path (string join "/" $cwd_split)
    end

    if test $fallback -eq 1 -a -n "$ZEPHYR_BASE"
        west-topdir "$ZEPHYR_BASE" 0
    end
end

function __zephyr_west_manifest_path
    set -l west_topdir (__zephyr_west_topdir)

    set -l config (cat $west_topdir/.west/config)

    set -l manifest_path ""
    set -l manifest_file ""

    set -l in_manifest_group 0

    for line in $config
        if string match -rq '^\s*\[manifest\]\s*$' $line
            set in_manifest_group 1
            continue
        else if string match -rq '^\[.*\]$' $line
            set in_manifest_group 0
            continue
        end

        if test $in_manifest_group -eq 1
            set -l tmp_manifest_path (string match -r '^path\s*=\s*(\S*)\s*$' $line)[2]
            if test $status -eq 0
                set manifest_path "$tmp_manifest_path"
                continue
            end

            set -l tmp_manifest_file (string match -r '^file\s*=\s*(\S*)\s*$' $line)[2]
            if test $status -eq 0
                set manifest_file "$tmp_manifest_file"
                continue
            end
        end
    end

    if test -z "$manifest_path" -o -z "$manifest_file"
        return
    end

    echo (path normalize "$west_topdir"/"$manifest_path"/"$manifest_file")
end

function __zephyr_west_get_cache_dir
    set -l manifest_path $argv[1]
    set -l manifest_path_hash (echo "$manifest_path" | md5sum | string trim --chars=' -')

    echo (__zephyr_west_topdir)/.west/fish/$manifest_path_hash
end

function __zephyr_west_complete_board
    set -l is_cache_valid 1 # 0: invalid; 1: valid

    set -l manifest_file (__zephyr_west_manifest_path)
    set -l manifest_dir (path dirname "$manifest_file")

    set -l cache_folder (__zephyr_west_get_cache_dir "$manifest_file")
    set -l cache_file $cache_folder/fish_boards_completion.cache

    set -l manifest_hash (git --work-tree "$manifest_dir" log -1 --pretty=format:'%H' 2> /dev/null)

    if test $status -ne 0
        # if the manifest folder is not a git repo, use the hash of the manifest file
        set manifest_hash (md5sum "$manifest_path")
    end

    if test ! -f $cache_file
        mkdir -p $cache_folder
        touch $cache_file

        set is_cache_valid 0
    else
        set -l cache_manifest_hash (head -n 1 $cache_file)

        if test -z "$manifest_hash" -o -z "$cache_manifest_hash" -o "$manifest_hash" != "$cache_manifest_hash"
            set is_cache_valid 0
        end
    end

    if test $is_cache_valid -eq 0
        set -l targets (west boards --all-targets 2> /dev/null)

        if test $status -eq 0
            echo $manifest_hash > $cache_file
        end

        for target in $targets
            printf "%s\n" $target >> $cache_file
        end
    end

    tail -n +2 $cache_file
end

function __zephyr_west_complete_shield
    set -l is_cache_valid 1 # 0: invalid; 1: valid

    set -l manifest_file (__zephyr_west_manifest_path)
    set -l manifest_dir (path dirname "$manifest_file")

    set -l cache_folder (__zephyr_west_get_cache_dir "$manifest_file")
    set -l cache_file $cache_folder/fish_shields_completion.cache

    set -l manifest_hash (git --work-tree "$manifest_dir" log -1 --pretty=format:'%H' 2> /dev/null)

    if test $status -ne 0
        # if the manifest folder is not a git repo, use the hash of the manifest file
        set manifest_hash (md5sum "$manifest_path")
    end

    if test ! -f $cache_file
        mkdir -p $cache_folder
        touch $cache_file

        set is_cache_valid 0
    else
        set -l cache_manifest_hash (head -n 1 $cache_file)

        if test -z "$manifest_hash" -o -z "$cache_manifest_hash" -o "$manifest_hash" != "$cache_manifest_hash"
            set is_cache_valid 0
        end
    end

    if test $is_cache_valid -eq 0
        set -l shields (west shields 2> /dev/null)

        if test $status -eq 0
            echo $manifest_hash > $cache_file
        end

        for shield in $shields
            printf "%s\n" $shield >> $cache_file
        end
    end

    tail -n +2 $cache_file
end

# disable file completion, if an option need it, it should use '--force-files'
complete -c west -f

# global options
complete -c west -n "__zephyr_west_exclude -h --help" -o h -l help -d "show help"
complete -c west -o v -l verbose -d "enable verbosity"
complete -c west -n "__zephyr_west_exclude -V --version" -o V -l version -d "print version"
complete -c west -n "__zephyr_west_exclude -z --zephyr-base; or __zephyr_west_is_last_token -z --zephyr-base" -o z -l zephyr-base -xa "(__zephyr_west_complete_directories)" -d "zephyr base folder"

# init
complete -c west -n __zephyr_west_use_subcommand -ra init -d "create a west workspace"
complete -c west -n "__zephyr_west_seen_subcommand_from init" -ra "(__zephyr_west_complete_directories)"
complete -c west -n "__zephyr_west_seen_subcommand_from init; and __zephyr_west_exclude -l --local" -l mr -l manifest-rev -r -d "manifest revision"
complete -c west -n "__zephyr_west_seen_subcommand_from init" -l mf -l manifest-file -r -d "manifest file"
complete -c west -n "__zephyr_west_seen_subcommand_from init; and __zephyr_west_exclude -l --local" -o m -l manifest -ra "(__zephyr_west_complete_directories)" -d "manifest URL"
complete -c west -n "__zephyr_west_seen_subcommand_from init; and __zephyr_west_exclude -m --manifest --mr --manifest-rev" -o l -l local -ra "(__zephyr_west_complete_directories)" -d "use local directory as manifest repository"

# update
complete -c west -n __zephyr_west_use_subcommand -ra update -d "update projects described in west manifest"
complete -c west -n "__zephyr_west_seen_subcommand_from update" -ra "(__zephyr_west_complete_projects)"
complete -c west -n "__zephyr_west_seen_subcommand_from update" -l stats -d "print performance stats"
complete -c west -n "__zephyr_west_seen_subcommand_from update" -l name-cache -ra "(__zephyr_west_complete_directories)" -d "name-based cache"
complete -c west -n "__zephyr_west_seen_subcommand_from update" -l path-cache -ra "(__zephyr_west_complete_directories)" -d "path-based cache"
complete -c west -n "__zephyr_west_seen_subcommand_from update" -o f -l fetch -ra "always smart" -d "fetch strategy"
complete -c west -n "__zephyr_west_seen_subcommand_from update" -o o -l fetch-opt -d "fetch options"
complete -c west -n "__zephyr_west_seen_subcommand_from update" -o n -l narrow -d "narrow fetch"
complete -c west -n "__zephyr_west_seen_subcommand_from update" -o k -l keep-descendants -d "keep manifest-rev descendants checked out"
complete -c west -n "__zephyr_west_seen_subcommand_from update" -o r -l rebase -d "rebase checked out branch onto the new manifest-rev"

# list
complete -c west -n __zephyr_west_use_subcommand -ra list -d "print information about projects"
complete -c west -n "__zephyr_west_seen_subcommand_from list; and not __fish_seen_subcommand_from blobs" -ra "(__zephyr_west_complete_projects)"
complete -c west -n "__zephyr_west_seen_subcommand_from list; and not __fish_seen_subcommand_from blobs" -o a -l all -d "include inactive projects"
complete -c west -n "__zephyr_west_seen_subcommand_from list; and not __fish_seen_subcommand_from blobs" -l manifest-path-from-yaml -d "print performance stats"
complete -c west -n "__zephyr_west_seen_subcommand_from list; and not __fish_seen_subcommand_from blobs" -o f -l format -d "format string"

# manifest
complete -c west -n __zephyr_west_use_subcommand -ra manifest -d "manage the west manifest"
complete -c west -n "__zephyr_west_seen_subcommand_from manifest" -l resolve -d "resolve into single manifest"
complete -c west -n "__zephyr_west_seen_subcommand_from manifest" -l freeze -d "resolve into single manifest, with SHAs"
complete -c west -n "__zephyr_west_seen_subcommand_from manifest" -l validate -d "silently validate manifest"
complete -c west -n "__zephyr_west_seen_subcommand_from manifest" -l path -d "print the path to the top level manifest file"
complete -c west -n "__zephyr_west_seen_subcommand_from manifest" -o o -l output -rF -d "output file"

# diff
complete -c west -n __zephyr_west_use_subcommand -ra diff -d '"git diff" for one or more projects'
complete -c west -n "__zephyr_west_seen_subcommand_from diff" -ra "(__zephyr_west_complete_projects)"
complete -c west -n "__zephyr_west_seen_subcommand_from diff" -o a -l all -d "include inactive projects"

# status
complete -c west -n __zephyr_west_use_subcommand -ra status -d '"git status" for one or more projects'
complete -c west -n "__zephyr_west_seen_subcommand_from status" -ra "(__zephyr_west_complete_projects)"
complete -c west -n "__zephyr_west_seen_subcommand_from status" -o a -l all -d "include inactive projects"

# forall
complete -c west -n __zephyr_west_use_subcommand -ra forall -d "run a command in one or more local projects"
complete -c west -n "__zephyr_west_seen_subcommand_from forall" -ra "(__zephyr_west_complete_projects)"
complete -c west -n "__zephyr_west_seen_subcommand_from forall" -o c -x -d "command to execute"
complete -c west -n "__zephyr_west_seen_subcommand_from forall" -o a -l all -d "include inactive projects"
complete -c west -n "__zephyr_west_seen_subcommand_from forall" -o g -l group -x -d "run command on projects in one of the group"

# config
complete -c west -n __zephyr_west_use_subcommand -ra config -d "get or set config file values"
complete -c west -n "__zephyr_west_seen_subcommand_from config" -o l -l list -d "list all options and values"
complete -c west -n "__zephyr_west_seen_subcommand_from config" -o d -l delete -d "delete an option in one config file"
complete -c west -n "__zephyr_west_seen_subcommand_from config" -o D -l delete-all -d "delete an option everywhere it's set"
complete -c west -n "__zephyr_west_seen_subcommand_from config" -l system -d "system-wide file"
complete -c west -n "__zephyr_west_seen_subcommand_from config" -l global -d "global user-wide"
complete -c west -n "__zephyr_west_seen_subcommand_from config" -l local -d "this workspace's file"

# topdir
complete -c west -n __zephyr_west_use_subcommand -a topdir -d "print the top level directory of the workspace"

# help
complete -c west -n __zephyr_west_use_subcommand -ra help -d "get help for west or a command"
complete -c west -n "__fish_seen_subcommand_from help; and __zephyr_west_max_args 1" -ra "(__zephyr_west_complete_help)"



# completion
complete -c west -n "__zephyr_west_use_subcommand; and __zephyr_west_check_if_in_workspace" -ra completion -d "display shell completion scripts"
complete -c west -n "__zephyr_west_seen_subcommand_from completion; and __zephyr_west_max_args 1" -ra "bash zsh fish"

# boards
complete -c west -n "__zephyr_west_use_subcommand; and __zephyr_west_check_if_in_workspace" -ra boards -d "display information about supported boards"
complete -c west -n "__zephyr_west_seen_subcommand_from boards" -o f -l format -d "format string"
complete -c west -n "__zephyr_west_seen_subcommand_from boards" -o n -l name -d "name regex"
complete -c west -n "__zephyr_west_seen_subcommand_from boards" -o a -l all-targets -d "output all board target combinations"
complete -c west -n "__zephyr_west_seen_subcommand_from boards" -l arch-root -xa "(__zephyr_west_complete_directories)" -d "add an arch root"
complete -c west -n "__zephyr_west_seen_subcommand_from boards" -l board-root -xa "(__zephyr_west_complete_directories)" -d "add a board root"
complete -c west -n "__zephyr_west_seen_subcommand_from boards" -l soc-root -xa "(__zephyr_west_complete_directories)" -d "add a soc root"
complete -c west -n "__zephyr_west_seen_subcommand_from boards" -l board -xa "(__zephyr_west_complete_board)" -d "lookup the specific board"
complete -c west -n "__zephyr_west_seen_subcommand_from boards" -l board-dir -xa "(__zephyr_west_complete_directories)" -d "only look for boards in this directory"

# shields
complete -c west -n "__zephyr_west_use_subcommand; and __zephyr_west_check_if_in_workspace" -ra shields -d "display list of supported shields"
complete -c west -n "__zephyr_west_seen_subcommand_from shields" -o f -l format -d "format string"
complete -c west -n "__zephyr_west_seen_subcommand_from shields" -o n -l name -d "name regex"
complete -c west -n "__zephyr_west_seen_subcommand_from shields" -l board-root -xa "(__zephyr_west_complete_directories)" -d "add a board root"

# build
complete -c west -n "__zephyr_west_use_subcommand; and __zephyr_west_check_if_in_workspace" -ra build -d "compile a Zephyr application"
complete -c west -n "__zephyr_west_seen_subcommand_from build" -ra "(__zephyr_west_complete_directories)"
complete -c west -n "__zephyr_west_seen_subcommand_from build" -o b -l board -xa "(__zephyr_west_complete_board)"
complete -c west -n "__zephyr_west_seen_subcommand_from build" -l shield -xa "(__zephyr_west_complete_shield)" -d "shield to use"
complete -c west -n "__zephyr_west_seen_subcommand_from build" -o d -l build-dir -xa "(__zephyr_west_complete_directories)" -d "build directory to create or use"
complete -c west -n "__zephyr_west_seen_subcommand_from build" -o f -l force -d "ignore errors and continue"
complete -c west -n "__zephyr_west_seen_subcommand_from build" -l sysbuild -d "create multi-domain build system"
complete -c west -n "__zephyr_west_seen_subcommand_from build" -l no-sysbuild -d "do not create multi-domain build system"
complete -c west -n "__zephyr_west_seen_subcommand_from build" -o c -l cmake -d "force a cmake run"
complete -c west -n "__zephyr_west_seen_subcommand_from build" -l domain -d "execute build tool (make or ninja) for a given domain"
complete -c west -n "__zephyr_west_seen_subcommand_from build" -o t -l target -d "run build system target"
complete -c west -n "__zephyr_west_seen_subcommand_from build" -o T -l test-item -d "build based on test data in .yml"
complete -c west -n "__zephyr_west_seen_subcommand_from build" -o o -l build-opt -d "options to pass to build tool (make or ninja)"
complete -c west -n "__zephyr_west_seen_subcommand_from build" -o n -l just-print -l dry-run -l recon -d "just print build commands, don't run them"
complete -c west -n "__zephyr_west_seen_subcommand_from build" -o p -l pristine -ra "auto always never" -d "pristine build setting"

# twister
complete -c west -n "__zephyr_west_use_subcommand; and __zephyr_west_check_if_in_workspace" -ra twister -d "west twister wrapper"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l aggressive-no-clean -d "do not clean if test case is not a subset of previous run"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o l -l all -d "select all possible test cases"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o D -l all-deltas -d "show all footprint deltas"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l allow-installed-plugin -d "allow the use of installed pytest plugin"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o b -l build-only -d "build only, do not run"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o c -l clobber-output -d "clobber test runner output"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l cmake-only -d "run cmake only"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o C -l coverage -d "generate coverage reports"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l create-rom-ram-report -d "create ROM/RAM report"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l detailed-skipped-report -d "generate detailed report of skipped tests"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l detailed-test-id -d "use a detailed test identifier"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l device-flash-with-test -d "flash device together with test"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l device-testing -d "test on a connected device"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l disable-suite-name-check -d "disable suite name check"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l disable-unrecognized-section-test -d "disable unrecognized section test"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o W -l disable-warnings-as-errors -d "do not treat warnings as errors"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o y -l dry-run -d "do a dry run, no actual build or run"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l emulation-only -d "select only emulation platforms"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l enable-asan -d "enable address sanitizer"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l enable-coverage -d "enable code coverage"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l enable-lsan -d "enable leak sanitizer"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l enable-size-report -d "enable size report"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o S -l enable-slow -d "include slow tests"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l enable-slow-only -d "run only slow tests"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l enable-ubsan -d "enable undefined behavior sanitizer"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l enable-valgrind -d "enable valgrind"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l flash-before -d "flash before running test"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l footprint-from-buildlog -d "extract footprint from build log"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l force-color -d "force colored output"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o K -l force-platform -d "force testing on selected platforms"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l force-toolchain -d "force toolchain"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l ignore-platform-key -d "ignore platform key"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o i -l inline-logs -d "inline logs for failed tests"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o G -l integration -d "integration mode"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o m -l last-metrics -d "compare metrics to the last completed run"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l list-tags -d "list tags"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l list-tests -d "list tests"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o k -l make -d "use make"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o N -l ninja -d "use ninja"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o n -l no-clean -d "do not clean results"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l no-detailed-test-id -d "do not use a detailed test identifier"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o u -l no-update -d "do not update the hardware map"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o f -l only-failed -d "run only tests that failed in last run"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l overflow-as-errors -d "treat overflows as errors"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l persistent-hardware-map -d "create persistent hardware map"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l platform-reports -d "create individual reports for each platform"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l prep-artifacts-for-testing -d "prepare artifacts for testing"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l quarantine-verify -d "verify the quarantine list"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l retry-build-errors -d "retry on build errors"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l short-build-path -d "use short build path"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l show-footprint -d "show footprint"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l shuffle-tests -d "shuffle tests"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l test-only -d "test only, do not build"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l test-tree -d "print the test tree"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l timestamps -d "print timestamps"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o v -l verbose -d "increase verbosity"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o a -l arch -r -d "architecture filter"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o A -l board-root -xa "(__zephyr_west_complete_directories)" -d "add a board root"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l alt-config-root -xa "(__zephyr_west_complete_directories)" -d "alternative configuration root"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l coverage-basedir -xa "(__zephyr_west_complete_directories)" -d "base directory for coverage"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o O -l outdir -xa "(__zephyr_west_complete_directories)" -d "output directory"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o o -l report-dir -xa "(__zephyr_west_complete_directories)" -d "output reports directory"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o T -l testsuite-root -xa "(__zephyr_west_complete_directories)" -d "testsuite root directory"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l compare-report -rF -d "compare against a report"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l device-serial -rF -d "device serial port"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l device-serial-pty -rF -d "device serial PTY"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l gcov-tool -rF -d "path to gcov tool"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l generate-hardware-map -rF -d "generate a hardware map"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l hardware-map -rF -d "use a hardware map"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o F -l load-tests -rF -d "load tests from a file"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l log-file -rF -d "log to a file"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l package-artifacts -rF -d "package artifacts into a file"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l pre-script -rF -d "pre-script to run"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l quarantine-list -rF -d "load a quarantine list"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o E -l save-tests -rF -d "save tests to a file"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o z -l size -rF -d "show binary sizes"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l test-config -rF -d "test configuration file"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l coverage-tool -xa "gcovr lcov" -d "coverage tool"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o P -l exclude-platform -xa "(__zephyr_west_complete_board)" -d "exclude a platform"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l filter -xa "buildable runnable" -d "filter by status"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l log-level -xa "CRITICAL DEBUG ERROR INFO NOTSET WARNING" -d "log level"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o p -l platform -xa "(__zephyr_west_complete_board)" -d "platform filter"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l coverage-platform -xa "(__zephyr_west_complete_board)" -d "platform to use for coverage"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o M -l runtime-artifact-cleanup -xa "all pass" -d "artifact cleanup strategy"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l coverage-formats -r -d "coverage formats"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l device-flash-timeout -r -d "device flash timeout"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l device-serial-baud -r -d "device serial baud rate"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o e -l exclude-tag -r -d "exclude tests with a tag"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o x -l extra-args -r -d "extra CMake arguments"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o X -l fixture -r -d "required fixture"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o H -l footprint-threshold -r -d "footprint threshold"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o j -l jobs -r -d "number of jobs"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l level -r -d "test level"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l pytest-args -r -d "extra pytest arguments"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l report-name -r -d "report name"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l report-suffix -r -d "report suffix"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l retry-failed -r -d "retry failed tests"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l retry-interval -r -d "retry interval"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o s -l scenario -l test -r -d "run only this scenario"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l seed -r -d "random seed"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l shuffle-tests-seed -r -d "shuffle tests seed"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l sub-test -r -d "run only this sub-test"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o B -l subset -r -d "test subset (e.g. 1/5)"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -o t -l tag -r -d "tag filter"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l timeout-multiplier -r -d "timeout multiplier"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l vendor -r -d "vendor filter"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l west-flash -r -d "west flash arguments"
complete -c west -n "__zephyr_west_seen_subcommand_from twister" -l west-runner -r -d "west runner"

# sign
complete -c west -n "__zephyr_west_use_subcommand; and __zephyr_west_check_if_in_workspace" -ra sign -d "sign a Zephyr binary for bootloader chain-loading"
complete -c west -n "__zephyr_west_seen_subcommand_from sign" -o d -l build-dir -ra "(__zephyr_west_complete_directories)" -d "build directory to create or use"
complete -c west -n "__zephyr_west_seen_subcommand_from sign" -o q -l quiet -d "suppress non-error output"
complete -c west -n "__zephyr_west_seen_subcommand_from sign" -o f -l force -d "ignore errors and continue"
complete -c west -n "__zephyr_west_seen_subcommand_from sign" -o t -l tool -ra "imgtool rimage" -d "image signing tool name"
complete -c west -n "__zephyr_west_seen_subcommand_from sign" -o p -l tool-path -ra "(__zephyr_west_complete_directories)" -d "path to the tool"
complete -c west -n "__zephyr_west_seen_subcommand_from sign" -o P -l tool-data -ra "(__zephyr_west_complete_directories)" -d "path to tool data"
complete -c west -n "__zephyr_west_seen_subcommand_from sign; and __zephyr_west_exclude --no-bin" -l bin -d "produce a signed bin file"
complete -c west -n "__zephyr_west_seen_subcommand_from sign; and __zephyr_west_exclude --bin" -l no-bin -d "do not produce a signed bin file"
complete -c west -n "__zephyr_west_seen_subcommand_from sign" -o B -l sbin -rF -d "signed .bin filename"
complete -c west -n "__zephyr_west_seen_subcommand_from sign; and __zephyr_west_exclude --no-hex" -l hex -d "produce a signed hex file"
complete -c west -n "__zephyr_west_seen_subcommand_from sign; and __zephyr_west_exclude --hex" -l no-hex -d "do not produce a signed hex file"
complete -c west -n "__zephyr_west_seen_subcommand_from sign" -o H -l shex -rF -d "signed .hex filename"

# flash
complete -c west -n "__zephyr_west_use_subcommand; and __zephyr_west_check_if_in_workspace" -ra flash -d "flash and run a binary on a board"

# debug
complete -c west -n "__zephyr_west_use_subcommand; and __zephyr_west_check_if_in_workspace" -ra debug -d "flash and interactively debug a Zephyr application"

# debugserver
complete -c west -n "__zephyr_west_use_subcommand; and __zephyr_west_check_if_in_workspace" -ra debugserver -d "connect to board and launch a debug server"

# attach
complete -c west -n "__zephyr_west_use_subcommand; and __zephyr_west_check_if_in_workspace" -ra attach -d "interactively debug a board"

## flash, debug, debugserver, attach
complete -c west -n "__zephyr_west_seen_subcommand_from flash debug debugserver attach" -o d -l build-dir -ra "(__zephyr_west_complete_directories)" -d "build directory to create or use"
complete -c west -n "__zephyr_west_seen_subcommand_from flash debug debugserver attach" -o r -l runner -r -d "override default runner from build-dir"
complete -c west -n "__zephyr_west_seen_subcommand_from flash debug debugserver attach" -l rebuild -l no-rebuild -d "manually specify to reinvoke cmake or not"
complete -c west -n "__zephyr_west_seen_subcommand_from flash debug debugserver attach" -l domain -r -d "execute build tool (make or ninja) for a given domain"
complete -c west -n "__zephyr_west_seen_subcommand_from flash debug debugserver attach" -o H -l context -d "print runner-specific options"
complete -c west -n "__zephyr_west_seen_subcommand_from flash debug debugserver attach" -l board-dir -ra "(__zephyr_west_complete_directories)" -d "board directory"
complete -c west -n "__zephyr_west_seen_subcommand_from flash debug debugserver attach" -o f -l file -Fr -d "path to binary"
complete -c west -n "__zephyr_west_seen_subcommand_from flash debug debugserver attach" -o t -l file-type -ra "hex bin elf" -d "type of binary"
complete -c west -n "__zephyr_west_seen_subcommand_from flash debug debugserver attach" -l elf-file -rka "(__fish_complete_suffix .elf)" -d "path to zephyr.elf"
complete -c west -n "__zephyr_west_seen_subcommand_from flash debug debugserver attach" -l hex-file -rka "(__fish_complete_suffix .hex)" -d "path to zephyr.hex"
complete -c west -n "__zephyr_west_seen_subcommand_from flash debug debugserver attach" -l bin-file -rka "(__fish_complete_suffix .bin)" -d "path to zephyr.bin"
complete -c west -n "__zephyr_west_seen_subcommand_from flash debug debugserver attach" -l gdb -Fr -d "path to GDB"
complete -c west -n "__zephyr_west_seen_subcommand_from flash debug debugserver attach" -l openocd -Fr -d "path to openocd"
complete -c west -n "__zephyr_west_seen_subcommand_from flash debug debugserver attach" -l openocd-search -ra "(__zephyr_west_complete_directories)" -d "path to add to openocd search path"

# zephyr-export
complete -c west -n "__zephyr_west_use_subcommand; and __zephyr_west_check_if_in_workspace" -ra zephyr-export -d "export Zephyr installation as a CMake config package"

# spdx
complete -c west -n "__zephyr_west_use_subcommand; and __zephyr_west_check_if_in_workspace" -ra spdx -d "create SPDX bill of materials"
complete -c west -n "__zephyr_west_seen_subcommand_from spdx" -o i -l init -d "initialize CMake file-based API"
complete -c west -n "__zephyr_west_seen_subcommand_from spdx" -o d -l build-dir -ra "(__zephyr_west_complete_directories)" -d "build directory to create or use"
complete -c west -n "__zephyr_west_seen_subcommand_from spdx" -o n -l namespace-prefix -rf -d "namespace prefix"
complete -c west -n "__zephyr_west_seen_subcommand_from spdx" -o s -l spdx-dir -ra "(__zephyr_west_complete_directories)" -d "SPDX output directory"
complete -c west -n "__zephyr_west_seen_subcommand_from spdx" -l analyze-includes -d "also analyze included header files"
complete -c west -n "__zephyr_west_seen_subcommand_from spdx" -l include-sdk -d "also generate SPDX document for SDK"

# blobs
complete -c west -n "__zephyr_west_use_subcommand; and __zephyr_west_check_if_in_workspace" -ra blobs -d "work with binary blobs"
complete -c west -n "__zephyr_west_seen_subcommand_from blobs; and not __fish_seen_subcommand_from list fetch clean" -ra "list\t'list binary blobs' fetch\t'fetch binary blobs' clean\t'clean working tree of binary blobs'"
complete -c west -n "__zephyr_west_seen_subcommand_from blobs; and __fish_seen_subcommand_from list fetch clean" -ra "(__zephyr_west_complete_projects)"
complete -c west -n "__zephyr_west_seen_subcommand_from blobs; and not __fish_seen_subcommand_from fetch clean" -o f -l format -r -d "format string"

# sdk
complete -c west -n "__zephyr_west_use_subcommand; and __zephyr_west_check_if_in_workspace" -ra sdk -d "manage SDKs"
complete -c west -n "__zephyr_west_seen_subcommand_from sdk; and not __fish_seen_subcommand_from list install" -ra "list\t'list installed SDKs' install\t'install SDK'"
complete -c west -n "__zephyr_west_seen_subcommand_from sdk; and __fish_seen_subcommand_from install" -l version -d "version of the Zephyr SDK to install"
complete -c west -n "__zephyr_west_seen_subcommand_from sdk; and __fish_seen_subcommand_from install" -o b -l install-base -d "SDK isntall base directory"
complete -c west -n "__zephyr_west_seen_subcommand_from sdk; and __fish_seen_subcommand_from install" -o d -l install-dir -d "SDK isntall destination directory"
complete -c west -n "__zephyr_west_seen_subcommand_from sdk; and __fish_seen_subcommand_from install" -o i -l interactive -d "interactive"
complete -c west -n "__zephyr_west_seen_subcommand_from sdk; and __fish_seen_subcommand_from install" -o t -l toolchains -d "toolchain(s) to install"
complete -c west -n "__zephyr_west_seen_subcommand_from sdk; and __fish_seen_subcommand_from install" -o T -l no-toolchains -d "do not install toolchains"
complete -c west -n "__zephyr_west_seen_subcommand_from sdk; and __fish_seen_subcommand_from install" -o H -l no-hosttools -d "do not install host-tools"
complete -c west -n "__zephyr_west_seen_subcommand_from sdk; and __fish_seen_subcommand_from install" -l personal-access-token -d "GitHub personal access token"
complete -c west -n "__zephyr_west_seen_subcommand_from sdk; and __fish_seen_subcommand_from install" -l api-url -d "GitHub releases API endpoint URL"
