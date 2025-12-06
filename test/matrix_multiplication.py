import csv
import random
import numpy as np

dim1 = 50
dim2 = 20
dim3 = 30
csvdata = [["Type", "Address", "Data"]]

matA = np.random.randint(low=0, high=100, size=(dim1,dim2))
matB = np.random.randint(low=0, high=100, size=(dim2,dim3))
matC = np.random.randint(low=0, high=100, size=(dim1,dim3))

addrA = 2000000
addrB = 4000000
addrC = 6000000
addrAstride = [dim2*4,4]
addrBstride = [dim3*4,4]
addrCstride = [dim3*4,4]

for j in range(dim3):
    for k in range(dim2):
        for i in range(dim1):
            RW = "R"
            addr = hex(addrA+i*addrAstride[0]+k*addrAstride[1])
            csvdata.append([RW,addr,""])
            
            RW = "R"
            addr = hex(addrB+k*addrBstride[0]+j*addrBstride[1])
            csvdata.append([RW,addr,""])

            RW = "R"
            addr = hex(addrC+i*addrCstride[0]+j*addrCstride[1])
            csvdata.append([RW,addr,""])

            matC[i][j] = matC[i][j] + matA[i][k]*matB[k][j]
            
            RW = "W"
            addr = hex(addrC+i*addrCstride[0]+j*addrCstride[1])
            data = hex(matC[i][j]+matA[i][k]*matB[k][j])
            csvdata.append([RW,addr,data])

with open("test/matrix_multiplication_"+str(dim1)+"x"+str(dim2)+"x"+str(dim3)+"_unoptimal.csv", "w", newline="") as file:
    writer = csv.writer(file)
    writer.writerows(csvdata)