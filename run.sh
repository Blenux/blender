#!/usr/bin/env bash
set -euo pipefail

# Auto-detect current branch
branch=$(git symbolic-ref --short HEAD)
echo "==> Detected branch: ${branch}"

# Map branches to upstream
declare -A upstream_map=(
  ["keep-collada"]="main"
  ["keep-collada-5.0"]="blender-v5.0-release"
)

if [[ -z "${upstream_map[$branch]:-}" ]]; then
    echo "Error: Unsupported branch '${branch}'"
    exit 1
fi

upstream_branch=${upstream_map[$branch]}
echo "==> Upstream branch: lfs-fallback/${upstream_branch}"

# Fetch upstream
git fetch lfs-fallback "$upstream_branch" --prune

# Rebase on upstream (skip smudge for speed)
GIT_LFS_SKIP_SMUDGE=1 git rebase "lfs-fallback/${upstream_branch}"

# Update submodules (skip incompatible platforms)
for sm in lib/linux_x64_collada lib/windows_x64_collada lib/windows_arm64_collada lib/macos_arm64_collada; do
    if git config --file .gitmodules --get-regexp ".*${sm}" >/dev/null 2>&1; then
        case "$(uname -s)" in
            Linux) [[ "$sm" == *windows* || "$sm" == *macos* ]] && continue ;;
            Darwin) [[ "$sm" == *windows* || "$sm" == *linux* ]] && continue ;;
            MINGW*|MSYS*|CYGWIN*) [[ "$sm" == *linux* || "$sm" == *macos* ]] && continue ;;
        esac
        echo "  -> updating submodule $sm"
        GIT_LFS_SKIP_SMUDGE=1 git submodule update --init --progress "$sm" || true
        git -C "$sm" lfs fetch --all || true
    fi
done

# Check/fix missing or corrupt LFS objects
echo "==> Checking for missing/corrupt LFS objects"
git lfs fsck || echo "Some LFS objects missing, fetching..."
git lfs fetch --all

# Remove workflows to avoid GitHub push errors
if [ -d ".github/workflows" ]; then
    git rm -r --cached .github/workflows || true
    git commit -m "Remove workflow files for push" || echo "No workflow changes to commit"
fi

# Push all LFS objects (repo + submodules)
echo "==> Pushing all LFS objects for main repo"
git lfs push --all origin "$branch"

for sm in lib/linux_x64_collada lib/windows_x64_collada lib/windows_arm64_collada lib/macos_arm64_collada; do
    if [ -d "$sm" ]; then
        echo "==> Pushing all LFS objects for submodule $sm"
        git -C "$sm" lfs push --all origin || true
    fi
done

# Push branch itself
echo "==> Pushing branch $branch"
git push --force-with-lease origin "$branch" --no-verify

echo "==> Done updating $branch"
