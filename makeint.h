/* Miscellaneous global declarations and portability cruft for GNU Make.
Copyright (C) 1988-2014 Free Software Foundation, Inc.
This file is part of GNU Make.

GNU Make is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation; either version 3 of the License, or (at your option) any later
version.

GNU Make is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* We use <config.h> instead of "config.h" so that a compilation
   using -I. -I$srcdir will use ./config.h rather than $srcdir/config.h
   (which it would do because makeint.h was found in $srcdir).  */
#include <config.h>

/* Specify we want GNU source code.  This must be defined before any
   system headers are included.  */

#define _GNU_SOURCE 1

# include <alloca.h>

/* Include the externally-visible content.
   Be sure to use the local one, and not one installed on the system.
   Define GMK_BUILDING_MAKE for proper selection of dllexport/dllimport
   declarations for MS-Windows.  */
#include "gnumake.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>   
#include <errno.h>
#include <stdint.h>

#ifndef isblank
# define isblank(c)     ((c) == ' ' || (c) == '\t')
#endif

#include <unistd.h>
#include <limits.h>
#include <sys/param.h>

#define GET_PATH_MAX   PATH_MAX
#define PATH_VAR(var)  char var[PATH_MAX]

#define IS_ABSOLUTE(n) (n[0] == '/')
#define ROOT_LEN 1

#define FILE_TIMESTAMP uintmax_t
#define RETSIGTYPE     void

#define MAP_VMSCOMMA   0x0000

/* Nonzero if the integer type T is signed.
   Use <= to avoid GCC warnings about always-false expressions.  */
#define INTEGER_TYPE_SIGNED(t) ((t) -1 <= 0)

/* The minimum and maximum values for the integer type T.
   Use ~ (t) 0, not -1, for portability to 1's complement hosts.  */
#define INTEGER_TYPE_MINIMUM(t) \
  (! INTEGER_TYPE_SIGNED (t) ? (t) 0 : ~ (t) 0 << (sizeof (t) * CHAR_BIT - 1))
#define INTEGER_TYPE_MAXIMUM(t) (~ (t) 0 - INTEGER_TYPE_MINIMUM (t))

#ifndef CHAR_MAX
# define CHAR_MAX INTEGER_TYPE_MAXIMUM (char)
#endif

#define UNUSED  __attribute__ ((unused))


/* ISDIGIT offers the following features:
   - Its arg may be any int or unsigned int; it need not be an unsigned char.
   - It's guaranteed to evaluate its argument exactly once.
      NOTE!  Make relies on this behavior, don't change it!
   - It's typically faster.
   POSIX 1003.2-1992 section 2.5.2.1 page 50 lines 1556-1558 says that
   only '0' through '9' are digits.  Prefer ISDIGIT to isdigit() unless
   it's important to use the locale's definition of 'digit' even when the
   host does not conform to POSIX.  */
#define ISDIGIT(c) ((unsigned) (c) - '0' <= 9)

/* Test if two strings are equal. Is this worthwhile?  Should be profiled.  */
#define streq(a, b) \
   ((a) == (b) || \
    (*(a) == *(b) && (*(a) == '\0' || !strcmp ((a) + 1, (b) + 1))))

/* Test if two strings are equal, but match case-insensitively on systems
   which have case-insensitive filesystems.  Should only be used for
   filenames!  */
#define patheq(a, b) streq(a, b)

#define strneq(a, b, l) (strncmp ((a), (b), (l)) == 0)

#if defined(__GNUC__) || defined(ENUM_BITFIELDS)
# define ENUM_BITFIELD(bits)    :bits
#else
# define ENUM_BITFIELD(bits)
#endif

/* Handle gettext and locales.  */

#include <locale.h>

#define _(msgid)  msgid
#define N_(msgid) msgid
#define S_(msg1,msg2,num)  msgid

#define ANY_SET(_v,_m)  (((_v)&(_m)) != 0)
#define NONE_SET(_v,_m) (! ANY_SET ((_v),(_m)))

#define MAP_NUL         0x0001
#define MAP_BLANK       0x0002
#define MAP_SPACE       0x0004
#define MAP_COMMENT     0x0008
#define MAP_SEMI        0x0010
#define MAP_EQUALS      0x0020
#define MAP_COLON       0x0040
#define MAP_PERCENT     0x0080
#define MAP_PIPE        0x0100
#define MAP_DOT         0x0200
#define MAP_COMMA       0x0400

/* These are the valid characters for a user-defined function.  */
#define MAP_USERFUNC    0x2000
/* This means not only a '$', but skip the variable reference.  */
#define MAP_VARIABLE    0x4000
/* The set of characters which are directory separators is OS-specific.  */
#define MAP_DIRSEP      0x8000

#define STOP_SET(_v,_m) ANY_SET (stopchar_map[(unsigned char)(_v)],(_m))

#include <sys/resource.h>

#define NILF ((gmk_floc *)0)

#define CSTRLEN(_s)           (sizeof (_s)-1)
#define STRING_SIZE_TUPLE(_s) (_s), CSTRLEN(_s)

/* The number of bytes needed to represent the largest integer as a string.  */
#define INTSTR_LENGTH         CSTRLEN ("18446744073709551616")

#define DEFAULT_TTYNAME "true"
#ifdef HAVE_TTYNAME
# define TTYNAME(_f) ttyname (_f)
#else
# define TTYNAME(_f) DEFAULT_TTYNAME
#endif


const char *concat (unsigned int, ...);
void message (int prefix, size_t length, const char *fmt, ...)
              __attribute__ ((__format__ (__printf__, 3, 4)));
void error (const gmk_floc *flocp, size_t length, const char *fmt, ...)
            __attribute__ ((__format__ (__printf__, 3, 4)));
void fatal (const gmk_floc *flocp, size_t length, const char *fmt, ...)
                   __attribute__ ((noreturn, __format__ (__printf__, 3, 4)));

#define O(_t,_a,_f)           _t((_a), 0, (_f))
#define OS(_t,_a,_f,_s)       _t((_a), strlen (_s), (_f), (_s))
#define OSS(_t,_a,_f,_s1,_s2) _t((_a), strlen (_s1) + strlen (_s2), \
                                 (_f), (_s1), (_s2))
#define OSSS(_t,_a,_f,_s1,_s2,_s3) _t((_a), strlen (_s1) + strlen (_s2) + strlen (_s3), \
                                      (_f), (_s1), (_s2), (_s3))
#define ON(_t,_a,_f,_n)       _t((_a), INTSTR_LENGTH, (_f), (_n))
#define ONN(_t,_a,_f,_n1,_n2) _t((_a), INTSTR_LENGTH*2, (_f), (_n1), (_n2))

#define OSN(_t,_a,_f,_s,_n)   _t((_a), strlen (_s) + INTSTR_LENGTH, \
                                 (_f), (_s), (_n))
#define ONS(_t,_a,_f,_n,_s)   _t((_a), INTSTR_LENGTH + strlen (_s), \
                                 (_f), (_n), (_s))

#define OUT_OF_MEM() O (fatal, NILF, _("virtual memory exhausted"))

void die (int) __attribute__ ((noreturn));
void pfatal_with_name (const char *) __attribute__ ((noreturn));
void perror_with_name (const char *, const char *);
#define xstrlen(_s) ((_s)==NULL ? 0 : strlen (_s))
void *xmalloc (unsigned int);
void *xcalloc (unsigned int);
void *xrealloc (void *, unsigned int);
char *xstrdup (const char *);
char *xstrndup (const char *, unsigned int);
char *find_next_token (const char **, unsigned int *);
char *next_token (const char *);
char *end_of_token (const char *);
void collapse_continuations (char *);
char *lindex (const char *, const char *, int);
int alpha_compare (const void *, const void *);
void print_spaces (unsigned int);
char *find_percent (char *);
const char *find_percent_cached (const char **);

#ifndef NO_ARCHIVES
int ar_name (const char *);
void ar_parse_name (const char *, char **, char **);
int ar_touch (const char *);
time_t ar_member_date (const char *);

typedef long int (*ar_member_func_t) (int desc, const char *mem, int truncated,
                                      long int hdrpos, long int datapos,
                                      long int size, long int date, int uid,
                                      int gid, int mode, const void *arg);

long int ar_scan (const char *archive, ar_member_func_t function, const void *arg);
int ar_name_equal (const char *name, const char *mem, int truncated);
#ifndef VMS
int ar_member_touch (const char *arname, const char *memname);
#endif
#endif

int dir_file_exists_p (const char *, const char *);
int file_exists_p (const char *);
int file_impossible_p (const char *);
void file_impossible (const char *);
const char *dir_name (const char *);
void hash_init_directories (void);

void define_default_variables (void);
void undefine_default_variables (void);
void set_default_suffixes (void);
void install_default_suffix_rules (void);
void install_default_implicit_rules (void);

void build_vpath_lists (void);
void construct_vpath_list (char *pattern, char *dirpath);
const char *vpath_search (const char *file, FILE_TIMESTAMP *mtime_ptr,
                          unsigned int* vpath_index, unsigned int* path_index);
int gpath_search (const char *file, unsigned int len);

void construct_include_path (const char **arg_dirs);

void user_access (void);
void make_access (void);
void child_access (void);

char *strip_whitespace (const char **begpp, const char **endpp);

/* String caching  */
void strcache_init (void);
void strcache_print_stats (const char *prefix);
int strcache_iscached (const char *str);
const char *strcache_add (const char *str);
const char *strcache_add_len (const char *str, unsigned int len);

/* Guile support  */
int guile_gmake_setup (const gmk_floc *flocp);

/* Loadable object support.  Sets to the strcached name of the loaded file.  */
typedef int (*load_func_t)(const gmk_floc *flocp);
int load_file (const gmk_floc *flocp, const char **filename, int noerror);
void unload_file (const char *name);

#define OUTPUT_SYNC_NONE    0
#define OUTPUT_SYNC_LINE    1
#define OUTPUT_SYNC_TARGET  2
#define OUTPUT_SYNC_RECURSE 3

extern const gmk_floc *reading_file;
extern const gmk_floc **expanding_var;

extern char **environ;

extern unsigned short stopchar_map[];

extern int just_print_flag, silent_flag, ignore_errors_flag, keep_going_flag;
extern int print_data_base_flag, question_flag, touch_flag, always_make_flag;
extern int env_overrides, no_builtin_rules_flag, no_builtin_variables_flag;
extern int print_version_flag, print_directory_flag, check_symlink_flag;
extern int warn_undefined_variables_flag, trace_flag, posix_pedantic;
extern int not_parallel, second_expansion, clock_skew_detected;
extern int rebuilding_makefiles, one_shell, output_sync, verify_flag;

/* can we run commands via 'sh -c xxx' or must we use batch files? */
extern int batch_mode_shell;

/* Resetting the command script introduction prefix character.  */
#define RECIPEPREFIX_NAME          ".RECIPEPREFIX"
#define RECIPEPREFIX_DEFAULT       '\t'
extern char cmd_prefix;

extern unsigned int job_slots;
extern int job_fds[2];
extern int job_rfd;
#ifndef NO_FLOAT
extern double max_load_average;
#else
extern int max_load_average;
#endif

extern const char *program;
extern char *starting_directory;
extern unsigned int makelevel;
extern char *version_string, *remote_description, *make_host;

extern unsigned int commands_started;

extern int handling_fatal_signal;


#ifndef MIN
#define MIN(_a,_b) ((_a)<(_b)?(_a):(_b))
#endif
#ifndef MAX
#define MAX(_a,_b) ((_a)>(_b)?(_a):(_b))
#endif


#define MAKE_SUCCESS 0
#define MAKE_TROUBLE 1
#define MAKE_FAILURE 2

/* Set up heap debugging library dmalloc.  */

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#ifndef initialize_main
# ifdef __EMX__
#  define initialize_main(pargc, pargv) \
                          { _wildcard(pargc, pargv); _response(pargc, pargv); }
# else
#  define initialize_main(pargc, pargv)
# endif
#endif

#ifndef initialize_main
# define initialize_main(pargc, pargv)
#endif


/* Some systems (like Solaris, PTX, etc.) do not support the SA_RESTART flag
   properly according to POSIX.  So, we try to wrap common system calls with
   checks for EINTR.  Note that there are still plenty of system calls that
   can fail with EINTR but this, reportedly, gets the vast majority of
   failure cases.  If you still experience failures you'll need to either get
   a system where SA_RESTART works, or you need to avoid -j.  */

#define EINTRLOOP(_v,_c)   while (((_v)=_c)==-1 && errno==EINTR)

/* While system calls that return integers are pretty consistent about
   returning -1 on failure and setting errno in that case, functions that
   return pointers are not always so well behaved.  Sometimes they return
   NULL for expected behavior: one good example is readdir() which returns
   NULL at the end of the directory--and _doesn't_ reset errno.  So, we have
   to do it ourselves here.  */

#define ENULLLOOP(_v,_c)   do { errno = 0; (_v) = _c; } \
                           while((_v)==0 && errno==EINTR)
