#
# Makefile for sudoku game.
#

DEBUG    := -g -DDEBUG #-DSUDOKU_SOLVE_DEBUG
#OPTIMIZE := -O3
#PROFILE  := -pg -a
WARNINGS :=  -Wall -pedantic

X11LIB   :=  -L/usr/X11R6/lib -lX11
XFORMS   := -lm -lXpm  -L/usr/local/lib -lforms
XSUDOKU  := xforms/

GLIBS    := `pkg-config --libs gtk+-3.0`
GDEFS    := `pkg-config --cflags gtk+-3.0`
GSUDOKU  := gtk3/

#SUDOKUD = $(XSUDOKU)
#SUDOKUL = $(XFORMS) $(X11LIB)
#SUDOKUF = $(SUDOKUD)graphic.o $(SUDOKUD)filesel.o $(SUDOKUD)alert.o

SUDOKUD = $(GSUDOKU)
SUDOKUL = $(GLIBS)
SUDOKUF = $(SUDOKUD)graphic.o $(SUDOKUD)dialogs.o

CFLAGS := -std=c11 $(DEBUG) $(GDEFS) $(WARNINGS) $(OPTIMIZE) $(DEFINES)
CC := gcc
DOC := doxygen

all: sudoku thint

sudoku:    game.o rand.o gen.o hint.o state.o stack.o files.o $(SUDOKUF)
	   $(CC) $(CFLAGS) -o $@ $^ $(SUDOKUL)

thint:     thint.o hint.o state.o stack.o files.o gen.o rand.o $(SUDOKUF)
	   $(CC) $(CFLAGS) -o $@ $^ $(SUDOKUL)

doc:      sudoku.h Doxyfile
	   $(DOC)
	   touch doc

thint.o:   thint.c sudoku.h state.h stack.h files.h

game.o:    game.c game.h sudoku.h gen.h sudoku.h state.h stack.h files.h rand.h

stack.o:   stack.c stack.h sudoku.h

state.o:   state.c state.h stack.h sudoku.h gen.h

rand.o:    rand.c rand.h

gen.o:     gen.c gen.h sudoku.h rand.h state.h stack.h

hint.o:    hint.c hint.h sudoku.h state.h stack.h

files.o:   files.c game.h sudoku.h files.h

clean:	  
	  rm *.o sudoku thint $(SUDOKUF)
