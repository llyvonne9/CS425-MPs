#!/bin/bash
#echo $SERVER
for i in 01 02 03 04 05 06 07 08 09 10
  do
    SERVER="${1}@fa19-cs425-g44-${i}.cs.illinois.edu"
    ssh-copy-id ${SERVER}
  done
