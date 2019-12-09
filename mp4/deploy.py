import os
import sys
import subprocess

for i in range(1, 7):
    server = "%s@fa19-cs425-g44-%02d.cs.illinois.edu" % (sys.argv[1],i)
    #os.system('ssh -t %s "sudo mkdir /home/deploy/mp2; sudo chgrp csvm425-cls /home/deploy/mp2; sudo chmod -R g+rw /home/deploy/mp2; sudo pkill -f \"node\""' %(server))
    os.system('ssh -t %s "sudo mkdir /home/deploy/mp4;sudo chgrp csvm425-cls /home/deploy/mp4; sudo chmod -R g+rw /home/deploy/mp4; sudo pkill -f \"node\""' %(server))

    os.system('scp node.cpp test.cpp introducer.cpp maple_juice.h rm_init.sh%s:/home/deploy/mp4' %(server))
    os.system('scp serverss.txt %s:/home/deploy/mp4/servers.txt' %(server))
    os.system('ssh %s "cd /home/deploy/mp4; ./run.sh"' %(server))
    # subprocess.Popen(['ssh', server, "sh /home/deploy/mp2/run.sh"]) 
