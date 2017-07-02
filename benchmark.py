#!/usr/bin/env python3

import sys
import subprocess

MIN_LENGTH=4
MAX_LENGTH=8
REPETITIONS=10
STEP=1

M=100

urandom=open("/dev/urandom","rb")

def randint():
        return ord(urandom.read(1))

def random_matrix(filename, size1, size2):
	ll="\n".join([" ".join([str(randint()) for j in range(0, size2)]) for i in range(0, size1)])
	f=open(filename, "w")
	print(n,file=f)
	print(ll,file=f)

for n in range(MIN_LENGTH, MAX_LENGTH+1, STEP):
    time_sum=0
    for i in range(0, REPETITIONS):
        random_matrix("mat1", n, M)
        random_matrix("mat2", M, n)
        time=subprocess.Popen(["./test.sh"], stderr=subprocess.PIPE).communicate()[1].split()
        time_sum+=float(time[1])
    print("{0} {1}".format(n, time_sum/REPETITIONS).replace('.', ','), file=sys.stderr)

