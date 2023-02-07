$cwd = Get-Location

try {
	$SrcDir = Join-Path "$PSScriptRoot" "libraries\liblmdb\"

	Set-Location "$SrcDir"

	# prepare
	cmake -DBUILD_SHARED_LIBS=ON -DLMDB_BUILD_TESTS=ON -B build

	# build targets
	cmake --build build --config Release
} finally {
	Set-Location "$cwd"
}
