@echo off
cd /d "%~dp0"
copy barcodes\barcode_100001.bmp barcodes\100001.bmp >nul 2>&1
copy barcodes\barcode_100002.bmp barcodes\100002.bmp >nul 2>&1
copy barcodes\barcode_100003.bmp barcodes\100003.bmp >nul 2>&1
copy barcodes\barcode_100004.bmp barcodes\100004.bmp >nul 2>&1
copy barcodes\barcode_100005.bmp barcodes\100005.bmp >nul 2>&1
copy barcodes\barcode_100006.bmp barcodes\100006.bmp >nul 2>&1
copy barcodes\barcode_100007.bmp barcodes\100007.bmp >nul 2>&1
copy barcodes\barcode_100008.bmp barcodes\100008.bmp >nul 2>&1
copy barcodes\barcode_100009.bmp barcodes\100009.bmp >nul 2>&1
copy barcodes\barcode_234567.bmp barcodes\234567.bmp >nul 2>&1
copy barcodes\barcode_567890.bmp barcodes\567890.bmp >nul 2>&1

if not exist "assets\barcodes" mkdir "assets\barcodes"
copy barcodes\*.bmp assets\barcodes\ >nul 2>&1
echo Barcodes synced successfully.
