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
void saveBitplanesAsPNG( int w, int h, int d, int *r, int *g, int *b, unsigned char *raw, FILE *out, bool trans, bool ham=false ) {
  // pixel array of palette indices
  unsigned char *raw3, *raw2 = (unsigned char*)calloc( w*8*h,1);

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

  // decode HAM data
  if(ham) {
    raw3 = (unsigned char*)calloc( 3*w*8*h,1);
    int ch,cl;
    for( int y=0; y<h; ++y ) {
      int lr=0,lg=0,lb=0;
      for( int x=0; x<w*8; ++x ) {
        ch = raw2[x+y*w*8];
        cl = ch%16; ch/=16;
        if( ch==0 ) { // pick base color
          lr=r[cl];
          lg=g[cl];
          lb=b[cl];
        } else { // hold-and-modify
          if( ch==1 ) lb=cl*17;
          if( ch==2 ) lr=cl*17;
          if( ch==3 ) lg=cl*17;
          //lr=lg=lb=128;
        }
        raw3[3*(x+y*w*8)] = lr;
        raw3[3*(x+y*w*8)+1] = lg;
        raw3[3*(x+y*w*8)+2] = lb;
      }
    }
    // debug: output raw data
    //fwrite( raw3, w*8*h, 3, out );
    //return;
  }


  // initialize libpng
  //png_structp png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, png_voidp_NULL, png_error_ptr_NULL, png_error_ptr_NULL );
  png_structp png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
  if (!png_ptr) exit(1);

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_write_struct( &png_ptr, (png_infopp)NULL );
    exit(1);
  }

  png_init_io( png_ptr, out );
  //png_set_compression_level( png_ptr, Z_BEST_COMPRESSION );
  png_set_compression_level( png_ptr, PNG_Z_DEFAULT_COMPRESSION );
  png_set_IHDR( png_ptr, info_ptr, w*8, h, 8, ham?PNG_COLOR_TYPE_RGB:PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT );

  // setup row pointers
  png_byte **row_pointers = (png_byte**)calloc( h, sizeof(png_byte*) );
  if(ham) {
   for( int i=0; i<h; ++i )
      row_pointers[i] = (png_byte*)&raw3[i*w*8*3];
  } else {
   for( int i=0; i<h; ++i )
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
      // Extra Half-Brite (EHB)
      palette[i].red   = r[i-32]/2;
      palette[i].green = g[i-32]/2;
      palette[i].blue  = b[i-32]/2;
    }
  }
  if(!ham) {
    png_set_PLTE( png_ptr, info_ptr, palette, ncol );

    //set transparency for color 0
    if(trans) {
      png_byte trans_alpha = 0;
      png_set_tRNS( png_ptr, info_ptr, &trans_alpha, 1, NULL );
    }
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
  bool ham = false;
  if( get4(o) == 0x12031990 ) {
    // detect HAM
    if( get2(o+20) & 0x800 ) {
      ham = true;
      printf("HAM data is not yet supported, output will look garbled!\n");
    }
    // fetch palette
    for( int i=0; i<32; ++i ) { 
      getRGB( o+26+i*2, r[i],g[i],b[i] ); 
      printf( "color %d, (%d,%d,%d)\n", i, r[i],g[i],b[i] );
    }
    o+=90;
  }
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
  saveBitplanesAsPNG( w,h*ll,d, r,g,b, raw, out, false, ham );
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

    // test for Samples
    if( strcmp(bname,"Samples ")==0 ) {
      int f,l, o, nsam = get2(20);
      int16_t i16;
      int32_t i32, s1size, s2size;
      const char *tag[] = { "RIFF", "WAVE", "fmt ", "data" };
      printf( "Found %d samples\n", nsam );
      for( i=0; i<nsam; i++ ) {
        // get sample name and open output file
        o = get4(22+i*4)+20;
        char *sname = getString(o,8);
        snprintf( fname, 1000, "%s.%d.%s.wav", argv[1],i,sname );
        printf("%s\n", fname);
        FILE* out = fopen( fname, "wb" );

        // get sample properties
        f = get2(o+8);
        l = get4(o+10);
        printf("%d Hz, %d bytes\n",f,l);

        // write WAV header
        fwrite( tag[0], 4, 1, out );
        s1size = 16;
        s2size = l;
        i32 = 4 + (8 + s1size) + (8 + s2size); fwrite( &i32, 1, 4, out);
        fwrite( tag[1], 4, 1, out );
        fwrite( tag[2], 4, 1, out );
        fwrite( &s1size, 1, 4, out);
        i16 = 1; fwrite( &i16, 1, 2, out); // PCM
        i16 = 1; fwrite( &i16, 1, 2, out); // mono
        i32 = f; fwrite( &i32, 1, 4, out); // Sample rate
        i32 = f*1*1; fwrite( &i32, 1, 4, out); // byte rate (f*chan*bytepersample)
        i16 = 1; fwrite( &i16, 1, 2, out); // blockalign
        i16 = 8; fwrite( &i16, 1, 2, out); // bits per sample

        // 'data' chunk
        fwrite( tag[3], 4, 1, out );
        fwrite( &s2size, 1, 4, out);

        // convert signed 8bit to unsigned 8bit
        char *sdata = (char*)data;
        for( int j=0; j<l; ++j ) data[o+14+j] = (unsigned char)(int(sdata[o+14+j])+128);
        fwrite( &data[o+14], l, 1, out);

        // close file
        fclose(out);
        free(sname);
      }
      return 0;
    }

    // build JSON output filename
    snprintf( fname, 1000, "%s.json", argv[1] );

    // test for Work
    if( strcmp(bname,"Work    ")==0 ) {
      FILE* out = fopen( fname, "wt" );
      printf("Work bank detected\nnumber of 2byte words: %d\n", (size-20)/2 );
      fprintf( out, "{ \"words\": [ " );
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
    printf("Unknown bank name!\n");
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
