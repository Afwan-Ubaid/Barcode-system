$src = "D:\3semester\ISE\PROJECT TO TRANSFER"
$dst = "E:\PROJECT TO TRANSFER"

Write-Host "Copying $src to $dst..."
if (-not (Test-Path "E:\")) {
    Write-Error "Drive E:\ is not available!"
    exit 1
}

robocopy "$src" "$dst" /E /R:2 /W:2 /NP /NFL /NDL

# Robocopy exit codes 0-7 indicate success
if ($LASTEXITCODE -le 7) {
    Write-Host "Robocopy to E:\PROJECT TO TRANSFER succeeded with code $LASTEXITCODE"
} else {
    Write-Error "Robocopy failed with exit code $LASTEXITCODE"
    exit 1
}

# Verify key files
$keyFiles = @("ScanIT.exe", "barcode_gui.exe", "barcode_console.exe", "sfml-graphics-2.dll", "data\Order.txt", "data\Bill.txt", "data\inventory.txt", "README.txt")
foreach ($f in $keyFiles) {
    if (Test-Path "$dst\$f") {
        $item = Get-Item "$dst\$f"
        Write-Host "Verified: $f ($($item.Length) bytes, $($item.LastWriteTime))"
    } else {
        Write-Error "Missing file: $dst\$f"
    }
}
