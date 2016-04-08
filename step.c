#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "main.h"
#include "step.h"
#include "transforms.h"

typedef struct action {
	int len;
	int *locs;
	uint8_t *colors;
} action;

rule *firstRule = NULL;

static action *actions = NULL;
static int numActions = 0, maxActions = 0;

static void registerAction(rule *r, int ix, int orient) {
	if (numActions == maxActions) {
		maxActions += 5;
		actions = realloc(actions, sizeof(action)*maxActions);
	}
	action *n = actions + numActions++;
	n->len = r->Wlen;
	n->locs = malloc(sizeof(int) * n->len);
	//TODO: Make this a new array
	n->colors = r->Wcolors;
	int i;
	uint8_t *loc;
	for (i = n->len - 1; i >= 0; i--) {
		n->locs[i] = ix + xMult[orient]*r->Wpts[i].x + yMult[orient]*r->Wpts[i].y;
		loc = changeBoard + n->locs[i];
		if (*loc != 2) (*loc)++;
	}
}

static void runAllActions()
{
	int i, j, ix;
	char flag;
	action *me;
	for (i = numActions - 1; i >= 0; i--) {
		me = actions + i;

		flag = 1;
		for (j = me->len-1; j >= 0; j--) {
			ix = me->locs[j];
			if (changeBoard[ix] != 1) flag = 0;
			changeBoard[ix] = 0;
		}
		if (flag) {
			for (j = me->len-1; j >= 0; j--) {
				ix = me->locs[j];
				board[ix] = me->colors[j];
				updatePixel(ix);
			}
		}
		//TODO: Free colors
		free(me->locs);
	}
	numActions = 0;
}

static void tryOrientation(rule *r, int ix, int orient) {
	uint8_t *b = board + ix;
	int i, j = 0;
	int lim = 0;
	for (i = 0; i < numColors; i++) {
		lim += r->colorCounts[i];
		for (; j < lim; j++) {
			if (b[
				xMult[orient] * r->Rpts[j].x +
				yMult[orient] * r->Rpts[j].y
			     ] != i) return;
		}
	}
	registerAction(r, ix, orient);
}

static void tryRule(rule *r, int ix, int x, int y) {
	//Up, Right, Down, Left
	int space[4] = {ix/width, width-1-ix%width, height-1-ix/width, ix%width};
	int needed[4] = {y, r->w-1-x, r->h-1-y, x};
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
		tryOrientation(r, ix-xMult[i]*x-yMult[i]*y, i);
	}
}

void doStep()
{
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
		pt offset = r->Rpts[start];
		for (i = colorStarts[bestColor]; i >= 0; i = nextBoard[i]) {
			tryRule(r, i, offset.x, offset.y);
		}
		if (r->apply) runAllActions(); // If this isn't the case, it will be delayed and applied with others.
	}
}
