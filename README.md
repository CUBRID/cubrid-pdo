CUBRID PDO Driver
=================

The official PDO driver (`new PDO("cubrid:...")`) for the [CUBRID](https://www.cubrid.org) database.
The CUBRID CCI library is included (`cci-src` submodule) and built together with the driver,
so CUBRID does not need to be installed on the client.

Supported platforms
-------------------

| Platform | PHP | Package |
|---|---|---|
| Linux x86_64 | 8.3, 8.4, 8.5 | `php_pdo_cubrid-<version>-src.tgz` (built on install) |
| Windows x64 | 8.4, 8.5 (NTS, TS) | `php_pdo_cubrid-<version>-<8.4\|8.5>-<nts\|ts>-vs17-x86_64.zip` |

PHP 8.2 and earlier are not supported by this version.
The packages are published on [GitHub Releases](https://github.com/CUBRID/cubrid-pdo/releases)
with a `.sha256` checksum file each.

Installation
------------

### With PIE (Linux, Windows)

[PIE](https://github.com/php/pie) downloads, builds (Linux) and enables the extension.
Run `pie` without `sudo`; on Windows use `pie.exe` with `--with-php-path=C:\path\to\php.exe`.
See [build-guide.md](build-guide.md#4-installing-with-pie) for details.

```
pie repository:add vcs https://github.com/CUBRID/cubrid-pdo   # once per PHP installation
pie install cubrid/pdo_cubrid
pie install cubrid/pdo_cubrid:<version>     # a specific version
```

On Linux the extension is built from source, so a compiler, CMake, autoconf and the PHP
development files (`phpize`, PDO headers) are required, e.g. `dnf install gcc gcc-c++ make cmake autoconf php-devel`.

### From the source package (Linux)

```
sha256sum -c php_pdo_cubrid-<version>-src.tgz.sha256
tar xzf php_pdo_cubrid-<version>-src.tgz
cd php_pdo_cubrid-<version>-src
phpize
./configure --with-php-config=/usr/bin/php-config
make
sudo make install
```

Then add `extension=pdo_cubrid` to `php.ini` (or `/etc/php.d/pdo_cubrid.ini`) and check with `php --ri pdo_cubrid`.
When PDO is a shared module (`pdo.so`), it must be loaded before `pdo_cubrid`.
Without network access, the extracted package can also be installed with PIE:
`pie repository:add path /path/to/php_pdo_cubrid-<version>-src` and `pie install cubrid/pdo_cubrid:*@dev`.

### From the binary package (Windows)

1. Extract the zip that matches the PHP version and thread safety (`php -v`: NTS or ZTS/TS).
2. Copy `php_pdo_cubrid-<...>.dll` to the PHP `ext` directory as `php_pdo_cubrid.dll`.
3. Copy `cascci.dll` next to `php.exe` (or to a directory in `PATH`).
4. Add `extension=pdo_cubrid` to `php.ini`. PDO is built into the official Windows PHP builds.

The Microsoft Visual C++ 2015-2022 Redistributable (x64) is required.

Building
--------

```
git clone --recursive https://github.com/CUBRID/cubrid-pdo.git
cd cubrid-pdo
./build.sh 84            # Linux, PHP 8.3/8.4/8.5 -> build_linux/84/modules/pdo_cubrid.so
.\build_win.ps1 84       # Windows, PHP 8.4/8.5  -> build_win\84-nts\modules\php_pdo_cubrid.dll
```

See [build-guide.md](build-guide.md) for the requirements, tests and how to make a release
(`script/release.sh`, `script/release.ps1`).

Documentation
-------------

- CUBRID PDO driver manual: https://www.cubrid.org/manual/en/11.4/api/pdo.html
- CUBRID APIs: https://www.cubrid.org/manual/en/11.4/api/index.html

License
-------

The driver and CCI are distributed under the BSD 3-Clause License (the CUBRID database server
engine is under the Apache License 2.0). The binaries include OpenSSL. See [LICENSE](LICENSE).
