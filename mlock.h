/* Copyright (c) 2020 Stuart Steven Calder
 * See accompanying LICENSE file for licensing information.
 */
#ifndef SHIM_MLOCK_H
#define SHIM_MLOCK_H

#include "macros.h"

#ifdef ENABLE_MEMORYLOCKING_
#	error "ENABLE_MEMORYLOCKING_ already defined."
#endif

#if    (defined (SHIM_OS_UNIXLIKE) && !defined (__OpenBSD__)) || defined (SHIM_OS_WINDOWS)
#	define ENABLE_MEMORYLOCKING_
#endif /* ~ if defined (SHIM_OS_UNIXLIKE) and !defined (__OpenBSD__) or defined (SHIM_OS_WINDOWS) */

#if    defined (ENABLE_MEMORYLOCKING_) && !defined (SHIM_EXT_DISABLE_MEMORYLOCKING)
#	ifdef SHIM_FEATURE_MEMORYLOCKING
#		error "SHIM_FEATURE_MEMORYLOCKING already defined"
#	endif /* ~ ifdef SHIM_FEATURE_MEMORYLOCKING */
#	define SHIM_FEATURE_MEMORYLOCKING

#	include "errors.h"
#	include <stdint.h>
#	include <stdlib.h>

#	if    defined (SHIM_OS_UNIXLIKE)
#		include <sys/mman.h>
#	elif  defined (SHIM_OS_WINDOWS)
#		include <memoryapi.h>
#		include <windows.h>
#	else
#		error "Unsupported operating system."
#	endif /* ~ if defined (SHIM_OS_UNIXLIKE) elif defined (SHIM_OS_WINDOWS) */


SHIM_BEGIN_DECLS

SHIM_API void
shim_enforce_lock_memory (void * SHIM_RESTRICT, size_t const);

SHIM_API void
shim_enforce_unlock_memory (void * SHIM_RESTRICT, size_t const);

SHIM_API int
shim_lock_memory (void * SHIM_RESTRICT, size_t const);

SHIM_API int
shim_unlock_memory (void * SHIM_RESTRICT, size_t const);

SHIM_END_DECLS

#endif /* ~ if defined (ENABLE_MEMORYLOCKING_) and !defined (SHIM_EXT_DISABLE_MEMORYLOCKING) */
#undef ENABLE_MEMORYLOCKING_

#endif /* ~ ifndef SHIM_MLOCK_H */
