/*
  sudoku files.h

  Suduku files: file related declarations
*/

#ifndef __FILES_H__
#define __FILES_H__

/** path separator for different systems */
#ifndef DOS_STYLE_SEPARATOR
#define PATH_SEPARATOR        '/'
#else
#define PATH_SEPARATOR        '\\'
#endif

extern bool load_file( const char *path );

#endif /* __FILES_H__ */
