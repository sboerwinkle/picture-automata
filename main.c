#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <Imlib2.h>
#include "main.h"
#include "step.h"
#include "loadRules.h"
#include "transforms.h"

static int screenWidth = 800;
static int screenHeight = 800;

//Don't make these > 8192 (product must be storable in 26 bits)
int width = 50;
int height = 50;
static int pixelSize;
uint8_t *board, *changeBoard;
//These are used to make big doubly-linked lists for each color.
//Also, prevBoard stores which list we're in (by multiplying by colorMask, defined in main.h)
int32_t *nextBoard, *prevBoard;
int32_t colorCounts[numColors];
int32_t colorStarts[numColors];

static SDL_Window* window;
static SDL_Renderer* render;
static char running = 1;
static char simPaused = 0;
static char simStep = 0;
static int delay = 3;

char keys[numKeys] = {0};
static int keyBindings[numKeys] = {SDLK_RIGHT, SDLK_UP, SDLK_LEFT, SDLK_DOWN, SDLK_SPACE};
static int values[4] = {0, 0x55, 0xaa, 0xff};

static void save(char *file) {
	uint32_t *data = malloc(width*height*sizeof(uint32_t));
	int i;
	for (i = width*height-1; i >= 0; i--) {
		data[i] = (0xFF<<24) + (values[board[i]>>3]<<16) + (values[(board[i]>>1)&3]<<8) + (board[i]&1)*255;
	}
	Imlib_Image img = imlib_create_image_using_data(width, height, data);
	imlib_context_set_image(img);
	imlib_image_set_format("png");
	imlib_save_image(file);
	imlib_free_image();
}

static void open(char *file) {
	Imlib_Image img = imlib_load_image(file);
	imlib_context_set_image(img);
	uint32_t *data = imlib_image_get_data_for_reading_only();
	int w = imlib_image_get_width();
	int h = imlib_image_get_height();
	if (w > width || h > height) {
		//free(data);
		imlib_free_image();
		puts("Refusing to load image.");
		printf("Image dimensions of (%d, %d) exceed window dimensions of (%d, %d)\n", w, h, width, height);
		return;
	}
	int i, j;
	uint8_t *b = board;
	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			uint8_t item = convertColor(data[i*w+j]);
			if (item >= numColors) item = 0;
			b[j] = item;
		}
		b += width;
	}
	//free(data);
	imlib_free_image();
}

static void paint() {
	SDL_RenderPresent(render);
}

static void addColorToList(int ix, uint8_t color) {
	colorCounts[color]++;
	if (colorStarts[color] >= 0) {
		prevBoard[colorStarts[color]] = color*colorMask + ix;
	}
	nextBoard[ix] = colorStarts[color];
	prevBoard[ix] = color*colorMask + INT32_MIN;
	colorStarts[color] = ix;
}

static void initColorLists() {
	int i;
	for (i = numColors-1; i >= 0; i--) {
		colorCounts[i] = 0;
		colorStarts[i] = INT32_MIN;
	}
	for (i = width*height-1; i >= 0; i--) {
		addColorToList(i, board[i]);
	}
}

/*static void checkColorLists() {
	int c;
	for (c = 0; c < numColors; c++) {
		int32_t i = 0;
		int32_t l = colorStarts[c]+c*colorMask;
		int32_t n = INT32_MIN;
		while (l != INT32_MIN + c*colorMask) {
			n = l&(colorMask-1);
			if ((l&((numColors-1)*colorMask)) != c*colorMask) {
				puts("Error!");
				printf("l has a value of %X\n", l);
				return;
			}
			i++;
			l = nextBoard[n];
		}
		if (i != colorCounts[c]) printf("For color %d, expected %d, found %d\n", c, colorCounts[c], i);
		while (i--) {
			n = prevBoard[n];
		}
		if (n != INT32_MIN) printf("Color %d ended w/ prev value %d\n", c, n);
	}
	puts("===DONE===");
}*/

static void drawPixel(int ix, uint8_t color) {
	int x = ix % width;
	int y = ix / width;
	SDL_SetRenderDrawColor(render, values[color>>3], values[(color>>1)&3], 255*(color&1), 255);
	SDL_Rect rect = {.x = x * pixelSize, .y = y * pixelSize, .w = pixelSize, .h = pixelSize};
	SDL_RenderFillRect(render, &rect);
#ifdef DRAWGRID
	SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
	SDL_RenderDrawLine(render, x * pixelSize, y * pixelSize,
		x * pixelSize + pixelSize - 1, y * pixelSize);
	SDL_RenderDrawLine(render, x * pixelSize, y * pixelSize,
		x * pixelSize, y * pixelSize + pixelSize - 1);
#endif
}

void updatePixel(int ix) {
	//Remove me from my old list
	uint8_t oldColor = (prevBoard[ix]&((numColors-1)*colorMask))/colorMask;
	uint8_t color = board[ix];
	if (oldColor == color) return;
	colorCounts[oldColor]--;
	if (prevBoard[ix] >= 0) {
		nextBoard[prevBoard[ix]&(colorMask-1)] = nextBoard[ix];
	} else {
		colorStarts[oldColor] = nextBoard[ix];
	}
	if (nextBoard[ix] >= 0) {
		prevBoard[nextBoard[ix]] = prevBoard[ix];
	}
	//Add me to my next list
	addColorToList(ix, color);
	//Draw me
	drawPixel(ix, color);
}

static int keyAction(int code, char pressed)
{
	int i = numKeys;
	for (; i >= 0; i--) {
		if (code == keyBindings[i]) {
			keys[i] = pressed;
			return -100;
		}
	}
	if (!pressed) return -100;
	switch(code) {
		case SDLK_d:
			return (uint8_t)1;
		case SDLK_c:
			return (uint8_t)-1;
		case SDLK_s:
			return (uint8_t)2;
		case SDLK_x:
			return (uint8_t)-2;
		case SDLK_a:
			return (uint8_t)8;
		case SDLK_z:
			return (uint8_t)-8;
		/*case SDLK_q:
			checkColorLists();
			return 0;*/
		case SDLK_q:
			return 0;
		case SDLK_w:
			return -16;
		case SDLK_e:
			return -4;
		case SDLK_r:
			return -5;
		case SDLK_i:
			simPaused ^= 1;
			return -100;
		case SDLK_k:
			simStep = 1;
			return -100;
		case SDLK_j:
			delay++;
			return -100;
		case SDLK_l:
			if (delay > 1) delay--;
			return -100;
		case SDLK_p:
			puts("Saving to save.png...");
			save("save.png");
			puts("Done.");
			return -100;
		default:
			return -100;
	}
}

static void handleResize() {
	pixelSize = screenWidth / width;
	if (screenHeight / height < pixelSize)
		pixelSize = screenHeight / height;
	SDL_SetRenderDrawColor(render, random()%256, random()%256, random()%256, 255);
	SDL_RenderClear(render);
	int i;
	for (i = width*height - 1; i >= 0; i--) {
		drawPixel(i, (prevBoard[i]&((numColors-1)*colorMask))/colorMask);
	}
}

int main(int argc, char** argv)
{
	srandom(time(NULL));

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	window = SDL_CreateWindow("Pixels", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenWidth, screenHeight, SDL_WINDOW_RESIZABLE);
	if(window == NULL){
		fputs("No SDL2 window.\n", stderr);
		fputs(SDL_GetError(), stderr);
		SDL_Quit();
		return 1;
	}
	render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	board = calloc(width, height);
	changeBoard = calloc(width, height);
	nextBoard = malloc(sizeof(int32_t)*width*height);
	prevBoard = malloc(sizeof(int32_t)*width*height);

	if (argc > 1) open(argv[1]);

	initColorLists();
	handleResize();
	loadRules();
	calcTransforms(width);

	int mouseX = 0, mouseY = 0;

	struct timespec otherTime, lastTime, t = {.tv_sec = 0};
	clock_gettime(CLOCK_MONOTONIC, &lastTime);

	SDL_Event evnt;
	while (running){
		while (SDL_PollEvent(&evnt)) {
			if(evnt.type == SDL_QUIT) {
				running = 0;
			} else if (evnt.type == SDL_MOUSEMOTION) {
				mouseX = evnt.motion.x;
				mouseY = evnt.motion.y;
			} else if (evnt.type == SDL_KEYDOWN){
				int ret = keyAction(evnt.key.keysym.sym, 1);
				if (ret != -100) {
					int x = mouseX/pixelSize;
					if (x >= width) continue;
					int y = mouseY/pixelSize;
					if (y >= height) continue;
					x += y*width;
					if (ret > 0)
						board[x] = (board[x]+ret)&(numColors-1);
					else
						board[x] = -ret;
					updatePixel(x);
				}
			} else if (evnt.type == SDL_KEYUP) {
				keyAction(evnt.key.keysym.sym, 0);
			} else if (evnt.type == SDL_WINDOWEVENT) {
				/*if (evnt.window.event == SDL_WINDOWEVENT_SHOWN ||
					evnt.window.event == SDL_WINDOWEVENT_EXPOSED ||
					evnt.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
					evnt.window.event == SDL_WINDOWEVENT_MAXIMIZED ||
					evnt.window.event == SDL_WINDOWEVENT_RESTORED) {*/
					if (evnt.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
						screenWidth = evnt.window.data1;
						screenHeight = evnt.window.data2;
						handleResize();
					}
					//paint();
				//}
			}
		}

		if (!simPaused || simStep) {
			doStep();
			simStep = 0;
		}
		paint();

		clock_gettime(CLOCK_MONOTONIC, &otherTime);
		t.tv_nsec =
		    50000000*delay -
		    (otherTime.tv_nsec - lastTime.tv_nsec +
		    1000000000L * (otherTime.tv_sec -
				    lastTime.tv_sec));
		if (t.tv_nsec > 0) {
			nanosleep(&t, NULL);
		}
		clock_gettime(CLOCK_MONOTONIC, &lastTime);
	}
	SDL_DestroyRenderer(render);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
