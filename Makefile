CC = gcc
CSTD = -std=c99
FLAGS = -O3 -fPIC -Wall -pedantic -I../zfp/include
CFLAGS = $(CSTD) $(FLAGS)
LIBS = -L../zfp/lib -lzfp
CLIBS = $(LIBS) -lm
TARGETS = zfp_rgb\
		zfp_yuv\
		video_zfp_yuv\
		video_zfp_rgb

all: $(TARGETS)

zfp_rgb: zfp_rgb.c ../zfp/lib/libzfp.a
	$(CC) $(CFLAGS) zfp_rgb.c $(CLIBS) -o $@

zfp_yuv: zfp_yuv.c ../zfp/lib/libzfp.a
	$(CC) $(CFLAGS) zfp_yuv.c $(CLIBS) -o $@

video_zfp_yuv: video_zfp_yuv.c ../zfp/lib/libzfp.a
	$(CC) $(CFLAGS) video_zfp_yuv.c $(CLIBS) -o $@

video_zfp_rgb: video_zfp_rgb.c ../zfp/lib/libzfp.a
	$(CC) $(CFLAGS) video_zfp_rgb.c $(CLIBS) -o $@

clean:
	rm -rf zfp_rgb zfp_yuv video_zfp_yuv video_zfp_rgb