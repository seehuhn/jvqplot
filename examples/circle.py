#! /usr/bin/env python

from __future__ import division

from math import cos, sin, pi

f = open("circle.dat", "w")
n = 100
for i in range(n+1):
    phi = 2*pi*i/n
    f.write("%f %f\n"%(cos(phi),sin(phi)))
f.close()
