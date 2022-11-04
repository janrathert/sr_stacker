
import numpy as np
import matplotlib.pyplot as plt

import skimage.io
from skimage import filters
from skimage.color import rgb2gray
from skimage.filters import window, difference_of_gaussians

import scipy 

plt.rcParams['figure.figsize'] = [10, 10]

frame = skimage.io.imread('test.jpg')

plt.imshow(frame)
plt.show()


