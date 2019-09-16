#An alternative deploy that needs Ctrl+C after each server is done
import os
import sys
import subprocess

#syn_server = "cyu17@fa19-cs425-g44-01.cs.illinois.edu"
for i in range(1, 11):
    server = "%s@fa19-cs425-g44-%02d.cs.illinois.edu" % (sys.argv[1],i)

    os.system('scp run.sh client.cpp server.cpp servers.txt %s:/home/deploy/mp1' %(server))
    os.system('ssh -t %s "sudo chmod -R g+rw /home/deploy/mp1; sudo pkill -f \"server\""' %(server))
    os.system('ssh %s "sh /home/deploy/mp1/run.sh"' %(server))
