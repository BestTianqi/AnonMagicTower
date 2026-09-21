param([string]$Verifier = (Join-Path $PSScriptRoot 'verify_item_candidates.ps1'))

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

function New-TestPng([string]$Path, [int]$Width = 60, [int]$Height = 60, [bool]$TouchGuard = $false) {
    $parent = Split-Path -Parent $Path
    New-Item -ItemType Directory -Force -Path $parent | Out-Null
    $bmp = [Drawing.Bitmap]::new($Width, $Height, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        $g = [Drawing.Graphics]::FromImage($bmp)
        try {
            $g.Clear([Drawing.Color]::Transparent)
            $x = if ($TouchGuard) { 0 } else { 8 }
            $g.FillEllipse([Drawing.Brushes]::Red, $x, 8, 40, 40)
        } finally { $g.Dispose() }
        $bmp.Save($Path, [Drawing.Imaging.ImageFormat]::Png)
    } finally { $bmp.Dispose() }
}

function Invoke-Verify([string[]]$Arguments) {
    & powershell -NoProfile -ExecutionPolicy Bypass -File $Verifier @Arguments | Out-Null
    return $LASTEXITCODE
}

function Assert-Exit([int]$Expected, [int]$Actual, [string]$Case) {
    if ($Expected -ne $Actual) { throw "$Case expected exit $Expected, got $Actual" }
}

$tmp = Join-Path ([IO.Path]::GetTempPath()) ('mota-item-candidates-' + [guid]::NewGuid().ToString('N'))
try {
    $source = Join-Path $tmp 'source'
    $candidate = Join-Path $tmp 'candidate'
    New-TestPng (Join-Path $source 'a.png')
    New-TestPng (Join-Path $source 'mygo/b.png')
    New-TestPng (Join-Path $candidate 'a.png')
    New-TestPng (Join-Path $candidate 'mygo/b.png')
    $manifest = Join-Path $tmp 'manifest.json'
    '{"version":1,"status":"pending","source_root":"source","candidate_root":"candidate","items":[{"relative_path":"a.png","semantic_label":"A","asset_group":"test","validation_status":"pending","review_status":"pending"},{"relative_path":"mygo/b.png","semantic_label":"B","asset_group":"test","validation_status":"pending","review_status":"pending"}]}' | Set-Content -LiteralPath $manifest -Encoding UTF8
    $qrc = Join-Path $tmp 'resources.qrc'
    '<RCC><qresource prefix="/"><file>images/runtime/items/a.png</file></qresource></RCC>' | Set-Content -LiteralPath $qrc -Encoding UTF8

    Assert-Exit 0 (Invoke-Verify @('-Mode','Validate','-Source',$source,'-Candidate',$candidate,'-Manifest',$manifest,'-Resources',$qrc,'-ExpectedCount','2')) 'valid fixture'

    Remove-Item -LiteralPath (Join-Path $candidate 'mygo/b.png')
    Assert-Exit 1 (Invoke-Verify @('-Mode','Validate','-Source',$source,'-Candidate',$candidate,'-Manifest',$manifest,'-Resources',$qrc,'-ExpectedCount','2')) 'missing candidate'
    New-TestPng (Join-Path $candidate 'mygo/b.png')

    New-TestPng (Join-Path $candidate 'mygo/b.png') 61 60
    Assert-Exit 1 (Invoke-Verify @('-Mode','Validate','-Source',$source,'-Candidate',$candidate,'-Manifest',$manifest,'-Resources',$qrc,'-ExpectedCount','2')) 'bad dimensions'
    New-TestPng (Join-Path $candidate 'mygo/b.png')

    New-TestPng (Join-Path $candidate 'mygo/b.png') 60 60 $true
    Assert-Exit 1 (Invoke-Verify @('-Mode','Validate','-Source',$source,'-Candidate',$candidate,'-Manifest',$manifest,'-Resources',$qrc,'-ExpectedCount','2')) 'guard violation'
    New-TestPng (Join-Path $candidate 'mygo/b.png')

    '<RCC><qresource prefix="/"><file>images/candidates/item_redesign_20260921/items/a.png</file></qresource></RCC>' | Set-Content -LiteralPath $qrc -Encoding UTF8
    Assert-Exit 1 (Invoke-Verify @('-Mode','Validate','-Source',$source,'-Candidate',$candidate,'-Manifest',$manifest,'-Resources',$qrc,'-ExpectedCount','2')) 'candidate qrc reference'
    '<RCC><qresource prefix="/"><file>images/runtime/items/a.png</file></qresource></RCC>' | Set-Content -LiteralPath $qrc -Encoding UTF8

    Assert-Exit 1 (Invoke-Verify @('-Mode','ReplacementGate','-Manifest',$manifest)) 'pending replacement gate'
    $m = Get-Content -Raw $manifest | ConvertFrom-Json
    $m.status = 'approved'
    $m | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $manifest -Encoding UTF8
    Assert-Exit 0 (Invoke-Verify @('-Mode','ReplacementGate','-Manifest',$manifest)) 'approved replacement gate'

    $run = Join-Path $tmp 'run-manifest.json'
    '{"batches":[{"id":"g1","status":"failed","attempts":3}]}' | Set-Content -LiteralPath $run -Encoding UTF8
    Assert-Exit 1 (Invoke-Verify @('-Mode','ComparisonGate','-Manifest',$manifest,'-RunManifest',$run)) 'failed batch gate'

    Write-Output 'PASS: item candidate verifier fixtures'
} finally {
    if (Test-Path -LiteralPath $tmp) { Remove-Item -LiteralPath $tmp -Recurse -Force }
}
