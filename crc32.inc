
enum {
  maxwinsize=32768
};

uint CRCTab[256];

//  c = CRCTab[byte(crc)^Addr[i]^0xFF] ^ 0xFF000000 ^ (crc>>8);
uint InitCRC(void) {
  int i, j; uint c;
  for( i=0; i<256; i++ ) {
    for( c=i,j=0; j<8; j++ ) c=(c & 1) ? (c>>1)^0xEDB88320L : (c>>1);
    CRCTab[i^0xFF] = c ^ 0xFF000000;
  }
  return CRCTab[0];
}

struct InitCRCTab { InitCRCTab() { 
  InitCRC();
} } initcrctab;


struct crc {
  uint x;
  crc( void ) : x(0) {}
  void Update( byte c ) {
    x = CRCTab[byte(x)^c] ^ (x>>8);
  }
};

//--------------------------------------------------------------------------------

uint IncWinTab[256];

void IncWinTab_Init( int cursize ) {
  int i,c;
  for( c=0; c<256; c++ ) {
    crc x,y; x.Update(c);
    for( i=0; i<cursize; i++ ) x.Update(0), y.Update(0);
    IncWinTab[c] = x.x^y.x;
  }
}

struct wincrc {

  uint x,i,winsize;
  byte buf[maxwinsize];

  wincrc( int ws ) { 
    x = 0; i = 0; winsize=ws;
    int j;
    for( j=0; j<winsize; j++ ) Update0(0), buf[j]=0;
    IncWinTab_Init( winsize );
  }

  void Update0( const byte c ) {
    x = CRCTab[byte(x)^c] ^ (x>>8);
  }

  void Update( const byte c ) {
    Update0( c );
    x ^= IncWinTab[buf[i]];
    buf[i] = c;
    i = (i+1) % winsize;
  }

};

