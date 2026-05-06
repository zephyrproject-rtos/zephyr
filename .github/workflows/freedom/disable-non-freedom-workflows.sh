#!/usr/bin/env bash
set -euo pipefail

: "${GH_TOKEN:?GH_TOKEN required}"
: "${REPO:?REPO required (owner/name)}"

gh api "repos/${REPO}/actions/workflows" --paginate \
  --jq '.workflows[] | [.id, .state, .path] | @tsv' |
while IFS=$'\t' read -r id state path; do
  base=$(basename "$path")
  shopt -s nocasematch
  if [[ "$base" =~ ^freedom[_-] ]]; then
    shopt -u nocasematch
    continue
  fi
  shopt -u nocasematch
  if [[ "$state" == "disabled_manually" ]]; then
    continue
  fi
  echo "Disabling: $path (id=$id, was=$state)"
  gh api -X PUT "repos/${REPO}/actions/workflows/${id}/disable"
done
