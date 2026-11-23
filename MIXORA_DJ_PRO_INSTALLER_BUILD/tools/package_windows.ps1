# MIXORA DJ PRO 2.5 - Windows x64 build and portable distribution.
# Run inside a VS 2022 x64 developer PowerShell with Qt 6.4+ MSVC2022 x64 installed.
# Usage: powershell -ExecutionPolicy Bypass -File .\tools\package_windows.ps1 -QtRoot "C:\Qt\6.8.3\msvc2022_64"
param(
 [Parameter(Mandatory=$true)][string]$QtRoot,
 [switch]$SkipTests
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$qt = (Resolve-Path $QtRoot).Path
$cmakeQt = Join-Path $qt 'lib\cmake'
$deploy = Join-Path $qt 'bin\windeployqt.exe'
$target = Join-Path $root 'build-win64'
$dist = Join-Path $root 'dist\MIXORA-DJ-PRO-2.5-Windows-x64'
$exe = Join-Path $target 'Release\MIXORA DJ PRO.exe'
if (-not (Test-Path $deploy)) { throw "Qt MSVC windeployqt.exe missing: $deploy" }
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { throw 'CMake missing from PATH.' }
if (-not (Get-Command ctest -ErrorAction SilentlyContinue)) { throw 'CTest missing from PATH.' }
cmake -S $root -B $target -G 'Visual Studio 17 2022' -A x64 -DCMAKE_PREFIX_PATH="$qt" -DBUILD_TESTING=ON
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
cmake --build $target --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed.' }
if (-not $SkipTests) {
 ctest --test-dir $target -C Release --output-on-failure
 if ($LASTEXITCODE -ne 0) { throw 'Unit tests failed; packaging halted.' }
}
if (-not (Test-Path $exe)) { throw "Executable not found: $exe" }
if (Test-Path $dist) { Remove-Item $dist -Recurse -Force }
New-Item -ItemType Directory -Path $dist -Force | Out-Null
Copy-Item $exe -Destination $dist
& $deploy --release --compiler-runtime --no-translations --dir $dist (Join-Path $dist 'MIXORA DJ PRO.exe')
if ($LASTEXITCODE -ne 0) { throw 'Qt runtime deployment failed.' }
@'
MIXORA DJ PRO 2.5 - PORTABLE WINDOWS TEST BUILD
Run "MIXORA DJ PRO.exe". This is not an installer and not production-ready.
Install compatible Native Instruments drivers where required.
Use a multichannel audio interface to test independent Master/Cue.
Controller mappings require hardware validation; LED mode is opt-in only.
'@ | Set-Content -LiteralPath (Join-Path $dist 'LEGGIMI.txt') -Encoding UTF8
$zip = Join-Path $root 'dist\MIXORA-DJ-PRO-2.5-Windows-x64-PORTABLE.zip'
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path $dist -DestinationPath $zip -CompressionLevel Optimal
Write-Host "SUCCESS: $zip" -ForegroundColor Green
