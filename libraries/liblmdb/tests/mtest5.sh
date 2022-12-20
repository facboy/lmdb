#!/bin/bash

rm -rf testdb && mkdir testdb
./mtest5 && ./mdb_stat testdb
