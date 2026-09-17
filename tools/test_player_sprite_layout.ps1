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

    # Every 60x60 frame must keep a two-pixel transparent guard band so that
    # a previous frame's feet cannot bleed into the next frame's head.
    for ($row = 0; $row -lt 8; $row++) {
        for ($col = 0; $col -lt 8; $col++) {
            foreach ($offset in 0, 1, 58, 59) {
                for ($i = 0; $i -lt 60; $i++) {
                    $x = if ($offset -lt 2 -or $offset -gt 57) { $col * 60 + $offset } else { $col * 60 + $i }
                    $y = if ($offset -lt 2 -or $offset -gt 57) { $row * 60 + $offset } else { $row * 60 + $i }
                    if ($bitmap.GetPixel($x, $y).A -ne 0) {
                        throw "opaque pixel in frame guard at row=$row col=$col"
                    }
                }
            }
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

'PASS: player sprite layout'
