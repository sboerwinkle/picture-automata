CC=gcc -Wall -O2 -g $(DEBUG)
LFLAGS=-lSDL2 -lImlib2
#ALLCOMPLEXES=complexes/box.o complexes/bullet.o complexes/person.o complexes/beetle.o complexes/gun.o complexes/elevator.o

.PHONY: clean remake debug all

MANUAL= $(shell if [ -f touchstone ]; then echo Y; else echo N; fi;)

ifeq (${MANUAL}, Y)
all: game
else
all:; @echo "Read the readme first, this isn't simple."
endif

game: main.o step.o loadRules.o transforms.o
	$(CC) main.o step.o loadRules.o transforms.o $(LFLAGS) -o game

main.o: main.c main.h step.h loadRules.h transforms.h
	$(CC) -DDRAWGRID -c main.c

step.o: step.c main.h step.h
	$(CC) -c step.c

loadRules.o: loadRules.c main.h step.h
	$(CC) -c loadRules.c

transforms.o: transforms.c
	$(CC) -c transforms.c

#complexes/%.o: complexes/%.c step.h main.h
#	$(CC) -c $< -o complexes/$*.o
#More obfuscation: $* is whatever the % matched, and $< is the first dependency, i.e. the source file.

clean:
	rm -f *.o game

remake:
	$(MAKE) clean
	$(MAKE)

debug:
	$(MAKE) DEBUG='-O0'
