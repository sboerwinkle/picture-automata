#include <stdio.h>

/*
Transforms go in the following order:

Default
90 Degree CCW
180 Degree spin
90 Degree CW (270 CCW)

Flip horizontally
Flip horiz, 90 degree CCW
Flip vertically (Flip horiz, 180 CCW)
Flip vertically, 90 CCW (Flip horiz 270 CCW)
*/

/*
This means the matrices are:

 1  0
 0  1

 0  1
-1  0

-1  0
 0 -1

 0 -1
 1  0


-1  0
 0  1

 0  1
 1  0

 1  0
 0 -1

 0 -1
-1  0
*/

/*
Which in turn means xMult is...
1	-width	-1	width	-1	width	1	-width
and yMult is...
width	1	-width	-1	width	1	-width	-1
*/

//Except this can't be hard-coded in, it's got to be calculated, at least a bit. For reasons.

int xMult[8];
int yMult[8];

void calcTransforms(int width) {
	int i;
	for (i = 0; i < 8; i++) {
		if (i & 1) {
			xMult[i] = width;
			yMult[i] = 1;
		} else {
			xMult[i] = 1;
			yMult[i] = width;
		}
	}
	for (i = 0; i < 8; i++) {
		if ( ((i>>0)&1) ^ ((i>>1)&1) ^ ((i>>2)&1) ) xMult[i] *= -1;
		if ( i & 2 ) yMult[i] *= -1;
	}
}
