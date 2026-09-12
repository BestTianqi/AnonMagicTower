[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$InputPath,
    [Parameter(Mandatory=$true)][string]$OutputPath,
    [ValidateRange(1,254)][int]$AlphaThreshold = 128
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

try {
    $source = [Drawing.Bitmap]::new((Resolve-Path -LiteralPath $InputPath).Path)
} catch {
    Write-Output 'ERROR: source image cannot be decoded'
    exit 1
}

$hasTransparency = $false
for ($y=0; $y -lt $source.Height -and -not $hasTransparency; $y++) {
    for ($x=0; $x -lt $source.Width; $x++) {
        if ($source.GetPixel($x,$y).A -lt 255) { $hasTransparency=$true; break }
    }
}
if (-not $hasTransparency) {
    $source.Dispose()
    Write-Output 'ERROR: source has no transparency; background removal requires image editing'
    exit 1
}
$target = [Drawing.Bitmap]::new(240,240,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
try {
    for ($y=0; $y -lt 240; $y++) {
        $sourceY=[Math]::Min($source.Height-1,[Math]::Floor(($y * $source.Height) / 240))
        for ($x=0; $x -lt 240; $x++) {
            $sourceX=[Math]::Min($source.Width-1,[Math]::Floor(($x * $source.Width) / 240))
            $pixel=$source.GetPixel($sourceX,$sourceY)
            $alpha=if ($pixel.A -lt $AlphaThreshold) { 0 } else { 255 }
            $target.SetPixel($x,$y,[Drawing.Color]::FromArgb($alpha,$pixel.R,$pixel.G,$pixel.B))
        }
    }
    # Enforce the sprite contract's two-pixel transparent guard in every 60x60 cell.
    for ($row=0; $row -lt 4; $row++) {
        for ($column=0; $column -lt 4; $column++) {
            $left=$column*60; $top=$row*60
            for ($local=0; $local -lt 60; $local++) {
                foreach ($point in @(
                    [pscustomobject]@{X=$left+$local;Y=$top}, [pscustomobject]@{X=$left+$local;Y=$top+1},
                    [pscustomobject]@{X=$left+$local;Y=$top+58}, [pscustomobject]@{X=$left+$local;Y=$top+59},
                    [pscustomobject]@{X=$left;Y=$top+$local}, [pscustomobject]@{X=$left+1;Y=$top+$local},
                    [pscustomobject]@{X=$left+58;Y=$top+$local}, [pscustomobject]@{X=$left+59;Y=$top+$local}
                )) {
                    $p=$target.GetPixel($point.X,$point.Y); $target.SetPixel($point.X,$point.Y,[Drawing.Color]::FromArgb(0,$p.R,$p.G,$p.B))
                }
            }
        }
    }
    $source.Dispose()
    $source=$null

    $outputFull=[IO.Path]::GetFullPath($OutputPath)
    $outputDirectory=Split-Path -Parent $outputFull
    New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
    $temp=Join-Path $outputDirectory ('.sprite-normalize-{0}.png' -f [guid]::NewGuid().ToString('N'))
    try {
        $target.Save($temp,[Drawing.Imaging.ImageFormat]::Png)
        Move-Item -LiteralPath $temp -Destination $outputFull -Force
    } finally {
        Remove-Item -LiteralPath $temp -Force -ErrorAction SilentlyContinue
    }
} finally {
    if ($null -ne $source) { $source.Dispose() }
    $target.Dispose()
}

Write-Output 'PASS: normalized 240x240 binary-alpha sprite'
exit 0
