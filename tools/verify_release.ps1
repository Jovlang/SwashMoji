param([string]$BuildDirectory = 'build-m6')
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$build = [IO.Path]::GetFullPath((Join-Path $repo $BuildDirectory))
if (-not $build.StartsWith($repo + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Use a build directory inside the repository.'
}
if (-not (Split-Path -Leaf $build).StartsWith('build')) { throw 'Use a build* directory.' }
if (-not $env:SWASHMOJI_PYTHON) { throw 'Set SWASHMOJI_PYTHON to Python 3.10+; release verification requires the offline generator suite.' }
$run = Join-Path $build ('release-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0,8))
New-Item -ItemType Directory -Path $run | Out-Null
Push-Location $repo
try {
    & .\build.cmd test $build 2>&1 | Tee-Object -FilePath (Join-Path $run 'build-test.txt')
    if ($LASTEXITCODE -ne 0) { throw 'Build or CTest failed; no package produced.' }
    $cache = Get-Content -LiteralPath (Join-Path $build 'CMakeCache.txt')
    $cmake = ($cache | Where-Object { $_ -like 'CMAKE_COMMAND:INTERNAL=*' }) -replace '^CMAKE_COMMAND:INTERNAL=', ''
    $ctest = Join-Path (Split-Path -Parent $cmake) 'ctest.exe'
    $registered = & $ctest --test-dir $build --show-only=json-v1
    if ($LASTEXITCODE -ne 0) { throw 'Could not list CTest suites.' }
    if ('catalog_generator' -notin ($registered | ConvertFrom-Json).tests.name) { throw 'Python suite was not registered.' }
    $package = Join-Path $run 'portable'
    & $cmake --install $build --prefix $package | Tee-Object -FilePath (Join-Path $run 'install.txt')
    if ($LASTEXITCODE -ne 0) { throw 'Portable install failed.' }
    $expected = @('SwashMoji.exe', 'emojis.txt', 'intent_phrases.tsv', 'LICENSE', 'UNICODE_LICENSE.txt')
    $actual = @(Get-ChildItem -LiteralPath $package -File | Select-Object -ExpandProperty Name)
    if (Compare-Object ($expected | Sort-Object) ($actual | Sort-Object)) { throw 'Unexpected portable payload.' }
    $verification = Join-Path $run 'verification'
    New-Item -ItemType Directory -Path $verification | Out-Null
    Push-Location $verification
    try {
        & (Join-Path $build 'SwashMojiReleaseTests.exe') $package 2>&1 | Tee-Object -FilePath (Join-Path $run 'package-smoke.txt')
        if ($LASTEXITCODE -ne 0) { throw 'Staged data/profile smoke check failed.' }
        & (Join-Path $build 'SwashMojiPerformance.exe') 2>&1 | Tee-Object -FilePath (Join-Path $run 'performance.csv')
        if ($LASTEXITCODE -ne 0) { throw 'Performance runner failed.' }
    } finally { Pop-Location }
    $hashes = foreach ($name in $expected) {
        $hash = Get-FileHash -LiteralPath (Join-Path $package $name) -Algorithm SHA256
        [ordered]@{file=$name; sha256=$hash.Hash; bytes=(Get-Item -LiteralPath $hash.Path).Length}
    }
    $zip = Join-Path $run 'SwashMoji-portable.zip'
    Compress-Archive -LiteralPath ($expected | ForEach-Object { Join-Path $package $_ }) -DestinationPath $zip
    $extracted = Join-Path $run 'extracted package æøå'
    Expand-Archive -LiteralPath $zip -DestinationPath $extracted
    foreach ($entry in $hashes) {
        if ((Get-FileHash -LiteralPath (Join-Path $extracted $entry.file) -Algorithm SHA256).Hash -ne $entry.sha256) {
            throw 'ZIP round-trip hash mismatch.'
        }
    }
    Push-Location $verification
    try {
        & (Join-Path $build 'SwashMojiReleaseTests.exe') $extracted 2>&1 | Tee-Object -FilePath (Join-Path $run 'zip-smoke.txt')
        if ($LASTEXITCODE -ne 0) { throw 'Extracted ZIP smoke check failed.' }
    } finally { Pop-Location }
    $sourceFiles = @(& git ls-files --cached --others --exclude-standard) | Sort-Object -Unique
    if ($LASTEXITCODE -ne 0) { throw 'Could not record source inventory.' }
    $sources = foreach ($name in $sourceFiles) {
        if (Test-Path -LiteralPath $name -PathType Leaf) {
            [ordered]@{file=$name; sha256=(Get-FileHash -LiteralPath $name -Algorithm SHA256).Hash}
        }
    }
    # Registry reads work in environments where WMI/CIM access is unavailable.
    $os = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion' -Name CurrentBuild, UBR
    $cpu = Get-ItemProperty 'HKLM:\HARDWARE\DESCRIPTION\System\CentralProcessor\0' -Name ProcessorNameString
    [ordered]@{
        recordedUtc=(Get-Date).ToUniversalTime().ToString('o'); commit=(& git rev-parse HEAD)
        workingTree=@(& git status --short); osVersion=[Environment]::OSVersion.VersionString
        osBuild=($os.CurrentBuild + '.' + $os.UBR); cpu=$cpu.ProcessorNameString
        logicalProcessors=[Environment]::ProcessorCount
        cmake=$cmake; python=$env:SWASHMOJI_PYTHON
        buildType=($cache | Where-Object { $_ -like 'CMAKE_BUILD_TYPE:*' })
        compiler=($cache | Where-Object { $_ -like 'CMAKE_CXX_COMPILER:*' })
        tests=($registered | ConvertFrom-Json).tests.name
        payload=$hashes; zipSha256=(Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash
        sourceFiles=$sources
        acceptance='Automated evidence only. Desktop, visible-opening latency, compatibility, accessibility and consented user trials remain separate gates.'
    } | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $run 'manifest.json') -Encoding UTF8
    Write-Output "Verification evidence and portable candidate: $run"
} finally { Pop-Location }
