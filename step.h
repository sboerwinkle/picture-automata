typedef struct {
	int16_t x, y;
} pt;

typedef struct rule{
	int w, h;
	int *transforms;
	char apply;
	int colorCounts[numColors];
	int Rlen;
	pt *Rpts;
	int Wlen;
	pt *Wpts;
	uint8_t *Wcolors;
	struct rule *next;
} rule;

extern rule *firstRule;

extern void doStep();
