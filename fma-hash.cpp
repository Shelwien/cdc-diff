
#include "common.inc"

#include "crc32.inc"

#include "anchor.inc"

enum {
  inpbufsize = 1<<16
};

byte inpbuf[inpbufsize];

int main( int argc, char** argv ) {

  int i,j;

  retry:

  if( argc<3 ) {
printusage:
    printf( 
"Usage:\n" 
"  /x# - hash window size (4..32767)\n"
"  /y# - min fragment size (4..32767)\n"
"  /z# - max fragment size (4..32767)\n"
"  /m# - anchormask len (0..32)\n"
"  /b# - number of lookup bits (1..24)\n"
    );
    return 1;
  }

  if( argv[1][0]=='/' ) {
    int k = atoi( &argv[1][2] );
    switch( argv[1][1]&0xDF ) {
      case 'X': cfg.winsize = Max(4,Min(32767,k)); break;
      case 'Y': cfg.minblklen = Max(4,Min(cfg.maxblklen-1,k)); break;
      case 'Z': cfg.maxblklen = Max(cfg.minblklen,Min(32767,k)); break;
      case 'M': cfg.anchormask = (1<<Max(0,Min(32,k)))-1; break;
      case 'B': cfg.checkbits = Max(0,Min(32,k)); break;
      default: goto printusage;
    }
    memcpy( &argv[1], &argv[2], (argc-2)*sizeof(argv[0]) );
    argc--; goto retry;
  }

  FILE* f = fopen( argv[1], "rb" );
  if( f==0 ) return 1;

  FILE* g = fopen( argv[2], "wb" );
  if( g==0 ) return 1;

  cfg.save( g );

  anchor = Y.x;
  X.x = 0;

  while(1) {
    int l = fread( inpbuf, 1,inpbufsize, f );
    if( l<=0 ) break;
    for( i=0; i<l; i++ ) {

      X.Update( inpbuf[i] );
      Y.Update( inpbuf[i] ); blklen++;
      int flag = 0;
      if( blklen>=cfg.minblklen ) {
        flag = ((Y.x&cfg.anchormask)==0) | (blklen>=cfg.maxblklen);
        if( flag ) FlushAnchor(g);
      }
    }
  };

  if( blklen>0 ) FlushAnchor(g);

  printf( "sumblklen=%i\n", sumblklen );

  return 0;
}



