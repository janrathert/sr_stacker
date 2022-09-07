all: find_needle simple_stacker read_raw select_star art_pre imgzoom sirt drizzle cg stacks_to_ppm process_flat debayer
#  sobel read_raw debayer imgzoom img_diff_matrix find_needle

sobel: sobel.c circle.c gauss_jordan.c image_match.c
	 gcc -O2 sobel.c circle.c gauss_jordan.c image_match.c -lm -o sobel

read_raw: read_raw.c showpic.c
	gcc read_raw.c showpic.c -I/usr/include/SDL -lSDL -lX11 -o readraw

select_star: select_star.c track_star.c image_match.c
	gcc  select_star.c track_star.c image_match.c -I/usr/include/SDL -lSDL -lX11 -lm -o select_star

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

art_pre: art_pre.c 
	gcc -O0 -g art_pre.c  -o art_pre

sirt: sirt.c  gauss_distribution.c sphere_transform.c
	gcc -O2  -Dfilter_size=3 -DSIRT sirt.c gauss_distribution.c sphere_transform.c -lm  -o sirt

drizzle: drizzle.c  gauss_distribution.c sphere_transform.c
	gcc -O2  -Dfilter_size=3 drizzle.c gauss_distribution.c sphere_transform.c -lm  -o drizzle

cg: sirt.c  gauss_distribution.c dct.c sphere_transform.c
	gcc -O2 -Dfilter_size=13 -DCG sirt.c gauss_distribution.c dct.c sphere_transform.c -lm  -o cg
