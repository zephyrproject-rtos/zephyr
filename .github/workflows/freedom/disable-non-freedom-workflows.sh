#!/usr/bin/env bash
set -euo pipefail

: "${GH_TOKEN:?GH_TOKEN required}"
: "${REPO:?REPO required (owner/name)}"

gh api "repos/${REPO}/actions/workflows" --paginate \
  --jq '.workflows[] | [.id, .state, .path] | @tsv' |
while IFS=$'\t' read -r id state path; do
  # Only sweep real files under .github/workflows/. Skip
  # GitHub-managed dynamic workflows (e.g. dependabot/update-graph,
  # pages-build-deployment) — the disable API returns 422 for those.
  case "$path" in
    .github/workflows/*.yml|.github/workflows/*.yaml) ;;
    *) continue ;;
  esac
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
  if ! gh api -X PUT \
       "repos/${REPO}/actions/workflows/${id}/disable"; then
    echo "  warn: disable failed for id=$id ($path); continuing" >&2
  fi
done
