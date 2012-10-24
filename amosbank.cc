#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>

// data will contain the entire bank file 
unsigned char *data;
int size;
FILE *in;

// palette data
int r[32]={0},g[32]={0},b[32]={0};

// known bank types
const int ntypes = 3;
const char* typelist[ntypes] = { "AmBk", "AmIc", "AmSp" };
enum { T_AMBK, T_AMIC, T_AMSP };

// function for fetching BigEndian data at a given offset
char* getString( int pos, int len ) {
  char *ret = (char*)calloc(len+1,1);
  for( int i=0; i<len; ++i ) ret[i]=(char)data[i+pos];
  ret[len]=0;
  return ret;
}
int get2( int pos ) { 
  int d = int(data[pos])*256 + int(data[pos+1]);
  return d; // negative numbers?!
}
int get4( int pos ) { 
  int d = ( ( int(data[pos])*256 + int(data[pos+1]) )*256 + int(data[pos+2]) ) * 256 + int(data[pos+3]);
  return d; // negative numbers?!
}
void getRGB( int pos, int &r, int &g, int &b ) { // get RGB converted to 0..255
  r = (data[pos] & 0x0F) * 17;
  g = ((data[pos+1] & 0xF0)/16) * 17;
  b = (data[pos+1] & 0x0F) * 17;
}

void loadBobPalette( char *fname ) {
  // get filesize
  struct stat filestatus;
  stat( fname, &filestatus );

  // open icon or sprite bank
  FILE *in = fopen(fname,"rb");
  if(!in) {
    printf("could open external palette file!\n");
    exit(1);
  }

  // load palette from end of file
  unsigned char pal[64];
  fseek( in, filestatus.st_size-64, SEEK_SET );
  if( fread(pal,1,64,in) != 64 ) {
    printf("could not read palette!\n");
    exit(1);
  }
  fclose(in);

  // extract color data
  for( int i=0; i<32; ++i ) {
    r[i] = (pal[i*2] & 0x0F) * 17;
    g[i] = ((pal[i*2+1] & 0xF0)/16) * 17;
    b[i] = (pal[i*2+1] & 0x0F) * 17;
  }
}

// save an image in bitplane format as PNG
//  w      image width in bytes
//  h      image height in pixels
//  d      depth (number of bitplanes)
//  r,g,b  palette
//  raw    memory containing consecutive bitplanes that form the image
//  out    open binary output file handle
//  trans  make color 0 transparent
void saveBitplanesAsPNG( int w, int h, int d, int *r, int *g, int *b, unsigned char *raw, FILE *out, bool trans ) {
  // pixel array of palette indices
  unsigned char *raw2 = (unsigned char*)calloc( w*8*h,1);

  // iterate over d bitplanes
  int bit = 1, o=0;
  for( int bp=0; bp<d; ++bp ) {
    //iterate over scanlines
    for( int y=0; y<h; ++y ) {
      // iterate over bytes
      for( int x=0; x<w; ++x ) {
        unsigned char v = raw[o++];
        // iterate over bits
        for( int j=0; j<8; ++j ) {
          if( v & (1<<j) ) {
            raw2[x*8+(7-j)+(w*8*y)] |= bit;
          }
        }
      }
    }
    bit*=2;
  }

  // debug: output raw data
  //fwrite( raw2, w*16*h,1, out );

  // initialize libpng
  png_structp png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, png_voidp_NULL, png_error_ptr_NULL, png_error_ptr_NULL );
  if (!png_ptr) exit(1);

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_write_struct( &png_ptr, (png_infopp)NULL );
    exit(1);
  }

  png_init_io( png_ptr, out );
  png_set_compression_level( png_ptr, Z_BEST_COMPRESSION );
  png_set_IHDR( png_ptr, info_ptr, w*8, h, 8, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT );

  // setup row pointers
  png_byte **row_pointers = (png_byte**)calloc( h, sizeof(png_byte*) );
  for( int i=0; i<h; ++i ) {
    row_pointers[i] = (png_byte*)&raw2[i*w*8];
  }

  // setup PNG palette
  png_color palette[64] = {0};
  int ncol = 1<<d;
  printf("setting up %d colors\n",ncol);
  for( int i=0; i<ncol; ++i ) {
    if( i<32 ) {
      palette[i].red   = r[i];
      palette[i].green = g[i];
      palette[i].blue  = b[i];
    } else {
      // Extra Half-Bright (EHB)
      palette[i].red   = r[i-32]/2;
      palette[i].green = g[i-32]/2;
      palette[i].blue  = b[i-32]/2;
    }
  }
  png_set_PLTE( png_ptr, info_ptr, palette, ncol );

  //set transparency for color 0
  if(trans) {
    png_byte trans_alpha = 0;
    png_set_tRNS( png_ptr, info_ptr, &trans_alpha, 1, NULL );
  }

  // start writing the file
  png_write_info( png_ptr, info_ptr );
  png_write_rows( png_ptr, row_pointers, h );

  // finish up
  png_write_end( png_ptr, info_ptr );
  png_destroy_write_struct( &png_ptr, &info_ptr );
}

// pac.pic. RLE decompressor
void convertPacPic( const char *base ) {
  int o = 20;
  if( get4(o) == 0x12031990 ) o+=90;
  if( get4(o) != 0x06071963 ) {
    printf("could not find picture header!\n");
    exit(1);
  }

  int w  = get2(o+8),
      h  = get2(o+10),
      ll = get2(o+12),
      d  = get2(o+14);
  printf("width: %d bytes\nheight: %d linelumps a %d lines\n", w,h,ll);

  // reserve bitplane memory
  unsigned char* raw = (unsigned char*)calloc(w*h*ll*d,1);
  unsigned char *picdata = &data[o+24];
  unsigned char *rledata = &data[o+get4(o+16)];
  unsigned char *points  = &data[o+get4(o+20)];

  int rrbit = 6, rbit = 7;
  int picbyte = *picdata++;
  int rlebyte = *rledata++;
  if (*points & 0x80) rlebyte = *rledata++;

  for( int i = 0; i < d; i++) {
    unsigned char *lump_start = &raw[i*w*h*ll];
    for( int j = 0; j < h; j++ ) {
      unsigned char *lump_offset = lump_start;
      for( int k = 0; k < w; k++ ) {
        unsigned char *dd = lump_offset;
        for( int l = 0; l < ll; l++ ) {
          /* if the current RLE bit is set to 1, read in a new picture byte */
          if (rlebyte & (1 << rbit--)) picbyte = *picdata++;

          /* write picture byte and move down by one line in the picture */
          *dd = picbyte;
          dd += w;

          /* if we've run out of RLE bits, check the POINTS bits to see if a new RLE byte is needed */
          if (rbit < 0) {
            rbit = 7;
            if (*points & (1 << rrbit--)) rlebyte = *rledata++;
            if (rrbit < 0)  rrbit = 7, points++;
          }
        }
        lump_offset++;
      }
      lump_start += w * ll;
    }
  }

  // save png
  char fname[1000];
  snprintf( fname, 1000, "%s.png", base );
  FILE* out = fopen( fname, "wb" );
  saveBitplanesAsPNG( w,h*ll,d, r,g,b, raw, out, false );
  fclose(out);
}

int main( int argc, char *argv[] ) {
  if( argc != 2 && argc != 3 ) {
    printf("Usage: amosbank filename [bobfile]\nif a second filename is supplies it is assumed to be an icon or sprite bank. Its palette is used for the image then.\n");
    return 1;
  }

  int i;

  // load external palette
  if( argc == 3 ) loadBobPalette(argv[2]);

  // get filesize
  struct stat filestatus;
  stat( argv[1], &filestatus );
  size = filestatus.st_size;
  data = (unsigned char*)calloc(size,1);

  // read entire file
  in = fopen(argv[1],"rb");
  if(!in) {
    printf("could open file!\n");
    return 1;
  }
  if( fread(data,1,size,in) != size ) {
    printf("could not read entire file!\n");
    return 1;
  }
  fclose(in);

  // determine type
  char *typestr = getString(0,4);
  int type;
  printf( "Magic: %s\n", typestr);
  for( type=0; type<ntypes; ++type ) {
    //printf( "%d: %s=%s ?\n", i, typestr,typelist[type] );
    if( strcmp(typestr,typelist[type]) == 0 ) break;
  }
  if( type==ntypes ) {
    printf("unknown bank type!\n");
    return 1;
  }

  // output filename buffer
  char fname[1000]; //const buffer = retarded (lazy)

  // generic bank
  if( type==T_AMBK ) {
    char *bname = getString(12,8);
    printf("Bankname: '%s'\n", bname );
    printf("Banknum:  %d\n", get2(4) );
    printf("Fast or Chip:  %d\n", get2(6) );
    printf("type=%d\n",type);

    // test for pac.pic.
    if( strcmp(bname,"Pac.Pic.")==0 ) {
      printf("Pac.Pic. detected\n");
      convertPacPic(argv[1]);
      return 0;
    }

    // build JSON output filename
    snprintf( fname, 1000, "%s.json", argv[1] );

    // test for Work
    if( strcmp(bname,"Work    ")==0 ) {
      FILE* out = fopen( fname, "wt" );
      printf("Work bank detected\nnumber of 2byte words: %d\n", (size-20)/2 );
      fprintf( out, "{ words: [ " );
      for( i=0; i<(size-20)/2; ++i ) {
        fprintf( out, i>0?" ,%d":"%d", get2(20+i*2) );
      }
      fprintf( out, " ] }\n" );
      fclose(out);
      return 0;
    }

    // test for Datas
    if( strcmp(bname,"Datas   ")==0 ) {
      FILE* out = fopen( fname, "wt" );
      printf("Data bank detected\nnumber of bytes: %d\n", (size-20) );
      fprintf( out, "{ bytes: [ " );
      for( i=0; i<(size-20); ++i ) {
        fprintf( out, i>0?" ,%d":"%d", int(data[20+i]) );
      }
      fprintf( out, " ] }\n" );
      fclose(out);
      return 0;
    }


    // nothing to do (dump binary data maybe?)
    return 0;
  }

  if( type==T_AMSP || type==T_AMIC ) {
    int nsp = get2(4);

    printf("Number of icons/sprites: %d\n", nsp);

    // reading palette from end of file
    for( i=0; i<32; ++i ) { 
      getRGB( size-64+i*2, r[i],g[i],b[i] ); 
      printf( "color %d, (%d,%d,%d)\n", i, r[i],g[i],b[i] );
    }

    //iterate over sprites
    int o = 6;
    for( i=0; i<nsp; ++i ) {
      int w = get2(o),
          h = get2(o+2),
          d = get2(o+4),
          hx = get2(o+6),
          hy = get2(o+8);
      o += 10;
      printf( "sprite %d: %d*%d %dpl hotspot(%d,%d)\n",i,w*16,h,d,hx,hy);
      
      // read data at o and output png (maybe?)
      snprintf( fname, 1000, "%s.%d.png", argv[1],i );
      FILE* out = fopen( fname, "wb" );
      saveBitplanesAsPNG( w*2,h,d, r,g,b, &data[o], out, true );
      fclose(out);

      o += w*h*d*2;
    }
  }


  return 0;
}
