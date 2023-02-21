/* Copyright (c) 2020-2022 Stuart Steven Calder
 * See accompanying LICENSE file for licensing information.
 */
#include "mlock.h"
#include "mem.h"
#include "errors.h"
#ifdef BASE_MLOCK_H /* When memorylocking is disabled, the entire header is discarded. */
#if defined(BASE_OS_UNIXLIKE)
 #include <sys/types.h>
 #include <sys/time.h>
 #include <sys/resource.h>
 #include <sys/mman.h>
 #ifndef RLIMIT_MEMLOCK
  #error "RLIMIT_MEMLOCK not defined!"
 #endif
#elif defined(BASE_OS_WINDOWS)
 #include "files.h"
 #include <windows.h>
 #include <memoryapi.h>
#else
 #error "Unsupported."
#endif

Base_MLock Base_MLock_g;

#define R_ BASE_RESTRICT

/* Static prototypes. */
static uint64_t num_locked_bytes_(
/* Calculate the number of bytes locked, when we lock
 * @n many virtual memory pages of size @page_size. */
 uint64_t n, uint64_t page_size);

/* Implementations. */
Base_Error_t Base_MLock_init(Base_MLock* ml)
{
  ml->page_size = (uint64_t)Base_get_pagesize();
#if defined(BASE_OS_UNIXLIKE)
  {
    struct rlimit rl;
    if (getrlimit(RLIMIT_MEMLOCK, &rl))
      return BASE_MLOCK_ERR_GET_LIMIT;
    if (rl.rlim_max > rl.rlim_cur) {
      rl.rlim_cur = rl.rlim_max;
    if (setrlimit(RLIMIT_MEMLOCK, &rl))
      return BASE_MLOCK_ERR_SET_LIMIT;
    }
    ml->limit = (uint64_t)rl.rlim_cur;
  }
#elif defined(BASE_OS_WINDOWS)
  {
    Base_File_t proc = GetCurrentProcess();
    SIZE_T minimum, maximum;
    if (!GetProcessWorkingSetSize(proc, &minimum, &maximum))
      return BASE_MLOCK_ERR_GET_LIMIT;
    /* FIXME: The win32 documentation is vague about how much memory
     * precisely is allowed to be locked at once. */
    ml->limit = (uint64_t)minimum - ml->page_size;
  }
#else
 #error "Unsupported."
#endif
  ml->n = 0;
  #ifdef BASE_EXTERN_MLOCK_THREADSAFE
  if (pthread_mutex_init(&ml->n_mtx, BASE_NULL))
    return BASE_MLOCK_ERR_MTX_INIT;
  #endif
  return 0;
}

#undef  ERR_
#define ERR_ "Error: Base_MLock_init_handled: %s\n"

void Base_MLock_init_handled(Base_MLock* ml)
{
  const int c = Base_MLock_init(ml);
  switch (c) {
  case 0:
    return;
  case BASE_MLOCK_ERR_GET_LIMIT:
    Base_errx(ERR_, "Failed to get memory limit.");
    return; /* This return will never happen but supresses compiler warnings. */
  #if BASE_MLOCK_INIT_MAYRETURN_ERR_SET_LIMIT
  case BASE_MLOCK_ERR_SET_LIMIT:
    Base_errx(ERR_, "Failed to increase memory limit.");
    return; /* This return will never happen but supresses compiler warnings. */
  #endif
  #if BASE_MLOCK_INIT_MAYRETURN_ERR_MTX_INIT
  case BASE_MLOCK_ERR_MTX_INIT:
    Base_errx(ERR_, "Failed to initialize n mutex.");
    return; /* This return will never happen but supresses compiler warnings. */
  #endif
  }
  Base_errx(ERR_, "Invalid Base_MLock_init return code.");
}

/* When enabled externally, these macros lock the
 * counter mutex of the Base_MLock pointer @Ml_Ptr.
 * Must be used in a procedure returning int for error codes.*/
#ifdef BASE_EXTERN_MLOCK_THREADSAFE
 #define LOCK_N_MTX_(Ml_Ptr) do {\
  if (pthread_mutex_lock(&Ml_Ptr->n_mtx))\
   return BASE_MLOCK_ERR_MTX_OP;\
 } while (0)
 #define UNLOCK_N_MTX_(Ml_Ptr) do {\
  if (pthread_mutex_unlock(&Ml_Ptr->n_mtx))\
   return BASE_MLOCK_ERR_MTX_OP;\
 } while (0)
#else
 #define LOCK_N_MTX_(Ml_Ptr)   /* Nil */
 #define UNLOCK_N_MTX_(Ml_Ptr) /* Nil */
#endif

uint64_t num_locked_bytes_(uint64_t n, uint64_t page_size)
{
  uint64_t locked = n / page_size; /* Floor division. The number of whole pages. */
  if (n % page_size) /* If n is not evenly divisible into pages. */
    ++locked; /* We lock all pages along the covered boundaries. */
  BASE_ASSERT_MSG((locked * page_size) > locked, "Overflow!\n");
  return locked * page_size; /* Number pages x Size of each page in bytes. */
}

Base_Error_t Base_mlock_ctx(void* R_ p, uint64_t n, Base_MLock* R_ ctx)
{
  const uint64_t locked = num_locked_bytes_(n, ctx->page_size); /* How many bytes would be locked? */
  LOCK_N_MTX_(ctx); /* Lock mutex. */
  if ((locked + ctx->n) > ctx->limit) {
    /* Went over the limit. */
    UNLOCK_N_MTX_(ctx);
    return BASE_MLOCK_ERR_OVER_MEMLIMIT;
  }
#if defined(BASE_OS_UNIXLIKE)
  if (mlock(p, n)) {
    UNLOCK_N_MTX_(ctx);
    return BASE_MLOCK_ERR_LOCK_OP  ;
  }
#elif defined(BASE_OS_WINDOWS)
  if (!VirtualLock((LPVOID)p, (SIZE_T)n)) {
    UNLOCK_N_MTX_(ctx);
    return BASE_MLOCK_ERR_LOCK_OP;
  }
#else
 #error "Unsupported."
#endif
  ctx->n += locked;
  UNLOCK_N_MTX_(ctx); /* Unlock mutex. */
  return 0;
}

#undef  ERR_
#define ERR_ "Error: Base_mlock_ctx_handled: %s\n"

void Base_mlock_ctx_handled(void* R_ p, uint64_t n, Base_MLock* R_ ctx, Base_MLock_F_t f)
{
  const int c = Base_mlock_ctx(p, n, ctx);
  switch (c) {
  case 0:
    return;
  #ifdef BASE_EXTERN_MLOCK_THREADSAFE
  case BASE_MLOCK_ERR_MTX_OP:
    Base_errx(ERR_, "Failed to lock or unlock mutex.");
    return;
  #endif
  case BASE_MLOCK_ERR_LOCK_OP:
    if (f & BASE_MLOCK_F_GRACEFUL_LOCK_FAIL)
      return;
    Base_errx(ERR_, "Failed to memlock or memunlock.");
    return;
  case BASE_MLOCK_ERR_OVER_MEMLIMIT:
    if (f & BASE_MLOCK_F_GRACEFUL_OVERMEMLIMIT_FAIL)
      return;
    Base_errx(ERR_, "Went over memory locking limit.");
    return;
  }
  Base_errx(ERR_, "Invalid Base_mlock_ctx return code.");
}

Base_Error_t Base_munlock_ctx(void* R_ p, uint64_t n, Base_MLock* R_ ctx)
{
  const uint64_t locked = num_locked_bytes_(n, ctx->page_size);
  LOCK_N_MTX_(ctx);
  if (locked > ctx->n) {
    UNLOCK_N_MTX_(ctx);
    return BASE_MLOCK_ERR_UNDER_MEMMIN;
  }
#if defined(BASE_OS_UNIXLIKE)
  if (munlock(p, n)) {
    UNLOCK_N_MTX_(ctx);
    return BASE_MLOCK_ERR_LOCK_OP;
  }
#elif defined(BASE_OS_WINDOWS)
  if (!VirtualUnlock((LPVOID)p, (SIZE_T)n)) {
    UNLOCK_N_MTX_(ctx);
    return BASE_MLOCK_ERR_LOCK_OP;
  }
#else
 #error "Unsupported."
#endif
  ctx->n -= locked;
  UNLOCK_N_MTX_(ctx);
  return 0;
}

#undef  ERR_
#define ERR_ "Error: Base_munlock_ctx_handled: %s\n"

void Base_munlock_ctx_handled(void* R_ p, uint64_t n, Base_MLock* R_ ctx, Base_MLock_F_t f)
{
  const int c = Base_munlock_ctx(p, n, ctx);
  switch (c) {
  case 0:
    return;
  #ifdef BASE_EXTERN_MLOCK_THREADSAFE
  case BASE_MLOCK_ERR_MTX_OP:
    Base_errx(ERR_, "Failed to lock or unlock mutex.");
    return;
  #endif
  case BASE_MLOCK_ERR_LOCK_OP:
    if (f & BASE_MLOCK_F_GRACEFUL_UNLOCK_FAIL)
      return;
    Base_errx(ERR_, "Failed to memlock or memunlock.");
    return;
  case BASE_MLOCK_ERR_UNDER_MEMMIN:
    Base_errx(ERR_, "Went under minimum memory locking limit.");
    return;
  }
  Base_errx(ERR_, "Invalid Base_munlock_ctx return code.");
}

#endif /* ~ ifdef BASE_MLOCK_H */
