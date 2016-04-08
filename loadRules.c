#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <Imlib2.h>
#include "main.h"
#include "step.h"

static int noChange[2] = {0, -1};
static int flipOnly[3] = {0, 4, -1};
static int spinOnly[5] = {0, 1, 2, 3, -1};
static int allTrans[9] = {0, 1, 2, 3, 4, 5, 6, 7, -1};

uint8_t convertColor(uint32_t data) {
	return ((data>>7)&1) | ((data>>13)&6) | ((data>>19)&24);
}

static void loadPic(char *addr, int* ruleTransforms, char apply) {
	rule *r = calloc(1, sizeof(rule));
	Imlib_Image img = imlib_load_image(addr);
	if (img == NULL) {
		printf("Unable to load image \"%s\"\n", addr);
		exit(5);
	} else puts(addr);
	imlib_context_set_image(img);
	int w = imlib_image_get_width();
	int h = imlib_image_get_height()/2;
	r->w = w;
	r->h = h;
	uint32_t *myData = imlib_image_get_data_for_reading_only();
	pt *nextRpts;
	int i, j, k, l;
	uint8_t color;
	for (i = w*h-1; i >= 0; i--) {
		if (myData[i] < 0x80000000) continue;
		r->Rlen++;
		nextRpts = malloc(sizeof(pt)*r->Rlen);
		color = convertColor(myData[i]);
		k = 0;
		l = 0;
		for (j = 0; j < color; j++) {
			k += r->colorCounts[j];
		}
		for (; j < numColors; j++) {
			l += r->colorCounts[j];
		}
		memcpy(nextRpts, r->Rpts, sizeof(pt) * k);
		memcpy(nextRpts+k+1, r->Rpts+k, sizeof(pt) * l);
		nextRpts[k].x = i%w;
		nextRpts[k].y = i/w;
		r->colorCounts[color]++;
		free(r->Rpts);
		r->Rpts = nextRpts;
	}
	myData += w*h;
	for (i = w*h-1; i >= 0; i--) {
		if (myData[i] < 0x80000000) continue;
		r->Wlen++;
		r->Wpts = realloc(r->Wpts, sizeof(pt)*r->Wlen);
		r->Wcolors = realloc(r->Wcolors, r->Wlen);
		r->Wpts[r->Wlen-1].x = i%w;
		r->Wpts[r->Wlen-1].y = i/w;
		r->Wcolors[r->Wlen-1] = ((myData[i]>>7)&1) | ((myData[i]>>13)&6) | ((myData[i]>>19)&24);
	}
	//free(myData);
	imlib_free_image();
	r->transforms = ruleTransforms;
	r->apply = apply;
	r->next = firstRule;
	firstRule = r;
}

void loadRules() {
	FILE *fin = fopen("rules/index.txt", "r");
	char name[91];
	int *transforms;
	char apply = 1;
	//6 chars for "rules/", 80 for the name, 4 for ".png", and 1 for '\0'

	if (fin != NULL) {
		name[86] = '\0';
		for (name[86]='\0'; fgets(name+5, 83, fin); name[86]='\0') {
			switch (name[5]) {
				case '#': //Process a comment
					while (name[86] != '\0' && name[86] != '\n') {
						name[86] = '\0';
						fgets(name, 88, fin);
					}
					continue;
				case '!':
					transforms = spinOnly;
					break;
				case '@':
					transforms = allTrans;
					break;
				case '$':
					transforms = flipOnly;
					break;
				case '%':
					transforms = noChange;
					break;
				case '\n':
					apply = 1;
					continue;
				default:
					printf("Unknown transform identifier '%c' from \"%s\" in index.txt\n", name[5], name+5);
					continue;
			}
			strncpy(name, "rules/", 6);
			strcpy(name+strlen(name)-1, ".png");
			loadPic(name, transforms, apply);
			apply = 0;
		}
		fclose(fin);
	} else puts("Couldn't open rules index (rules/index.txt)");
}
