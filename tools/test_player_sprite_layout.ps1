$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$sheetPath = Join-Path $root 'images/characters/player_outfits/anon_reference_walk_8x8.png'
$mainPath = Join-Path $root 'UI/MainWindow.cpp'
$mapPath = Join-Path $root 'UI/MapWidget.cpp'

if (-not (Test-Path -LiteralPath $sheetPath)) { throw "missing player sheet" }
if (-not (Test-Path -LiteralPath $mainPath)) { throw "missing MainWindow.cpp" }
if (-not (Test-Path -LiteralPath $mapPath)) { throw "missing MapWidget.cpp" }

Add-Type -AssemblyName System.Drawing
$bitmap = [System.Drawing.Bitmap]::new($sheetPath)
try {
    if ($bitmap.Width -ne 480 -or $bitmap.Height -ne 480) {
        throw "player sheet must be 480x480, got $($bitmap.Width)x$($bitmap.Height)"
    }

    # Every 60x60 frame must keep an eight-pixel transparent guard band so that
    # a previous frame's feet cannot bleed into the next frame's head.
    for ($row = 0; $row -lt 8; $row++) {
        for ($col = 0; $col -lt 8; $col++) {
            foreach ($offset in 0, 1, 2, 3, 4, 5, 6, 7, 52, 53, 54, 55, 56, 57, 58, 59) {
                for ($i = 0; $i -lt 60; $i++) {
                    $x = if ($offset -lt 8 -or $offset -gt 51) { $col * 60 + $offset } else { $col * 60 + $i }
                    $y = if ($offset -lt 8 -or $offset -gt 51) { $row * 60 + $offset } else { $row * 60 + $i }
                    if ($bitmap.GetPixel($x, $y).A -ne 0) {
                        throw "opaque pixel in frame guard at row=$row col=$col"
                    }
                }
            }
        }
    }

    # Walking frames must share a stable foot anchor. A large horizontal
    # spread makes the character visibly wobble even when map interpolation
    # itself is smooth.
    foreach ($row in 1, 3, 5, 7) {
        $anchors = @()
        for ($col = 0; $col -lt 8; $col++) {
            $sumX = 0.0
            $count = 0
            for ($y = 44; $y -lt 52; $y++) {
                for ($x = 8; $x -lt 52; $x++) {
                    if ($bitmap.GetPixel($col * 60 + $x, $row * 60 + $y).A -gt 64) {
                        $sumX += $x
                        $count++
                    }
                }
            }
            if ($count -eq 0) { throw "missing foot anchor at row=$row col=$col" }
            $anchors += $sumX / $count
        }
        $spread = ($anchors | Measure-Object -Maximum).Maximum -
                  ($anchors | Measure-Object -Minimum).Minimum
        if ($spread -gt 1.5) {
            throw "walking foot anchors wobble by $([math]::Round($spread, 2))px on row=$row"
        }
    }
}
finally { $bitmap.Dispose() }

$source = Get-Content -Raw -LiteralPath $mainPath
$mapSource = Get-Content -Raw -LiteralPath $mapPath
if ($source -notmatch 'setPlayerOutfit\("reference_walk"\)') {
    throw 'latest reference walk sheet is not the default player outfit'
}
if ($mapSource -notmatch 'frameWidth\s*=\s*sheet\.width\(\)\s*/\s*columns' -or
    $mapSource -notmatch 'frameHeight\s*=\s*sheet\.height\(\)\s*/\s*rows') {
    throw 'sprite slicing must derive frame dimensions from the source sheet'
}
if ($source -match 'loadPlayerOutfitSpriteSheet\("[^"]*_actions"' -or
    $mapSource -notmatch 'endsWith\(QStringLiteral\("_actions"\).*CaseInsensitive') {
    throw 'action-only sheets must not be selectable as walking outfits'
}

'PASS: player sprite layout'
