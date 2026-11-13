<#
.SYNOPSIS
    Run tests_8x with the driver built by ..\build_win.ps1 <ver>.

.DESCRIPTION
    .\run_tests.ps1 <ver> [-ThreadSafe] [-PhpExe PATH] [-Ext PATH] [tests...] [-- run-tests.php options]
        ver: 84, 85
        no test given: all tests
    .\run_tests.ps1 <ver> -Php -- script.php [args...]
        run a PHP script with the driver loaded

    Options for run-tests.php or php that start with "-" must come after "--":
    PowerShell matches parameter names by prefix (-d would be -Debug, -p -PhpExe).

    Windows PowerShell 5.1 strips double quotes from arguments passed to native
    programs, so PHP code given with -r must use single quotes for PHP strings.

    The driver is ..\build_win\<ver>-<nts|ts>\modules\php_pdo_cubrid.dll and php.exe is
    the one build_win.ps1 used (written to ..\build_win\<ver>-<nts|ts>\.php_exe).
    php.ini is not read (-n); PDO is built into php8.dll, and php_pdo_sqlite.dll of the
    PHP zip (used by some PDO common tests) is loaded when present. cascci.dll is found
    through PATH (the modules directory is prepended while the script runs).
    The tests connect to demodb on localhost:33000.

.EXAMPLE
    .\run_tests.ps1 84
    .\run_tests.ps1 85 php8_lob.phpt pdo_005.phpt
    .\run_tests.ps1 85 -ThreadSafe -- --show-diff php8_lob.phpt
    .\run_tests.ps1 84 -Php -- -r "var_dump(PDO::getAvailableDrivers());"
#>
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateSet('84', '85')]
    [string]$Target,
    [switch]$ThreadSafe,
    [string]$PhpExe,
    [string]$Ext,
    [switch]$Php,
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Rest
)

$ErrorActionPreference = 'Stop'

# Restore the current directory and PATH of the calling session at the end.
$SavedEnv = @{}
foreach ($name in 'PATH', 'NO_COLOR', 'TEST_PHP_EXECUTABLE', 'TEST_PHP_CGI_EXECUTABLE') {
    $SavedEnv[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
}
Push-Location $PSScriptRoot
$exitCode = 1
try {
    $Flavor = if ($ThreadSafe) { 'ts' } else { 'nts' }
    $BuildDir = Join-Path (Split-Path $PSScriptRoot) "build_win\$Target-$Flavor"
    $Modules = Join-Path $BuildDir 'modules'

    if (-not $Ext) { $Ext = Join-Path $Modules 'php_pdo_cubrid.dll' }
    if (-not (Test-Path $Ext)) {
        throw "$Ext not found. Run '..\build_win.ps1 $Target$(if ($ThreadSafe) { ' -ThreadSafe' })' first."
    }
    $Ext = (Resolve-Path $Ext).Path

    if (-not $PhpExe) {
        $phpExeFile = Join-Path $BuildDir '.php_exe'
        if (-not (Test-Path $phpExeFile)) { throw "$phpExeFile not found (use -PhpExe PATH)" }
        $PhpExe = (Get-Content $phpExeFile -TotalCount 1).Trim()
    }
    if (-not (Test-Path $PhpExe)) { throw "php.exe not found: $PhpExe" }

    $phpVer = & $PhpExe -n -r 'echo PHP_MAJOR_VERSION, PHP_MINOR_VERSION;'
    if ($phpVer -ne $Target) { throw "$PhpExe is PHP $phpVer, not $Target" }

    # cascci.dll is next to php_pdo_cubrid.dll
    $env:PATH = "$(Split-Path $Ext);$env:PATH"
    $phpOpts = @('-n', '-d', "extension=$Ext")

    # Some PDO common tests use PDO_SQLite (ext\php_pdo_sqlite.dll of the PHP zip).
    # With -n, extension_dir is the compiled-in default, so give it explicitly.
    $phpExtDir = Join-Path (Split-Path $PhpExe) 'ext'
    if (Test-Path (Join-Path $phpExtDir 'php_pdo_sqlite.dll')) {
        $phpOpts += @('-d', "extension_dir=$phpExtDir", '-d', 'extension=pdo_sqlite')
    }

    # No double quotes in the PHP code: Windows PowerShell 5.1 strips them when passing
    # arguments to native programs ("cubrid" would become an undefined constant).
    $loaded = & $PhpExe @phpOpts -r "echo in_array('cubrid', PDO::getAvailableDrivers()) ? 'ok' : '';"
    if ($LASTEXITCODE -ne 0 -or $loaded -ne 'ok') { throw "$Ext does not load in $PhpExe" }

    if ($Php) {
        & $PhpExe @phpOpts @Rest
        $exitCode = $LASTEXITCODE
    } else {
        # Run the default test set when no test path is given.
        $hasTarget = $false
        foreach ($arg in $Rest) {
            if ($arg -and (Test-Path $arg)) { $hasTarget = $true }
        }
        $targets = if ($hasTarget) { @() } else { @('.') }

        $env:NO_COLOR = '1'
        # run-tests.php prefers these over -p, so a stale value in the user environment
        # (e.g. TEST_PHP_CGI_EXECUTABLE=\php-cgi.exe) would be used. Point them at this PHP.
        $env:TEST_PHP_EXECUTABLE = $PhpExe
        $phpCgi = Join-Path (Split-Path $PhpExe) 'php-cgi.exe'
        if (Test-Path $phpCgi) { $env:TEST_PHP_CGI_EXECUTABLE = $phpCgi }
        else { Remove-Item -ErrorAction SilentlyContinue env:TEST_PHP_CGI_EXECUTABLE }
        & $PhpExe run-tests.php -q @phpOpts -p $PhpExe @Rest @targets | Tee-Object -FilePath "pdo-tests_${phpVer}_${Flavor}.log"
        $exitCode = $LASTEXITCODE
    }
} finally {
    foreach ($name in $SavedEnv.Keys) {
        [Environment]::SetEnvironmentVariable($name, $SavedEnv[$name], 'Process')
    }
    Pop-Location
}
exit $exitCode
