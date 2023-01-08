#!/usr/bin/env bash

LMDB_DLL="msys-lmdb-0.dll"

if [[ -z "${PREFIX}" ]]; then
  PREFIX="/usr/local"
fi

make \
  prefix="${PREFIX}" \
  XIPROGS="${LMDB_DLL}" \
  SOEXT=".dll.a" \
  CPPFLAGS="-DMDB_USE_POSIX_SEM=1" \
  LDOUTFLAGS='-Wl,--out-implib,$@ -o '"${LMDB_DLL}" \
  LMDB_DLL="${LMDB_DLL}" \
  "$@"
