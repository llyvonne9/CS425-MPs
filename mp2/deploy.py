import os
import sys
import subprocess

#syn_server = "cyu17@fa19-cs425-g44-01.cs.illinois.edu"
for i in range(3, 11):
    server = "%s@fa19-cs425-g44-%02d.cs.illinois.edu" % (sys.argv[1],i)
    #os.system('ssh -t %s "sudo mkdir /home/deploy/mp2; sudo chgrp csvm425-cls /home/deploy/mp2; sudo chmod -R g+rw /home/deploy/mp2; sudo pkill -f \"node\""' %(server))
    os.system('ssh -t %s "sudo chmod -R g+rw /home/deploy/mp2; sudo pkill -f \"node\""' %(server))

    os.system('scp node.cpp test.cpp introducer.cpp servers.txt run.sh %s:/home/deploy/mp2' %(server))
    #subprocess.Popen(['ssh', server, "sh /home/deploy/mp2/run.sh"]) 
