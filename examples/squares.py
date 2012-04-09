#! /usr/bin/env python

from random import uniform

f = open("squares.dat", "w")
for i in range(10):
    for j in range(10):
        if uniform(0, 1) < 0.6:
            continue
        f.write("%f %f\n"%(i,j))
        f.write("%f %f\n"%(i+0.9,j))
        f.write("%f %f\n"%(i+0.9,j+0.9))
        f.write("%f %f\n"%(i,j+0.9))
        f.write("%f %f\n"%(i,j))
        f.write("\n")
f.close()
