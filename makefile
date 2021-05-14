#
# Makefile for sudoku game.
#

DEBUG    := -g -DDEBUG #-DSUDOKU_SOLVE_DEBUG
#OPTIMIZE := -O3
#PROFILE  := -pg -a
WARNINGS :=  -Wall -Wextra -pedantic

GLIBS    := `pkg-config --libs gtk+-3.0`
GDEFS    := `pkg-config --cflags gtk+-3.0`
GSUDOKU  := gtk3/

SUDOKUD = $(GSUDOKU)
SUDOKUL = $(GLIBS)
SUDOKUF = $(SUDOKUD)graphic.o $(SUDOKUD)dialogs.o

CFLAGS := -std=c11 $(DEBUG) $(GDEFS) $(WARNINGS) $(OPTIMIZE) $(DEFINES)
CC := gcc
AR := ar
DOC := doxygen

all: libsudoku.a sudoku

sudoku:    $(SUDOKUF) libsudoku.a
	   $(CC) $(CFLAGS) -o $@ $^ $(SUDOKUL)

doc:      sudoku.h Doxyfile
	   $(DOC)
	   touch doc

sudoku.o:  sudoku.c sudoku.h game.h grid.h stack.h gen.h files.h

game.o:    game.c game.h grid.h stack.h sudoku.h

grid.o:    grid.c stack.h sudoku.h

stack.o:   stack.c stack.h sudoku.h

files.o:   files.c  files.h sudoku.h

rand.o:    rand.c rand.h

gen.o:     gen.c gen.h grid.h stack.h rand.h sudoku.h

hint.o:    hint.c hint.h grid.h stack.h sudoku.h


libsudoku.a: sudoku.o game.o grid.o stack.o files.o rand.o gen.o hint.o
	   $(AR) -crs $@ $^

clean:	  
	  rm *.[oa] sudoku $(SUDOKUF)
