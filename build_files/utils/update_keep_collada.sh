#!/usr/bin/env bash

set -euo pipefail

# Update `keep-collada-5.0` with the latest lfs-fallback/blender-v5.0-release and push to origin.
# This version merges upstream instead of rebasing, so your COLLADA commits stay intact.

branch="keep-collada-5.0"
upstream_remote="lfs-fallback"
upstream_branch="blender-v5.0-release"

echo "==> Ensuring branch: ${branch}"
git checkout "${branch}"

echo "==> Fetching latest from ${upstream_remote}/${upstream_branch}"
git fetch "${upstream_remote}" "${upstream_branch}" --prune

echo "==> Merging latest ${upstream_remote}/${upstream_branch} into ${branch}"
# Do not skip smudge — we want Blender LFS pointers to be valid
git lfs pull "${upstream_remote}" "${upstream_branch}"
git merge --no-edit "${upstream_remote}/${upstream_branch}" || {
  echo "⚠️ Merge conflicts detected. Resolve them, then run:"
  echo "   git add <resolved files> && git merge --continue"
  exit 1
}


echo "==> Initializing/updating COLLADA submodules (all platforms if present)"
for sm in lib/linux_x64_collada lib/windows_x64_collada lib/windows_arm64_collada lib/macos_arm64_collada; do
  if git config --file .gitmodules --get-regexp ".*${sm}" >/dev/null 2>&1; then
    echo "  -> updating ${sm}"
    GIT_LFS_SKIP_SMUDGE=1 git submodule update --init --progress "${sm}" || true
    git -C "${sm}" lfs pull || true
  fi
done

echo "==> Pushing to origin and uploading LFS objects"
git lfs push --all origin "${branch}" || true
git push origin "${branch}"


echo "==> Done"
