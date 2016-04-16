#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "main.h"
#include "step.h"
#include "transforms.h"

typedef struct writePair {
	int loc;
	uint8_t color;
} writePair;

typedef struct action {
	int len;
	writePair *pairs;
} action;

rule *firstRule = NULL;

static action *actions = NULL;
static int checkedActions = 0, numActions = 0, maxActions = 0;

static int ptToInt(pt* p, int orient) {
	return xMult[orient]*p->x + yMult[orient]*p->y;
}

static void registerAction(rule *r, int ix, int orient) {
	if (numActions == maxActions) {
		maxActions += 5;
		actions = realloc(actions, sizeof(action)*maxActions);
	}
	action *n = actions + numActions++;
	//n->len = r->Wlen;
	n->len = r->totWlen;
	n->pairs = malloc(sizeof(writePair) * n->len);
	int i;
	//Populate locs
	for (i = r->Wlen-1; i >= 0; i--) {
		n->pairs[i].loc = ix + xMult[orient]*r->Wpts[i].x + yMult[orient]*r->Wpts[i].y;
		n->pairs[i].color = r->Wcolors[i];
	}
	pattern *p;
	int offset = r->Wlen;
	//populate more locs
	for (p = r->patterns; p; p = p->next) {
		uint8_t color = board[ix + ptToInt(p->Rpts, orient)];
		for (i = p->Wlen-1; i >= 0; i--) {
			n->pairs[offset+i].loc = ix + ptToInt(p->Wpts+i, orient);
			n->pairs[offset+i].color = color;
		}
		offset += p->Wlen;
	}
	uint8_t *loc;
	char marker = 1; // This kind of marker means I'm good to apply
	for (i = n->len - 1; i >= 0; i--) {
		loc = changeBoard + n->pairs[i].loc;
		if (*loc) {
			if (*loc == 128) {
				free(n->pairs);
				numActions--;
				return;
			}
			marker = 2; // This kind of marker means I am not.
		}
	}
	for (i = n->len - 1; i >= 0; i--) {
		changeBoard[n->pairs[i].loc] = marker;
	}
}

static void checkActions() {
	action *me;
	int i, ix;
	while (checkedActions < numActions) {
		me = actions + checkedActions;
		for (i = me->len-1; i >= 0; i--) {
			ix = me->pairs[i].loc;
			if (changeBoard[ix] != 1) break;
			changeBoard[ix] = 128;
		}
		if (i >= 0) {
			for (i = me->len-1; i >= 0; i--) {
				changeBoard[me->pairs[i].loc] = 0;
			}
			free(me->pairs);
			*me = actions[--numActions];
		} else {
			checkedActions++;
		}
	}
}

static void runAllActions()
{
	int i, j, ix;
	action *me;
	for (i = 0; i < numActions; i++) {
		me = actions + i;
		for (j = me->len-1; j >= 0; j--) {
			ix = me->pairs[j].loc;
			changeBoard[ix] = 0;
			board[ix] = me->pairs[j].color;
			updatePixel(ix);
		}
		free(me->pairs);
	}
	numActions = 0;
	checkedActions = 0;
}

static void tryOrientation(rule *r, int ix, int orient) {
	uint8_t *b = board + ix;
	int i, j = 0;
	int lim = 0;
	for (i = 0; i < numColors; i++) {
		lim += r->colorCounts[i];
		for (; j < lim; j++) {
			if (b[ptToInt(r->Rpts+j, orient)] != i) return;
		}
	}
	pattern *p;
	for (p = r->patterns; p; p = p->next) {
		uint8_t color = b[ptToInt(p->Rpts, orient)];
		for (i = p->Rlen-1; i > 0; i--) {
			if (color != b[ptToInt(p->Rpts+i, orient)]) return;
		}
	}
	registerAction(r, ix, orient);
}

static void tryRule(rule *r, int ix, pt *p) {
	//Up, Right, Down, Left
	int space[4] = {ix/width, width-1-ix%width, height-1-ix/width, ix%width};
	int needed[4] = {p->y, r->w-1-p->x, r->h-1-p->y, p->x};
	int i, j;
	int *transform;
	for (transform = r->transforms; (i=*transform) != -1; transform++) {
		//Make sure the pattern space fits on the board in this orientation
		//If we're at 4, we're just starting the flipped orientations, so flip.
		if (i == 4) {
			int tmp = needed[1];
			needed[1] = needed[3];
			needed[3] = tmp;
		}
		//Check each direction...
		for (j = 3; j >= 0; j--) {
			if (space[j] < needed[(j+i)%4]) break;
		}
		//And bail if any failed.
		if (j >= 0) continue;
		//Now check each of the spaces in the pattern.
		tryOrientation(r, ix-ptToInt(p, i), i);
	}
}

void doStep() {
	rule *r;
	int i;
	for (r = firstRule; r; r = r->next) {
		int bestColor = -1;
		int32_t bestScore = width*height+1;
		int start;
		int total = 0;
		for (i = 0; i < numColors; i++) {
			if (r->colorCounts[i] == 0) continue;
			if (colorCounts[i] < bestScore) {
				bestScore = colorCounts[i];
				bestColor = i;
				start = total;
			}
			total += r->colorCounts[i];
		}
		if (bestColor == -1) {
			static char warningNecessary = 1;
			if (warningNecessary) {
				puts("Please remove rule with no certain inputs");
				warningNecessary = 0;
			}
			continue;
		}
		pt *offset = r->Rpts + start;
		for (i = colorStarts[bestColor]; i >= 0; i = nextBoard[i]) {
			tryRule(r, i, offset);
		}
		if (r->endOfGroup) checkActions();
	}
	runAllActions();
}
