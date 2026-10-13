#!/usr/bin/env bash
set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SDK_DIR="$REPO_ROOT/third_party/perfetto/sdk"

required_files=(
    "$SDK_DIR/perfetto.h"
    "$SDK_DIR/perfetto.cc"
    "$SDK_DIR/foam_tracing_categories.H"
)

for f in "${required_files[@]}"; do
    if [[ ! -f "$f" ]]; then
        echo "ERROR: Missing required file: $f" >&2
        exit 1
    fi
done

include_hits="$(grep -RIn --exclude-dir=.git --exclude-dir=third_party --exclude-dir=platforms --exclude-dir=tests --exclude-dir=Make --exclude-dir=lnInclude '^[[:space:]]*#include[[:space:]]*[<\"]perfetto\.cc[>\"]' "$REPO_ROOT" || true)"

if [[ -z "$include_hits" ]]; then
    echo "ERROR: No perfetto.cc include found in repository sources." >&2
    exit 1
fi

hit_count="$(printf '%s\n' "$include_hits" | sed '/^$/d' | wc -l | tr -d ' ')"
if [[ "$hit_count" != "1" ]]; then
    echo "ERROR: Expected exactly one perfetto.cc include, found $hit_count" >&2
    printf '%s\n' "$include_hits" >&2
    exit 1
fi

expected_hit="src/ODE/perfetto_sdk.C"
if ! printf '%s\n' "$include_hits" | grep -q "$expected_hit"; then
    echo "ERROR: perfetto.cc include is not in $expected_hit" >&2
    printf '%s\n' "$include_hits" >&2
    exit 1
fi

echo "Perfetto setup check passed."
echo "SDK dir: $SDK_DIR"
echo "Single perfetto.cc include: $expected_hit"
