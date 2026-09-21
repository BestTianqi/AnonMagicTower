param(
    [Parameter(Mandatory = $true)][string]$Sheet,
    [Parameter(Mandatory = $true)][string]$RunManifest,
    [Parameter(Mandatory = $true)][string]$BatchId,
    [Parameter(Mandatory = $true)][string]$StagingRoot
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$runText = [IO.File]::ReadAllText((Resolve-Path -LiteralPath $RunManifest), [Text.Encoding]::UTF8)
$run = $runText | ConvertFrom-Json
$batch = @($run.batches | Where-Object id -eq $BatchId)
if ($batch.Count -ne 1) { throw "Batch not found or duplicated: $BatchId" }

$source = [Drawing.Bitmap]::FromFile((Resolve-Path -LiteralPath $Sheet))
try {
    foreach ($cell in @($batch[0].cells | Where-Object { -not $_.unused })) {
        $x0 = [int][Math]::Floor($cell.column * $source.Width / 4.0)
        $x1 = [int][Math]::Floor(($cell.column + 1) * $source.Width / 4.0) - 1
        $y0 = [int][Math]::Floor($cell.row * $source.Height / 4.0)
        $y1 = [int][Math]::Floor(($cell.row + 1) * $source.Height / 4.0) - 1

        $minX = $x1; $minY = $y1; $maxX = $x0; $maxY = $y0; $visible = 0
        for ($y = $y0; $y -le $y1; $y++) {
            for ($x = $x0; $x -le $x1; $x++) {
                if ($source.GetPixel($x,$y).A -gt 8) {
                    $visible++
                    if ($x -lt $minX) { $minX = $x }
                    if ($x -gt $maxX) { $maxX = $x }
                    if ($y -lt $minY) { $minY = $y }
                    if ($y -gt $maxY) { $maxY = $y }
                }
            }
        }
        if ($visible -eq 0) { throw "$BatchId cell ($($cell.row),$($cell.column)) is empty" }

        $cropW = $maxX - $minX + 1
        $cropH = $maxY - $minY + 1
        $scale = [Math]::Min(54.0 / $cropW, 54.0 / $cropH)
        $drawW = [Math]::Max(1, [int][Math]::Round($cropW * $scale))
        $drawH = [Math]::Max(1, [int][Math]::Round($cropH * $scale))
        $drawX = [int][Math]::Floor((60 - $drawW) / 2)
        $drawY = [int][Math]::Floor((60 - $drawH) / 2)

        $target = [Drawing.Bitmap]::new(60,60,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
            $g = [Drawing.Graphics]::FromImage($target)
            try {
                $g.CompositingMode = [Drawing.Drawing2D.CompositingMode]::SourceCopy
                $g.Clear([Drawing.Color]::Transparent)
                $g.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                $g.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::HighQuality
                $g.DrawImage($source,
                    [Drawing.Rectangle]::new($drawX,$drawY,$drawW,$drawH),
                    [Drawing.Rectangle]::new($minX,$minY,$cropW,$cropH),
                    [Drawing.GraphicsUnit]::Pixel)
            } finally { $g.Dispose() }

            $outPath = Join-Path $StagingRoot ([string]$cell.relative_path)
            $parent = Split-Path -Parent $outPath
            if ($parent) { New-Item -ItemType Directory -Force -Path $parent | Out-Null }
            $target.Save($outPath, [Drawing.Imaging.ImageFormat]::Png)
            Write-Output "WROTE: $($cell.relative_path)"
        } finally { $target.Dispose() }
    }
} finally { $source.Dispose() }
