#!/usr/bin/python2
from PIL import Image
import numpy as np
import sys
# import matplotlib.pyplot as plt
# from scipy.fft import fft2, fftshift
from skimage import img_as_float
from skimage.color import rgb2gray
from skimage.data import astronaut
# from skimage.filters import window


def gen_whitenoise( shape , std ):
	out = np.zeros(shape,float)
	for y in range(0,shape[0]):
		samples = np.random.normal(0, std, size=shape[1])
		for x in range(0,shape[1]):
			out[y,x] = samples[x]
	return out

def gen_whitenoise_complex( shape , std ):
	out = np.zeros(shape,complex)
	for y in range(0,shape[0]):
		samples = np.random.normal(0, std, size=shape[1])
		for x in range(0,shape[1]):
			out[y,x] = std * np.exp( 1j*samples[x] ) 
	return out




def bandpass_2d(r1,r2,shape,dtype):
	out = np.zeros(shape,dtype)
	cx = shape[1]/2
	cy = shape[0]/2
	for y in range(0,shape[0]):
		for x in range(0,shape[1]):
			dx = x-cx
			dy = y-cy
			d2 = dx*dx+dy*dy
			if d2>=r1*r1 and d2<r2*r2 :
				out[y,x] = 1

	return( out )
 
	

def convolve_2d(img,kernel):
	out = np.zeros(img.shape,img.dtype)
	tmp = np.pad(img, (kernel.shape[0]/2,kernel.shape[0]/2) , 'constant' );
	fs = kernel.shape[0]
	for y in range(0,img.shape[0]):
		for x in range(0,img.shape[1]):
			out[y,x] = np.sum( tmp[y:y+fs,x:x+fs]*kernel )

	return(out)
			


gauss_kernel = np.array( [
    [1/16.0, 1/8.0, 1/16.0], 
    [1/8.0, 1/4.0, 1/8.0], 
    [1/16.0, 1/8.0, 1/16.0] ] )


    
im1 = np.array(Image.open(sys.argv[1]))

im2 = np.array(Image.open(sys.argv[2]))

# im1 = convolve_2d( im1 , gauss_kernel )
# im2 = convolve_2d( im2 , gauss_kernel )



window1d = np.abs(np.hanning(im1.shape[0]))
window2d = np.sqrt(np.outer(window1d,window1d))

# im1 = im1 + gen_whitenoise( im1.shape , 500 )
# im2 = im2 + gen_whitenoise( im2.shape , 500 )
im1 = window2d * im1;
im2 = window2d * im2;

# pad1 = (im1.shape[0]*2 - im1.shape[0])/2
# pad2 = (im1.shape[0]*2 - im1.shape[0])-pad1
# im1 = np.pad(im1,  (pad1,pad2) , 'constant' );
# im2 = np.pad(im2,  (pad1,pad2) , 'constant' );



bandpass = bandpass_2d( 0 , im2.shape[0]/4 , im2.shape , float ) 


# <class 'numpy.ndarray'>


img1_fs = np.fft.fft2(im1)
img2_fs = np.fft.fft2(im2)
img1_fs =  np.fft.fftshift(img1_fs)
img2_fs =  np.fft.fftshift(img2_fs) 

diff = np.sqrt( np.sum( np.power( np.abs(img1_fs)-np.abs(img2_fs) , 2 ) ) / (img1_fs.shape[0]*img1_fs.shape[1]) ).astype(np.int32)
img1_fs = img1_fs + gen_whitenoise_complex( img1_fs.shape , 5000 )
img2_fs = img2_fs + gen_whitenoise_complex( img2_fs.shape , 5000 )

r = np.abs( img1_fs ) / 10
r = r.astype(np.int32)
r = np.fmin( r , 65535 );
r = np.fmax( r , 0 );
Image.fromarray(r).save('fft1.pgm')

r = np.abs( img2_fs ) / 10
r = r.astype(np.int32)
r = np.fmin( r , 65535 );
r = np.fmax( r , 0 );
Image.fromarray(r).save('fft2.pgm')


threshold = 0
cross_complex = img1_fs * img2_fs.conj()
cross_abs = np.abs(cross_complex)
cross_power_spectrum  = np.zeros( cross_abs.shape , np.complex128 )
for i in range(0,cross_abs.shape[0]) :
	for j in range(0,cross_abs.shape[1]) :
		if cross_abs[i,j] > threshold : 
			cross_power_spectrum[i,j] = cross_complex[i,j]/cross_abs[i,j]

cross_power_spectrum = cross_power_spectrum  * bandpass 

r = np.zeros( ( cross_power_spectrum.shape[0], cross_power_spectrum.shape[1] , 3 ) , np.uint8 )
for y in range(0,r.shape[0]) :
	for x in range(0,r.shape[1]) :
		dx = x-r.shape[0]/2
		dy = y-r.shape[0]/2
		if np.abs( cross_power_spectrum[y,x] ) :
			if dx != 0 :
				r[y,x,0] = np.angle( cross_power_spectrum[y,x] )/np.pi *  r.shape[0]*1.0/dx + 100
			if dy != 0 :
				r[y,x,1] = np.angle( cross_power_spectrum[y,x] )/np.pi *  r.shape[0]*1.0/dy + 100
		r[y,x,2] = np.abs( cross_power_spectrum[y,x] ) * 100

Image.fromarray(r).save('cross.pgm')
zoom = 20
pad1 = (cross_power_spectrum.shape[0]*zoom - cross_power_spectrum.shape[0])/2
pad2 = (cross_power_spectrum.shape[0]*zoom - cross_power_spectrum.shape[0])-pad1

cross_power_spectrum = np.pad(cross_power_spectrum,  (pad1,pad2) , 'constant' );


			

r = np.abs(np.fft.ifft2(cross_power_spectrum))
r = np.fft.fftshift(r)

max = 0
for i in range(0,r.shape[0]) :
	for j in range(0,r.shape[1]) :
		if r[i,j] > max : 
			max = r[i,j]
			imax = i
			jmax = j

r = 65535*(r/np.amax(r))

center = r[imax-1:imax+2,jmax-1:jmax+2]
# center = center - 5000
center = np.fmax( center , 0 );
# print(np.sum(im)/im.size)
a = np.array( [-1,0,1] )
# print 'x=' , -( ( jmax+np.sum( a * center ) / np.sum(center) ) / zoom - im1.shape[0]/2 )
# print 'y=' , -( ( imax+np.sum( a * np.transpose( center ) ) / np.sum(center) ) / zoom - im1.shape[0]/2 )
print 'x=' , -( jmax * 1.0 / zoom - im2.shape[0]/2 )
print 'y=' , -( imax * 1.0 / zoom - im2.shape[0]/2 )
print 'diff=' , diff

r = r.astype(np.int32)
r = np.fmin( r , 65535 );
r = np.fmax( r , 0 );

# Image.fromarray(r).save('out.pgm')

