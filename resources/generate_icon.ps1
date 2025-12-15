# Generate a simple placeholder icon for Phoenix SDR Controller
# This creates a basic 256-color icon with multiple sizes
# Replace with a proper icon later

Add-Type -AssemblyName System.Drawing

$iconSizes = @(16, 32, 48, 256)
$outputPath = "$PSScriptRoot\phoenix_sdr.ico"

# Create a simple radio wave icon
function Create-IconBitmap($size) {
    $bmp = New-Object System.Drawing.Bitmap($size, $size)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    
    # Background - dark blue
    $bgBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(30, 40, 60))
    $g.FillRectangle($bgBrush, 0, 0, $size, $size)
    
    # Radio waves - cyan/teal
    $wavePen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(0, 200, 200), [Math]::Max(1, $size / 16))
    
    $centerX = $size * 0.3
    $centerY = $size * 0.5
    
    # Draw 3 arc waves
    for ($i = 1; $i -le 3; $i++) {
        $radius = $size * 0.15 * $i
        $rect = New-Object System.Drawing.RectangleF(
            ($centerX - $radius), 
            ($centerY - $radius), 
            ($radius * 2), 
            ($radius * 2)
        )
        $g.DrawArc($wavePen, $rect, -60, 120)
    }
    
    # Antenna dot
    $dotBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 200, 50))
    $dotSize = [Math]::Max(2, $size / 8)
    $g.FillEllipse($dotBrush, ($centerX - $dotSize/2), ($centerY - $dotSize/2), $dotSize, $dotSize)
    
    # "SDR" text for larger sizes
    if ($size -ge 48) {
        $fontSize = [Math]::Max(8, $size / 6)
        $font = New-Object System.Drawing.Font("Consolas", $fontSize, [System.Drawing.FontStyle]::Bold)
        $textBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
        $sf = New-Object System.Drawing.StringFormat
        $sf.Alignment = [System.Drawing.StringAlignment]::Center
        $sf.LineAlignment = [System.Drawing.StringAlignment]::Center
        $textRect = New-Object System.Drawing.RectangleF(($size * 0.4), ($size * 0.3), ($size * 0.6), ($size * 0.5))
        $g.DrawString("SDR", $font, $textBrush, $textRect, $sf)
        $font.Dispose()
        $textBrush.Dispose()
    }
    
    $g.Dispose()
    $bgBrush.Dispose()
    $wavePen.Dispose()
    $dotBrush.Dispose()
    
    return $bmp
}

# Create icon with multiple sizes
$bitmaps = @()
foreach ($size in $iconSizes) {
    $bitmaps += Create-IconBitmap $size
}

# Save as ICO file using the 256x256 bitmap (Windows will scale)
# For a proper multi-resolution icon, we need to write the ICO format manually

# ICO file format
$ms = New-Object System.IO.MemoryStream

# ICONDIR header
$writer = New-Object System.IO.BinaryWriter($ms)
$writer.Write([UInt16]0)  # Reserved
$writer.Write([UInt16]1)  # Type (1 = icon)
$writer.Write([UInt16]$bitmaps.Count)  # Number of images

# Calculate offsets
$headerSize = 6 + (16 * $bitmaps.Count)
$currentOffset = $headerSize

# Prepare image data
$imageData = @()
foreach ($bmp in $bitmaps) {
    $imgMs = New-Object System.IO.MemoryStream
    $bmp.Save($imgMs, [System.Drawing.Imaging.ImageFormat]::Png)
    $imageData += ,($imgMs.ToArray())
    $imgMs.Dispose()
}

# Write ICONDIRENTRY for each image
for ($i = 0; $i -lt $bitmaps.Count; $i++) {
    $size = $iconSizes[$i]
    $data = $imageData[$i]
    
    $writer.Write([byte]$(if ($size -ge 256) { 0 } else { $size }))  # Width
    $writer.Write([byte]$(if ($size -ge 256) { 0 } else { $size }))  # Height
    $writer.Write([byte]0)   # Color palette
    $writer.Write([byte]0)   # Reserved
    $writer.Write([UInt16]1) # Color planes
    $writer.Write([UInt16]32) # Bits per pixel
    $writer.Write([UInt32]$data.Length)  # Size of image data
    $writer.Write([UInt32]$currentOffset)  # Offset to image data
    
    $currentOffset += $data.Length
}

# Write image data
foreach ($data in $imageData) {
    $writer.Write($data)
}

# Save to file
[System.IO.File]::WriteAllBytes($outputPath, $ms.ToArray())

$writer.Dispose()
$ms.Dispose()

foreach ($bmp in $bitmaps) {
    $bmp.Dispose()
}

Write-Host "Icon created: $outputPath"
Write-Host "Sizes: $($iconSizes -join ', ') pixels"
