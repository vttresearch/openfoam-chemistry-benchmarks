#!/usr/bin/env bash
set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
PERFETTO_ROOT="$REPO_ROOT/third_party/perfetto"
SDK_DIR="$PERFETTO_ROOT/sdk"
CANONICAL_CATEGORIES="$REPO_ROOT/tools/perfetto/foam_tracing_categories.H"

DEFAULT_VERSION="54.0"
VERSION="${1:-$DEFAULT_VERSION}"

mkdir -p "$PERFETTO_ROOT"
ZIP_PATH="$PERFETTO_ROOT/perfetto-cpp-sdk-src-v${VERSION}.zip"
URL="https://github.com/google/perfetto/releases/download/v${VERSION}/perfetto-cpp-sdk-src.zip"

if command -v curl >/dev/null 2>&1; then
    curl -L --fail --output "$ZIP_PATH" "$URL"
else
    wget -O "$ZIP_PATH" "$URL"
fi

TMP_DIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT

unzip -q "$ZIP_PATH" -d "$TMP_DIR"

FOUND_H="$(find "$TMP_DIR" -type f -name perfetto.h | head -n 1)"
FOUND_CC="$(find "$TMP_DIR" -type f -name perfetto.cc | head -n 1)"

rm -rf "$SDK_DIR"
mkdir -p "$SDK_DIR"

cp "$FOUND_H" "$SDK_DIR/perfetto.h"
cp "$FOUND_CC" "$SDK_DIR/perfetto.cc"
cp "$CANONICAL_CATEGORIES" "$SDK_DIR/foam_tracing_categories.H"

FOUND_CH="$(find "$TMP_DIR" -type f -name perfetto_c.h | head -n 1 || true)"
FOUND_CC_CAPI="$(find "$TMP_DIR" -type f -name perfetto_c.cc | head -n 1 || true)"
if [[ -n "$FOUND_CH" ]]; then
    cp "$FOUND_CH" "$SDK_DIR/perfetto_c.h"
fi
if [[ -n "$FOUND_CC_CAPI" ]]; then
    cp "$FOUND_CC_CAPI" "$SDK_DIR/perfetto_c.cc"
fi

cat > "$PERFETTO_ROOT/.install-info" <<EOF
version=v${VERSION}
source_zip=${ZIP_PATH}
installed_at=$(date -u +%Y-%m-%dT%H:%M:%SZ)
EOF

rm -f "$ZIP_PATH"

echo "Perfetto SDK installed: $SDK_DIR"
