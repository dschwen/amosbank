#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

unsigned char *data;
FILE *in;
int size;
bool pacpic = false;

const int ntypes = 3;
const char* typelist[ntypes] = { "AmBk", "AmIc", "AmSp" };
enum { T_AMBK, T_AMIC, T_AMSP };

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

void convertPacPic() {
  int o = 20;
  if( get4(o) == 0x12031990 ) o+=90;
  if( get4(o) != 0x06071963 ) {
    printf("could not find picture header!\n");
    exit(1);
  }

  int w = get2(o+8),
      h = get2(o+10),
      l = get2(o+12);
  printf("width: %d bytes\nheight: %d linelumps a %d lines\n", w,h,l);
}

int main( int argc, char *argv[] ) {
  if( argc != 2 ) {
    printf("Usage: amosbank filename\n");
    return 1;
  }

  int i;

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

  // generic bank
  if( type==T_AMBK ) {
    printf("Bankname: %s\n", getString(12,8) );
    printf("Banknum:  %d\n", get2(4) );
    printf("Fast or Chip:  %d\n", get2(6) );
    printf("type=%d\n",type);

    // test for pac.pic.
    if( strcmp(getString(12,8),"Pac.Pic.")==0 ) {
      printf("Pac.Pic. detected\n");
      convertPacPic();
      return 0;
    }

    // nothing to do (dump binary data maybe?)
    return 0;
  }

  if( type==T_AMSP || type==T_AMIC ) {
    int nsp = get2(4);
    int r[32],g[32],b[32];
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

      o += w*h*d*2;
    }
  }


  return 0;
}
