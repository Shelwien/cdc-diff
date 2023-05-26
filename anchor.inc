
struct config {

  word winsize;
  word minblklen;
  word maxblklen;
  word anchormask;
  byte checkbits;

  config( void ) {
    winsize = 256;
    minblklen = winsize;
    maxblklen = 2*winsize;
    anchormask = 0x3F;
    checkbits = 16;
  }

  void save( FILE* g ) {
    fwrite( this, 1,sizeof(*this), g );
  }

  void load( FILE* f ) {
    fread( this, 1,sizeof(*this), f );
  }

} cfg;

struct hashrec0 {
  uint anchor;
  uint blkcrc;
  word blklen;
};

struct hashrec {
  uint x; // hash position number in the hashfile
  uint anchor; // content anchor
  uint blkcrc; // block crc
  uint blklen; // block length
  uint blkofs; // block offset in the file

  uint Init( hashrec0& a, uint j, uint i ) {
    anchor = a.anchor;
    blkcrc = a.blkcrc;
    blklen = a.blklen;
    blkofs = j;
    x = i;
    return j+blklen;
  }

  int match( uint a, uint b, uint c ) {
    return (anchor==a) && (blkcrc==b) && (blklen==c);
  }
  
};

struct hashpart {
  uint x; // offset for first crc with given check bits 
  uint n; // number of matching crcs
  hashpart() {}
  hashpart( int ) : x(0), n(0) {}
  void Init( uint X, uint N ) { x = X; n = N; }
};

#ifdef SH_QSORT
struct TEST_hashrec {
  // Returns neg if 1<2, 0 if 1=2, pos if 1>2.
  static int c( hashrec* restrict A, uint x, uint y ) {
    uint a = A[x].blkcrc;
    uint b = A[y].blkcrc;
//    return (a==b)?0:(a<b)?-1:1;
    return (a==b)?   A[x].blkofs<A[y].blkofs?-1:1  :(a<b)?-1:1;
  }
  static void s( hashrec* restrict A, uint x, uint y ) {
    SWAP( A[x], A[y] );
  }
};
#endif


wincrc Y(cfg.winsize);
crc X;

hashrec* restrict hashbuf; uint g_len;
uint* restrict hashidx;

uint anchor, blklen, blkcrc, sumblklen;
uint inpofs, lastidx;
uint outofs, altofs, outlen=0;
uint lastout=0; // offset of next unchecked byte
uint offsloc; // offset of literal string length location (in "h") 

hashpart* restrict hashparts;

#ifdef SH_QSORT
void FlushAnchor( int i, FILE* f, FILE* h ) {
  int j;

  anchor ^= Y.x;
  blkcrc = X.x;

  if( hashbuf[lastidx].match(anchor,blkcrc,blklen) ) j=lastidx; else {
    uint c = blkcrc >> (32-cfg.checkbits);
    uint n = hashparts[c].n;
    j = g_len; // not found
    if( n ) {
      uint x = hashparts[c].x; n+=x;
      for( x; x<n; x++ ) if( hashbuf[x].match(anchor,blkcrc,blklen) ) { j=x; break; }
    }
  }

  if( ((j!=lastidx) || (j>=g_len)) && (outlen>0) ) { // not found, or not adjacent
    int d = outofs - lastout;
    fwrite( &d, 1,4, h );

    uint tmp = ftell(f);
    fseek( f, lastout, SEEK_SET );
    fcopy( f, h, d ); // copy d bytes
    fseek( f, tmp, SEEK_SET );

    fwrite( &outlen, 1,4, h ); // data to copy
    fwrite( &altofs, 1,4, h ); // offset from where to copy

    lastout = outofs+outlen;
//    printf( "%08X %08X @ %08X\n", outofs, outlen, altofs );
    outlen = 0;
//    fprintf(h, "%08X %08X @ %08X\n", outofs, outlen, altofs );
  }

  if( j<g_len ) { // something found
    if( outlen==0 ) outofs=inpofs+i+1-blklen, altofs=hashbuf[j].blkofs;
    outlen += blklen; // only increment len if it was nonzero
    lastidx = hashidx[hashbuf[j].x+1]; // maybe next block would match too?
  }

  anchor = Y.x; blklen = 0; X.x = 0;
}
#endif

void FlushAnchor( FILE* g ) {
//printf( "%i ", blklen );
  hashrec0 tmp[] = { anchor^Y.x, X.x, blklen };
  fwrite( &tmp, 1,sizeof(tmp), g );
  sumblklen += blklen;
  anchor = Y.x; blklen = 0; X.x = 0;
}
