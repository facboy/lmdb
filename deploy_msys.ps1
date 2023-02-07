$cwd = Get-Location

try {
	$SrcDir = Join-Path "$PSScriptRoot" "libraries\liblmdb\"
	$BuildDir = "build\"
	$UsrLocalDir = "c:\msys64\usr\local\"
	$BinDir = Join-Path "$UsrLocalDir" "bin\"
	$LibDir = Join-Path "$UsrLocalDir" "lib\"

	$ReleaseDir = Join-Path "$BuildDir" "Release\"

	Set-Location "$SrcDir"

	# create liblmdb.dll.a
	& "c:\msys64\usr\bin\dlltool.exe" --def ".\lmdb.def" --dllname "lmdb.dll" --output-lib (Join-Path "$ReleaseDir" liblmdb.dll.a)

	robocopy "$ReleaseDir" "$BinDir" lmdb.dll /w:5
	robocopy "$ReleaseDir" "$LibDir" liblmdb.dll.a /w:5
	robocopy "." (Join-Path "$UsrLocalDir" "include\") lmdb.h /w:5
	robocopy "$BuildDir" (Join-Path "$LibDir" "pkgconfig\") lmdb.pc /w:5
} finally {
	Set-Location "$cwd"
}
