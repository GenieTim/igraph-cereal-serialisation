# igraph-cereal-serialisation

A simply repo trying to understand how to serialize an igraph graph using cereal.

## Current State

Things compile! Yay. However, the AddressSanitizer usually throws a `allocation-size-too-big` when deserializing.

## Reproduce

Simply clone this repository, have CMake installed, and run `./bin/compileAndRun.sh`
