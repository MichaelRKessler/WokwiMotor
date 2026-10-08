# Compiles the Wokwi custom chips in ./chips to WebAssembly in ./chips/build.
# Downloads wasi-sdk to ~/.wasi-sdk on first run.
# Runs on Windows PowerShell 5.1, and on PowerShell 7+ on Windows, macOS and Linux.

$ErrorActionPreference = 'Stop'

# $IsLinux and $IsMacOS only exist in PowerShell 7+; unset means Windows PowerShell.
$arch = if ([System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture -eq 'Arm64') { 'arm64' } else { 'x86_64' }
if ($IsLinux) {
    $platform = "$arch-linux"; $exe = ''
} elseif ($IsMacOS) {
    $platform = "$arch-macos"; $exe = ''
} else {
    $platform = "$arch-windows"; $exe = '.exe'
}

$sdkVersion = '34'
$sdkName = "wasi-sdk-$sdkVersion.0-$platform"
$sdkRoot = Join-Path $HOME '.wasi-sdk'
$sdkDir = Join-Path $sdkRoot $sdkName
$clang = Join-Path $sdkDir "bin/clang$exe"

if (-not (Test-Path $clang)) {
    Write-Host "Downloading wasi-sdk $sdkVersion ($platform)..."
    New-Item -ItemType Directory -Force $sdkRoot | Out-Null
    $archive = Join-Path $sdkRoot 'wasi-sdk.tar.gz'
    Invoke-WebRequest "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-$sdkVersion/$sdkName.tar.gz" -OutFile $archive
    tar -xzf $archive -C $sdkRoot
    Remove-Item $archive
}

$chipsDir = Join-Path $PSScriptRoot 'chips'
$outDir = Join-Path $chipsDir 'build'
New-Item -ItemType Directory -Force $outDir | Out-Null

foreach ($src in Get-ChildItem $chipsDir -Filter '*.chip.c') {
    $name = $src.Name -replace '\.chip\.c$', ''
    Write-Host "Building $name"
    & $clang --target=wasm32-wasip1 -O2 -Wall -Werror -Wno-unused-function `
        -nostartfiles '-Wl,--import-memory' '-Wl,--export-table' '-Wl,--no-entry' `
        -o (Join-Path $outDir "$name.chip.wasm") $src.FullName
    if ($LASTEXITCODE -ne 0) { throw "Failed to build $name" }
    # Wokwi expects the chip definition next to the binary.
    Copy-Item (Join-Path $chipsDir "$name.chip.json") $outDir
}
