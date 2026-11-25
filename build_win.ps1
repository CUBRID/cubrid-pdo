<#
.SYNOPSIS
    Build the CUBRID PDO driver (php_pdo_cubrid.dll) on Windows with Visual Studio 2022 (vs17).

.DESCRIPTION
    .\build_win.ps1 <ver> [options]   ->  build_win\<ver>-<nts|ts>\modules\php_pdo_cubrid.dll
        ver: 84, 85 (pdo_cubrid8.c, cubrid_driver8.c, cubrid_statement8.c)

    1. CCI (cci-src) is built once with CMake as cascci.dll and shared by all targets
       (build_win\cci\{bin,lib,include}).
    2. The PHP devel pack (phpize), the PHP binary of the same version and the
       PHP SDK binary tools (bison, sed, re2c required by configure) are downloaded
       into build_win\downloads.
    3. The sources are copied into build_win\<ver>-<nts|ts> and built with
       phpize / configure --with-pdo-cubrid / nmake, so the source tree stays clean.
       PDO itself is built into php8.dll of the official Windows builds.
    4. The path of php.exe is written to build_win\<ver>-<nts|ts>\.php_exe for
       tests_8x\run_tests.ps1.

    Requirements: Visual Studio 2022 with "Desktop development with C++" (MSVC v143,
    CMake), Git for Windows (the CCI CMake uses git and awk of Git\usr\bin).

.PARAMETER PhpVersion
    Full PHP version as in the file names of windows.php.net, such as 8.4.14 or
    8.4.27-1 (default: latest release of <ver>).

.PARAMETER DevelPack
    Existing PHP devel pack directory (containing phpize.bat); skips the download.

.PARAMETER PhpDir
    Existing PHP directory (containing php.exe) used for the load check;
    skips the download.

.PARAMETER PhpSdk
    Existing php-sdk-binary-tools directory; skips the download.

.PARAMETER ThreadSafe
    Build for thread safe (TS) PHP. Default is NTS.

.PARAMETER Clean
    Remove the build directory (and the CCI build) and build from scratch.

.EXAMPLE
    .\build_win.ps1 84
    .\build_win.ps1 85 -ThreadSafe
    .\build_win.ps1 84 -PhpVersion 8.4.14 -Clean
#>
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateSet('84', '85')]
    [string]$Target,
    [string]$PhpVersion,
    [string]$DevelPack,
    [string]$PhpDir,
    [string]$PhpSdk,
    [switch]$ThreadSafe,
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'   # Invoke-WebRequest is very slow with the progress bar
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$Root = $PSScriptRoot
$PhpXY = $Target.Substring(0, 1) + '.' + $Target.Substring(1)
$Flavor = if ($ThreadSafe) { 'ts' } else { 'nts' }
$WinRoot = Join-Path $Root 'build_win'
$Downloads = Join-Path $WinRoot 'downloads'
$CciBuild = Join-Path $WinRoot 'cci-build'
$CciOut = Join-Path $WinRoot 'cci'
$BuildDir = Join-Path $WinRoot "$Target-$Flavor"
$PhpSdkTag = 'php-sdk-2.9.0'

# The script changes the environment of the calling session (vcvars64.bat, PATH).
# Restore it at the end so that repeated runs in the same terminal do not keep
# growing PATH, INCLUDE and LIB ("The input line is too long").
$SavedEnv = @{}
Get-ChildItem env: | ForEach-Object { $SavedEnv[$_.Name] = $_.Value }

function Restore-Env {
    @(Get-ChildItem env: | Where-Object { -not $SavedEnv.ContainsKey($_.Name) }) |
        ForEach-Object { Remove-Item -Path "env:$($_.Name)" }
    foreach ($name in $SavedEnv.Keys) { Set-Item -Path "env:$name" -Value $SavedEnv[$name] }
}

function Invoke-Checked {
    param([string]$Exe, [string[]]$Arguments)
    Write-Host "> $Exe $($Arguments -join ' ')"
    & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Exe failed (exit code $LASTEXITCODE)" }
}

# Load the x64 build environment of Visual Studio 2022 into this process.
function Import-VsEnv {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path $vswhere)) { throw "vswhere.exe not found. Install Visual Studio 2022." }
    $vs = & $vswhere -version '[17.0,18.0)' -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath |
        Select-Object -First 1
    if (-not $vs) { throw "Visual Studio 2022 (vs17) with the C++ x64 tools not found." }
    $vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
    Write-Host "Visual Studio: $vs"
    # 2>&1: vcvars64.bat may print warnings to stderr, which PowerShell 5.1 turns into
    # a terminating error under $ErrorActionPreference = 'Stop' when output is redirected.
    cmd /c "`"$vcvars`" >nul 2>&1 && set" | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$') { Set-Item -Path "env:$($Matches[1])" -Value $Matches[2] }
    }
    if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) { throw "cl.exe not found after vcvars64.bat" }
    if (-not (Get-Command cmake.exe -ErrorAction SilentlyContinue)) {
        throw "cmake.exe not found. Install the 'C++ CMake tools for Windows' component or CMake."
    }

    # The CCI CMake runs git and awk. Append (not prepend) Git\usr\bin so that
    # its link.exe does not hide the MSVC linker.
    $git = Get-Command git.exe -ErrorAction SilentlyContinue
    if (-not $git) { throw "git.exe not found. Install Git for Windows." }
    $gitUsrBin = Join-Path (Split-Path (Split-Path $git.Source)) 'usr\bin'
    if (Test-Path $gitUsrBin) { $env:PATH = "$env:PATH;$gitUsrBin" }
}

function Get-Package {
    param([string]$FileName, [string[]]$Urls)
    $zip = Join-Path $Downloads $FileName
    $dir = $zip -replace '\.zip$', ''
    if (-not (Test-Path $dir)) {
        New-Item -ItemType Directory -Force -Path $Downloads | Out-Null
        if (-not (Test-Path $zip)) {
            $ok = $false
            foreach ($url in $Urls) {
                try {
                    Write-Host "Downloading $url"
                    Invoke-WebRequest -Uri $url -OutFile $zip -UseBasicParsing
                    $ok = $true
                    break
                } catch {
                    Remove-Item -Force -ErrorAction SilentlyContinue $zip
                }
            }
            if (-not $ok) { throw "Cannot download $FileName" }
        }
        Expand-Archive -Path $zip -DestinationPath $dir
    }
    return $dir
}

function Get-PhpPackage {
    param([string]$FileName)
    return Get-Package $FileName @("https://windows.php.net/downloads/releases/$FileName",
                                   "https://windows.php.net/downloads/releases/archives/$FileName")
}

function Build-Cci {
    if ((Test-Path (Join-Path $CciOut 'lib\cascci.lib')) -and (Test-Path (Join-Path $CciOut 'bin\cascci.dll'))) {
        Write-Host "CCI: $CciOut (already built)"
        return
    }
    # CMAKE_POLICY_VERSION_MINIMUM: cci-src requires CMake 2.8, which CMake 4.x rejects.
    Invoke-Checked cmake @('-S', (Join-Path $Root 'cci-src'), '-B', $CciBuild,
        '-G', 'Visual Studio 17 2022', '-A', 'x64', '-DCMAKE_POLICY_VERSION_MINIMUM=3.5')
    Invoke-Checked cmake @('--build', $CciBuild, '--config', 'Release', '--target', 'cascci')

    $dll = Get-ChildItem -Path $CciBuild -Recurse -Filter cascci.dll |
        Where-Object { $_.DirectoryName -like '*\Release' } | Select-Object -First 1
    $lib = Get-ChildItem -Path $CciBuild -Recurse -Filter cascci.lib |
        Where-Object { $_.DirectoryName -like '*\Release' } | Select-Object -First 1
    if (-not $dll -or -not $lib) { throw "cascci.dll / cascci.lib not found under $CciBuild" }

    foreach ($d in 'bin', 'lib', 'include') {
        New-Item -ItemType Directory -Force -Path (Join-Path $CciOut $d) | Out-Null
    }
    Copy-Item $dll.FullName (Join-Path $CciOut 'bin')
    Copy-Item $lib.FullName (Join-Path $CciOut 'lib')
    foreach ($h in 'cas_cci.h', 'broker_cas_error.h', 'compat_dbtran_def.h') {
        Copy-Item (Join-Path $Root "cci-src\src\cci\$h") (Join-Path $CciOut 'include')
    }
    Write-Host "CCI: $CciOut"
}

try {
    if ($Clean) {
        Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $BuildDir, $CciBuild, $CciOut
    }

    Import-VsEnv

    # PHP devel pack and binary
    $pkgFlavor = if ($ThreadSafe) { '' } else { 'nts-' }
    $develZip = $null
    $phpZip = $null
    if (-not $DevelPack -or -not $PhpDir) {
        if ($PhpVersion) {
            $develZip = "php-devel-pack-$PhpVersion-${pkgFlavor}Win32-vs17-x64.zip"
            $phpZip = "php-$PhpVersion-${pkgFlavor}Win32-vs17-x64.zip"
        } else {
            # The version may carry a respin suffix (e.g. 8.4.27-1), so take the file names as listed.
            $releases = Invoke-RestMethod -Uri 'https://windows.php.net/downloads/releases/releases.json' -UseBasicParsing
            $build = $releases.$PhpXY."$Flavor-vs17-x64"
            if (-not $build) { throw "No PHP $PhpXY $Flavor-vs17-x64 release found in releases.json (use -PhpVersion)" }
            $PhpVersion = $releases.$PhpXY.version
            $develZip = $build.devel_pack.path
            $phpZip = $build.zip.path
        }
    }
    if ($PhpVersion -and -not $PhpVersion.StartsWith("$PhpXY.")) { throw "-PhpVersion $PhpVersion is not PHP $PhpXY" }
    if (-not $DevelPack) {
        $DevelPack = Get-PhpPackage $develZip
        # The zip has one top directory (php-<ver>-devel-vs17-x64).
        $DevelPack = (Get-ChildItem -Path $DevelPack -Recurse -Filter phpize.bat | Select-Object -First 1).DirectoryName
    }
    if (-not $PhpDir) {
        $PhpDir = Get-PhpPackage $phpZip
    }
    if (-not $PhpSdk) {
        $PhpSdk = Get-Package "$PhpSdkTag.zip" @("https://github.com/php/php-sdk-binary-tools/archive/refs/tags/$PhpSdkTag.zip")
        # The zip has one top directory (php-sdk-binary-tools-<tag>).
        $PhpSdk = (Get-ChildItem -Path $PhpSdk -Recurse -Filter phpsdk_version.bat | Select-Object -First 1).Directory.Parent.FullName
    }
    $sdkTools = (Join-Path $PhpSdk 'bin'), (Join-Path $PhpSdk 'msys2\usr\bin')
    foreach ($d in $sdkTools) { if (-not (Test-Path $d)) { throw "$d not found (PHP SDK: $PhpSdk)" } }
    # Appended so that the MSVC tools keep precedence.
    $env:PATH = "$env:PATH;$($sdkTools -join ';')"

    $phpize = Join-Path $DevelPack 'phpize.bat'
    $phpExe = Join-Path $PhpDir 'php.exe'
    if (-not (Test-Path $phpize)) { throw "phpize.bat not found in $DevelPack" }
    if (-not (Test-Path $phpExe)) { throw "php.exe not found in $PhpDir" }

    Build-Cci

    # Build the extension out of the source tree.
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
    Copy-Item (Join-Path $Root 'config.w32') $BuildDir
    Copy-Item (Join-Path $Root 'pdo_cubrid*.c'), (Join-Path $Root 'cubrid_driver*.c'),
              (Join-Path $Root 'cubrid_statement*.c'), (Join-Path $Root 'php_pdo_cubrid*.h'),
              (Join-Path $Root 'pdo_cubrid_version.h') $BuildDir
    Push-Location $BuildDir
    try {
        Invoke-Checked $phpize @()
        Invoke-Checked (Join-Path $BuildDir 'configure.bat') @("--with-pdo-cubrid=$CciOut")
        Invoke-Checked nmake.exe @('/nologo')
    } finally {
        Pop-Location
    }

    $dll = Get-ChildItem -Path $BuildDir -Recurse -Filter php_pdo_cubrid.dll |
        Where-Object { $_.DirectoryName -notlike '*\modules' } |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $dll) { throw "php_pdo_cubrid.dll not found under $BuildDir" }

    # php_pdo_cubrid.dll needs cascci.dll next to it (or in PATH).
    $Modules = Join-Path $BuildDir 'modules'
    New-Item -ItemType Directory -Force -Path $Modules | Out-Null
    Copy-Item $dll.FullName $Modules
    Copy-Item (Join-Path $CciOut 'bin\cascci.dll') $Modules
    $ext = Join-Path $Modules 'php_pdo_cubrid.dll'

    # Load check
    $env:PATH = "$Modules;$env:PATH"
    # No double quotes in the PHP code: Windows PowerShell 5.1 strips them when passing
    # arguments to native programs (pdo_cubrid would become an undefined constant).
    $loaded = & $phpExe -n -d "extension=$ext" -r "echo in_array('cubrid', PDO::getAvailableDrivers()) ? phpversion('pdo_cubrid') : '';"
    if ($LASTEXITCODE -ne 0 -or -not $loaded) { throw "php_pdo_cubrid.dll does not load in $phpExe" }

    # Used by tests_8x\run_tests.ps1
    Set-Content -Path (Join-Path $BuildDir '.php_exe') -Value $phpExe

    Write-Host ''
    Write-Host "PHP     : $phpExe ($(& $phpExe -n -r 'echo PHP_VERSION;'), $Flavor)"
    Write-Host "Module  : $ext (pdo_cubrid $loaded)"
    Write-Host "Test    : .\tests_8x\run_tests.ps1 $Target$(if ($ThreadSafe) { ' -ThreadSafe' })"
} finally {
    Restore-Env
}
