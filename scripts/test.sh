#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cmake -S "$ROOT" -B "$ROOT/build" >/dev/null
cmake --build "$ROOT/build" -j 2 >/dev/null
"$ROOT/build/browser_history_native_tests"
if [[ "${1:-}" == "--xunit" ]]; then
  if ! command -v dotnet >/dev/null 2>&1; then
    echo 'xUnit.net requires .NET SDK 8, which is not installed on this machine.' >&2
    exit 2
  fi
  export BROWSER_HISTORY_EXE="$ROOT/build/browser_history"
  dotnet test "$ROOT/tests/xunit/BrowserHistory.XunitTests.csproj" --nologo
fi
