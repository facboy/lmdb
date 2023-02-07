#!/bin/bash

uname | grep -qe "Linux"
if [[ $? -ne 0 ]]; then
	>&2 echo "Must be run on Linux"
	exit 1
fi

set -e

MINE_BRANCH="win-0.9"
MINE_BASE="win-0.9-base"
UPSTREAM_BRANCH="mdb.RE/0.9"
UPSTREAM="upstream/${UPSTREAM_BRANCH}"

if [[ "$(git branch --show-current)" != "${MINE_BRANCH}" ]]; then
	>&2 echo "Not on ${MINE_BRANCH} branch"
	exit 1
fi

git fetch upstream --prune

merge_base="$(git merge-base "${MINE_BRANCH}" "${UPSTREAM}")"
upstream_commit="$(git rev-parse "${UPSTREAM}")"

if [[ "${merge_base}" != "${upstream_commit}" ]]; then
	git rebase --onto "${UPSTREAM}" "${MINE_BASE}"
fi

git fetch upstream "${UPSTREAM_BRANCH}:${MINE_BASE}"
git fetch upstream "${UPSTREAM_BRANCH}:${UPSTREAM_BRANCH}"
# git push origin "${MINE_BRANCH}" # needs --force
git push origin "${UPSTREAM_BRANCH}"
