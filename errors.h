/* Copyright (c) 2020 Stuart Steven Calder
 * See accompanying LICENSE file for licensing information.
 */
#ifndef SHIM_ERRORS_H
#define SHIM_ERRORS_H

#include "macros.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/* In general, when failing to allocate memory display this error string
 * as the default on stderr.
 */
#define SHIM_ERR_STR_ALLOC_FAILURE "Error: Generic Allocation Failure!\n"

#ifdef SHIM_OS_UNIXLIKE
/* Pretty sure all the Unixlike platforms have err.h
 */
#	include <err.h>

/* SHIM_ERRX_CODE(exit_code, message_string, additional params...)
 */
#	define SHIM_ERRX_CODE(code, ...) \
		errx( code, __VA_ARGS__ )

#else
/* On any other platform we'll just do it ourselves with fprintf
 * and exit.
 */
#	define SHIM_ERRX_CODE(code, ...) \
		do { \
			fprintf( stderr, __VA_ARGS__ ); \
			exit( code ); \
		} while( 0 )

#endif /* ~ #if defined (SHIM_OS_UNIXLIKE) ... */

#define SHIM_ERRX(...) \
	SHIM_ERRX_CODE (EXIT_FAILURE, __VA_ARGS__)

#endif /* ~ SHIM_ERRORS_H */
