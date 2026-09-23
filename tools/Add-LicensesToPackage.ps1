param(
    [Parameter(Mandatory = $false)]
    [string]$PackagePath = (Join-Path $PSScriptRoot "..\dist\LocalMapUpgrade-3.1.0-AE-1.7.104.0.zip")
)

$ErrorActionPreference = "Stop"
$sourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$package = (Resolve-Path $PackagePath).Path
$tempRoot = Join-Path $sourceRoot ".lmu-license-tmp"
$tempPackage = Join-Path $tempRoot ([IO.Path]::GetFileName($package))
$docsRoot = Join-Path $tempRoot "Docs"

if (Test-Path -LiteralPath $tempRoot) {
    Remove-Item -LiteralPath $tempRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $docsRoot -Force | Out-Null
Copy-Item -LiteralPath $package -Destination $tempPackage

foreach ($file in @("LICENSE", "NOTICE.md", "THIRD_PARTY_NOTICES.md")) {
    Copy-Item -LiteralPath (Join-Path $sourceRoot $file) -Destination $docsRoot
}
Copy-Item -LiteralPath (Join-Path $sourceRoot "licenses") -Destination $docsRoot -Recurse

Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::Open($tempPackage, [IO.Compression.ZipArchiveMode]::Update)
try {
    $prefix = "Docs/"
    foreach ($entry in @($archive.Entries)) {
        $name = $entry.FullName.Replace('\\', '/')
        if ($name.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
            $entry.Delete()
        }
    }
    Get-ChildItem -LiteralPath $docsRoot -File -Recurse | ForEach-Object {
        $relative = $_.FullName.Substring($docsRoot.Length).TrimStart('\\').Replace('\\', '/')
        [IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
            $archive, $_.FullName, "$prefix$relative",
            [IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
}
finally {
    $archive.Dispose()
}

$required = @(
    "SKSE/Plugins/LocalMapUpgrade.dll",
    "Docs/LICENSE",
    "Docs/NOTICE.md",
    "Docs/THIRD_PARTY_NOTICES.md",
    "Docs/licenses/LocalMapUpgrade-MIT.txt",
    "Docs/licenses/CommonLibSSE-NG-GPL-3.0-or-later.txt",
    "Docs/licenses/CommonLibSSE-NG-EXCEPTIONS.md",
    "Docs/licenses/SimpleIni-MIT.txt"
)
$check = [IO.Compression.ZipFile]::OpenRead($tempPackage)
try {
    $names = @($check.Entries | ForEach-Object { $_.FullName.Replace('\\', '/') })
    foreach ($name in $required) {
        if ($names -notcontains $name) {
            throw "Package validation failed: missing $name"
        }
    }
}
finally {
    $check.Dispose()
}

Copy-Item -LiteralPath $tempPackage -Destination $package -Force
Remove-Item -LiteralPath $tempRoot -Recurse -Force
Write-Host "Updated and validated: $package"
