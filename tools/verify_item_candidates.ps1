param(
    [ValidateSet('Preflight','Validate','ReplacementGate','ComparisonGate')]
    [string]$Mode = 'Validate',
    [string]$Source,
    [string]$Candidate,
    [Parameter(Mandatory = $true)][string]$Manifest,
    [string]$Resources,
    [string]$RunManifest,
    [int]$ExpectedCount = 57
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

function Stop-Fail([string]$Message) {
    Write-Output "ERROR: $Message"
    exit 1
}

function Get-RelativePngs([string]$Root) {
    if (-not (Test-Path -LiteralPath $Root -PathType Container)) { Stop-Fail "directory missing: $Root" }
    $resolved = (Resolve-Path -LiteralPath $Root).Path
    return @(Get-ChildItem -LiteralPath $resolved -Recurse -File -Filter '*.png' |
        ForEach-Object { $_.FullName.Substring($resolved.Length + 1).Replace('\','/') } |
        Sort-Object)
}

function Assert-Decodable([string]$Root, [string[]]$RelativePaths, [bool]$CheckCandidate) {
    foreach ($relative in $RelativePaths) {
        $path = Join-Path $Root $relative
        try { $bmp = [Drawing.Bitmap]::FromFile($path) } catch { Stop-Fail "$relative is not a decodable PNG" }
        try {
            if ($CheckCandidate) {
                if ($bmp.Width -ne 60 -or $bmp.Height -ne 60) {
                    Stop-Fail "$relative must be 60x60, got $($bmp.Width)x$($bmp.Height)"
                }
                if (($bmp.PixelFormat -band [Drawing.Imaging.PixelFormat]::Alpha) -eq 0 -and
                    ($bmp.PixelFormat -band [Drawing.Imaging.PixelFormat]::PAlpha) -eq 0) {
                    Stop-Fail "$relative must have an alpha channel"
                }
                $opaque = 0
                for ($y = 0; $y -lt 60; $y++) {
                    for ($x = 0; $x -lt 60; $x++) {
                        $a = $bmp.GetPixel($x,$y).A
                        if ($a -gt 0) { $opaque++ }
                        if (($x -lt 2 -or $x -gt 57 -or $y -lt 2 -or $y -gt 57) -and $a -ne 0) {
                            Stop-Fail "$relative touches the two-pixel transparent guard"
                        }
                    }
                }
                if ($opaque -eq 0) { Stop-Fail "$relative has no visible subject" }
            }
        } finally { $bmp.Dispose() }
    }
}

if (-not (Test-Path -LiteralPath $Manifest -PathType Leaf)) { Stop-Fail "manifest missing: $Manifest" }
try {
    $manifestText = [IO.File]::ReadAllText((Resolve-Path -LiteralPath $Manifest), [Text.Encoding]::UTF8)
    $manifestData = $manifestText | ConvertFrom-Json
} catch { Stop-Fail "manifest is not valid JSON: $($_.Exception.Message)" }

if ($Mode -eq 'ReplacementGate') {
    if ($manifestData.status -ne 'approved') { Stop-Fail 'item candidate set is not approved' }
    Write-Output 'ALLOW: item candidate replacement approved'
    exit 0
}

if ($Mode -eq 'ComparisonGate') {
    if ($RunManifest -and (Test-Path -LiteralPath $RunManifest)) {
        $runText = [IO.File]::ReadAllText((Resolve-Path -LiteralPath $RunManifest), [Text.Encoding]::UTF8)
        $run = $runText | ConvertFrom-Json
        if (@($run.batches | Where-Object status -eq 'failed').Count -gt 0) { Stop-Fail 'one or more generation batches failed' }
    }
    if (@($manifestData.items | Where-Object review_status -ne 'approved').Count -gt 0) {
        Stop-Fail 'all 57 item reviews must be approved before comparison generation'
    }
    Write-Output 'ALLOW: item candidate comparison generation'
    exit 0
}

if (-not $Source) { Stop-Fail 'Source is required' }
$sourcePaths = Get-RelativePngs $Source
if ($sourcePaths.Count -ne $ExpectedCount) { Stop-Fail "source must contain exactly $ExpectedCount PNGs, got $($sourcePaths.Count)" }
if (@($sourcePaths | Group-Object | Where-Object Count -gt 1).Count -gt 0) { Stop-Fail 'source contains duplicate relative paths' }
Assert-Decodable $Source $sourcePaths $false

$manifestPaths = @($manifestData.items | ForEach-Object { [string]$_.relative_path } | Sort-Object)
if ($manifestPaths.Count -ne $ExpectedCount) { Stop-Fail "manifest must contain exactly $ExpectedCount items" }
if (@($manifestPaths | Group-Object | Where-Object Count -gt 1).Count -gt 0) { Stop-Fail 'manifest contains duplicate relative paths' }
if (($sourcePaths -join "`n") -ne ($manifestPaths -join "`n")) { Stop-Fail 'manifest relative paths do not match source paths' }

if ($Mode -eq 'Preflight') {
    Write-Output "PASS: $ExpectedCount/$ExpectedCount source item PNGs"
    exit 0
}

if (-not $Candidate) { Stop-Fail 'Candidate is required' }
$candidatePaths = Get-RelativePngs $Candidate
if (($sourcePaths -join "`n") -ne ($candidatePaths -join "`n")) { Stop-Fail 'candidate relative paths do not match source paths' }
Assert-Decodable $Candidate $candidatePaths $true

if ($Resources) {
    $resourceText = Get-Content -Raw -LiteralPath $Resources
    if ($resourceText -match 'images[\\/]candidates[\\/]item_redesign_20260921') {
        Stop-Fail 'resources.qrc references item candidates'
    }
}

Write-Output "PASS: $ExpectedCount/$ExpectedCount item candidates"
exit 0
