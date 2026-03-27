#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SOURCE_URL="${TERM4K_SOURCE_URL:-}"
WORK_DIR="${TERM4K_WORK_DIR:-}"

usage() {
    cat <<'EOF'
Usage: install.sh [options]

Options:
  --yes                 non-interactive installation where supported
  --source-url <url>    source archive URL (.tar.gz) for remote mode
  --work-dir <path>     custom build workspace (default: temp dir in remote mode)
  --help                show this help

Notes:
  - Local mode is used when CMakeLists.txt exists beside this script.
  - Remote mode is used when only this script is downloaded.
EOF
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --yes)
            shift
            ;;
        --source-url)
            [ "$#" -ge 2 ] || { echo "[term4k installer] --source-url requires a value." >&2; exit 1; }
            SOURCE_URL="$2"
            shift 2
            ;;
        --work-dir)
            [ "$#" -ge 2 ] || { echo "[term4k installer] --work-dir requires a value." >&2; exit 1; }
            WORK_DIR="$2"
            shift 2
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            echo "[term4k installer] unknown option: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

need_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "[term4k installer] missing required command: $1" >&2
        exit 1
    fi
}

run_as_root() {
    if [ "$(id -u)" -eq 0 ]; then
        "$@"
    elif command -v sudo >/dev/null 2>&1; then
        sudo "$@"
    else
        echo "[term4k installer] root privilege is required. Re-run as root or install sudo." >&2
        exit 1
    fi
}

fetch_url() {
    url="$1"
    out="$2"
    if command -v curl >/dev/null 2>&1; then
        curl -fsSL "$url" -o "$out"
    elif command -v wget >/dev/null 2>&1; then
        wget -qO "$out" "$url"
    else
        echo "[term4k installer] remote mode requires curl or wget." >&2
        exit 1
    fi
}

install_i18n_assets() {
    src_dir="$1"
    dst_dir="/usr/share/term4k/i18n"

    if [ ! -d "$src_dir" ]; then
        return
    fi

    run_as_root mkdir -p "$dst_dir"
    set -- "$src_dir"/*.json
    if [ ! -e "$1" ]; then
        return
    fi

    for locale_file in "$@"; do
        run_as_root install -m 0644 "$locale_file" "$dst_dir/"
    done
}

find_first_file() {
    dir="$1"
    pattern="$2"
    find "$dir" -maxdepth 1 -type f -name "$pattern" | sort | tail -n 1
}

detect_pkg_manager() {
    if command -v apt >/dev/null 2>&1; then
        echo "apt"
    elif command -v dnf >/dev/null 2>&1; then
        echo "dnf"
    elif command -v yum >/dev/null 2>&1; then
        echo "yum"
    elif command -v zypper >/dev/null 2>&1; then
        echo "zypper"
    else
        echo ""
    fi
}

local_mode=0
if [ -f "$SCRIPT_DIR/../CMakeLists.txt" ]; then
    local_mode=1
fi

TMP_DIR=""
OWN_TMP_DIR=0
cleanup() {
    if [ "$OWN_TMP_DIR" -eq 1 ] && [ -n "$TMP_DIR" ] && [ -d "$TMP_DIR" ]; then
        rm -rf "$TMP_DIR"
    fi
}
trap cleanup EXIT INT TERM

if [ "$local_mode" -eq 1 ]; then
    ROOT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
else
    need_cmd tar
    if [ -z "$SOURCE_URL" ]; then
        echo "[term4k installer] remote mode requires --source-url <tar.gz URL>." >&2
        exit 1
    fi

    if [ -n "$WORK_DIR" ]; then
        TMP_DIR="$WORK_DIR"
        mkdir -p "$TMP_DIR"
    else
        TMP_DIR=$(mktemp -d "${TMPDIR:-/tmp}/term4k-install.XXXXXX")
        OWN_TMP_DIR=1
    fi

    archive="$TMP_DIR/source.tar.gz"
    echo "[term4k installer] downloading source archive..."
    fetch_url "$SOURCE_URL" "$archive"

    src_root="$TMP_DIR/src"
    mkdir -p "$src_root"
    tar -xzf "$archive" -C "$src_root"

    ROOT_DIR=$(find "$src_root" -maxdepth 2 -type f -name CMakeLists.txt | head -n 1 | sed 's|/CMakeLists.txt$||')
    if [ -z "$ROOT_DIR" ] || [ ! -f "$ROOT_DIR/CMakeLists.txt" ]; then
        echo "[term4k installer] downloaded archive does not contain a valid term4k source tree." >&2
        exit 1
    fi
fi

BUILD_DIR="${ROOT_DIR}/cmake-build-release"

need_cmd cmake
need_cmd cpack

echo "[term4k installer] configuring project..."
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DTERM4K_BUILD_TESTS=OFF

echo "[term4k installer] building package..."
cmake --build "$BUILD_DIR" --target package -j

PKG_MANAGER=$(detect_pkg_manager)
DEB_PKG=$(find_first_file "$BUILD_DIR" "term4k-*.deb" || true)
RPM_PKG=$(find_first_file "$BUILD_DIR" "term4k-*.rpm" || true)

if [ "$PKG_MANAGER" = "apt" ] && [ -n "${DEB_PKG:-}" ]; then
    echo "[term4k installer] installing $(basename "$DEB_PKG") via apt..."
    run_as_root apt install -y "$DEB_PKG"
elif [ "$PKG_MANAGER" = "dnf" ] && [ -n "${RPM_PKG:-}" ]; then
    echo "[term4k installer] installing $(basename "$RPM_PKG") via dnf..."
    run_as_root dnf install -y "$RPM_PKG"
elif [ "$PKG_MANAGER" = "yum" ] && [ -n "${RPM_PKG:-}" ]; then
    echo "[term4k installer] installing $(basename "$RPM_PKG") via yum..."
    run_as_root yum install -y "$RPM_PKG"
elif [ "$PKG_MANAGER" = "zypper" ] && [ -n "${RPM_PKG:-}" ]; then
    echo "[term4k installer] installing $(basename "$RPM_PKG") via zypper..."
    run_as_root zypper --non-interactive install "$RPM_PKG"
else
    echo "[term4k installer] package manager or package format not detected; using 'cmake --install' fallback."
    run_as_root cmake --install "$BUILD_DIR"
fi

# Ensure locale assets from repository are present in system path.
install_i18n_assets "$ROOT_DIR/src/resources/i18n"

echo "[term4k installer] done. Run: term4k"

