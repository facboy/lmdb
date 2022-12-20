#!/bin/bash

rm -rf testdb && mkdir testdb
./mtest2 && ./mdb_stat testdb
