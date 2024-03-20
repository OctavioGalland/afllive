#!/bin/bash

if [ "$#" -ne 3 ]
then
    echo "Usage: $0 [codeql query path] [codeql db path] [output filename]"
    exit
fi

codeql_query_path="$1"
db_path="$2"
out_file_path="$3"

codeql query run --database="$db_path" "$codeql_query_path" -o .tmpres.bqrs
codeql bqrs decode --format=csv --output=.tmpres.csv .tmpres.bqrs
python3 derive_constraints.py .tmpres.csv "$out_file_path"
rm .tmpres.bqrs .tmpres.csv
