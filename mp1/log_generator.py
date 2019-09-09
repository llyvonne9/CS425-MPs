import string
import random
import time

def generate_log(machine, possible_chars, log_types, length_per_line, file_length, start, end):
	lines = ""
	for i in range(0, file_length):
		line = ""
		line_type = log_types[random.randint(0, len(log_types) - 1)]
		line += '[' + line_type + '] '
		
		r = random.random()
		stime = time.mktime(time.strptime(start, '%m/%d/%Y %I:%M %p '))
		etime = time.mktime(time.strptime(end, '%m/%d/%Y %I:%M %p '))
		ptime = stime + r * (etime - stime)
		line_time = time.strftime('%m/%d/%Y %I:%M %p ', time.localtime(ptime))
		line += line_time
		
		line += ''.join(random.choice(possible_chars) for x in range(length_per_line))

		lines += line + '\n'

	if machine % 3 == 0:
		lines += "CS425 IS AWESOME NO." + str(machine)

	return lines


def write_log(log_file_name, lines):
	 f = open(log_file_name, "w+")
	 f.write(lines)
	 f.close()

if __name__ == '__main__':
	log_types = ['ERROR', 'WARN', 'INFO','DEBUG']
	possible_chars = string.ascii_uppercase + string.digits
	for i in range(1, 11):
		log_file_name = 'machine.' + str(i).zfill(2) + '.log'
		lines = generate_log(i, possible_chars, log_types, 200, 99999, "1/1/2018 1:30 PM ", "12/31/2019 4:50 AM ")
		write_log(log_file_name, lines)

		