# MagrisLauncher.dll (veya stub exe) dosya boyutunu 8 MiB - 9 MiB araligina getirir (PE sonuna sifir padding).
param(
    [Parameter(Mandatory = $true)]
    [string] $ExePath
)

$minB = 8L * 1024L * 1024L
$maxB = 9L * 1024L * 1024L
# Orta hedef: ~8.5 MiB (her derlemede sabit, aralik icinde)
$targetB = [long][Math]::Floor(($minB + $maxB) / 2)

if (-not (Test-Path -LiteralPath $ExePath)) {
    Write-Error "Dosya yok: $ExePath"
    exit 1
}

$item = Get-Item -LiteralPath $ExePath
$cur = $item.Length

if ($cur -ge $minB -and $cur -le $maxB) {
    Write-Host "[pad] Launcher zaten aralikta: $cur bayt"
    exit 0
}

if ($cur -gt $maxB) {
    Write-Warning "[pad] Dosya zaten 9 MiB uzerinde ($cur); kesme yapilmadi."
    exit 0
}

$pad = $targetB - $cur
if ($pad -le 0) {
    exit 0
}

$fs = [System.IO.File]::Open($ExePath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Write)
try {
    $fs.Seek(0, [System.IO.SeekOrigin]::End) | Out-Null
    $chunk = New-Object byte[] 65536
    $written = 0L
    while ($written -lt $pad) {
        $n = [int][Math]::Min($chunk.Length, $pad - $written)
        $fs.Write($chunk, 0, $n)
        $written += $n
    }
}
finally {
    $fs.Dispose()
}

$final = (Get-Item -LiteralPath $ExePath).Length
Write-Host "[pad] Launcher: +$pad bayt -> $final bayt (~$([math]::Round($final/1MB, 2)) MB)"
