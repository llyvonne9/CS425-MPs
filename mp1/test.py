import os
import subprocess
import pickle

pattern_frequncy_file = "pattern_frequncy.pkl"


# Get groundthruth from logs statistics
def getCorrectLines():
	pkl_file = open(pattern_frequncy_file, 'rb')
	pattern_dicts = pickle.load(pkl_file)
	return pattern_dicts

# Run the client and get the results
def getRecievedLines(command):
	msg3 = subprocess.Popen("g++ client.cpp -o client -std=c++0x -pthread", shell=True)
	cmd2 = "nohup ./client -l machine. -m \"" + command + "\" -s servers.txt &"
	msg4 = subprocess.Popen(cmd2, shell=True)

if __name__ == '__main__':
	grep = "grep" 
	mode = '-E'
	pattern = "'^[0-9]*[a-z]{5}'"
	# patterns with ground truth
	patterns = ["rare", "infrequent", "regular", "frequent", "ERROR", "WARN", "DEBUG", "INFO"]
	
	pattern = patterns[1]
	pattern_dicts = getCorrectLines()
	command = ' '.join([grep, pattern])
	getRecievedLines(command)

	for i in range(1, 11):
		result_file = "result_received_" + str(i) + ".txt"
		
		pattern_dict = pattern_dicts[i]

		# count the retrieved lines of result
		with open(result_file) as f:
			size = sum(1 for _ in f)

		print("[Test]: Pattern: ", pattern, " Times:", str(pattern_dict[pattern]))

		# Compare the groundtruth lines with the retrieved ones
		if pattern_dict[pattern] == size:
			print("Result of VM " + str(i + 1) + " pass the unit test")
		else:
			print("Result of VM " + str(i + 1) + " did not pass the unit test because the search result contains " + str(size) + " lines")

