
#include "common.inc"

enum {
  inpbufsize = 1<<16
};

byte buf[inpbufsize];

void fcopy( FILE* g, FILE* h, uint n ) {
  int j; 
  do {
    j = n; if( j>=inpbufsize ) j=inpbufsize;
    n -= j;
    fread( buf, 1,j, g );
    fwrite( buf, 1,j, h );
  } while( n>0 );
}

int main( int argc, char** argv ) {
  int i,j,l; uint d,e;

  if( argc<4 ) return 1;

  // source file (broken?)
  FILE* f = fopen( argv[1], "rb" );
  if( f==0 ) return 1;

  // patch file
  FILE* g = fopen( argv[2], "rb" );
  if( g==0 ) return 1;

  // result (patch file)
  FILE* h = fopen( argv[3], "wb" );
  if( h==0 ) return 1;

  while(1) {
    // literal length
    l = fread( &d, 1,4, g ); if( l!=4 ) break; 
    fcopy( g, h, d ); // d bytes from g to h

    // match length
    l = fread( &d, 1,4, g ); if( l!=4 ) break;

    // match offset
    l = fread( &e, 1,4, g ); if( l!=4 ) break;
    printf( "%08X %08X @ %08X\n", ftell(h), d, e );
    fseek( f, e, SEEK_SET );
    fcopy( f, h, d );
  }

  fclose(h);

  return 0;
}



