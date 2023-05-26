
#include "common.inc"

enum {
  inpbufsize = 1<<16
};

byte buf[inpbufsize];

void fcopy( FILE* g, FILE* h, uint n ) {
  int j; 
  do {
    j = n; if( j>inpbufsize ) j=inpbufsize;
    n -= j;
    fread( buf, 1,j, g );
    fwrite( buf, 1,j, h );
  } while( n>0 );
}

#include "sh_qsort.inc"

#include "crc32.inc"

#include "anchor.inc"

byte inpbuf[inpbufsize];

int main( int argc, char** argv ) {
  int i,j,l;

  if( argc<4 ) return 1;

  // original file
  FILE* f = fopen( argv[1], "rb" );
  if( f==0 ) return 1;

  // broken file's blockcrc
  FILE* g = fopen( argv[2], "rb" );
  if( g==0 ) return 1;

  // result (crk?)
  FILE* h = fopen( argv[3], "wb" );
  if( h==0 ) return 1;

  g_len = flen(g);

  cfg.load(g); 
  g_len -= sizeof(cfg);

  int checkN = 1<<cfg.checkbits;
  g_len = g_len/sizeof(hashrec0);
  hashbuf = new hashrec[ g_len ];
  hashidx = new uint[ g_len+1 ];
  hashparts = new hashpart[checkN];

  hashrec0* hashbuf0 = (hashrec0*)(((byte*)hashbuf)+g_len*(sizeof(hashrec)-sizeof(hashrec0)));
  fread( hashbuf0, 1,sizeof(hashrec0)*g_len, g );
  fclose( g );

  for( i=0,j=0; i<g_len; i++ ) j = hashbuf[i].Init( hashbuf0[i], j, i );

  printf( "%i anchors loaded. hashed file length = %i\n", g_len, j );

  printf( "Bulding the anchor index... " ); fflush(stdout);
  sh_qsort<TEST_hashrec>( hashbuf, 0, g_len-1 );

  // index for hash location by its pos in hashfile
  for( i=0; i<g_len; i++ ) hashidx[hashbuf[i].x]=i; hashidx[i]=0;

  //bzero( hashparts );
  for( i=0; i<checkN; i++ ) hashparts[i].Init(0,0);

  uint chash;
  for( i=0,l=0; i<g_len; i++ ) {
    uint c = hashbuf[i].blkcrc >> (32-cfg.checkbits);
    if( l==0 ) l=1, chash=c; else if( c==chash ) l++; else {
      hashparts[chash].Init( i-l, l ); l=1, chash=c;
    }
  } 
  if( l ) hashparts[chash].Init( i-l, l );

  printf( "\b\b\b Done.\n" );

//  fwrite( &i, 1,4, h ); // placeholder for literal length
//  offsloc = 0;

  blklen = 0; lastidx=0;
  anchor = Y.x;
  X.x = 0;

  uint f_len = flen(f);
  uint f_lem = f_len;

  for( inpofs=0; f_len>0; inpofs+=l ) {
    l = f_len; if( l>inpbufsize ) l=inpbufsize;
    f_len -= l;
    fread( inpbuf, 1,l, f );
    for( i=0; i<l; i++ ) {

      uint c = inpbuf[i];
      X.Update( c );
      Y.Update( c ); 
      blklen++;

      int flag = 0;
      if( blklen>=cfg.minblklen ) {
        flag = ((Y.x&cfg.anchormask)==0) | (blklen>=cfg.maxblklen);
        if( flag ) FlushAnchor(i,f,h);
      }

    }
  };

  if( blklen>0 ) FlushAnchor(0,f,h);
  if( outlen>0 ) lastidx=g_len, FlushAnchor(0,f,h);

  if( lastout<f_lem ) {
    uint d = f_lem - lastout;
    fwrite( &d, 1,4, h );
    fseek( f, lastout, SEEK_SET );
    fcopy( f, h, d ); // copy d bytes
  }

//  fseek( h, offsloc, SEEK_SET ); // to literal count
//  fwrite( &blklen, 1,4, h );
//  chsize( fileno(h), offsloc+4+blklen );

  return 0;
}



