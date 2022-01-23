
typedef int8 byte;
typedef uint8 ubyte;
typedef uint16 ushort;

struct OAMEntry {
	byte xCoord; //0
	byte yCoord; //1
	ubyte startingTile; //2
	ubyte flags; //3
};
typedef struct OAMEntry OAMEntry;

struct HDMAWrite {
  // When the PPU's vertical counter hits `vcounter`,
  // write `value` into the address `(0x2100 | addr)`.
  ushort vcounter;
  byte addr;
  byte value;
};
typedef struct HDMAWrite HDMAWrite;

struct SnesFrameData {
  ubyte INIDISP;
  ubyte OBSEL;
  ushort OAMADDR;
  ubyte BGMODE;
  ubyte MOSAIC;
  ubyte BG1SC;
  ubyte BG2SC;
  ubyte BG3SC;
  ubyte BG4SC;
  ubyte BG12NBA;
  ubyte BG34NBA;
  ushort BG1HOFS;
  ushort BG1VOFS;
  ushort BG2HOFS;
  ushort BG2VOFS;
  ushort BG3HOFS;
  ushort BG3VOFS;
  ushort BG4HOFS;
  ushort BG4VOFS;
  ubyte M7SEL;
  ushort M7A;
  ushort M7B;
  ushort M7C;
  ushort M7D;
  ushort M7X;
  ushort M7Y;
  ubyte W12SEL;
  ubyte W34SEL;
  ubyte WOBJSEL;
  ubyte WH0;
  ubyte WH1;
  ubyte WH2;
  ubyte WH3;
  ubyte WBGLOG;
  ubyte WOBJLOG;
  ubyte TM;
  ubyte TS;
  ubyte TMW;
  ubyte TSW;
  ubyte CGWSEL;
  ubyte CGADSUB;
  // Since COLDATA is normally set in a weird method, we break it out
  // instead into three bytes, each containing one colour channel.
  ubyte FIXED_COLOR_DATA_R;
  ubyte FIXED_COLOR_DATA_G;
  ubyte FIXED_COLOR_DATA_B;
  ubyte SETINI;

  ushort vram[0x8000];
  ushort cgram[0x100];
  OAMEntry oam1[128];
  ubyte oam2[32];

  ushort numHdmaWrites; // The number of valid entries in `hdmaData`.
  HDMAWrite hdmaData[4*8*240];
};
typedef struct SnesFrameData SnesFrameData;
