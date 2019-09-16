import string
import random
import time
import pickle
import collections
import os

pattern_frequncy_file = "pattern_frequncy.pkl"
logs_path = "testlogs"

def generate_log(machine, possible_chars, log_types, length_per_line, file_length, start, end, mod_list, pattern_list):
	lines = ""
	pattern_map = collections.defaultdict(int)

	for i in range(0, file_length):
		line = ""

		# get a random number to random generate type
		line_type = log_types[random.randint(0, len(log_types) - 1)]
		line += '[' + line_type + '] '
		pattern_map[line_type] += 1

		# get a random time
		r = random.random()
		stime = time.mktime(time.strptime(start, '%m/%d/%Y %I:%M %p '))
		etime = time.mktime(time.strptime(end, '%m/%d/%Y %I:%M %p '))
		ptime = stime + r * (etime - stime)
		line_time = time.strftime('%m/%d/%Y %I:%M %p ', time.localtime(ptime))
		line += line_time
		
		# get random contents
		line += ''.join(random.choice(possible_chars) for x in range(length_per_line))

		ran_num = random.randint(0, 11)

		# Element 0 - n in mod_list is increasing significantly in mod_list, so the occurrence of (ran_num + i) % mod_list[0] is dicreasing significantly
		# Add pattern "rare", "infrequent", "regular", "frequent" based on the mod, so the line ending with "frequent" will occur frequently and so as other patterns
		if machine % 5 == 0 and i % mod_list[0] == 0:
			line += "==== " + pattern_list[0] + " ===="
			pattern_map[pattern_list[0]] += 1

		if (ran_num + i) % mod_list[1] == 0:
			line += "==== " + pattern_list[1] + " ===="
			pattern_map[pattern_list[1]] += 1

		if (ran_num + i) % mod_list[2] == 0:
			line += "==== " + pattern_list[2] + " ===="
			pattern_map[pattern_list[2]] += 1

		if (ran_num + i) % mod_list[3] == 0:
			line += "==== " + pattern_list[3] + " ===="
			pattern_map[pattern_list[3]] += 1

		lines += line + '\n'

	# if machine % 3 == 0:
	# 	lines += "CS425 IS AWESOME NO." + str(machine)
	# return the generated logs as well as testing ground truth
	return lines, pattern_map


def write_log(log_file_name, lines):
	 f = open(log_file_name, "w+")
	 f.write(lines)
	 f.close()

if __name__ == '__main__':
	# some known patterns
	log_types = ['ERROR', 'WARN', 'INFO','DEBUG']
	mod_list = [199999, 1999, 199, 19]
	pattern_list = ["rare", "infrequent", "regular", "frequent"]
	possible_chars = string.ascii_uppercase + string.digits + string.ascii_lowercase
	length_per_line = 200
	file_length = 999999

	pattern_dicts = {}

	os.mkdir(logs_path)

	for i in range(1, 11):
		log_file_name = logs_path + '/machine.' + str(i).zfill(2) + '.test.log'
		lines, pattern_dict = generate_log(i, possible_chars, log_types, length_per_line, file_length, "1/1/2018 1:30 PM ", "12/31/2019 4:50 AM ", mod_list, pattern_list)
		pattern_dicts[i] = pattern_dict
		write_log(log_file_name, lines)
		# send logs to every machine
		server = "linlyu2@fa19-cs425-g44-" + str(i).zfill(2) + ".cs.illinois.edu"
    	os.system("scp " + log_file_name + " %s:/home/deploy/mp1" % (server))
		print("Logs " + str(i) + " are ditributed")

	# write groundtruth for unit test into pickle file.
	fp = open(pattern_frequncy_file, 'wb')

	pickle.dump(pattern_dicts, fp)

	fp.close()
		