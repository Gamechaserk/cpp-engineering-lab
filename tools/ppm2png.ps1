param([Parameter(Mandatory)][string]$In, [Parameter(Mandatory)][string]$Out)
$fs = [System.IO.File]::OpenRead($In); $br = New-Object System.IO.BinaryReader($fs)
function ReadToken($br) {
    $sb = New-Object System.Text.StringBuilder
    while ($true) {
        $b = $br.ReadByte()
        if ($b -eq 35) { while ($br.ReadByte() -ne 10) {}; continue }        # '#' 注释行
        if ([char]$b -match '\s') { if ($sb.Length) { break } else { continue } }
        [void]$sb.Append([char]$b)
    }
    return $sb.ToString()
}
$magic = ReadToken $br
if ($magic -ne 'P6') { throw "只支持 P6（P3 请先转成 P6，或手动看数值）：读到 $magic" }
$w = [int](ReadToken $br); $h = [int](ReadToken $br); $null = ReadToken $br   # 第三个数是 maxval
$bmp  = New-Object System.Drawing.Bitmap($w, $h)
$rect = New-Object System.Drawing.Rectangle(0, 0, $w, $h)
$data = $bmp.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::WriteOnly,
                      [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
$buf = New-Object byte[] ($data.Stride * $h)
for ($y = 0; $y -lt $h; $y++) {
    for ($x = 0; $x -lt $w; $x++) {
        $r = $br.ReadByte(); $g = $br.ReadByte(); $b = $br.ReadByte()
        $i = $y * $data.Stride + $x * 3
        $buf[$i] = $b; $buf[$i+1] = $g; $buf[$i+2] = $r         # ★ GDI+ 是 BGR 顺序
    }
}
[System.Runtime.InteropServices.Marshal]::Copy($buf, 0, $data.Scan0, $buf.Length)
$bmp.UnlockBits($data); $bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose(); $br.Close(); $fs.Close()
Write-Host "已写出 $Out  ($w x $h)"