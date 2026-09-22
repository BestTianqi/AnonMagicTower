param(
    [Parameter(Mandatory = $true)][string]$Board,
    [Parameter(Mandatory = $true)][string]$PortraitOut,
    [Parameter(Mandatory = $true)][string]$ModelOut,
    [Parameter(Mandatory = $true)][int[]]$PortraitRect,
    [Parameter(Mandatory = $true)][int[]]$ModelRect,
    [switch]$TransparentModel
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

if ($PortraitRect.Count -ne 4 -or $ModelRect.Count -ne 4) {
    throw "PortraitRect and ModelRect must contain x,y,width,height."
}

$source = [System.Drawing.Bitmap]::FromFile((Resolve-Path -LiteralPath $Board))
try {
    $portraitArea = [System.Drawing.Rectangle]::new(
        $PortraitRect[0], $PortraitRect[1], $PortraitRect[2], $PortraitRect[3])
    $modelArea = [System.Drawing.Rectangle]::new(
        $ModelRect[0], $ModelRect[1], $ModelRect[2], $ModelRect[3])

    $portrait = [System.Drawing.Bitmap]::new(
        $portraitArea.Width, $portraitArea.Height,
        [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        $graphics = [System.Drawing.Graphics]::FromImage($portrait)
        try {
            $graphics.DrawImage(
                $source,
                [System.Drawing.Rectangle]::new(0, 0, $portraitArea.Width, $portraitArea.Height),
                $portraitArea,
                [System.Drawing.GraphicsUnit]::Pixel)
        } finally {
            $graphics.Dispose()
        }
        $portrait.Save($PortraitOut, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $portrait.Dispose()
    }

    $model = [System.Drawing.Bitmap]::new(
        60, 60, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        $graphics = [System.Drawing.Graphics]::FromImage($model)
        try {
            if ($TransparentModel) {
                $graphics.Clear([System.Drawing.Color]::Transparent)
                $graphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
            } else {
                $graphics.Clear([System.Drawing.Color]::FromArgb(255, 239, 239, 239))
            }
            $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
            $scale = [Math]::Min(54.0 / $modelArea.Width, 56.0 / $modelArea.Height)
            $width = [Math]::Max(1, [int][Math]::Round($modelArea.Width * $scale))
            $height = [Math]::Max(1, [int][Math]::Round($modelArea.Height * $scale))
            $x = [int][Math]::Floor((60 - $width) / 2)
            $y = 60 - $height - 2
            $graphics.DrawImage(
                $source,
                [System.Drawing.Rectangle]::new($x, $y, $width, $height),
                $modelArea,
                [System.Drawing.GraphicsUnit]::Pixel)
        } finally {
            $graphics.Dispose()
        }
        $model.Save($ModelOut, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $model.Dispose()
    }
} finally {
    $source.Dispose()
}

$check = [System.Drawing.Image]::FromFile((Resolve-Path -LiteralPath $ModelOut))
try {
    if ($check.Width -ne 60 -or $check.Height -ne 60) {
        throw "Model output is not 60x60: $ModelOut"
    }
} finally {
    $check.Dispose()
}
