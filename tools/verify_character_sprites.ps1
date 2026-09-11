[CmdletBinding()]
param(
    [string]$Mode,
    [string]$Root,
    [string]$Manifest,
    [string]$File,
    [string]$FormId,
    [string]$Approval,
    [string]$StyleGuide
)

$ErrorActionPreference = 'Stop'

function Stop-With {
    param([int]$Code, [string]$Message)
    Write-Output $Message
    exit $Code
}

function Get-NormalizedRelativePath {
    param([string]$BasePath, [string]$TargetPath)
    $baseFull = [IO.Path]::GetFullPath($BasePath).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    $targetFull = [IO.Path]::GetFullPath($TargetPath)
    $baseUri = [Uri]::new($baseFull)
    $targetUri = [Uri]::new($targetFull)
    return [Uri]::UnescapeDataString($baseUri.MakeRelativeUri($targetUri).ToString()).Replace('\', '/')
}

function Get-DeliveryRelativePath {
    param([object]$Form, [string]$Id)
    $repoRelative = ([string]$Form.relative_path).Replace('\','/')
    $prefix = 'images/characters/'
    if (-not $repoRelative.StartsWith($prefix, [StringComparison]::Ordinal) -or $repoRelative.Length -le $prefix.Length) {
        Stop-With 2 "ERROR: invalid relative_path for $Id"
    }
    if ($repoRelative -match '(^|/)\.\.?(/|$)') {
        Stop-With 2 "ERROR: path traversal in relative_path for $Id"
    }
    return $repoRelative.Substring($prefix.Length)
}

if ($Mode -notin @('Single', 'Pilot', 'Full', 'Gate')) {
    Stop-With 2 'ERROR: invalid mode; expected Single, Pilot, Full, or Gate'
}
if ([string]::IsNullOrWhiteSpace($Root) -or [string]::IsNullOrWhiteSpace($Manifest)) {
    Stop-With 2 'ERROR: -Root and -Manifest are required'
}
if ($Mode -eq 'Single' -and ([string]::IsNullOrWhiteSpace($File) -or [string]::IsNullOrWhiteSpace($FormId))) {
    Stop-With 2 'ERROR: Single mode requires -File and -FormId'
}
try {
    $manifestData = Get-Content -LiteralPath $Manifest -Raw | ConvertFrom-Json
    if ($null -eq $manifestData.forms) { throw 'forms missing' }
} catch {
    Stop-With 2 'ERROR: invalid manifest metadata'
}
function Assert-DeliveryRoot {
    try {
        if (-not (Test-Path -LiteralPath $Root -PathType Container -ErrorAction Stop)) { throw 'root missing' }
        $script:Root = [IO.Path]::GetFullPath((Resolve-Path -LiteralPath $Root -ErrorAction Stop).Path).TrimEnd('\', '/')
    } catch {
        Stop-With 2 'ERROR: -Root must resolve to an existing directory'
    }
    $rootParts = $Root.Replace('\','/').Split('/')
    if ($rootParts.Count -lt 2 -or $rootParts[-1] -cne 'characters' -or $rootParts[-2] -cne 'images') {
        Stop-With 2 'ERROR: -Root must be the images/characters delivery root'
    }
}

function Test-SpriteFile {
    param([string]$SpriteFile, [string]$SpriteFormId)
    $File = $SpriteFile
    $FormId = $SpriteFormId
    $formProperty = $manifestData.forms.PSObject.Properties[$FormId]
    if ($null -eq $formProperty) { Stop-With 2 "ERROR: unknown form id $FormId" }
    if (-not (Test-Path -LiteralPath $File -PathType Leaf)) { Stop-With 1 "${File}: FILE missing" }
    $actualRelativePath = 'images/characters/' + (Get-NormalizedRelativePath $Root $File)
    $expectedRelativePath = ([string]$formProperty.Value.relative_path).Replace('\', '/')
    [void](Get-DeliveryRelativePath $formProperty.Value $FormId)
    if ($actualRelativePath -cne $expectedRelativePath) {
        Stop-With 1 "$([IO.Path]::GetFileName($File)): PATH expected $expectedRelativePath, got $actualRelativePath (cell n/a)"
    }
    $allowances = @($formProperty.Value.allowed_translucency)
    foreach ($allowance in $allowances) {
        $values = @($allowance.cell.row, $allowance.cell.column, $allowance.rect.x, $allowance.rect.y,
            $allowance.rect.width, $allowance.rect.height, $allowance.min_alpha, $allowance.max_alpha)
        $allIntegers = $true
        foreach ($value in $values) {
            if ($null -eq $value -or [int64]$value -ne [double]$value) { $allIntegers = $false; break }
        }
        $valid = $allIntegers -and
            $allowance.cell.row -ge 0 -and $allowance.cell.row -le 3 -and
            $allowance.cell.column -ge 0 -and $allowance.cell.column -le 3 -and
            $allowance.rect.x -ge 2 -and $allowance.rect.y -ge 2 -and
            $allowance.rect.width -gt 0 -and $allowance.rect.height -gt 0 -and
            ($allowance.rect.x + $allowance.rect.width) -le 58 -and
            ($allowance.rect.y + $allowance.rect.height) -le 58 -and
            $allowance.min_alpha -ge 1 -and $allowance.max_alpha -le 254 -and
            $allowance.min_alpha -le $allowance.max_alpha
        if (-not $valid) { Stop-With 2 "ERROR: invalid allowed_translucency for $FormId" }
    }
    Add-Type -AssemblyName System.Drawing
    try {
        $bitmap = [Drawing.Bitmap]::new((Resolve-Path -LiteralPath $File).Path)
    } catch {
        Stop-With 1 "$([IO.Path]::GetFileName($File)): DECODE failed (cell n/a)"
    }
    if ($bitmap.Width -ne 240 -or $bitmap.Height -ne 240) {
        $bitmap.Dispose()
        Stop-With 1 "$([IO.Path]::GetFileName($File)): DIMENSIONS expected 240x240 (cell n/a)"
    }
    if ($bitmap.PixelFormat -ne [Drawing.Imaging.PixelFormat]::Format32bppArgb) {
        $bitmap.Dispose()
        Stop-With 1 "$([IO.Path]::GetFileName($File)): PIXEL_FORMAT expected Format32bppArgb (cell n/a)"
    }
    for ($row = 0; $row -lt 4; $row++) {
        for ($column = 0; $column -lt 4; $column++) {
            for ($localY = 0; $localY -lt 60; $localY++) {
                for ($localX = 0; $localX -lt 60; $localX++) {
                    if ($localX -ge 2 -and $localX -le 57 -and $localY -ge 2 -and $localY -le 57) { continue }
                    $alpha = $bitmap.GetPixel(($column * 60) + $localX, ($row * 60) + $localY).A
                    if ($alpha -ne 0) {
                        $bitmap.Dispose()
                        Stop-With 1 "$([IO.Path]::GetFileName($File)): cell row=$row column=$column GUARD alpha must be 0"
                    }
                }
            }
        }
    }
    for ($row = 0; $row -lt 4; $row++) {
        for ($column = 0; $column -lt 4; $column++) {
            for ($localY = 2; $localY -le 57; $localY++) {
                for ($localX = 2; $localX -le 57; $localX++) {
                    $alpha = $bitmap.GetPixel(($column * 60) + $localX, ($row * 60) + $localY).A
                    if ($alpha -ne 0 -and $alpha -ne 255) {
                        $covering = @($allowances | Where-Object {
                            $_.cell.row -eq $row -and $_.cell.column -eq $column -and
                            $localX -ge $_.rect.x -and $localX -lt ($_.rect.x + $_.rect.width) -and
                            $localY -ge $_.rect.y -and $localY -lt ($_.rect.y + $_.rect.height)
                        })
                        if ($covering.Count -eq 0) {
                            $bitmap.Dispose()
                            Stop-With 1 "$([IO.Path]::GetFileName($File)): cell row=$row column=$column ALPHA $alpha is not declared"
                        }
                        $accepted = @($covering | Where-Object { $alpha -ge $_.min_alpha -and $alpha -le $_.max_alpha })
                        if ($accepted.Count -eq 0) {
                            $bounds = $covering[0]
                            $bitmap.Dispose()
                            Stop-With 1 "$([IO.Path]::GetFileName($File)): cell row=$row column=$column ALPHA $alpha outside declared range $($bounds.min_alpha)..$($bounds.max_alpha)"
                        }
                    }
                }
            }
        }
    }
    $bitmap.Dispose()
}

if ($Mode -eq 'Single') {
    Assert-DeliveryRoot
    Test-SpriteFile $File $FormId
    Write-Output "PASS: $FormId, 16 frames"
    exit 0
}

if ($Mode -in @('Pilot', 'Full')) {
    $allRosterIds = @('anon_casual','anon_stage','soyo_casual','soyo_stage','tomori_casual','tomori_stage',
        'uika_casual','doloris_stage','arale_casual','arale_stage','viola_normal','viola_corrupted')
    foreach ($id in $allRosterIds) {
        if ($null -eq $manifestData.forms.PSObject.Properties[$id]) {
            Stop-With 2 "ERROR: invalid manifest metadata; missing form $id"
        }
    }
    if (@($manifestData.forms.PSObject.Properties).Count -ne $allRosterIds.Count) {
        Stop-With 2 'ERROR: manifest forms must contain exactly the 12 approved IDs'
    }
    Assert-DeliveryRoot
    $expectedIds = if ($Mode -eq 'Pilot') { @('anon_casual','anon_stage') } else { $allRosterIds }
    $expectedPaths = @($expectedIds | ForEach-Object { Get-DeliveryRelativePath $manifestData.forms.PSObject.Properties[$_].Value $_ })
    $actualFiles = @(Get-ChildItem -LiteralPath $Root -Filter '*.png' -File -Recurse -ErrorAction SilentlyContinue |
        Where-Object { (Get-NormalizedRelativePath $Root $_.FullName).Split('/') -notcontains '.quarantine' })
    $actualPaths = @($actualFiles | ForEach-Object { Get-NormalizedRelativePath $Root $_.FullName })

    foreach ($expectedPath in $expectedPaths) {
        if ($actualPaths -cnotcontains $expectedPath) { Stop-With 1 "ROSTER missing images/characters/$expectedPath" }
    }
    foreach ($actualPath in $actualPaths) {
        if ($expectedPaths -cnotcontains $actualPath) { Stop-With 1 "ROSTER unexpected images/characters/$actualPath" }
    }
    foreach ($id in $expectedIds) {
        $path = Get-DeliveryRelativePath $manifestData.forms.PSObject.Properties[$id].Value $id
        Test-SpriteFile (Join-Path $Root $path) $id
    }
    if ($Mode -eq 'Pilot') { Write-Output 'PASS: 2 sheets, 32 frames' }
    else { Write-Output 'PASS: 12 sheets, 192 frames' }
    exit 0
}

if ($Mode -eq 'Gate') {
    if ([string]::IsNullOrWhiteSpace($Approval) -or [string]::IsNullOrWhiteSpace($StyleGuide)) {
        Stop-With 2 'ERROR: Gate mode requires -Approval and -StyleGuide'
    }
    try {
        $approvalData = Get-Content -LiteralPath $Approval -Raw | ConvertFrom-Json
        $styleText = Get-Content -LiteralPath $StyleGuide -Raw
        $styleMatch = [regex]::Match($styleText, '\A---\r?\nstyle_guide_version: "([^"]+)"\r?\n---(?:\r?\n|\z)')
        if (-not $styleMatch.Success) { throw 'style front matter missing' }
        $styleVersion = $styleMatch.Groups[1].Value
        if ($approvalData.status -notin @('pending','approved')) { throw 'approval status invalid' }
    } catch {
        Stop-With 2 'ERROR: invalid gate metadata'
    }
    Assert-DeliveryRoot

    $pilotIds = @('anon_casual','anon_stage')
    foreach ($id in $pilotIds) {
        if ($null -eq $manifestData.forms.PSObject.Properties[$id]) { Stop-With 2 "ERROR: invalid manifest metadata; missing form $id" }
    }
    $pilotPaths = @($pilotIds | ForEach-Object { Get-DeliveryRelativePath $manifestData.forms.PSObject.Properties[$_].Value $_ })
    $pilotFiles = @($pilotPaths | ForEach-Object { Join-Path $Root $_ })
    foreach ($pilotFile in $pilotFiles) {
        if (-not (Test-Path -LiteralPath $pilotFile -PathType Leaf)) { Stop-With 3 'BLOCK: pilot sheet missing' }
    }
    $currentCasualHash = (Get-FileHash -LiteralPath $pilotFiles[0] -Algorithm SHA256).Hash.ToLowerInvariant()
    $currentStageHash = (Get-FileHash -LiteralPath $pilotFiles[1] -Algorithm SHA256).Hash.ToLowerInvariant()
    $markerPath = Join-Path $Root '.gate-invalidation.json'

    function Invoke-Invalidation {
        param([string]$Reason)
        if (Test-Path -LiteralPath $markerPath) {
            try { $marker = Get-Content -LiteralPath $markerPath -Raw | ConvertFrom-Json }
            catch { Stop-With 2 'ERROR: invalid gate invalidation marker' }
            if ([string]$marker.transaction -notmatch '^\d{8}T\d{9}Z-[0-9a-f]{8}$') {
                Stop-With 2 'ERROR: invalid gate invalidation transaction'
            }
            $Reason = [string]$marker.reason
        } else {
            $transaction = '{0}-{1}' -f ([DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')), ([guid]::NewGuid().ToString('N').Substring(0,8))
            $marker = [ordered]@{ transaction=$transaction; reason=$Reason }
            $marker | ConvertTo-Json | Set-Content -LiteralPath $markerPath -Encoding UTF8
            if ($env:SPRITE_GATE_INTERRUPT_AFTER_MARKER -eq '1') {
                Stop-With 3 'BLOCK: invalidation interrupted after marker'
            }
        }

        $quarantineRoot = Join-Path $Root ('.quarantine/{0}' -f $marker.transaction)
        $quarantineFull = [IO.Path]::GetFullPath($quarantineRoot)
        if (-not $quarantineFull.StartsWith($Root + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
            Stop-With 2 'ERROR: quarantine path escapes delivery root'
        }
        $deliveryPngs = @(Get-ChildItem -LiteralPath $Root -Filter '*.png' -File -Recurse -ErrorAction Stop | Where-Object {
            $relative = Get-NormalizedRelativePath $Root $_.FullName
            $relative.Split('/') -notcontains '.quarantine' -and $pilotPaths -cnotcontains $relative
        })
        foreach ($png in $deliveryPngs) {
            $relative = Get-NormalizedRelativePath $Root $png.FullName
            $destination = Join-Path $quarantineRoot $relative
            New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
            Move-Item -LiteralPath $png.FullName -Destination $destination
        }

        $reset = [ordered]@{
            status='pending'
            casual_sha256=$currentCasualHash
            stage_sha256=$currentStageHash
            style_guide_version=[string]$manifestData.style_guide_version
            reviewed_at=$null
            accepted_exceptions=@()
        }
        $approvalDirectory = Split-Path -Parent ([IO.Path]::GetFullPath($Approval))
        $approvalTemp = Join-Path $approvalDirectory ('.pilot-approval-{0}.tmp' -f [guid]::NewGuid().ToString('N'))
        try {
            $reset | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $approvalTemp -Encoding UTF8
            Move-Item -LiteralPath $approvalTemp -Destination $Approval -Force
        } finally {
            Remove-Item -LiteralPath $approvalTemp -Force -ErrorAction SilentlyContinue
        }
        Remove-Item -LiteralPath $markerPath -Force
        Stop-With 3 "BLOCK: $Reason"
    }

    if (Test-Path -LiteralPath $markerPath) { Invoke-Invalidation 'invalidation recovery' }
    if ($approvalData.status -ne 'approved') { Stop-With 3 'BLOCK: pilot approval pending' }
    if ([string]$manifestData.style_guide_version -cne [string]$approvalData.style_guide_version -or
        [string]$manifestData.style_guide_version -cne $styleVersion) {
        Invoke-Invalidation 'style-guide version mismatch'
    }
    if ([string]$approvalData.casual_sha256 -cne $currentCasualHash -or
        [string]$approvalData.stage_sha256 -cne $currentStageHash) {
        Invoke-Invalidation 'pilot hash mismatch'
    }
    Test-SpriteFile $pilotFiles[0] 'anon_casual'
    Test-SpriteFile $pilotFiles[1] 'anon_stage'
    Write-Output 'ALLOW: pilot approved'
    exit 0
}

Stop-With 2 'ERROR: mode is not implemented'
