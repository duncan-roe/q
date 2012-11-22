/*
 * tcl.h --
 *
 * This header file describes the externally-visible facilities
 * of the Tcl interpreter.
 *
 * Copyright (c) 1987-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1993-1996 Lucent Technologies.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * SCCS: @(#) tcl.h 1.326 97/11/20 12:40:43
 */

#ifndef CKALLOC_H
#define CKALLOC_H

/*
 * The following declarations either map ckalloc and ckfree to
 * malloc and free, or they map them to procedures with all sorts
 * of debugging hooks defined in tclCkalloc.c.
 */

void *  Tcl_Alloc (unsigned int size);
void    Tcl_Free (void *ptr);
void *  Tcl_Realloc (void *ptr,
				unsigned int size);

#ifdef TCL_MEM_DEBUG

#  define Tcl_Alloc(x) Tcl_DbCkalloc(x, __FILE__, __LINE__)
#  define Tcl_Free(x)  Tcl_DbCkfree(x, __FILE__, __LINE__)
#  define Tcl_Realloc(x,y) Tcl_DbCkrealloc((x), (y),__FILE__, __LINE__)
#  define ckalloc(x) Tcl_DbCkalloc(x, __FILE__, __LINE__)
#  define ckfree(x)  Tcl_DbCkfree(x, __FILE__, __LINE__)
#  define ckrealloc(x,y) Tcl_DbCkrealloc((x), (y),__FILE__, __LINE__)

int  Tcl_DumpActiveMemory (char *fileName);
void  Tcl_ValidateAllMemory (char *file,
				int line);

#else

#  define ckalloc(x) Tcl_Alloc(x)
#  define ckfree(x) Tcl_Free(x)
#  define ckrealloc(x,y) Tcl_Realloc(x,y)
#  define Tcl_DumpActiveMemory(x)
#  define Tcl_ValidateAllMemory(x,y)

#endif /* TCL_MEM_DEBUG */

void *Tcl_DbCkalloc(unsigned int size, char *file, int line);
int Tcl_DbCkfree(void *ptr, char *file, int line);
void *Tcl_DbCkrealloc(void *ptr, unsigned int size, char *file, int line);

#define malloc ckalloc
#define free ckfree
#define realloc ckrealloc

#endif /* _TCL */
