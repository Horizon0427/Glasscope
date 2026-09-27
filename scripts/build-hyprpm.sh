#!/usr/bin/env bash

set -Eeuo pipefail

repo_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
build_dir="$repo_dir/.hyprpm-build"
dist_dir="$repo_dir/dist"
output="$dist_dir/glasscope.so"
temporary="${output}.new.$$"
jobs="${HYPRPM_BUILD_JOBS:-4}"

[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || {
    printf 'HYPRPM_BUILD_JOBS must be a positive integer\n' >&2
    exit 2
}

cleanup() {
    cmake -E rm -f "$temporary"
}
trap cleanup EXIT

cmake -E rm -f "$output"
cmake -E remove_directory "$build_dir"
cmake -E make_directory "$build_dir" "$dist_dir"

cmake -S "$repo_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build "$build_dir" --parallel "$jobs"

artifact="$build_dir/glasscope.so"
[[ -x "$artifact" ]] || {
    printf 'Glasscope build produced no shared object\n' >&2
    exit 1
}
if ldd "$artifact" | grep -F 'not found'; then
    printf 'Glasscope has unresolved shared-library dependencies\n' >&2
    exit 1
fi

install -m755 "$artifact" "$temporary"
mv -f -- "$temporary" "$output"
printf 'hyprpm-output=%s\nsha256=%s\n' \
    "$output" "$(sha256sum "$output" | cut -d' ' -f1)"
