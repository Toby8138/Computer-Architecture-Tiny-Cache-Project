import csv
import random

csvdata = [["Type", "Address", "Data"]]

numCommands = 100000
numAddressBits = 32

for i in range(0,numCommands):
  if random.randint(1,2)==1:
    RW = "W"
    addr = hex(random.getrandbits(numAddressBits))
    data = hex(random.getrandbits(32))
    csvdata.append([RW,addr,data])
  else:
    RW = "R"
    addr = hex(random.getrandbits(numAddressBits))
    csvdata.append([RW,addr,""])

with open("test/random_"+str(numAddressBits)+"bit.csv", "w", newline="") as file:
  writer = csv.writer(file)
  writer.writerows(csvdata)