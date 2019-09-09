#!/usr/bin/python
import os
import sys
import subprocess

FNULL = open(os.devnull, 'w')
for i in range(1, 11):
    server = "cyu17@fa17-cs425-g44-%02d.cs.illinois.edu" % (i)

    cmd = "chmod 777 /usr/local/mp1/run.sh"
    os.system("scp dgrep.cpp worker.cpp run.sh %s:/usr/local/mp1/" % (server))
    ret = subprocess.Popen(["ssh", server, cmd])

    os.system("ssh %s \"/usr/local/mp1/run.sh >>server.log\"" % (server))
    print "server %d up" % (i)
