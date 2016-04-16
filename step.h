typedef struct {
	int16_t x, y;
} pt;

typedef struct pattern {
	struct pattern *next;
	int gray; // Only used when reading them in from file
	int Rlen;
	pt *Rpts;
	int Wlen;
	pt *Wpts;
} pattern;

typedef struct rule{
	int w, h;
	int *transforms;
	char endOfGroup;
	//uint8_t group;
	int colorCounts[numColors];
	int Rlen;
	pt *Rpts;
	int Wlen;
	int totWlen; // including all the pattern elements
	pt *Wpts;
	pattern *patterns;
	uint8_t *Wcolors;
	struct rule *next;
} rule;

extern rule *firstRule;

extern void doStep();
