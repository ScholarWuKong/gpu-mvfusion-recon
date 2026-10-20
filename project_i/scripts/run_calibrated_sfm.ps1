param(
    [Parameter(Mandatory = $true)]
    [string]$outputDirectory,
    [int]$featureCount = 20000,
    [switch]$baseline,
    [string]$dataDirectory = 'data',
    [string]$calibrationFile = 'configs/camera_vase.json'
)

$ErrorActionPreference = 'Stop'
if (Test-Path -LiteralPath $outputDirectory)
{
    throw "Output already exists; choose a new directory: $outputDirectory"
}
$imageFiles = @(Get-ChildItem -LiteralPath $dataDirectory -File -Filter 'image*.jpg' |
    Sort-Object { [int]($_.BaseName -replace '^image', '') })
if ($imageFiles.Count -lt 2)
{
    throw 'At least two image files are required.'
}
$camera = Get-Content -LiteralPath $calibrationFile -Raw | ConvertFrom-Json
if ($camera.model -ne 'PINHOLE' -or @($camera.distortion | Where-Object { $_ -ne 0 }).Count -ne 0)
{
    throw 'This runner requires zero-distortion PINHOLE calibration.'
}
$culture = [System.Globalization.CultureInfo]::InvariantCulture
$intrinsics = (@($camera.fx, $camera.fy, $camera.cx, $camera.cy) |
    ForEach-Object { ([double]$_).ToString('R', $culture) }) -join ','
$runArguments = @('--feature-backend', 'gpu', '--ba', 'gpu', '--intrinsics', $intrinsics,
    '--nfeatures', $featureCount.ToString($culture), '--output', $outputDirectory)
if (-not $baseline)
{
    $runArguments += @('--gpu-feature-grid', '8x6', '--gpu-orb-blur', 'on',
        '--fast-threshold', '10', '--robust-triangulation', 'on')
}
$runArguments += @($imageFiles.FullName)
New-Item -ItemType Directory -Path $outputDirectory | Out-Null
Copy-Item -LiteralPath $calibrationFile -Destination (Join-Path $outputDirectory 'calibration.json')
$startTime = Get-Date
Write-Output ('Running {0} images, {1} features; baseline={2}; output={3}' -f
    $imageFiles.Count, $featureCount, [bool]$baseline, $outputDirectory)
& .\build\RelWithDebInfo\project_i.exe @runArguments *> (Join-Path $outputDirectory 'run.log')
$applicationExitCode = $LASTEXITCODE
$elapsedSeconds = ((Get-Date) - $startTime).TotalSeconds
[ordered]@{
    applicationExitCode = $applicationExitCode
    elapsedSeconds = $elapsedSeconds
    imageCount = $imageFiles.Count
    arguments = $runArguments
    calibration = $camera
    profile = $(if ($baseline) { 'baseline' } else { 'coverage' })
    executableSha256 = (Get-FileHash -LiteralPath 'build/RelWithDebInfo/project_i.exe' -Algorithm SHA256).Hash
} | ConvertTo-Json -Depth 6 |
    Set-Content -LiteralPath (Join-Path $outputDirectory 'run_metadata.json') -Encoding utf8
Get-Content -LiteralPath (Join-Path $outputDirectory 'run.log') |
    Select-String '^Calibrated|^Image 35|^BA:|^Tracks:|^Partial|^Reconstruction|^Error:'
Write-Output ('ApplicationExitCode={0}; ElapsedSeconds={1:N2}' -f $applicationExitCode, $elapsedSeconds)
if ($applicationExitCode -eq 1)
{
    throw 'Reconstruction reported an error. See the saved log.'
}
