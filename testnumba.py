from numba import njit
import random
import time

@njit
def monte_carlo_pi(nsamples):
    acc = 0
    for i in range(nsamples):
        x = random.random()
        y = random.random()
        if (x ** 2 + y ** 2) < 1.0:
            acc += 1
    return 4.0 * acc / nsamples

@njit
def addVector(x,y):
    z = []
    for i in range(len(x)):
        z.append(x[i]+y[i])
      #  z[i] = x[i]+y[i]
    return z

import numpy as np

@njit
def addVectorN(x,y):
    return x+y

size = 10000000
x = [random.randint(10,20) for i in range(size)]
y = [random.randint(10,20) for i in range(size)]

start = time.time()
z = addVector(x,y)
print("Lista- %f"%(time.time()-start))


x = np.array(x)
y = np.array(y)
start = time.time()
z = addVectorN(x,y)
print("Numpy- %f"%(time.time()-start))





#print(monte_carlo_pi(100000000))

