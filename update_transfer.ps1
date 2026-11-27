$src = "D:\3semester\ISE\ISE PROJECT"
$dst = "D:\3semester\ISE\PROJECT TO TRANSFER"

Write-Host "Preparing clean transfer directory at $dst..."
if (-not (Test-Path $dst)) {
    New-Item -ItemType Directory -Path $dst -Force | Out-Null
}

# Remove stale loose .txt files in transfer root (except README.txt) so all data lives cleanly in data\
Get-ChildItem -Path $dst -Filter "*.txt" -File | Where-Object { $_.Name -ne "README.txt" } | ForEach-Object {
    Remove-Item $_.FullName -Force
    Write-Host "Removed stale root file: $($_.Name)"
}

# Remove any debug PDBs or debug DLLs if present
Get-ChildItem -Path $dst -Include "*.pdb", "*-d-2.dll" -File | ForEach-Object {
    Remove-Item $_.FullName -Force
    Write-Host "Removed debug file: $($_.Name)"
}

Write-Host "Updating Release binaries in $dst..."
Copy-Item "$src\bin\Release\barcode_gui.exe" "$dst\barcode_gui.exe" -Force
Copy-Item "$src\bin\Release\barcode_gui.exe" "$dst\ScanIT.exe" -Force
Copy-Item "$src\bin\Release\barcode_console.exe" "$dst\barcode_console.exe" -Force

Write-Host "Updating SFML Release DLLs..."
$sfmlDlls = @(
    "openal32.dll",
    "sfml-audio-2.dll",
    "sfml-graphics-2.dll",
    "sfml-network-2.dll",
    "sfml-system-2.dll",
    "sfml-window-2.dll"
)
foreach ($dll in $sfmlDlls) {
    if (Test-Path "$src\bin\Release\$dll") {
        Copy-Item "$src\bin\Release\$dll" "$dst\$dll" -Force
    }
}

Write-Host "Updating VC++ Runtime (x64) DLLs..."
$vcRedistDir = Get-ChildItem "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Redist\MSVC" -Directory -ErrorAction SilentlyContinue |
    Sort-Object Name -Descending |
    ForEach-Object { Join-Path $_.FullName "x64\Microsoft.VC145.CRT" } |
    Where-Object { Test-Path $_ } |
    Select-Object -First 1

if (-not $vcRedistDir) {
    $vcRedistDir = Get-ChildItem "C:\Program Files\Microsoft Visual Studio" -Recurse -Directory -Filter "Microsoft.VC*.CRT" -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match "\\x64\\" -and $_.FullName -notmatch "\\onecore\\" } |
        Sort-Object FullName -Descending |
        Select-Object -ExpandProperty FullName -First 1
}

if ($vcRedistDir) {
    Copy-Item "$vcRedistDir\*.dll" "$dst\" -Force
    Write-Host "Copied VC++ Runtime DLLs from $vcRedistDir"
} else {
    Write-Warning "VC++ Redist CRT directory not found; keeping existing VC++ DLLs."
}

Write-Host "Updating data\ folder..."
New-Item -ItemType Directory -Path "$dst\data" -Force | Out-Null
robocopy "$src\data" "$dst\data" /MIR /R:2 /W:2 /NP /NFL /NDL | Out-Null

Write-Host "Updating assets\ folder..."
New-Item -ItemType Directory -Path "$dst\assets" -Force | Out-Null
robocopy "$src\assets" "$dst\assets" /MIR /R:2 /W:2 /NP /NFL /NDL | Out-Null

if (Test-Path "$src\barcodes") {
    Write-Host "Updating barcodes\ folder..."
    New-Item -ItemType Directory -Path "$dst\barcodes" -Force | Out-Null
    robocopy "$src\barcodes" "$dst\barcodes" /MIR /R:2 /W:2 /NP /NFL /NDL | Out-Null
}

if (Test-Path "$src\ScanIT_Keyboard_Shortcuts.pdf") {
    Copy-Item "$src\ScanIT_Keyboard_Shortcuts.pdf" "$dst\ScanIT_Keyboard_Shortcuts.pdf" -Force
}

Write-Host "Transfer folder updated successfully!"
