#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

mode="${1:---check}"

files=$(find include src tests benchmarks examples -name '*.hpp' -o -name '*.cpp')

if [ "$mode" = "--fix" ]; then
    clang-format -i $files
    echo "formatted $(echo "$files" | wc -l) files"
else
    clang-format --dry-run --Werror $files
    echo "format check passed"
fi
