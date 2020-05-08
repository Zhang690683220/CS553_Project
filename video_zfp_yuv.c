#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zfp.h"

const int range = 256;

void write_yuv444(char *outputPath, int width, int height, int* y_buf,
                    int* u_buf, int* v_buf);
void split_yuv444(char *inputPath, int width, int height, int* y_buf,
                    int* u_buf, int* v_buf);
static int compress(char* filepath, int* array, int nx, int ny, double rate, int decompress);

int main(int argc, char** argv){
    double arg = atof(argv[1]);
    printf("Compression with relative error %lf\n", arg);
    int height = 536;
    int width = 1280;
    //int i =0;
    int frame = 4754;
    int* y_buffer = (int*) malloc(sizeof(int)*height*width);
    int* u_buffer = (int*) malloc(sizeof(int)*height*width);
    int* v_buffer = (int*) malloc(sizeof(int)*height*width);
    

    //double tolerance = range * arg;
    double rate = arg;
    for (int i = 0; i < frame; i++)
    {
        char inputYUVName[128];
        sprintf(inputYUVName, "./video/yuv/frames%d.yuv", i);
        

        split_yuv444(inputYUVName, width, height, y_buffer, u_buffer, v_buffer);
        
        char zfpYFileName[128];
        char zfpUFileName[128];
        char zfpVFileName[128];
        sprintf(zfpYFileName, "./video/yuv/Y_%sframes%d.zfp",argv[1], i);
        sprintf(zfpUFileName, "./video/yuv/U_%sframes%d.zfp",argv[1], i);
        sprintf(zfpVFileName, "./video/yuv/V_%sframes%d.zfp",argv[1], i);

        compress(zfpYFileName, y_buffer, height, width, rate, 0);
        compress(zfpUFileName, u_buffer, height, width, rate, 0);
        compress(zfpVFileName, v_buffer, height, width, rate, 0);

    
        compress(zfpYFileName, y_buffer, height, width, rate, 1);
        compress(zfpUFileName, u_buffer, height, width, rate, 1);
        compress(zfpVFileName, v_buffer, height, width, rate, 1);

        char outputYUVName[128];
        sprintf(outputYUVName, "./video/yuv/zfp_%s_frame%d.yuv",argv[1], i);

        write_yuv444(outputYUVName, width, height, y_buffer, u_buffer, v_buffer);

        
        
    }   
    printf("YUV File generated!!\n");
    free(y_buffer);
    free(u_buffer);
    free(v_buffer);
    

    

    
    return 0;
}





void split_yuv444(char *inputPath, int width, int height, int* y_buf,
                    int* u_buf, int* v_buf) {
    FILE *fp_iyuv = fopen(inputPath, "rb+");

    unsigned char *data = (unsigned char *) malloc(width * height * 3);
    fread(data, 1, width * height * 3, fp_iyuv);
    int index = 0;
    for(index=0; index<width*height; index++) {
        *(y_buf+index) = (int) *(data+index);
        *(u_buf+index) = (int) *(data+width*height+index);
        *(v_buf+index) = (int) *(data+width*height*2+index);
    }

    free(data);
    fclose(fp_iyuv);
}

void write_yuv444(char *outputPath, int width, int height, int* y_buf,
                    int* u_buf, int* v_buf) {
    FILE *fp_oyuv = fopen(outputPath, "wb");

    unsigned char *data = (unsigned char *) malloc(width * height * 3);

    int index = 0;
    for(index=0; index<width*height; index++) {
        *(data+index) = (unsigned char) *(y_buf+index);
        *(data+width*height+index) = (unsigned char) *(u_buf+index);
        *(data+width*height*2+index) = (unsigned char) *(v_buf+index);
    }

    fwrite(data, 1, width*height*3, fp_oyuv);

    free(data);
    fclose(fp_oyuv);
}

/* compress or decompress array */
static int compress(char* filepath, int* array, int nx, int ny, double rate, int decompress)
{
  int status = 0;    /* return value: 0 = success */
  zfp_type type;     /* array scalar type */
  zfp_field* field;  /* array meta data */
  zfp_stream* zfp;   /* compressed stream */
  void* buffer;      /* storage for compressed stream */
  size_t bufsize;    /* byte size of compressed buffer */
  bitstream* stream; /* bit stream to write to or read from */
  size_t zfpsize;    /* byte size of compressed stream */

  /* allocate meta data for the 2D array a[nz][ny] */
  type = zfp_type_int32;
  field = zfp_field_2d(array, type, nx, ny);

  /* allocate meta data for a compressed stream */
  zfp = zfp_stream_open(NULL);

  /* set compression mode and parameters via one of three functions */
  zfp_stream_set_rate(zfp, rate, type, 2, 0); 
/*  zfp_stream_set_precision(zfp, precision); */
  //zfp_stream_set_accuracy(zfp, tolerance);

  /* allocate buffer for compressed data */
  bufsize = zfp_stream_maximum_size(zfp, field);
  buffer = malloc(bufsize);

  /* associate bit stream with allocated buffer */
  stream = stream_open(buffer, bufsize);
  zfp_stream_set_bit_stream(zfp, stream);
  zfp_stream_rewind(zfp);

  /* compress or decompress entire array */
  if (decompress) {
    FILE *fp_izfp = NULL;
    if((fp_izfp=fopen(filepath,"rb"))==NULL){
		printf("Error: Cannot open input zfp file.\n");
		return -1;
	}
    /* read compressed stream and decompress array */
    zfpsize = fread(buffer, 1, bufsize, fp_izfp);
    fclose(fp_izfp);
    if (!zfp_decompress(zfp, field)) {
      fprintf(stderr, "decompression failed\n");
      status = 1;
    }
  }
  else {
    /* compress array and output compressed stream */
    zfpsize = zfp_compress(zfp, field);
    if (!zfpsize) {
      fprintf(stderr, "compression failed\n");
      status = 1;
    }
    else {
        FILE *fp_ozfp = NULL;
        if((fp_ozfp=fopen(filepath,"wb"))==NULL){
		    printf("Error: Cannot open output zfp file.\n");
		    return -1;
	    }
        fwrite(buffer, 1, zfpsize, fp_ozfp);
        fclose(fp_ozfp);
    }
  }

  /* clean up */
  zfp_field_free(field);
  zfp_stream_close(zfp);
  stream_close(stream);
  free(buffer);

  return status;
}