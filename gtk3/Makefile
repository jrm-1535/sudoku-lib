

GLIBS    := `pkg-config --libs gtk+-3.0`
GDEFS    := `pkg-config --cflags gtk+-3.0`

CFLAGS += $(GDEFS)
SUDOKUL = $(GLIBS)

SUDOKUF = dialogs.o graphic.o

all: ../sudoku

dialogs.o:   dialogs.c dialog.h ../sudoku.h

graphic.o:  graphic.c ../sudoku.h

../sudoku:    $(SUDOKUF) ../libsudoku.a
	   $(CC) $(CFLAGS) -o $@ $^ $(SUDOKUL)

clean:	  
	  rm *.o
