#!/usr/bin/env bash
set -euo pipefail

preset="${1:-release}"

cd "$(dirname "$0")/.."
cmake --preset "$preset"
cmake --build --preset "$preset" --parallel
