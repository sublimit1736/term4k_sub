#!/usr/bin/env sh
set -eu

FORCE=0
KEEP_USER_DATA=0

usage() {
    cat <<'EOF'
Usage: uninstall.sh [options]

Options:
  --yes               non-interactive mode
  --keep-user-data    keep ~/.local/share/term4k and ~/.config/term4k
  --safe              alias of --keep-user-data
  --help              show this help
EOF
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --yes)
            FORCE=1
            shift
            ;;
        --keep-user-data|--safe)
            KEEP_USER_DATA=1
            shift
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            echo "[term4k uninstall] unknown option: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

run_as_root() {
    if [ "$(id -u)" -eq 0 ]; then
        "$@"
    elif command -v sudo >/dev/null 2>&1; then
        sudo "$@"
    else
        echo "[term4k uninstall] root privilege is required. Re-run as root or install sudo." >&2
        exit 1
    fi
}

confirm() {
    if [ "$FORCE" -eq 1 ]; then
        return
    fi

    echo "[term4k uninstall] This will remove term4k packages and data directories:"
    echo "  - /usr/bin/term4k"
    echo "  - /usr/share/term4k"
    echo "  - /etc/term4k"
    echo "  - /var/lib/term4k"
    if [ "$KEEP_USER_DATA" -eq 0 ]; then
        echo "  - ~/.local/share/term4k"
        echo "  - ~/.config/term4k"
    fi
    printf "Continue? [y/N] "
    read -r answer
    case "$answer" in
        y|Y|yes|YES) ;;
        *)
            echo "[term4k uninstall] cancelled."
            exit 0
            ;;
    esac
}

remove_packages() {
    if command -v apt >/dev/null 2>&1; then
        run_as_root apt remove -y term4k term4k-dev || true
        run_as_root apt purge -y term4k term4k-dev || true
    elif command -v dnf >/dev/null 2>&1; then
        run_as_root dnf remove -y term4k term4k-devel || true
    elif command -v yum >/dev/null 2>&1; then
        run_as_root yum remove -y term4k term4k-devel || true
    elif command -v zypper >/dev/null 2>&1; then
        run_as_root zypper --non-interactive remove term4k term4k-devel || true
    fi
}

remove_files() {
    run_as_root rm -f /usr/bin/term4k
    run_as_root rm -rf /usr/share/term4k
    run_as_root rm -rf /usr/include/term4k
    run_as_root rm -rf /etc/term4k
    run_as_root rm -rf /var/lib/term4k
}

remove_user_data() {
    if [ "$KEEP_USER_DATA" -eq 1 ]; then
        echo "[term4k uninstall] keeping user data as requested (--keep-user-data)."
        return
    fi

    # Remove data/config for the current user running the script.
    user_home="${HOME:-}"
    if [ -n "$user_home" ] && [ -d "$user_home" ]; then
        rm -rf "$user_home/.local/share/term4k"
        rm -rf "$user_home/.config/term4k"
    fi
}

confirm
remove_packages
remove_files
remove_user_data

echo "[term4k uninstall] term4k has been removed from this system."

