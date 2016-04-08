
//If this changes, we also need to change all the color encoding and decoding, and maybe more who knows
//2^5 = 32
#define numColors 32
//31-5 = 26
#define colorMask (1<<26)

#define numKeys 5
char keys[numKeys];

extern int width;
extern int height;
extern uint8_t *board, *changeBoard;

extern int *nextBoard, *prevBoard;
extern int colorCounts[numColors];
extern int colorStarts[numColors];

extern void updatePixel(int ix);
