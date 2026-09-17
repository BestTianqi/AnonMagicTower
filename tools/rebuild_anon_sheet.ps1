param(
    [Parameter(Mandatory = $true)][string]$Source,
    [Parameter(Mandatory = $true)][string]$Destination
)

Add-Type -AssemblyName System.Drawing

$srcBmp = [System.Drawing.Bitmap]::new($Source)
$dstBmp = [System.Drawing.Bitmap]::new(480, 480, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$gfx = [System.Drawing.Graphics]::FromImage($dstBmp)
$gfx.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
$gfx.Clear([System.Drawing.Color]::Transparent)
$gfx.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
$gfx.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half

try {
    for ($row = 0; $row -lt 8; $row++) {
        for ($col = 0; $col -lt 8; $col++) {
            # Ignore the generated cell's outer pixels, then find the actual
            # character bounds and fit that character into a 52px interior.
            # This gives every frame the same baseline and prevents any source
            # frame from contributing pixels to a neighboring frame.
            $cellLeft = [int][math]::Floor($col * $srcBmp.Width / 8) + 12
            $cellTop = [int][math]::Floor($row * $srcBmp.Height / 8) + 12
            $cellRight = [int][math]::Floor(($col + 1) * $srcBmp.Width / 8) - 12
            $cellBottom = [int][math]::Floor(($row + 1) * $srcBmp.Height / 8) - 12
            $minX = $cellRight
            $minY = $cellBottom
            $maxX = $cellLeft - 1
            $maxY = $cellTop - 1
            for ($y = $cellTop; $y -lt $cellBottom; $y++) {
                for ($x = $cellLeft; $x -lt $cellRight; $x++) {
                    if ($srcBmp.GetPixel($x, $y).A -gt 0) {
                        $minX = [math]::Min($minX, $x)
                        $minY = [math]::Min($minY, $y)
                        $maxX = [math]::Max($maxX, $x)
                        $maxY = [math]::Max($maxY, $y)
                    }
                }
            }
            if ($maxX -lt $minX -or $maxY -lt $minY) { continue }
            $sourceWidth = $maxX - $minX + 1
            $sourceHeight = $maxY - $minY + 1
            $scale = [math]::Min(52.0 / $sourceWidth, 52.0 / $sourceHeight)
            $destWidth = [math]::Max(1, [int][math]::Round($sourceWidth * $scale))
            $destHeight = [math]::Max(1, [int][math]::Round($sourceHeight * $scale))
            $destX = $col * 60 + [int][math]::Round((60 - $destWidth) / 2.0)
            $destY = $row * 60 + 56 - $destHeight
            $sourceRect = [System.Drawing.Rectangle]::new($minX, $minY, $sourceWidth, $sourceHeight)
            $destinationRect = [System.Drawing.Rectangle]::new($destX, $destY, $destWidth, $destHeight)
            $gfx.DrawImage($srcBmp, $destinationRect, $sourceRect.X, $sourceRect.Y,
                $sourceRect.Width, $sourceRect.Height, [System.Drawing.GraphicsUnit]::Pixel)
        }
    }
    # Remove any anti-aliased pixels that landed on a grid edge in the
    # generated source. The first four pixels of every frame are reserved as
    # a hard transparent guard.
    for ($row = 0; $row -lt 8; $row++) {
        for ($col = 0; $col -lt 8; $col++) {
            for ($y = 0; $y -lt 60; $y++) {
                for ($x = 0; $x -lt 60; $x++) {
                    if ($x -lt 4 -or $y -lt 4 -or $x -ge 56 -or $y -ge 56) {
                        $dstBmp.SetPixel($col * 60 + $x, $row * 60 + $y, [System.Drawing.Color]::Transparent)
                    }
                }
            }
        }
    }
}
finally {
    $gfx.Dispose()
    $srcBmp.Dispose()
}

$dstBmp.Save($Destination, [System.Drawing.Imaging.ImageFormat]::Png)
$dstBmp.Dispose()

Add-Type -AssemblyName System.Drawing
$check = [System.Drawing.Bitmap]::new($Destination)
if ($check.Width -ne 480 -or $check.Height -ne 480) {
    $check.Dispose()
    throw 'rebuilt sheet is not 480x480'
}
for ($row = 0; $row -lt 8; $row++) {
    for ($col = 0; $col -lt 8; $col++) {
        for ($i = 0; $i -lt 4; $i++) {
            for ($j = 0; $j -lt 60; $j++) {
                $edgePixels = @(
                    $check.GetPixel($col * 60 + $j, $row * 60 + $i),
                    $check.GetPixel($col * 60 + $j, $row * 60 + 59 - $i),
                    $check.GetPixel($col * 60 + $i, $row * 60 + $j),
                    $check.GetPixel($col * 60 + 59 - $i, $row * 60 + $j)
                )
                if ($edgePixels | Where-Object { $_.A -ne 0 }) {
                    $check.Dispose()
                    throw "frame ($col,$row) touches its 4px transparent guard"
                }
            }
        }
    }
}
$check.Dispose()
Write-Output 'PASS: rebuilt 480x480 sheet with 4px per-frame guards'
