param(
    [Parameter(Mandatory = $true)]
    [string]$Source,
    [string]$Destination = (Join-Path $PSScriptRoot "..\data\classic50_map.txt")
)

$ErrorActionPreference = "Stop"
$document = Get-Content -LiteralPath $Source -Raw | ConvertFrom-Json
$records = @($document.target | Where-Object { $_.Level -ge 1 -and $_.Level -le 50 })

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("CLASSIC50_MAP_V1")
$lines.Add("# level type id x y")
foreach ($record in $records) {
    $x = [int]$record.Point.x
    $y = [int]$record.Point.y
    $lines.Add("$($record.Level) $($record.Type) $($record.ID) $x $y")
}

$parent = Split-Path -Parent $Destination
New-Item -ItemType Directory -Force -Path $parent | Out-Null
[System.IO.File]::WriteAllLines($Destination, $lines, [System.Text.UTF8Encoding]::new($false))
Write-Output "Imported $($records.Count) objects to $Destination"
