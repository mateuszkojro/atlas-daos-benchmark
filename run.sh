#!/bin/bash

set -e 
docker build -t adb:rocky .
docker create --name dummy adb:rocky
docker cp dummy:/app/build/bench/bench /tmp/bench 
docker cp dummy:/app/build/DAOS-benchmark /tmp/DAOS-benchmark 
docker rm dummy
docker cp /tmp/bench daos-client:/
docker cp /tmp/DAOS-benchmark daos-client:/
docker exec -it daos-client ./bench
# docker exec -it daos-client ./DAOS-benchmark 2d3c30a4-feb8-4c63-a8a9-12a52e9e8640
