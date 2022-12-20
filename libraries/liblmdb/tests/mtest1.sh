#!/bin/bash

rm -rf testdb && mkdir testdb
./mtest && ./mdb_stat testdb
