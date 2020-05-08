#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zfp.h"


void split_rgb24(char *inputPath, int width, int height, int* r_buf,
                    int* g_buf, int* b_buf);
void write_rgb24(char *outputPath, int width, int height, int* r_buf,
                    int* g_buf, int* b_buf);
static int compress(char* filepath, int* array, int nx, int ny, double tolerance, int decompress);

int main(int argc, char** argv){
    double arg = atof(argv[1]);
    printf("Compression with relative error %lf\n", arg);
    int height = 1440;
    int width = 1080;
    

    int* r_buffer = (int*) malloc(sizeof(int)*height*width);
    int* g_buffer = (int*) malloc(sizeof(int)*height*width);
    int* b_buffer = (int*) malloc(sizeof(int)*height*width);

    split_rgb24("./image/photo/photo.rgb", width, height, r_buffer, g_buffer, b_buffer);

    const int range = 256;
    double tolerance = range * arg;
    //double rate = arg;
    char zfpRFileName[128];
    char zfpGFileName[128];
    char zfpBFileName[128];
    sprintf(zfpRFileName, "./image/photo/R_%s.zfp",argv[1]);
    sprintf(zfpGFileName, "./image/photo/G_%s.zfp",argv[1]);
    sprintf(zfpBFileName, "./image/photo/B_%s.zfp",argv[1]);
    /* Compression */
    compress(zfpRFileName, r_buffer, height, width, tolerance, 0);
    compress(zfpGFileName, g_buffer, height, width, tolerance, 0);
    compress(zfpBFileName, b_buffer, height, width, tolerance, 0);

    /* Decompression */
    compress(zfpRFileName, r_buffer, height, width, tolerance, 1);
    compress(zfpGFileName, g_buffer, height, width, tolerance, 1);
    compress(zfpBFileName, b_buffer, height, width, tolerance, 1);

    char outputRGBName[128];
    sprintf(outputRGBName, "./image/photo/zfp_%s.rgb",argv[1]);

    write_rgb24(outputRGBName, width, height, r_buffer, g_buffer, b_buffer);

    
    printf("RGB File generated!!\n");

    free(r_buffer);
    free(g_buffer);
    free(b_buffer);

    

    
    return 0;
}




void split_rgb24(char *inputPath, int width, int height, int* r_buf,
                    int* g_buf, int* b_buf) {
    FILE *fp_irgb = fopen(inputPath, "rb+");

    unsigned char *data = (unsigned char *) malloc(width * height * 3);

    fread(data, 1, width * height * 3, fp_irgb);
    int index = 0;
    for (index = 0; index < width * height * 3; index = index + 3) {
        //R
        *(r_buf+index/3) = (int) *(data+index);
        //G
        *(g_buf+index/3) = (int) *(data+index+1);
        //B
        *(b_buf+index/3) = (int) *(data+index+2);
    }

    //std::cout << index << std::endl;

    free(data);
    fclose(fp_irgb);
}

void write_rgb24(char *outputPath, int width, int height, int* r_buf,
                    int* g_buf, int* b_buf) {
    FILE *fp_orgb = fopen(outputPath, "wb");
    unsigned char *data = (unsigned char *) malloc(width * height * 3);

    int index = 0;
    for(index=0; index < width*height*3; index = index + 3) {
      *(data+index) = (unsigned char) *(r_buf+index/3);
      *(data+index+1) = (unsigned char) *(g_buf+index/3);
      *(data+index+2) = (unsigned char) *(b_buf+index/3);
    }

    fwrite(data, 1, width *height * 3, fp_orgb);
    free(data);
    fclose(fp_orgb);
}


/* compress or decompress array */
static int compress(char* filepath, int* array, int nx, int ny, double tolerance, int decompress)
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
  //zfp_stream_set_rate(zfp, rate, type, 2, 0); 
/*  zfp_stream_set_precision(zfp, precision); */
  zfp_stream_set_accuracy(zfp, tolerance);

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