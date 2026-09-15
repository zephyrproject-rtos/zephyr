# Copyright (c) 2006-2008, Ian Macdonald <ian@caliban.org>
# Copyright (c) 2009-2010, Bash Completion Maintainers
# Copyright (c) 2026 Zephyr Project contributors
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Bash auto-completion for west subcommands and flags. To initialize, run
#
#     source west-completion.bash
#
# To make it persistent, add it to e.g. your .bashrc.

__west_previous_extglob_setting=$(shopt -p extglob)
shopt -s extglob

# The following function is based on code from:
#
#   bash_completion - programmable completion functions for bash 3.2+
#
#   Copyright © 2006-2008, Ian Macdonald <ian@caliban.org>
#             © 2009-2010, Bash Completion Maintainers
#                     <bash-completion-devel@lists.alioth.debian.org>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2, or (at your option)
#   any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, see <http://www.gnu.org/licenses/>.
#
#   The latest version of this software can be obtained here:
#
#   http://bash-completion.alioth.debian.org/
#
#   RELEASE: 2.x

# This function can be used to access a tokenized list of words
# on the command line:
#
#	__git_reassemble_comp_words_by_ref '=:'
#	if test "${words_[cword_-1]}" = -w
#	then
#		...
#	fi
#
# The argument should be a collection of characters from the list of
# word completion separators (COMP_WORDBREAKS) to treat as ordinary
# characters.
#
# This is roughly equivalent to going back in time and setting
# COMP_WORDBREAKS to exclude those characters.  The intent is to
# make option types like --date=<type> and <rev>:<path> easy to
# recognize by treating each shell word as a single token.
#
# It is best not to set COMP_WORDBREAKS directly because the value is
# shared with other completion scripts.  By the time the completion
# function gets called, COMP_WORDS has already been populated so local
# changes to COMP_WORDBREAKS have no effect.
#
# Output: words_, cword_, cur_.

__west_reassemble_comp_words_by_ref()
{
	local exclude i j first
	# Which word separators to exclude?
	exclude="${1//[^$COMP_WORDBREAKS]}"
	cword_=$COMP_CWORD
	if [ -z "$exclude" ]; then
		words_=("${COMP_WORDS[@]}")
		return
	fi
	# List of word completion separators has shrunk;
	# re-assemble words to complete.
	for ((i=0, j=0; i < ${#COMP_WORDS[@]}; i++, j++)); do
		# Append each nonempty word consisting of just
		# word separator characters to the current word.
		first=t
		while
			[ $i -gt 0 ] &&
			[ -n "${COMP_WORDS[$i]}" ] &&
			# word consists of excluded word separators
			[ "${COMP_WORDS[$i]//[^$exclude]}" = "${COMP_WORDS[$i]}" ]
		do
			# Attach to the previous token,
			# unless the previous token is the command name.
			if [ $j -ge 2 ] && [ -n "$first" ]; then
				((j--))
			fi
			first=
			words_[$j]=${words_[j]}${COMP_WORDS[i]}
			if [ $i = $COMP_CWORD ]; then
				cword_=$j
			fi
			if (($i < ${#COMP_WORDS[@]} - 1)); then
				((i++))
			else
				# Done.
				return
			fi
		done
		words_[$j]=${words_[j]}${COMP_WORDS[i]}
		if [ $i = $COMP_CWORD ]; then
			cword_=$j
		fi
	done
}

if ! type _get_comp_words_by_ref >/dev/null 2>&1; then
_get_comp_words_by_ref ()
{
	local exclude cur_ words_ cword_
	if [ "$1" = "-n" ]; then
		exclude=$2
		shift 2
	fi
	__west_reassemble_comp_words_by_ref "$exclude"
	cur_=${words_[cword_]}
	while [ $# -gt 0 ]; do
		case "$1" in
		cur)
			cur=$cur_
			;;
		prev)
			prev=${words_[$cword_-1]}
			;;
		words)
			words=("${words_[@]}")
			;;
		cword)
			cword=$cword_
			;;
		esac
		shift
	done
}
fi

if ! type _tilde >/dev/null 2>&1; then
# Perform tilde (~) completion
# @return  True (0) if completion needs further processing,
#          False (> 0) if tilde is followed by a valid username, completions
#          are put in COMPREPLY and no further processing is necessary.
_tilde()
{
    local result=0
    if [[ $1 == \~* && $1 != */* ]]; then
        # Try generate ~username completions
        COMPREPLY=( $( compgen -P '~' -u -- "${1#\~}" ) )
        result=${#COMPREPLY[@]}
        # 2>/dev/null for direct invocation, e.g. in the _tilde unit test
        [[ $result -gt 0 ]] && compopt -o filenames 2>/dev/null
    fi
    return $result
}
fi

if ! type _quote_readline_by_ref >/dev/null 2>&1; then
# This function quotes the argument in a way so that readline dequoting
# results in the original argument.  This is necessary for at least
# `compgen' which requires its arguments quoted/escaped:
#
#     $ ls "a'b/"
#     c
#     $ compgen -f "a'b/"       # Wrong, doesn't return output
#     $ compgen -f "a\'b/"      # Good
#     a\'b/c
#
# See also:
# - http://lists.gnu.org/archive/html/bug-bash/2009-03/msg00155.html
# - http://www.mail-archive.com/bash-completion-devel@lists.alioth.\
#   debian.org/msg01944.html
# @param $1  Argument to quote
# @param $2  Name of variable to return result to
_quote_readline_by_ref()
{
    if [ -z "$1" ]; then
        # avoid quoting if empty
        printf -v $2 %s "$1"
    elif [[ $1 == \'* ]]; then
        # Leave out first character
        printf -v $2 %s "${1:1}"
    elif [[ $1 == \~* ]]; then
        # avoid escaping first ~
        printf -v $2 \~%q "${1:1}"
    else
        printf -v $2 %q "$1"
    fi

    # Replace double escaping ( \\ ) by single ( \ )
    # This happens always when argument is already escaped at cmdline,
    # and passed to this function as e.g.: file\ with\ spaces
    [[ ${!2} == *\\* ]] && printf -v $2 %s "${1//\\\\/\\}"

    # If result becomes quoted like this: $'string', re-evaluate in order to
    # drop the additional quoting.  See also: http://www.mail-archive.com/
    # bash-completion-devel@lists.alioth.debian.org/msg01942.html
    [[ ${!2} == \$* ]] && eval $2=${!2}
} # _quote_readline_by_ref()
fi

# This function turns on "-o filenames" behavior dynamically. It is present
# for bash < 4 reasons. See http://bugs.debian.org/272660#64 for info about
# the bash < 4 compgen hack.
_compopt_o_filenames()
{
    # We test for compopt availability first because directly invoking it on
    # bash < 4 at this point may cause terminal echo to be turned off for some
    # reason, see https://bugzilla.redhat.com/653669 for more info.
    type compopt &>/dev/null && compopt -o filenames 2>/dev/null || \
        compgen -f /non-existing-dir/ >/dev/null
}

if ! type _filedir >/dev/null 2>&1; then
# This function performs file and directory completion. It's better than
# simply using 'compgen -f', because it honours spaces in filenames.
# @param $1  If `-d', complete only on directories.  Otherwise filter/pick only
#            completions with `.$1' and the uppercase version of it as file
#            extension.
#
_filedir()
{
    local IFS=$'\n'

    _tilde "$cur" || return

    local -a toks
    local x tmp

    x=$( compgen -d -- "$cur" ) &&
    while read -r tmp; do
        toks+=( "$tmp" )
    done <<< "$x"

    if [[ "$1" != -d ]]; then
        local quoted
        _quote_readline_by_ref "$cur" quoted

        # Munge xspec to contain uppercase version too
        # http://thread.gmane.org/gmane.comp.shells.bash.bugs/15294/focus=15306
        local xspec=${1:+"!*.@($1|${1^^})"}
        x=$( compgen -f -X "$xspec" -- $quoted ) &&
        while read -r tmp; do
            toks+=( "$tmp" )
        done <<< "$x"

        # Try without filter if it failed to produce anything and configured to
        [[ -n ${COMP_FILEDIR_FALLBACK:-} && -n "$1" && ${#toks[@]} -lt 1 ]] && \
            x=$( compgen -f -- $quoted ) &&
            while read -r tmp; do
                toks+=( "$tmp" )
            done <<< "$x"
    fi

    if [[ ${#toks[@]} -ne 0 ]]; then
        # 2>/dev/null for direct invocation, e.g. in the _filedir unit test
        _compopt_o_filenames
        COMPREPLY+=( "${toks[@]}" )
    fi
} # _filedir()
fi

# Misc helpers taken from Docker:
# https://github.com/docker/docker-ce/blob/master/components/cli/contrib/completion/bash/docker

# __west_pos_first_nonflag finds the position of the first word that is neither
# option nor an option's argument. If there are options that require arguments,
# you should pass a glob describing those options, e.g. "--option1|-o|--option2"
# Use this function to restrict completions to exact positions after the argument list.
__west_pos_first_nonflag()
{
	local argument_flags=$1

	local counter=$((${subcommand_pos:-${command_pos}} + 1))
	while [ "$counter" -le "$cword" ]; do
		if [ -n "$argument_flags" ] && eval "case '${words[$counter]}' in $argument_flags) true ;; *) false ;; esac"; then
			(( counter++ ))
			# eat "=" in case of --option=arg syntax
			[ "${words[$counter]}" = "=" ] && (( counter++ ))
		else
			case "${words[$counter]}" in
				-*)
					;;
				*)
					break
					;;
			esac
		fi

		# Bash splits words at "=", retaining "=" as a word, examples:
		# "--debug=false" => 3 words, "--log-opt syslog-facility=daemon" => 4 words
		while [ "${words[$counter + 1]}" = "=" ] ; do
			counter=$(( counter + 2))
		done

		(( counter++ ))
	done

	echo $counter
}

# __west_map_key_of_current_option returns `key` if we are currently completing the
# value of a map option (`key=value`) which matches the extglob given as an argument.
# This function is needed for key-specific completions.
__west_map_key_of_current_option()
{
	local glob="$1"

	local key glob_pos
	if [ "$cur" = "=" ] ; then        # key= case
		key="$prev"
		glob_pos=$((cword - 2))
	elif [[ $cur == *=* ]] ; then     # key=value case (OSX)
		key=${cur%=*}
		glob_pos=$((cword - 1))
	elif [ "$prev" = "=" ] ; then
		key=${words[$cword - 2]}  # key=value case
		glob_pos=$((cword - 3))
	else
		return
	fi

	[ "${words[$glob_pos]}" = "=" ] && ((glob_pos--))  # --option=key=value syntax

	[[ ${words[$glob_pos]} == @($glob) ]] && echo "$key"
}

# __west_value_of_option returns the value of the first option matching `option_glob`.
# Valid values for `option_glob` are option names like `--log-level` and globs like
# `--log-level|-l`
# Only positions between the command and the current word are considered.
__west_value_of_option()
{
	local option_extglob=$(__west_to_extglob "$1")

	local counter=$((command_pos + 1))
	while [ "$counter" -lt "$cword" ]; do
		case ${words[$counter]} in
			$option_extglob )
				echo "${words[$counter + 1]}"
				break
				;;
		esac
		(( counter++ ))
	done
}

# __west_to_alternatives transforms a multiline list of strings into a single line
# string with the words separated by `|`.
# This is used to prepare arguments to __west_pos_first_nonflag().
__west_to_alternatives()
{
	local parts=( $1 )
	local IFS='|'
	echo "${parts[*]}"
}

# __west_to_extglob transforms a multiline list of options into an extglob pattern
# suitable for use in case statements.
__west_to_extglob()
{
	local extglob=$( __west_to_alternatives "$1" )
	echo "@($extglob)"
}

__set_comp_dirs()
{
	_filedir -d
}

__set_comp_files()
{
	_filedir
}

# Sets completions for $cur, from the possibilities in $1..n
__set_comp()
{
	# "${*:1}" gives a single argument with arguments $1..n
	COMPREPLY=($(compgen -W "${*:1}" -- "$cur"))
}


__west_x()
{
	west 2>/dev/null "$@"
}

# Walk up from PWD for .west - do not call `west topdir` (Python) on every Tab.
__west_topdir()
{
	local d="${PWD}"
	while [ -n "$d" ] && [ "$d" != "/" ]; do
		if [ -d "$d/.west" ]; then
			echo "$d"
			return 0
		fi
		d="${d%/*}"
		[ -z "$d" ] && d="/"
	done
	return 1
}

__west_zephyr_base()
{
	if [ -n "${ZEPHYR_BASE}" ]; then
		echo "$ZEPHYR_BASE"
		return 0
	fi
	local top base
	top=$(__west_topdir) || return 1
	base=$(awk '
		$0 ~ /^\[zephyr\]/ { in_z=1; next }
		$0 ~ /^\[/ { in_z=0 }
		in_z && $1 == "base" {
			sub(/^[^=]*=[[:space:]]*/, "")
			gsub(/[[:space:]]+$/, "")
			print
			exit
		}
	' "$top/.west/config" 2>/dev/null)
	if [ -n "$base" ]; then
		case "$base" in
			/*) echo "$base" ;;
			*) echo "$top/$base" ;;
		esac
		return 0
	fi
	if [ -d "$top/zephyr" ]; then
		echo "$top/zephyr"
		return 0
	fi
	return 1
}

# Stamp from ZEPHYR_BASE + board/shield/snippet-root mtimes (RFC §4).
# Manifest git hash is too coarse: an in-tree board add does not change west.yml.
# Do not `find` the whole workspace: walking every board dir is as slow as listing.
__west_completion_stamp()
{
	local top zb p root
	top=$(__west_topdir) || return 1
	zb=$(__west_zephyr_base)
	{
		printf '%s\n' "$zb" "$top/.west/config"
		if [ -n "$zb" ]; then
			[ -d "$zb/boards" ] && find "$zb/boards" -maxdepth 1
			[ -d "$zb/snippets" ] && find "$zb/snippets" -maxdepth 1
			[ -d "$zb/boards/shields" ] && find "$zb/boards/shields" -maxdepth 1
		fi
		for p in "$top"/*/zephyr/module.yml; do
			[ -f "$p" ] || continue
			printf '%s\n' "$p"
			root="${p%/zephyr/module.yml}"
			[ -d "$root/boards" ] && printf '%s\n' "$root/boards"
			[ -d "$root/snippets" ] && printf '%s\n' "$root/snippets"
		done
	} 2>/dev/null | {
		if stat -f %m "$top" >/dev/null 2>&1; then
			xargs stat -f '%m:%N'
		else
			xargs stat -c '%Y:%n'
		fi
	} 2>/dev/null | cksum
}

__west_completion_dir()
{
	local top
	top=$(__west_topdir) || return 1
	printf '%s/.west/completion' "$top"
}

# $1 = cache kind (boards|shields|snippets); remaining args are passed to west.
__west_completion_list()
{
	local kind="$1"
	shift
	local dir stamp cache_file cached
	dir=$(__west_completion_dir) || { __west_x "$@"; return; }
	stamp=$(__west_completion_stamp) || { __west_x "$@"; return; }
	cache_file="$dir/$kind"
	if [ -f "$cache_file" ]; then
		cached=$(head -n 1 "$cache_file")
		if [ -n "$stamp" ] && [ "$cached" = "$stamp" ]; then
			tail -n +2 "$cache_file"
			return 0
		fi
	fi
	local out
	out=$(__west_x "$@") || return $?
	mkdir -p "$dir" 2>/dev/null
	{ printf '%s\n' "$stamp"; printf '%s\n' "$out"; } > "$cache_file"
	printf '%s\n' "$out"
}

__set_comp_west_projs()
{
	__set_comp "$(__west_x list --format={name} "$@")"
}

__set_comp_west_boards()
{
	__set_comp "$(__west_completion_list boards boards --all-targets "$@")"
}

__set_comp_west_shields()
{
	__set_comp "$(__west_completion_list shields shields --format={name} "$@")"
}

__set_comp_west_snippets()
{
	__set_comp "$(__west_completion_list snippets snippets --format={name} "$@")"
}
