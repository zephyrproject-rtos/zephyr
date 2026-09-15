#compdef west

# Copyright (c) 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Ensure this works also when being source-ed

compdef _west west
typeset -A -g _opt_args

__west_x()
{
  west 2>/dev/null "$@"
}

__west_topdir() {
  local d="${PWD}"
  while [[ -n "$d" && "$d" != "/" ]]; do
    if [[ -d "$d/.west" ]]; then
      echo "$d"
      return 0
    fi
    d="${d%/*}"
    [[ -z "$d" ]] && d="/"
  done
  return 1
}

__west_zephyr_base() {
  if [[ -n "${ZEPHYR_BASE}" ]]; then
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
  if [[ -n "$base" ]]; then
    case "$base" in
      /*) echo "$base" ;;
      *) echo "$top/$base" ;;
    esac
    return 0
  fi
  if [[ -d "$top/zephyr" ]]; then
    echo "$top/zephyr"
    return 0
  fi
  return 1
}

__west_completion_stamp() {
  setopt local_options null_glob
  local top zb p root
  top=$(__west_topdir) || return 1
  zb=$(__west_zephyr_base)
  {
    print -r -- "$zb"
    print -r -- "$top/.west/config"
    if [[ -n "$zb" ]]; then
      [[ -d "$zb/boards" ]] && find "$zb/boards" -maxdepth 1
      [[ -d "$zb/snippets" ]] && find "$zb/snippets" -maxdepth 1
      [[ -d "$zb/boards/shields" ]] && find "$zb/boards/shields" -maxdepth 1
    fi
    for p in "$top"/*/zephyr/module.yml; do
      [[ -f "$p" ]] || continue
      print -r -- "$p"
      root="${p%/zephyr/module.yml}"
      [[ -d "$root/boards" ]] && print -r -- "$root/boards"
      [[ -d "$root/snippets" ]] && print -r -- "$root/snippets"
    done
  } 2>/dev/null | {
    if stat -f %m "$top" >/dev/null 2>&1; then
      xargs stat -f '%m:%N'
    else
      xargs stat -c '%Y:%n'
    fi
  } 2>/dev/null | cksum
}

__west_completion_dir() {
  local top
  top=$(__west_topdir) || return 1
  printf '%s/.west/completion' "$top"
}

__west_completion_list() {
  local kind="$1"
  shift
  local dir stamp cache_file cached
  dir=$(__west_completion_dir) || { __west_x "$@"; return; }
  stamp=$(__west_completion_stamp) || { __west_x "$@"; return; }
  cache_file="$dir/$kind"
  if [[ -f "$cache_file" ]]; then
    cached=$(head -n 1 "$cache_file")
    if [[ -n "$stamp" && "$cached" == "$stamp" ]]; then
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

_get_west_projs() {
  local extra_args
  [[ -v _opt_args[-z] ]] && extra_args="-z $_opt_args[-z]"
  [[ -v _opt_args[--zephyr-base] ]] && extra_args="-z $_opt_args[--zephyr-base]"

  _west_projs=($(__west_x $extra_args list --format={name}))
  _describe 'projs' _west_projs
}

_get_west_boards() {
  local -a boards=( ${(@f)"$(__west_completion_list boards boards --all-targets)"} )
  _describe 'boards' boards
}

_get_west_shields() {
  _west_shields=( ${(@f)"$(__west_completion_list shields shields --format={name})"} )
  for i in {1..${#_west_shields[@]}}; do
    local name="${_west_shields[$i]%%|*}"
    local transformed_shield="${_west_shields[$i]//|//}"
    _west_shields[$i]="${transformed_shield//,/ ${name}/}"
  done
  _west_shields=(${(@s/ /)_west_shields})

  _describe 'shields' _west_shields
}

_get_west_snippets() {
  local -a snippets=( ${(@f)"$(__west_completion_list snippets snippets --format={name})"} )
  _describe 'snippets' snippets
}

__west_tilde() {
    local cur=${words[CURRENT]}

    [[ $cur == "~"* && $cur != */* ]] || return 1

    compset -P '~'
    _users
}

_filedir()
{
    if [[ "$1" == "-d" ]]; then
        _files -/
    else
        _files
    fi
}
