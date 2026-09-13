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
                        # if we don't do that the folder will be caught as a subcommand and
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

function __zephyr_west_zephyr_base
    if test -n "$ZEPHYR_BASE"
        echo "$ZEPHYR_BASE"
        return 0
    end
    set -l top (__zephyr_west_topdir)
    or return 1
    set -l base (awk '
        $0 ~ /^\[zephyr\]/ { in_z=1; next }
        $0 ~ /^\[/ { in_z=0 }
        in_z && $1 == "base" {
            sub(/^[^=]*=[[:space:]]*/, "")
            gsub(/[[:space:]]+$/, "")
            print
            exit
        }
    ' $top/.west/config 2>/dev/null)
    if test -n "$base"
        if string match -q '/*' $base
            echo $base
        else
            echo $top/$base
        end
        return 0
    end
    if test -d $top/zephyr
        echo $top/zephyr
        return 0
    end
    return 1
end

function __zephyr_west_completion_stamp
    set -l top (__zephyr_west_topdir)
    or return 1
    set -l zb (__zephyr_west_zephyr_base)
    begin
        printf '%s\n' $zb $top/.west/config
        if test -n "$zb"
            test -d $zb/boards; and find $zb/boards -maxdepth 1
            test -d $zb/snippets; and find $zb/snippets -maxdepth 1
            test -d $zb/boards/shields; and find $zb/boards/shields -maxdepth 1
        end
        for p in $top/*/zephyr/module.yml
            if test -f $p
                printf '%s\n' $p
                set -l root (string replace -r '/zephyr/module.yml$' '' $p)
                test -d $root/boards; and printf '%s\n' $root/boards
                test -d $root/snippets; and printf '%s\n' $root/snippets
            end
        end
    end 2>/dev/null | begin
        if stat -f %m $top >/dev/null 2>&1
            xargs stat -f '%m:%N'
        else
            xargs stat -c '%Y:%n'
        end
    end 2>/dev/null | cksum
end

function __zephyr_west_completion_dir
    set -l top (__zephyr_west_topdir)
    or return 1
    echo $top/.west/completion
end

function __zephyr_west_completion_list
    set -l kind $argv[1]
    set -e argv[1]
    set -l dir (__zephyr_west_completion_dir)
    or begin
        west $argv 2>/dev/null
        return
    end
    set -l stamp (__zephyr_west_completion_stamp)
    or begin
        west $argv 2>/dev/null
        return
    end
    set -l cache_file $dir/$kind
    if test -f $cache_file
        set -l cached (head -n 1 $cache_file)
        if test -n "$stamp" -a "$cached" = "$stamp"
            tail -n +2 $cache_file
            return 0
        end
    end
    set -l out (west $argv 2>/dev/null)
    if test $status -ne 0
        return 1
    end
    mkdir -p $dir 2>/dev/null
    echo $stamp > $cache_file
    printf "%s\n" $out >> $cache_file
    printf "%s\n" $out
end

function __zephyr_west_complete_board
    __zephyr_west_completion_list boards boards --all-targets
end

function __zephyr_west_complete_shield
    __zephyr_west_completion_list shields shields --format={name}
end

function __zephyr_west_complete_snippets
    __zephyr_west_completion_list snippets snippets --format={name}
end
