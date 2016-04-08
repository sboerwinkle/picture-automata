I'm guessing you're only here because the Makefile wouldn't build. Don't worry, I'll be brief.



===== BASIC INVOCATION
./game
-or-
./game path/to/savefile.png

It does have to be run from the current directory, or at least somewhere with a rules/ folder.



===== COLOR FORMAT
This is important, trust me, you'll get confused.

Colors are stored with 2 bits for red, 2 bits for green, and 1 bit for blue.

This means you have:
0, 1/3, 2/3, or full RED,
0, 1/3, 2/3, or full GREEN,
0 or full BLUE

For the curious, Red has the 2 high bits and Blue has the low bit.



===== IN GAME CONTROLS
All keys affect the space under the mouse.

==Component changing
You see the grid of keys labelled ASDZXC?
The top row adds color:
A: +RED
S: +GREEN
D: +BLUE
The bottom row subtracts color:
Z: -RED
X: -GREEN
C: -BLUE

Note that trying to change any component outside its range results in a rollover affect.
For instance, increasing full Green winds up rolling Green over to 0 and increasing Red.
Enable the makefile by creating a file called touchstone in the current directory. I'd keep reading, though.

==Presets
Most of these were created for the circuitry rules.
Q: empty (black)
W: insulation (2/3 red)
E: wire off (2/3 green)
R: wire on (2/3 green, full blue)

==Time flow
I: pause
K: step (while paused)
J: slow down (increase frame delay)
L: speed up (decrease frame delay: min is 1, starts at 3)

==Saving
P: write screen to save.png. Console should generate a log message confirming save.



===== RULE FORMAT
As you probably know, this thing repeatedly applies rules to the board.
These rules are all in the rules folder, stored as png's.
The top half of each is the rule's requirements. Transparent spaces are "Don't care"s.
The bottom half is the rule's output, and indicates which spaces to change. Transparent spaces don't make changes.
For more info, see rules/index.txt, which also manages which rules are active.
