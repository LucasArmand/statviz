#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"

cd "$BUILD_DIR"
exec "./statviz" "${@:2}"
