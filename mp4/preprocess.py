import re
with open('pg42671.txt', 'r') as file:
    data = file.read()
# s = "string. With. Punctuation?\ndewdewdew"
s = re.sub(r'[^\w\s]','',data)
s = re.sub('_','',s)



f = open("test.txt", "w")
f.write(s)
f.close()