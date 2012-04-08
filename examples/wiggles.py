#! /usr/bin/env python

from __future__ import division

from math import sqrt
from random import gauss

def ornstein_uhlenbeck(alpha, sigma, level, tmax=10, n=400):
    """ dX_t = -alpha X_t dt + sqrt(2 alpha sigma) dB """
    res = []
    dt = tmax / n

    x = 0
    for i in range(n+1):
        res.append(x+level)
        x += - alpha * x * dt + sqrt(2*alpha*sigma) * gauss(0, sqrt(dt))
    return res

t = [ 10 * i / 400 for i in range(401) ]
rres = [ t ]
for i in range(0, 31):
    rres.append(ornstein_uhlenbeck(2+.2*i, 4, i*10))

for line in zip(*rres):
    print ' '.join('%f'%x for x in line)
