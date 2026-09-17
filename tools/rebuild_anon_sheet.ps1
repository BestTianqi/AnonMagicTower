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
            # character bounds and fit that character into a 44px interior.
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
            $scale = [math]::Min(44.0 / $sourceWidth, 44.0 / $sourceHeight)
            $destWidth = [math]::Max(1, [int][math]::Round($sourceWidth * $scale))
            $destHeight = [math]::Max(1, [int][math]::Round($sourceHeight * $scale))
            $destX = $col * 60 + [int][math]::Round((60 - $destWidth) / 2.0)
            $destY = $row * 60 + 52 - $destHeight
            $sourceRect = [System.Drawing.Rectangle]::new($minX, $minY, $sourceWidth, $sourceHeight)
            $destinationRect = [System.Drawing.Rectangle]::new($destX, $destY, $destWidth, $destHeight)
            $gfx.DrawImage($srcBmp, $destinationRect, $sourceRect.X, $sourceRect.Y,
                $sourceRect.Width, $sourceRect.Height, [System.Drawing.GraphicsUnit]::Pixel)
        }
    }
    # Remove any anti-aliased pixels that landed on a grid edge in the
    # generated source. The first eight pixels of every frame are reserved as
    # a hard transparent guard.
    for ($row = 0; $row -lt 8; $row++) {
        for ($col = 0; $col -lt 8; $col++) {
            for ($y = 0; $y -lt 60; $y++) {
                for ($x = 0; $x -lt 60; $x++) {
                    if ($x -lt 8 -or $y -lt 8 -or $x -ge 52 -or $y -ge 52) {
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

# Normalize the tile anchor after pose extraction. Generated frames can have
# consistent outer bounds while the feet still drift horizontally, which is
# perceived as the whole character shaking during animation.
$alignedBmp = [System.Drawing.Bitmap]::new(480, 480, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
for ($row = 0; $row -lt 8; $row++) {
    for ($col = 0; $col -lt 8; $col++) {
        $sumFootX = 0.0
        $footCount = 0
        $bottomY = -1
        for ($y = 8; $y -lt 52; $y++) {
            for ($x = 8; $x -lt 52; $x++) {
                $pixel = $dstBmp.GetPixel($col * 60 + $x, $row * 60 + $y)
                if ($pixel.A -gt 32) { $bottomY = [math]::Max($bottomY, $y) }
                if ($y -ge 44 -and $pixel.A -gt 64) {
                    $sumFootX += $x
                    $footCount++
                }
            }
        }
        $footX = if ($footCount -gt 0) { $sumFootX / $footCount } else { 30.0 }
        $shiftX = [int][math]::Round(30.0 - $footX)
        $shiftY = if ($bottomY -ge 0) { 51 - $bottomY } else { 0 }
        for ($y = 8; $y -lt 52; $y++) {
            for ($x = 8; $x -lt 52; $x++) {
                $pixel = $dstBmp.GetPixel($col * 60 + $x, $row * 60 + $y)
                if ($pixel.A -eq 0) { continue }
                $targetX = $x + $shiftX
                $targetY = $y + $shiftY
                if ($targetX -ge 8 -and $targetX -lt 52 -and
                    $targetY -ge 8 -and $targetY -lt 52) {
                    $alignedBmp.SetPixel($col * 60 + $targetX, $row * 60 + $targetY, $pixel)
                }
            }
        }
    }
}
$dstBmp.Dispose()
$dstBmp = $alignedBmp

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
        for ($i = 0; $i -lt 8; $i++) {
            for ($j = 0; $j -lt 60; $j++) {
                $edgePixels = @(
                    $check.GetPixel($col * 60 + $j, $row * 60 + $i),
                    $check.GetPixel($col * 60 + $j, $row * 60 + 59 - $i),
                    $check.GetPixel($col * 60 + $i, $row * 60 + $j),
                    $check.GetPixel($col * 60 + 59 - $i, $row * 60 + $j)
                )
                if ($edgePixels | Where-Object { $_.A -ne 0 }) {
                    $check.Dispose()
                    throw "frame ($col,$row) touches its 8px transparent guard"
                }
            }
        }
    }
}
$check.Dispose()
Write-Output 'PASS: rebuilt 480x480 sheet with 8px per-frame guards'
