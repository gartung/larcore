#!/bin/csh

if( $#argv < 1) then
    echo "must supply geometry base file name"
    exit 0
endif

set basename = $argv[1];

echo "testing geometry for $basename"

sed s/detector/$basename/ geotest.fcl > ${basename}test.fcl

totalview lar

# When the next-to-last totalview window pops up, 
# enable memory debugging and go to the "arguments" tab.
# type: -c {basename}test.fcl

# For example, -c lbne10kttest.fcl
