# Compiles the Wokwi custom chips in ./chips to WebAssembly in ./chips/build.
# Downloads wasi-sdk to ~/.wasi-sdk on first run.

$ErrorActionPreference = 'Stop'

$sdkVersion = '34'
$sdkRoot = Join-Path $HOME '.wasi-sdk'
$sdkDir = Join-Path $sdkRoot "wasi-sdk-$sdkVersion.0-x86_64-windows"
$clang = Join-Path $sdkDir 'bin\clang.exe'

if (-not (Test-Path $clang)) {
    Write-Host "Downloading wasi-sdk $sdkVersion..."
    New-Item -ItemType Directory -Force $sdkRoot | Out-Null
    $archive = Join-Path $sdkRoot 'wasi-sdk.tar.gz'
    Invoke-WebRequest "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-$sdkVersion/wasi-sdk-$sdkVersion.0-x86_64-windows.tar.gz" -OutFile $archive
    tar -xzf $archive -C $sdkRoot
    Remove-Item $archive
}

$chipsDir = Join-Path $PSScriptRoot 'chips'
$outDir = Join-Path $PSScriptRoot 'chips\build'
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
