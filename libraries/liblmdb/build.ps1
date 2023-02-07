# prepare
cmake -DBUILD_SHARED_LIBS=ON -DLMDB_BUILD_TESTS=ON -B build

# build targets
cmake --build build --config Release
