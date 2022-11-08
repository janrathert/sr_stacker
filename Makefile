all:  simple_stacker read_raw select_star imgzoom drizzle stacks_to_ppm process_flat debayer scale_pgm cg_deconvol

clean:	
	rm -f *.o
	rm -v simple_stacker readraw select_star imgzoom drizzle stacks_to_ppm process_flat debayer scale_pgm cg_deconvol

clean:

sobel: sobel.c circle.c gauss_jordan.c image_match.c
	 gcc -O2 sobel.c circle.c gauss_jordan.c image_match.c -lm -o sobel

read_raw: read_raw.c showpic.c
	gcc read_raw.c showpic.c -I/usr/include/SDL -lSDL -lX11 -o readraw

select_star: select_star.c track_star.c image_match.c part4_star_centroid.c normalize_u16.o otsu_u16.o
	gcc  select_star.c track_star.c image_match.c part4_star_centroid.c normalize_u16.o otsu_u16.o -I/usr/include/SDL2 -lSDL2 -lX11 -lm -o select_star

normalize_u16.o: normalize.c
	gcc -c -Dvaltype=uint16_t normalize.c -o normalize_u16.o

otsu_u16.o: otsu.c
	gcc -c -Dvaltype=uint16_t otsu.c -o otsu_u16.o

normalize_double.o: normalize.c
	gcc -c -Dvaltype=double normalize.c -o normalize_double.o

debayer: debayer.c
	gcc -O2 debayer.c -o debayer

imgzoom: imgzoom.c
	gcc -O2 imgzoom.c -o imgzoom

find_needle: find_needle.c image_match.c
	gcc -O2 find_needle.c image_match.c  -o find_needle

img_diff_matrix: img_diff_matrix.c image_match.c
	gcc -O2 img_diff_matrix.c image_match.c -o img_diff_matrix

simple_stacker: simple_stacker.c 
	gcc -O2 simple_stacker.c  -o simple_stacker

process_flat: process_flat.c 
	gcc -O2 process_flat.c  -o process_flat

stacks_to_ppm: stacks_to_ppm.c 
	gcc -O2 stacks_to_ppm.c  -o stacks_to_ppm

sirt: sirt.c  gauss_distribution.c sphere_transform.c
	gcc -O2  -Dfilter_size=3 -DSIRT sirt.c gauss_distribution.c sphere_transform.c -lm  -o sirt

drizzle: drizzle.c  gauss_distribution.c sphere_transform.c normalize_double.o Makefile
	gcc -O2  -Dfilter_size=3 drizzle.c gauss_distribution.c sphere_transform.c normalize_double.o -lm  -o drizzle

cg: sirt.c  gauss_distribution.c  sphere_transform.c
	gcc -O2 -Dfilter_size=45 -DCG sirt.c gauss_distribution.c  sphere_transform.c -lm  -o cg

cg_deconvol: cg_deconvol.c  gauss_distribution.c 
	gcc -O2 cg_deconvol.c gauss_distribution.c -lm  -o cg_deconvol
