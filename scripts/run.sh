#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cmake -S "$ROOT" -B "$ROOT/build" >/dev/null
cmake --build "$ROOT/build" --target browser_history -j 2 >/dev/null
exec "$ROOT/build/browser_history" "$@"
