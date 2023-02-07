$BuildDir = "build\"
$UsrLocalDir = "c:\msys64\usr\local\"
$BinDir = (Join-Path "$UsrLocalDir" "bin\")
$LibDir = (Join-Path "$UsrLocalDir" "lib\")

cd $PSScriptRoot
robocopy (Join-Path "$BuildDir" "Release\") "$BinDir" lmdb.dll /w:5
robocopy "." (Join-Path "$UsrLocalDir" "include\") lmdb.h /w:5
robocopy "$BuildDir" (Join-Path "$LibDir" "pkgconfig\") lmdb.pc /w:5
cmd /c mklink (Join-Path "$LibDir" "liblmdb.dll.a") (Join-Path "$BinDir" "lmdb.dll")
