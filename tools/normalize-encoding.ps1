# normalize-encoding.ps1
# Make every scanned source file "UTF-8 with BOM".
#
# Two repairs, both idempotent (safe to re-run):
#   1) not valid UTF-8  -> decode as GBK (code page 936) -> write UTF-8 with BOM
#   2) valid UTF-8 but missing the BOM -> write it again WITH a BOM
#
# Why the BOM matters here: MSVC defaults to code page 936 on this machine. A
# BOM-less file with non-ASCII text is therefore decoded as GBK, which produces
# warning C4819 and can corrupt multi-byte characters inside string literals.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\normalize-encoding.ps1
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\normalize-encoding.ps1 -Root .\renderer
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\normalize-encoding.ps1 -Extensions .h,.hpp,.cpp,.md
#
# NOTE 1: this script is intentionally ASCII-only. Windows PowerShell 5.1 reads a
#         BOM-less .ps1 file as ANSI, so non-ASCII text here would be a bad idea.
# NOTE 2: inside a catch block, $_ is the ERROR RECORD, not the pipeline object.
#         Always copy $_.FullName into a variable BEFORE the try/catch.

param(
    [string]   $Root       = (Split-Path -Parent $PSScriptRoot),
    [string[]] $Extensions = @('.h', '.hpp', '.cpp')
)

$utf8Strict = New-Object System.Text.UTF8Encoding($false, $true)   # throws on invalid bytes
$utf8Bom    = New-Object System.Text.UTF8Encoding($true)
$gbk        = [System.Text.Encoding]::GetEncoding(936)

$skipDirs   = '\\(build|out|\.vs|\.git|_deps)\\'
$converted  = 0    # GBK -> UTF-8+BOM
$bomAdded   = 0    # already UTF-8, only the BOM was missing
$ok         = 0    # already UTF-8 with BOM

Get-ChildItem -Path $Root -Recurse -File |
    Where-Object { $Extensions -contains $_.Extension -and $_.FullName -notmatch $skipDirs } |
    ForEach-Object {
        $path  = $_.FullName                       # <-- captured before try/catch (see NOTE 2)
        $bytes = [System.IO.File]::ReadAllBytes($path)
        $hasBom = ($bytes.Length -ge 3) -and ($bytes[0] -eq 0xEF) -and ($bytes[1] -eq 0xBB) -and ($bytes[2] -eq 0xBF)

        try {
            $text = $utf8Strict.GetString($bytes)  # valid UTF-8
            if ($hasBom) {
                $ok++
                Write-Host ("[ok]        " + $path)
            }
            else {
                [System.IO.File]::WriteAllText($path, $text, $utf8Bom)
                $bomAdded++
                Write-Host ("[bom added] " + $path)
            }
        }
        catch {
            $text = $gbk.GetString($bytes)         # treat as GBK
            [System.IO.File]::WriteAllText($path, $text, $utf8Bom)
            $converted++
            Write-Host ("[converted] " + $path)
        }
    }

Write-Host ("Done. converted=$converted bom_added=$bomAdded ok=$ok")