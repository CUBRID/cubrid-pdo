#!/bin/bash
#
# Run the tests with the driver built by ../build.sh <ver>.
#
#   ./run_tests.sh [run-tests.php options] [tests...]
#       no test given: all tests
#   ./run_tests.sh --php script.php [args...]
#       run a PHP script with the driver loaded
#
# Environment
#   PHP_BIN      PHP binary     (default: php in PATH), e.g. ~/php/8.4/bin/php
#   CUBRID_EXT   driver to test (default: ../build_linux/<ver>/modules/pdo_cubrid.so,
#                <ver> = 83, 84, 85 from PHP_BIN)
#
# php.ini is not read (-n); PDO (and PDO_SQLite, used by some PDO common
# tests, when installed) is loaded with -d extension=... when it is not built
# into PHP. The tests connect to demodb on localhost:33000.

cd "$(dirname "$0")"

PHP_BIN=$(command -v "${PHP_BIN:-php}") || { echo "[ERROR] php not found" >&2; exit 1; }

php_ver=$("$PHP_BIN" -n -r 'echo PHP_MAJOR_VERSION, PHP_MINOR_VERSION;')
CUBRID_EXT=${CUBRID_EXT:-../build_linux/$php_ver/modules/pdo_cubrid.so}

if [ ! -f "$CUBRID_EXT" ]; then
    echo "[ERROR] $CUBRID_EXT not found. Run '../build.sh $php_ver' first." >&2
    exit 1
fi
CUBRID_EXT=$(readlink -f "$CUBRID_EXT")

php_opts=(-n)
if ! "$PHP_BIN" -n -r 'exit(extension_loaded("pdo") ? 0 : 1);'; then
    php_opts+=(-d extension=pdo)
fi
# Some PDO common tests use PDO_SQLite; load it when it is installed.
if ! "$PHP_BIN" -n -r 'exit(extension_loaded("pdo_sqlite") ? 0 : 1);' \
   && "$PHP_BIN" -n "${php_opts[@]:1}" -d extension=pdo_sqlite -r 'exit(extension_loaded("pdo_sqlite") ? 0 : 1);' 2>/dev/null; then
    php_opts+=(-d extension=pdo_sqlite)
fi
php_opts+=(-d extension="$CUBRID_EXT")

if ! "$PHP_BIN" "${php_opts[@]}" -r 'exit(extension_loaded("pdo_cubrid") ? 0 : 1);'; then
    echo "[ERROR] $CUBRID_EXT does not load in $PHP_BIN" >&2
    exit 1
fi

if [ "$1" = "--php" ]; then
    shift
    exec "$PHP_BIN" "${php_opts[@]}" "$@"
fi

# Run the default test set when no test path is given.
has_target=0
for arg in "$@"; do
    [ -e "$arg" ] && has_target=1
done
targets=(.)
[ $has_target -eq 1 ] && targets=()

export NO_COLOR=1
exec "$PHP_BIN" run-tests.php -q "${php_opts[@]}" -p "$PHP_BIN" "$@" "${targets[@]}" | tee pdo-tests_linux_${php_ver}.log
