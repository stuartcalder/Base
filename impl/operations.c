#include "operations.h"
#include <stdlib.h>

#define GENERIC_SHIM_XOR_(bytes) \
	void \
	shim_xor_##bytes (void * SHIM_RESTRICT mem, void const * SHIM_RESTRICT add) { \
		for( int i = 0; i < bytes; ++i ) { \
			((uint8_t *)      mem)[ i ] ^= \
			((uint8_t const *)add)[ i ]; \
		} \
	}

GENERIC_SHIM_XOR_ ( 16)
GENERIC_SHIM_XOR_ ( 32)
GENERIC_SHIM_XOR_ ( 64)
GENERIC_SHIM_XOR_ (128)

void *
shim_enforce_malloc (size_t num_bytes) {
	void * mem = malloc( num_bytes );
	if( !mem )
		SHIM_ERRX (SHIM_ERR_STR_ALLOC_FAILURE);
	return mem;
}

void *
shim_enforce_calloc (size_t num_elements, size_t element_size) {
	void * mem = calloc( num_elements, element_size );
	if( !mem )
		SHIM_ERRX (SHIM_ERR_STR_ALLOC_FAILURE);
	return mem;
}

void *
shim_enforce_realloc (void * SHIM_RESTRICT ptr, size_t size) {
	void * mem = realloc( ptr, size );
	if( !mem )
		SHIM_ERRX (SHIM_ERR_STR_ALLOC_FAILURE);
	return mem;
}

#ifndef SHIM_OPERATIONS_INLINE_OBTAIN_OS_ENTROPY
void
shim_obtain_os_entropy (uint8_t * SHIM_RESTRICT mem, size_t size)
	SHIM_OPERATIONS_OBTAIN_OS_ENTROPY_IMPL (mem, size)
#endif /* ~ ifndef SHIM_OPERATIONS_INLINE_OBTAIN_OS_ENTROPY */

#ifdef SHIM_OPERATIONS_NO_INLINE_SWAP_FUNCTIONS
uint16_t
shim_swap_16 (uint16_t u16)
	SHIM_OPERATIONS_SWAP_16_IMPL (u16)
uint32_t
shim_swap_32 (uint32_t u32)
	SHIM_OPERATIONS_SWAP_32_IMPL (u32)
uint64_t
shim_swap_64 (uint64_t u64)
	SHIM_OPERATIONS_SWAP_64_IMPL (u64)
#endif /* ~ ifdef SHIM_OPERATIONS_NO_INLINE_SWAP_FUNCTIONS */

size_t
shim_ctime_memdiff (void const * SHIM_RESTRICT mem_0,
		    void const * SHIM_RESTRICT mem_1,
		    size_t const               size)
{
	size_t unequal_count = 0;
	for( size_t i = 0; i < size; ++i ) {
		uint8_t b = ((uint8_t const *)mem_0)[ i ] ^
			    ((uint8_t const *)mem_1)[ i ];
		b = ( (b >> 7) |
		      (b >> 6) |
		      (b >> 5) |
		      (b >> 4) |
		      (b >> 3) |
		      (b >> 2) |
		      (b >> 1) |
		      (b     ) );
		unequal_count += (b & 0x01);

	}
	return unequal_count;
}

bool
shim_iszero (void const * SHIM_RESTRICT mem,
             size_t const               num_bytes)
{
	for( size_t i = 0; i < num_bytes; ++i ) {
		if( ((uint8_t const *)mem)[ i ] )
			return false;
	}
	return true;
}

bool
shim_ctime_iszero (void const * SHIM_RESTRICT mem,
		   size_t const               num_bytes)
{
	uint8_t zero_test = 0;
	for( size_t i = 0; i < num_bytes; ++i ) {
		zero_test |= ((uint8_t const *)mem)[ i ];
	}
	return !zero_test;
}
