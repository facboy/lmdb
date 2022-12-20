#!/bin/bash

rm -rf testdb && mkdir testdb
./mtest4 && ./mdb_stat testdb
