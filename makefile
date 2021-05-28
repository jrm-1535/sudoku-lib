#
# Makefile for sudoku game.
#

DEBUG    := -g -DDEBUG
#OPTIMIZE := -O3
#PROFILE  := -pg -a
WARNINGS :=  -Wall -Wextra -pedantic

SUDOKUD := gtk3/

export CFLAGS := -std=c11 $(DEBUG) $(WARNINGS) $(OPTIMIZE) $(DEFINES)
export CC := gcc
AR := ar
DOC := doxygen

all: libsudoku.a sudoku

.PHONY: doc
doc: html/index.html

.PHONY: sudoku
sudoku:
	   $(MAKE) -C $(SUDOKUD)

html/index.html: sudoku.h Doxyfile
	   $(DOC)

sudoku.o:  sudoku.c sudoku.h game.h grid.h stack.h gen.h files.h debug.h

game.o:    game.c game.h grid.h stack.h sudoku.h debug.h

grid.o:    grid.c grid.h stack.h sudoku.h debug.h

stack.o:   stack.c stack.h sudoku.h debug.h

files.o:   files.c files.h grid.h sudoku.h debug.h

rand.o:    rand.c rand.h

gen.o:     gen.c gen.h grid.h game.h stack.h rand.h sudoku.h debug.h

hint.o:    hint.c hint.h hsupport.h singles.h locked.h subsets.h fishes.h xywings.h chains.h grid.h stack.h sudoku.h debug.h

singles.o:  singles.c singles.h hsupport.h grid.h sudoku.h debug.h

locked.o:  locked.c locked.h hsupport.h grid.h sudoku.h debug.h

subsets.o: subsets.c subsets.h hsupport.h grid.h sudoku.h debug.h

fishes.o: fishes.c fishes.h hsupport.h grid.h sudoku.h debug.h

xywings.o: xywings.c xywings.h hsupport.h grid.h sudoku.h debug.h

chains.o: chains.c chains.h hsupport.h grid.h sudoku.h debug.h

libsudoku.a: sudoku.o game.o grid.o stack.o files.o rand.o gen.o hint.o singles.o locked.o subsets.o fishes.o xywings.o chains.o
	   $(AR) -crs $@ $^

.PHONY: clean
clean:	  
	  rm *.[oa] sudoku
	  $(MAKE) -C $(SUDOKUD) clean
