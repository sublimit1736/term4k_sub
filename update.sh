#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
INSTALL_SCRIPT_URL="${TERM4K_INSTALL_SCRIPT_URL:-}"
SOURCE_URL="${TERM4K_SOURCE_URL:-}"
YES=0

usage() {
    cat <<'EOF'
Usage: update.sh [options]

Options:
  --yes                      non-interactive mode
  --source-url <url>         source archive URL (.tar.gz) for remote mode
  --install-script-url <url> install.sh URL for script-only remote updates
  --help                     show this help

Notes:
  - update.sh reuses install.sh and performs an in-place upgrade.
  - If install.sh exists beside this script, it is used directly.
EOF
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --yes)
            YES=1
            shift
            ;;
        --source-url)
            [ "$#" -ge 2 ] || { echo "[term4k update] --source-url requires a value." >&2; exit 1; }
            SOURCE_URL="$2"
            shift 2
            ;;
        --install-script-url)
            [ "$#" -ge 2 ] || { echo "[term4k update] --install-script-url requires a value." >&2; exit 1; }
            INSTALL_SCRIPT_URL="$2"
            shift 2
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            echo "[term4k update] unknown option: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

fetch_url() {
    url="$1"
    out="$2"
    if command -v curl >/dev/null 2>&1; then
        curl -fsSL "$url" -o "$out"
    elif command -v wget >/dev/null 2>&1; then
        wget -qO "$out" "$url"
    else
        echo "[term4k update] curl or wget is required in remote mode." >&2
        exit 1
    fi
}

INSTALL_SCRIPT_PATH=""
TMP_DIR=""
cleanup() {
    if [ -n "$TMP_DIR" ] && [ -d "$TMP_DIR" ]; then
        rm -rf "$TMP_DIR"
    fi
}
trap cleanup EXIT INT TERM

if [ -f "$SCRIPT_DIR/install.sh" ]; then
    INSTALL_SCRIPT_PATH="$SCRIPT_DIR/install.sh"
else
    if [ -z "$INSTALL_SCRIPT_URL" ]; then
        echo "[term4k update] remote mode requires --install-script-url <url>." >&2
        exit 1
    fi
    TMP_DIR=$(mktemp -d "${TMPDIR:-/tmp}/term4k-update.XXXXXX")
    INSTALL_SCRIPT_PATH="$TMP_DIR/install.sh"
    echo "[term4k update] downloading install script..."
    fetch_url "$INSTALL_SCRIPT_URL" "$INSTALL_SCRIPT_PATH"
fi

echo "[term4k update] updating term4k..."
set --
if [ "$YES" -eq 1 ]; then
    set -- "$@" --yes
fi
if [ -n "$SOURCE_URL" ]; then
    set -- "$@" --source-url "$SOURCE_URL"
fi

sh "$INSTALL_SCRIPT_PATH" "$@"

echo "[term4k update] update finished."


