# ppm2png.ps1 -- convert a binary PPM (P6) file to PNG using only .NET (no third-party libs).
#
# Usage (Windows PowerShell 5.1, which is what this machine has):
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\ppm2png.ps1 -In output\gradient.ppm -Out output\gradient.png
#
# NOTE 1: this script is intentionally ASCII-only. Windows PowerShell 5.1 reads a
#         BOM-less .ps1 file as ANSI, so non-ASCII text here is a bad idea.
# NOTE 2: System.Drawing is not always auto-loaded in Windows PowerShell 5.1,
#         hence the explicit Add-Type below.
# NOTE 3: New-Object is written as -TypeName / -ArgumentList on purpose; the
#         "New-Object Type($a, $b)" shorthand is ambiguous and is a common failure.

param(
    [Parameter(Mandatory)][string]$In,
    [Parameter(Mandatory)][string]$Out
)

Add-Type -AssemblyName System.Drawing

function Read-Token {
    param([System.IO.BinaryReader]$Reader)

    $sb = New-Object System.Text.StringBuilder
    while ($true) {
        $b = $Reader.ReadByte()
        if ($b -lt 0) { throw "unexpected end of file while reading the header" }

        if ($b -eq 35) {                                  # '#': skip to end of line
            while ($true) {
                $c = $Reader.ReadByte()
                if ($c -eq 10 -or $c -lt 0) { break }
            }
            continue
        }

        if ($b -eq 32 -or $b -eq 9 -or $b -eq 10 -or $b -eq 13) {   # space/tab/LF/CR
            if ($sb.Length -gt 0) { break } else { continue }
        }

        [void]$sb.Append([char]$b)
    }
    return $sb.ToString()
}

$fs = [System.IO.File]::OpenRead($In)
$br = New-Object System.IO.BinaryReader($fs)

try {
    $magic = Read-Token -Reader $br
    if ($magic -ne 'P6') { throw "only P6 is supported; got '$magic'" }

    $w = [int](Read-Token -Reader $br)
    $h = [int](Read-Token -Reader $br)
    $null = Read-Token -Reader $br                        # maxval

    $bmp  = New-Object -TypeName System.Drawing.Bitmap    -ArgumentList $w, $h
    $rect = New-Object -TypeName System.Drawing.Rectangle -ArgumentList 0, 0, $w, $h
    $data = $bmp.LockBits($rect,
                [System.Drawing.Imaging.ImageLockMode]::WriteOnly,
                [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)

    $stride = $data.Stride
    $buf = New-Object byte[] ($stride * $h)

    for ($y = 0; $y -lt $h; $y++) {
        for ($x = 0; $x -lt $w; $x++) {
            $r = $br.ReadByte(); $g = $br.ReadByte(); $b = $br.ReadByte()
            $i = $y * $stride + $x * 3
            $buf[$i]     = [byte]$b                       # GDI+ stores BGR, PPM stores RGB
            $buf[$i + 1] = [byte]$g
            $buf[$i + 2] = [byte]$r
        }
    }

    [System.Runtime.InteropServices.Marshal]::Copy($buf, 0, $data.Scan0, $buf.Length)
    $bmp.UnlockBits($data)
    $bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
}
finally {
    $br.Close()
    $fs.Close()
}

Write-Host ("wrote " + $Out + "  (" + $w + " x " + $h + ")")