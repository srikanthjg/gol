#!/bin/bash

EXECUTABLE=ca
LATTICE=lattice

function usage() {
  echo "Usage: ./test.sh <iterations>";
  exit 1;
}

function build() {
  mpic++ --prefix /usr/local/share/OpenMPI -o $EXECUTABLE $EXECUTABLE.cpp
}

function run() {
  rows=$( wc -l $LATTICE | cut -d' ' -f1 )
  columns=$( wc -L $LATTICE | cut -d' ' -f1 )
  mpirun --prefix /usr/local/share/OpenMPI -np $rows $EXECUTABLE $rows $columns $1 | sort -g
}

function cleanup() {
  rm -f *.o *~ $EXECUTABLE
}

if [ $# -eq 1 ]; then
  build;
  run $1;
  cleanup;
else
  usage;
fi;
