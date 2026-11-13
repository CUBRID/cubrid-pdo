#!/bin/bash
#
# Build the CUBRID PDO driver out of the source tree.
#
#   ./build.sh <ver> [options]   ->  build_linux/<ver>/modules/pdo_cubrid.so
#       ver: 83, 84, 85 (PHP 8.3 or later)
#
# Options
#   --php-config PATH   php-config of the target PHP
#                       (default: $PHP_HOME/<x.y>/bin/php-config, PHP_HOME=~/php,
#                                 otherwise php-config in PATH if the version matches)
#   --install           also install pdo_cubrid.so into the PHP extension_dir and
#                       add "extension=pdo_cubrid.so" to its php.ini
#                       (and "extension=pdo.so" when PDO is not loaded yet)
#   --clean             remove the build directory and build from scratch
#
# phpize/configure generate files in the directory they run in, so the
# sources are copied into build_linux/<ver>/ and the source tree stays clean.
# cci-src is linked, so the CCI static library is built once and shared.
# The target PHP must have PDO (static or pdo.so) and its headers.

set -e

usage()
{
    sed -n '3,20p' "$0" | sed 's/^# \{0,1\}//'
    exit 1
}

root_dir=$(cd "$(dirname "$0")" && pwd)

target=$1
case "$target" in
    83|84|85) ;;
    *) usage ;;
esac
shift
php_xy=${target:0:1}.${target:1}

php_config=""
install=0
clean=0
while [ $# -gt 0 ]; do
    case "$1" in
        --php-config) php_config=$2; shift 2 ;;
        --install) install=1; shift ;;
        --clean) clean=1; shift ;;
        *) usage ;;
    esac
done

if [ -z "$php_config" ]; then
    php_config=${PHP_HOME:-$HOME/php}/$php_xy/bin/php-config
    if [ ! -x "$php_config" ]; then
        php_config=$(command -v php-config || true)
    fi
fi
if [ ! -x "$php_config" ]; then
    echo "[ERROR] php-config for PHP $php_xy not found (use --php-config PATH)" >&2
    exit 1
fi

php_version=$("$php_config" --version)
case "$php_version" in
    $php_xy.*) ;;
    *)
        echo "[ERROR] $php_config is PHP $php_version, not PHP $php_xy" >&2
        exit 1
        ;;
esac

# phpize must come from the same PHP installation as php-config.
phpize=$(dirname "$php_config")/phpize
if [ ! -x "$phpize" ]; then
    echo "[ERROR] phpize not found next to $php_config" >&2
    exit 1
fi

# pdo_cubrid includes ext/pdo/php_pdo_driver.h of the target PHP.
php_include=$("$php_config" --include-dir)
if [ ! -f "$php_include/ext/pdo/php_pdo_driver.h" ]; then
    echo "[ERROR] PDO headers not found in $php_include/ext/pdo." >&2
    echo "        Build PHP with PDO (e.g. --enable-pdo=shared) or install php-pdo." >&2
    exit 1
fi

build_dir=$root_dir/build_linux/$target
if [ $clean -eq 1 ]; then
    rm -rf "$build_dir"
fi
mkdir -p "$build_dir"
cd "$build_dir"

# Reconfigure when built with another php-config before.
if [ -f .php_config ] && [ "$(cat .php_config)" != "$php_config" ]; then
    echo "[INFO] php-config changed, reconfiguring"
    rm -f Makefile
fi

# cp -p keeps mtimes, so make rebuilds only what changed in the source tree.
cp -p "$root_dir"/config.m4 \
      "$root_dir"/pdo_cubrid*.c "$root_dir"/cubrid_driver*.c "$root_dir"/cubrid_statement*.c \
      "$root_dir"/php_pdo_cubrid*.h "$root_dir"/pdo_cubrid_version.h .
ln -sfn "$root_dir/cci-src" cci-src

if [ ! -f Makefile ] || [ config.m4 -nt Makefile ]; then
    "$phpize"
    ./configure --with-pdo-cubrid --with-php-config="$php_config"
fi

make -j"$(nproc)"

echo "$php_config" > .php_config
php_binary=$("$php_config" --php-binary)

if [ $install -eq 1 ]; then
    make install
    ini_dir=$("$php_binary" -n -r 'echo PHP_CONFIG_FILE_PATH;')
    ini=$ini_dir/php.ini
    # PDO must be loaded before pdo_cubrid.
    if ! "$php_binary" -r 'exit(extension_loaded("pdo") ? 0 : 1);' 2>/dev/null; then
        echo "extension=pdo.so" >> "$ini"
        echo "[INFO] added extension=pdo.so to $ini"
    fi
    if ! grep -qs '^extension *= *"\{0,1\}pdo_cubrid\(\.so\)\{0,1\}"\{0,1\} *$' "$ini"; then
        echo "extension=pdo_cubrid.so" >> "$ini"
        echo "[INFO] added extension=pdo_cubrid.so to $ini"
    fi
fi

echo
echo "PHP     : $php_binary ($php_version)"
echo "Module  : $build_dir/modules/pdo_cubrid.so"
