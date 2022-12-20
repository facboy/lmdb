#!/bin/bash

rm -rf testdb && mkdir testdb
./mtest3 && ./mdb_stat testdb
