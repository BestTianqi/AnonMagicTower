[CmdletBinding()]
param(
    [ValidateSet('Parser', 'Single', 'Roster', 'Gate', 'Normalizer', 'Prepare', 'All')]
    [string]$Phase = 'All',
    [string]$Case = 'All',
    [string]$FixtureRoot
)

$ErrorActionPreference = 'Stop'
$script:ToolsRoot = Split-Path -Parent $PSCommandPath
$script:Verifier = Join-Path $script:ToolsRoot 'verify_character_sprites.ps1'
$script:Normalizer = Join-Path $script:ToolsRoot 'normalize_character_sprite.ps1'
$script:RosterPaths = [ordered]@{
    anon_casual = 'images/characters/pilot/anon_casual.png'; anon_stage = 'images/characters/pilot/anon_stage.png'
    soyo_casual = 'images/characters/mygo/soyo_casual.png'; soyo_stage = 'images/characters/mygo/soyo_stage.png'
    tomori_casual = 'images/characters/mygo/tomori_casual.png'; tomori_stage = 'images/characters/mygo/tomori_stage.png'
    uika_casual = 'images/characters/ave_mujica/uika_casual.png'; doloris_stage = 'images/characters/ave_mujica/doloris_stage.png'
    arale_casual = 'images/characters/mugendai_mewtype/arale_casual.png'; arale_stage = 'images/characters/mugendai_mewtype/arale_stage.png'
    viola_normal = 'images/characters/mugendai_mewtype/viola_normal.png'; viola_corrupted = 'images/characters/mugendai_mewtype/viola_corrupted.png'
}

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw "ASSERT: $Message" }
}

function Invoke-Script {
    param([string]$ScriptPath, [string[]]$Arguments)
    $output = & powershell -NoProfile -File $ScriptPath @Arguments 2>&1 | Out-String
    return [pscustomobject]@{ ExitCode = $LASTEXITCODE; Output = $output.Trim() }
}

function Assert-Result {
    param($Result, [int]$ExitCode, [string]$Contains)
    Assert-True ($Result.ExitCode -eq $ExitCode) "expected exit $ExitCode, got $($Result.ExitCode): $($Result.Output)"
    Assert-True ($Result.Output -like "*$Contains*") "expected output containing '$Contains', got: $($Result.Output)"
}

function New-TestBitmap {
    param(
        [string]$Path,
        [int]$Width = 240,
        [int]$Height = 240,
        [ValidateSet('Argb', 'Rgb')][string]$PixelFormat = 'Argb',
        [scriptblock]$Mutate
    )
    Add-Type -AssemblyName System.Drawing
    $format = if ($PixelFormat -eq 'Argb') {
        [Drawing.Imaging.PixelFormat]::Format32bppArgb
    } else {
        [Drawing.Imaging.PixelFormat]::Format24bppRgb
    }
    $bitmap = [Drawing.Bitmap]::new($Width, $Height, $format)
    try {
        if ($PixelFormat -eq 'Argb' -and $Width -ge 240 -and $Height -ge 240) {
            for ($row = 0; $row -lt 4; $row++) {
                for ($column = 0; $column -lt 4; $column++) {
                    $bitmap.SetPixel(($column * 60) + 30, ($row * 60) + 30, [Drawing.Color]::FromArgb(255, 20, 30, 40))
                }
            }
        }
        if ($Mutate) { & $Mutate $bitmap }
        $parent = Split-Path -Parent $Path
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
        $bitmap.Save($Path, [Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $bitmap.Dispose()
    }
}

function New-Manifest {
    param([string]$Path, [object[]]$AllowedTranslucency = @())
    $manifest = [ordered]@{
        manifest_version = '1.0.0'
        style_guide_version = '1.0.0'
        forms = [ordered]@{
            anon_casual = [ordered]@{
                relative_path = 'images/characters/pilot/anon_casual.png'
                allowed_translucency = $AllowedTranslucency
            }
        }
    }
    $manifest | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $Path -Encoding UTF8
}

function New-RosterManifest {
    param([string]$Path, [string]$Version = '1.0.0')
    $forms = [ordered]@{}
    foreach ($entry in $script:RosterPaths.GetEnumerator()) {
        $forms[$entry.Key] = [ordered]@{ relative_path = $entry.Value; allowed_translucency = @() }
    }
    [ordered]@{ manifest_version='1.0.0'; style_guide_version=$Version; forms=$forms } |
        ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Add-RosterImages {
    param([string]$Root, [string[]]$Ids)
    foreach ($id in $Ids) { New-TestBitmap (Join-Path $Root $script:RosterPaths[$id].Substring('images/characters/'.Length)) }
}

function Write-Approval {
    param([string]$Path, [string]$Status, [string]$CasualHash, [string]$StageHash, [string]$Version = '1.0.0')
    [ordered]@{ status=$Status; casual_sha256=$CasualHash; stage_sha256=$StageHash; style_guide_version=$Version; reviewed_at=$null; accepted_exceptions=@() } |
        ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $Path -Encoding UTF8
}

function New-GateFixture {
    param([string]$Fixture, [string]$Status = 'approved', [switch]$WrongHash, [string]$StyleVersion='1.0.0', [string]$ApprovalVersion='1.0.0', [switch]$WithBatch)
    $root=Join-Path $Fixture 'images/characters'; $manifest=Join-Path $Fixture 'manifest.json'; $approval=Join-Path $Fixture 'approval.json'; $style=Join-Path $Fixture 'style.md'
    New-RosterManifest $manifest
    Add-RosterImages $root @('anon_casual','anon_stage')
    if ($WithBatch) { Add-RosterImages $root @('soyo_casual') }
    $casual=(Get-FileHash -Algorithm SHA256 (Join-Path $root $script:RosterPaths.anon_casual.Substring('images/characters/'.Length))).Hash.ToLowerInvariant()
    $stage=(Get-FileHash -Algorithm SHA256 (Join-Path $root $script:RosterPaths.anon_stage.Substring('images/characters/'.Length))).Hash.ToLowerInvariant()
    if ($WrongHash) { $casual=('0' * 64) }
    Write-Approval $approval $Status $casual $stage $ApprovalVersion
    Set-Content -LiteralPath $style -Encoding UTF8 -Value @('---',("style_guide_version: `"{0}`"" -f $StyleVersion),'---','# Test')
    return [pscustomobject]@{Root=$root;Manifest=$manifest;Approval=$approval;Style=$style}
}

function Use-TempFixture {
    param([scriptblock]$Body)
    $directory = Join-Path ([IO.Path]::GetTempPath()) ("sprite-fixture-{0}" -f [guid]::NewGuid())
    New-Item -ItemType Directory -Path $directory | Out-Null
    try { & $Body $directory } finally { Remove-Item -LiteralPath $directory -Recurse -Force -ErrorAction SilentlyContinue }
}

if ($Phase -eq 'Prepare') {
    if ([string]::IsNullOrWhiteSpace($FixtureRoot) -or $Case -notin @('Pilot','Full')) { throw 'Prepare requires -FixtureRoot and -Case Pilot|Full' }
    New-Item -ItemType Directory -Path $FixtureRoot -Force | Out-Null
    $preparedRoot=Join-Path $FixtureRoot 'images/characters'; $preparedManifest=Join-Path $FixtureRoot 'manifest.json'
    New-RosterManifest $preparedManifest
    $ids=if($Case -eq 'Pilot'){@('anon_casual','anon_stage')}else{@($script:RosterPaths.Keys)}
    Add-RosterImages $preparedRoot $ids
    $casual=(Get-FileHash -Algorithm SHA256 (Join-Path $preparedRoot 'pilot/anon_casual.png')).Hash.ToLowerInvariant()
    $stage=(Get-FileHash -Algorithm SHA256 (Join-Path $preparedRoot 'pilot/anon_stage.png')).Hash.ToLowerInvariant()
    Write-Approval (Join-Path $FixtureRoot 'approval.json') approved $casual $stage
    Set-Content -LiteralPath (Join-Path $FixtureRoot 'style.md') -Encoding UTF8 -Value @('---','style_guide_version: "1.0.0"','---','# Fixture')
    Write-Output "PASS: prepared $Case fixture"
    exit 0
}

if (-not (Test-Path -LiteralPath $script:Verifier -PathType Leaf)) {
    throw 'RED: verifier missing'
}

if ($Phase -in @('Parser', 'All')) {
    $badMode = Invoke-Script $script:Verifier @('-Mode', 'Bogus', '-Root', '.', '-Manifest', 'missing.json')
    Assert-Result $badMode 2 'ERROR: invalid mode'

    $singleArgs = Invoke-Script $script:Verifier @('-Mode', 'Single', '-Root', '.', '-Manifest', 'missing.json')
    Assert-Result $singleArgs 2 'ERROR: Single mode requires -File and -FormId'

    $badManifest = Join-Path ([IO.Path]::GetTempPath()) ("sprite-bad-manifest-{0}.json" -f [guid]::NewGuid())
    try {
        Set-Content -LiteralPath $badManifest -Value '{bad json' -Encoding UTF8
        $badMetadata = Invoke-Script $script:Verifier @('-Mode', 'Pilot', '-Root', '.', '-Manifest', $badManifest)
        Assert-Result $badMetadata 2 'ERROR: invalid manifest metadata'
    } finally {
        Remove-Item -LiteralPath $badManifest -Force -ErrorAction SilentlyContinue
    }
}

if ($Phase -in @('Single', 'All')) {
    if ($Case -in @('Basic', 'All')) {
        Use-TempFixture {
            param($fixture)
            $root = Join-Path $fixture 'images/characters'
            $image = Join-Path $root 'pilot/anon_casual.png'
            $manifest = Join-Path $fixture 'manifest.json'
            New-TestBitmap $image
            New-Manifest $manifest
            $result = Invoke-Script $script:Verifier @('-Mode','Single','-Root',$root,'-Manifest',$manifest,'-File',$image,'-FormId','anon_casual')
            Assert-Result $result 0 'PASS: anon_casual, 16 frames'
        }
    }
    if ($Case -in @('Decode', 'All')) {
        Use-TempFixture {
            param($fixture)
            $root = Join-Path $fixture 'images/characters'
            $image = Join-Path $root 'pilot/anon_casual.png'
            $manifest = Join-Path $fixture 'manifest.json'
            New-Item -ItemType Directory -Path (Split-Path -Parent $image) -Force | Out-Null
            [IO.File]::WriteAllBytes($image, [byte[]](137,80,78,71,13,10,26,10,0,0,0))
            New-Manifest $manifest
            $result = Invoke-Script $script:Verifier @('-Mode','Single','-Root',$root,'-Manifest',$manifest,'-File',$image,'-FormId','anon_casual')
            Assert-Result $result 1 'anon_casual.png: DECODE failed'
        }
    }
    if ($Case -in @('Dimensions', 'All')) {
        Use-TempFixture {
            param($fixture)
            $root = Join-Path $fixture 'images/characters'; $image = Join-Path $root 'pilot/anon_casual.png'; $manifest = Join-Path $fixture 'manifest.json'
            New-TestBitmap $image -Width 239 -Height 240
            New-Manifest $manifest
            $result = Invoke-Script $script:Verifier @('-Mode','Single','-Root',$root,'-Manifest',$manifest,'-File',$image,'-FormId','anon_casual')
            Assert-Result $result 1 'anon_casual.png: DIMENSIONS expected 240x240 (cell n/a)'
        }
    }
    if ($Case -in @('Encoding', 'All')) {
        Use-TempFixture {
            param($fixture)
            $root = Join-Path $fixture 'images/characters'; $image = Join-Path $root 'pilot/anon_casual.png'; $manifest = Join-Path $fixture 'manifest.json'
            New-TestBitmap $image -PixelFormat Rgb
            New-Manifest $manifest
            $result = Invoke-Script $script:Verifier @('-Mode','Single','-Root',$root,'-Manifest',$manifest,'-File',$image,'-FormId','anon_casual')
            Assert-Result $result 1 'anon_casual.png: PIXEL_FORMAT expected Format32bppArgb (cell n/a)'
        }
    }
    if ($Case -in @('Guard', 'All')) {
        Use-TempFixture {
            param($fixture)
            $root = Join-Path $fixture 'images/characters'; $image = Join-Path $root 'pilot/anon_casual.png'; $manifest = Join-Path $fixture 'manifest.json'
            New-TestBitmap $image -Mutate { param($b) $b.SetPixel(1, 30, [Drawing.Color]::FromArgb(255, 1, 2, 3)) }
            New-Manifest $manifest
            $result = Invoke-Script $script:Verifier @('-Mode','Single','-Root',$root,'-Manifest',$manifest,'-File',$image,'-FormId','anon_casual')
            Assert-Result $result 1 'anon_casual.png: cell row=0 column=0 GUARD alpha must be 0'
        }
    }
    if ($Case -in @('UndeclaredAlpha', 'All')) {
        Use-TempFixture {
            param($fixture)
            $root = Join-Path $fixture 'images/characters'; $image = Join-Path $root 'pilot/anon_casual.png'; $manifest = Join-Path $fixture 'manifest.json'
            New-TestBitmap $image -Mutate { param($b) $b.SetPixel(10, 10, [Drawing.Color]::FromArgb(128, 1, 2, 3)) }
            New-Manifest $manifest
            $result = Invoke-Script $script:Verifier @('-Mode','Single','-Root',$root,'-Manifest',$manifest,'-File',$image,'-FormId','anon_casual')
            Assert-Result $result 1 'anon_casual.png: cell row=0 column=0 ALPHA 128 is not declared'
        }
    }
    if ($Case -in @('AlphaBounds', 'All')) {
        $allowance = [ordered]@{
            cell = [ordered]@{ row = 0; column = 0 }
            rect = [ordered]@{ x = 10; y = 10; width = 4; height = 1 }
            min_alpha = 100
            max_alpha = 200
        }
        foreach ($alphaCase in @(
            @{ Name = 'below'; Alpha = 99; Exit = 1; Message = 'ALPHA 99 outside declared range 100..200' },
            @{ Name = 'min'; Alpha = 100; Exit = 0; Message = 'PASS: anon_casual, 16 frames' },
            @{ Name = 'max'; Alpha = 200; Exit = 0; Message = 'PASS: anon_casual, 16 frames' },
            @{ Name = 'above'; Alpha = 201; Exit = 1; Message = 'ALPHA 201 outside declared range 100..200' }
        )) {
            Use-TempFixture {
                param($fixture)
                $root = Join-Path $fixture 'images/characters'; $image = Join-Path $root 'pilot/anon_casual.png'; $manifest = Join-Path $fixture 'manifest.json'
                $chosenAlpha = $alphaCase.Alpha
                New-TestBitmap $image -Mutate { param($b) $b.SetPixel(10, 10, [Drawing.Color]::FromArgb($chosenAlpha, 1, 2, 3)) }
                New-Manifest $manifest @($allowance)
                $result = Invoke-Script $script:Verifier @('-Mode','Single','-Root',$root,'-Manifest',$manifest,'-File',$image,'-FormId','anon_casual')
                Assert-Result $result $alphaCase.Exit $alphaCase.Message
            }
        }
    }
    if ($Case -in @('TranslucencyMetadata', 'All')) {
        Use-TempFixture {
            param($fixture)
            $root = Join-Path $fixture 'images/characters'; $image = Join-Path $root 'pilot/anon_casual.png'; $manifest = Join-Path $fixture 'manifest.json'
            New-TestBitmap $image
            $invalidAllowance = [ordered]@{ cell = [ordered]@{row=0;column=0}; rect = [ordered]@{x=1;y=2;width=2;height=2}; min_alpha=1; max_alpha=254 }
            New-Manifest $manifest @($invalidAllowance)
            $result = Invoke-Script $script:Verifier @('-Mode','Single','-Root',$root,'-Manifest',$manifest,'-File',$image,'-FormId','anon_casual')
            Assert-Result $result 2 'ERROR: invalid allowed_translucency for anon_casual'
        }
    }
    if ($Case -in @('ExactPath', 'All')) {
        Use-TempFixture {
            param($fixture)
            $root = Join-Path $fixture 'images/characters'; $image = Join-Path $root 'wrong/anon_casual.png'; $manifest = Join-Path $fixture 'manifest.json'
            New-TestBitmap $image
            New-Manifest $manifest
            $result = Invoke-Script $script:Verifier @('-Mode','Single','-Root',$root,'-Manifest',$manifest,'-File',$image,'-FormId','anon_casual')
            Assert-Result $result 1 'anon_casual.png: PATH expected images/characters/pilot/anon_casual.png, got images/characters/wrong/anon_casual.png (cell n/a)'
        }
    }
}

if ($Phase -in @('Roster', 'All')) {
    if ($Case -in @('PilotValid', 'All')) {
        Use-TempFixture {
            param($fixture)
            $root=Join-Path $fixture 'images/characters'; $manifest=Join-Path $fixture 'manifest.json'; New-RosterManifest $manifest
            Add-RosterImages $root @('anon_casual','anon_stage')
            $result=Invoke-Script $script:Verifier @('-Mode','Pilot','-Root',$root,'-Manifest',$manifest)
            Assert-Result $result 0 'PASS: 2 sheets, 32 frames'
        }
    }
    if ($Case -in @('PilotUnexpected', 'All')) {
        Use-TempFixture {
            param($fixture)
            $root=Join-Path $fixture 'images/characters'; $manifest=Join-Path $fixture 'manifest.json'; New-RosterManifest $manifest
            Add-RosterImages $root @('anon_casual','anon_stage','soyo_casual')
            $result=Invoke-Script $script:Verifier @('-Mode','Pilot','-Root',$root,'-Manifest',$manifest)
            Assert-Result $result 1 'ROSTER unexpected images/characters/mygo/soyo_casual.png'
        }
    }
    if ($Case -in @('FullMissing', 'All')) {
        Use-TempFixture {
            param($fixture)
            $root=Join-Path $fixture 'images/characters'; $manifest=Join-Path $fixture 'manifest.json'; New-RosterManifest $manifest
            Add-RosterImages $root @($script:RosterPaths.Keys | Where-Object { $_ -ne 'viola_corrupted' })
            $result=Invoke-Script $script:Verifier @('-Mode','Full','-Root',$root,'-Manifest',$manifest)
            Assert-Result $result 1 'ROSTER missing images/characters/mugendai_mewtype/viola_corrupted.png'
        }
    }
    if ($Case -in @('FullValid', 'All')) {
        Use-TempFixture {
            param($fixture)
            $root=Join-Path $fixture 'images/characters'; $manifest=Join-Path $fixture 'manifest.json'; New-RosterManifest $manifest
            Add-RosterImages $root @($script:RosterPaths.Keys)
            $result=Invoke-Script $script:Verifier @('-Mode','Full','-Root',$root,'-Manifest',$manifest)
            Assert-Result $result 0 'PASS: 12 sheets, 192 frames'
        }
    }
    if ($Case -in @('ManifestRoster', 'All')) {
        Use-TempFixture {
            param($fixture)
            $root=Join-Path $fixture 'images/characters'; $manifest=Join-Path $fixture 'manifest.json'; New-RosterManifest $manifest
            $data=Get-Content -Raw $manifest | ConvertFrom-Json
            $data.forms | Add-Member -NotePropertyName extra_form -NotePropertyValue ([pscustomobject]@{relative_path='images/characters/extra.png';allowed_translucency=@()})
            $data | ConvertTo-Json -Depth 8 | Set-Content $manifest -Encoding UTF8
            $result=Invoke-Script $script:Verifier @('-Mode','Full','-Root',$root,'-Manifest',$manifest)
            Assert-Result $result 2 'ERROR: manifest forms must contain exactly the 12 approved IDs'
        }
    }
}

if ($Phase -in @('Gate', 'All')) {
    if ($Case -in @('Approved', 'All')) {
        Use-TempFixture {
            param($fixture)
            $gate=New-GateFixture $fixture
            $result=Invoke-Script $script:Verifier @('-Mode','Gate','-Root',$gate.Root,'-Manifest',$gate.Manifest,'-Approval',$gate.Approval,'-StyleGuide',$gate.Style)
            Assert-Result $result 0 'ALLOW: pilot approved'
        }
    }
    if ($Case -in @('Pending', 'All')) {
        Use-TempFixture {
            param($fixture)
            $gate=New-GateFixture $fixture -Status pending
            $result=Invoke-Script $script:Verifier @('-Mode','Gate','-Root',$gate.Root,'-Manifest',$gate.Manifest,'-Approval',$gate.Approval,'-StyleGuide',$gate.Style)
            Assert-Result $result 3 'BLOCK: pilot approval pending'
        }
    }
    if ($Case -in @('HashMismatch', 'All')) {
        Use-TempFixture {
            param($fixture)
            $gate=New-GateFixture $fixture -WrongHash -WithBatch
            $result=Invoke-Script $script:Verifier @('-Mode','Gate','-Root',$gate.Root,'-Manifest',$gate.Manifest,'-Approval',$gate.Approval,'-StyleGuide',$gate.Style)
            Assert-Result $result 3 'BLOCK: pilot hash mismatch'
            $approvalData=Get-Content -Raw $gate.Approval | ConvertFrom-Json
            Assert-True ($approvalData.status -eq 'pending' -and $null -eq $approvalData.reviewed_at -and @($approvalData.accepted_exceptions).Count -eq 0) 'hash mismatch must reset approval atomically'
            Assert-True (-not (Test-Path (Join-Path $gate.Root 'mygo/soyo_casual.png'))) 'batch file must leave delivery path'
            Assert-True (@(Get-ChildItem (Join-Path $gate.Root '.quarantine') -Filter soyo_casual.png -Recurse).Count -eq 1) 'batch file must exist in one quarantine transaction'
        }
    }
    if ($Case -in @('VersionMismatch', 'All')) {
        foreach ($variant in @('style','approval','manifest')) {
            Use-TempFixture {
                param($fixture)
                if ($variant -eq 'style') { $gate=New-GateFixture $fixture -StyleVersion '2.0.0' -WithBatch }
                elseif ($variant -eq 'approval') { $gate=New-GateFixture $fixture -ApprovalVersion '2.0.0' -WithBatch }
                else { $gate=New-GateFixture $fixture -WithBatch; New-RosterManifest $gate.Manifest '2.0.0' }
                $result=Invoke-Script $script:Verifier @('-Mode','Gate','-Root',$gate.Root,'-Manifest',$gate.Manifest,'-Approval',$gate.Approval,'-StyleGuide',$gate.Style)
                Assert-Result $result 3 'BLOCK: style-guide version mismatch'
            }
        }
    }
    if ($Case -in @('Recovery', 'All')) {
        Use-TempFixture {
            param($fixture)
            $gate=New-GateFixture $fixture -WrongHash -WithBatch
            $old=$env:SPRITE_GATE_INTERRUPT_AFTER_MARKER
            try {
                $env:SPRITE_GATE_INTERRUPT_AFTER_MARKER='1'
                $first=Invoke-Script $script:Verifier @('-Mode','Gate','-Root',$gate.Root,'-Manifest',$gate.Manifest,'-Approval',$gate.Approval,'-StyleGuide',$gate.Style)
            } finally { $env:SPRITE_GATE_INTERRUPT_AFTER_MARKER=$old }
            Assert-Result $first 3 'BLOCK: invalidation interrupted after marker'
            Assert-True (Test-Path (Join-Path $gate.Root '.gate-invalidation.json')) 'interruption marker must remain'
            $second=Invoke-Script $script:Verifier @('-Mode','Gate','-Root',$gate.Root,'-Manifest',$gate.Manifest,'-Approval',$gate.Approval,'-StyleGuide',$gate.Style)
            Assert-Result $second 3 'BLOCK: pilot hash mismatch'
            Assert-True (-not (Test-Path (Join-Path $gate.Root '.gate-invalidation.json'))) 'recovery must remove marker'
            $third=Invoke-Script $script:Verifier @('-Mode','Gate','-Root',$gate.Root,'-Manifest',$gate.Manifest,'-Approval',$gate.Approval,'-StyleGuide',$gate.Style)
            Assert-Result $third 3 'BLOCK: pilot approval pending'
            Assert-True (@(Get-ChildItem (Join-Path $gate.Root '.quarantine') -Filter '*.png' -Recurse).Count -eq 1) 'retries must not duplicate quarantine files'
        }
    }
}

if ($Phase -in @('Normalizer', 'All')) {
    Assert-True (Test-Path -LiteralPath $script:Normalizer -PathType Leaf) 'RED: normalizer missing'
    if ($Case -in @('Threshold', 'All')) {
        Use-TempFixture {
            param($fixture)
            $input=Join-Path $fixture 'source.png'; $output=Join-Path $fixture 'normalized.png'
            New-TestBitmap $input -Width 2 -Height 1 -Mutate {
                param($b)
                $b.SetPixel(0,0,[Drawing.Color]::FromArgb(127,10,20,30))
                $b.SetPixel(1,0,[Drawing.Color]::FromArgb(128,40,50,60))
            }
            $result=Invoke-Script $script:Normalizer @('-InputPath',$input,'-OutputPath',$output)
            Assert-Result $result 0 'PASS: normalized 240x240 binary-alpha sprite'
            $bitmap=[Drawing.Bitmap]::new($output)
            try {
                Assert-True ($bitmap.Width -eq 240 -and $bitmap.Height -eq 240) 'normalizer must resize to 240x240'
                Assert-True ($bitmap.PixelFormat -eq [Drawing.Imaging.PixelFormat]::Format32bppArgb) 'normalizer must save Format32bppArgb'
                Assert-True ($bitmap.GetPixel(0,0).A -eq 0) 'alpha 127 must map to 0'
                Assert-True ($bitmap.GetPixel(119,0).A -eq 0) 'nearest-neighbor left half must retain source pixel'
                Assert-True ($bitmap.GetPixel(120,0).A -eq 255) 'alpha 128 must map to 255'
                Assert-True ($bitmap.GetPixel(239,239).A -eq 255) 'nearest-neighbor right half must retain source pixel'
            } finally { $bitmap.Dispose() }
            $firstHash=(Get-FileHash -Algorithm SHA256 $output).Hash
            $again=Invoke-Script $script:Normalizer @('-InputPath',$output,'-OutputPath',$output)
            Assert-Result $again 0 'PASS: normalized 240x240 binary-alpha sprite'
            $secondHash=(Get-FileHash -Algorithm SHA256 $output).Hash
            Assert-True ($firstHash -ceq $secondHash) 'normalization must be byte-idempotent'
        }
    }
    if ($Case -in @('Opaque', 'All')) {
        Use-TempFixture {
            param($fixture)
            $input=Join-Path $fixture 'opaque.png'; $output=Join-Path $fixture 'normalized.png'
            Add-Type -AssemblyName System.Drawing
            $b=[Drawing.Bitmap]::new(2,2,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
            try { $g=[Drawing.Graphics]::FromImage($b); try{$g.Clear([Drawing.Color]::Red)}finally{$g.Dispose()}; $b.Save($input,[Drawing.Imaging.ImageFormat]::Png) } finally {$b.Dispose()}
            $result=Invoke-Script $script:Normalizer @('-InputPath',$input,'-OutputPath',$output)
            Assert-Result $result 1 'ERROR: source has no transparency; background removal requires image editing'
            Assert-True (-not (Test-Path $output)) 'opaque rejection must not write output'
        }
    }
}

Write-Output 'PASS: verifier fixtures'
