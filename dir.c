/* Directory hashing for GNU Make.
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

#include "makeint.h"
#include "hash.h"
#include "filedef.h"
#include "dep.h"

#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)

# define REAL_DIR_ENTRY(dp) 1
# define FAKE_DIR_ENTRY(dp)

/* Hash table of directories.  */

#ifndef DIRECTORY_BUCKETS
#define DIRECTORY_BUCKETS 199
#endif

struct directory_contents
  {
    dev_t dev;                  /* Device and inode numbers of this dir.  */
    ino_t ino;
    struct hash_table dirfiles; /* Files in this directory.  */
    DIR *dirstream;             /* Stream reading this directory.  */
  };

static unsigned long
directory_contents_hash_1 (const void *key_0)
{
  const struct directory_contents *key = key_0;
  unsigned long hash;

  hash = ((unsigned int) key->dev << 4) ^ (unsigned int) key->ino;

  return hash;
}

static unsigned long
directory_contents_hash_2 (const void *key_0)
{
  const struct directory_contents *key = key_0;
  unsigned long hash;

  hash = ((unsigned int) key->dev << 4) ^ (unsigned int) ~key->ino;

  return hash;
}

/* Sometimes it's OK to use subtraction to get this value:
     result = X - Y;
   But, if we're not sure of the type of X and Y they may be too large for an
   int (on a 64-bit system for example).  So, use ?: instead.
   See Savannah bug #15534.

   NOTE!  This macro has side-effects!
*/

#define MAKECMP(_x,_y)  ((_x)<(_y)?-1:((_x)==(_y)?0:1))

static int
directory_contents_hash_cmp (const void *xv, const void *yv)
{
  const struct directory_contents *x = xv;
  const struct directory_contents *y = yv;
  int result;

  result = MAKECMP(x->ino, y->ino);
  if (result)
    return result;

  return MAKECMP(x->dev, y->dev);
}

/* Table of directory contents hashed by device and inode number.  */
static struct hash_table directory_contents;

struct directory
  {
    const char *name;                   /* Name of the directory.  */

    /* The directory's contents.  This data may be shared by several
       entries in the hash table, which refer to the same directory
       (identified uniquely by 'dev' and 'ino') under different names.  */
    struct directory_contents *contents;
  };

static unsigned long
directory_hash_1 (const void *key)
{
  return_ISTRING_HASH_1 (((const struct directory *) key)->name);
}

static unsigned long
directory_hash_2 (const void *key)
{
  return_ISTRING_HASH_2 (((const struct directory *) key)->name);
}

static int
directory_hash_cmp (const void *x, const void *y)
{
  return_ISTRING_COMPARE (((const struct directory *) x)->name,
                          ((const struct directory *) y)->name);
}

/* Table of directories hashed by name.  */
static struct hash_table directories;

/* Never have more than this many directories open at once.  */

#define MAX_OPEN_DIRECTORIES 10

static unsigned int open_directories = 0;


/* Hash table of files in each directory.  */

struct dirfile
  {
    const char *name;           /* Name of the file.  */
    short length;
    short impossible;           /* This file is impossible.  */
  };

static unsigned long
dirfile_hash_1 (const void *key)
{
  return_ISTRING_HASH_1 (((struct dirfile const *) key)->name);
}

static unsigned long
dirfile_hash_2 (const void *key)
{
  return_ISTRING_HASH_2 (((struct dirfile const *) key)->name);
}

static int
dirfile_hash_cmp (const void *xv, const void *yv)
{
  const struct dirfile *x = xv;
  const struct dirfile *y = yv;
  int result = x->length - y->length;
  if (result)
    return result;
  return_ISTRING_COMPARE (x->name, y->name);
}

#ifndef DIRFILE_BUCKETS
#define DIRFILE_BUCKETS 107
#endif

static int dir_contents_file_exists_p (struct directory_contents *dir,
                                       const char *filename);
static struct directory *find_directory (const char *name);

/* Find the directory named NAME and return its 'struct directory'.  */

static struct directory *
find_directory (const char *name)
{
  struct directory *dir;
  struct directory **dir_slot;
  struct directory dir_key;

  dir_key.name = name;
  dir_slot = (struct directory **) hash_find_slot (&directories, &dir_key);
  dir = *dir_slot;

  if (HASH_VACANT (dir))
    {
      /* The directory was not found.  Create a new entry for it.  */
      const char *p = name + strlen (name);
      struct stat st;
      int r;

      dir = xmalloc (sizeof (struct directory));
      dir->name = strcache_add_len (name, p - name);

      hash_insert_at (&directories, dir, dir_slot);
      /* The directory is not in the name hash table.
         Find its device and inode numbers, and look it up by them.  */

      EINTRLOOP (r, stat (name, &st));

      if (r < 0)
        {
        /* Couldn't stat the directory.  Mark this by
           setting the 'contents' member to a nil pointer.  */
          dir->contents = 0;
        }
      else
        {
          /* Search the contents hash table; device and inode are the key.  */

          struct directory_contents *dc;
          struct directory_contents **dc_slot;
          struct directory_contents dc_key;

          dc_key.dev = st.st_dev;
          dc_key.ino = st.st_ino;
          dc_slot = (struct directory_contents **) hash_find_slot (&directory_contents, &dc_key);
          dc = *dc_slot;

          if (HASH_VACANT (dc))
            {
              /* Nope; this really is a directory we haven't seen before.  */
              dc = (struct directory_contents *)
                xmalloc (sizeof (struct directory_contents));

              /* Enter it in the contents hash table.  */
              dc->dev = st.st_dev;
              dc->ino = st.st_ino;

              hash_insert_at (&directory_contents, dc, dc_slot);
              ENULLLOOP (dc->dirstream, opendir (name));
              if (dc->dirstream == 0)
                /* Couldn't open the directory.  Mark this by setting the
                   'files' member to a nil pointer.  */
                dc->dirfiles.ht_vec = 0;
              else
                {
                  hash_init (&dc->dirfiles, DIRFILE_BUCKETS,
                             dirfile_hash_1, dirfile_hash_2, dirfile_hash_cmp);
                  /* Keep track of how many directories are open.  */
                  ++open_directories;
                  if (open_directories == MAX_OPEN_DIRECTORIES)
                    /* We have too many directories open already.
                       Read the entire directory and then close it.  */
                    dir_contents_file_exists_p (dc, 0);
                }
            }

          /* Point the name-hashed entry for DIR at its contents data.  */
          dir->contents = dc;
        }
    }

  return dir;
}

/* Return 1 if the name FILENAME is entered in DIR's hash table.
   FILENAME must contain no slashes.  */

static int
dir_contents_file_exists_p (struct directory_contents *dir,
                            const char *filename)
{
  struct dirfile *df;
  struct dirent *d;

  if (dir == 0 || dir->dirfiles.ht_vec == 0)
    /* The directory could not be stat'd or opened.  */
    return 0;

  if (filename != 0)
    {
      struct dirfile dirfile_key;

      if (*filename == '\0')
        {
          /* Checking if the directory exists.  */
          return 1;
        }
      dirfile_key.name = filename;
      dirfile_key.length = strlen (filename);
      df = hash_find_item (&dir->dirfiles, &dirfile_key);
      if (df)
        return !df->impossible;
    }

  /* The file was not found in the hashed list.
     Try to read the directory further.  */

  if (dir->dirstream == 0)
    {
        /* The directory has been all read in.  */
        return 0;
    }

  while (1)
    {
      /* Enter the file in the hash table.  */
      unsigned int len;
      struct dirfile dirfile_key;
      struct dirfile **dirfile_slot;

      ENULLLOOP (d, readdir (dir->dirstream));
      if (d == 0)
        {
          if (errno)
            pfatal_with_name ("INTERNAL: readdir");
          break;
        }

      if (!REAL_DIR_ENTRY (d))
        continue;

      len = NAMLEN (d);
      dirfile_key.name = d->d_name;
      dirfile_key.length = len;
      dirfile_slot = (struct dirfile **) hash_find_slot (&dir->dirfiles, &dirfile_key);
        {
          df = xmalloc (sizeof (struct dirfile));
          df->name = strcache_add_len (d->d_name, len);
          df->length = len;
          df->impossible = 0;
          hash_insert_at (&dir->dirfiles, df, dirfile_slot);
        }

      /* Check if the name matches the one we're searching for.  */
      if (filename != 0 && patheq (d->d_name, filename))
        return 1;
    }

  /* If the directory has been completely read in,
     close the stream and reset the pointer to nil.  */
  if (d == 0)
    {
      --open_directories;
      closedir (dir->dirstream);
      dir->dirstream = 0;
    }
  return 0;
}

/* Return 1 if the name FILENAME in directory DIRNAME
   is entered in the dir hash table.
   FILENAME must contain no slashes.  */

int
dir_file_exists_p (const char *dirname, const char *filename)
{
  return dir_contents_file_exists_p (find_directory (dirname)->contents,
                                     filename);
}

/* Return 1 if the file named NAME exists.  */

int
file_exists_p (const char *name)
{
  const char *dirend;
  const char *dirname;
  const char *slash;

  if (ar_name (name))
    return ar_member_date (name) != (time_t) -1;

  dirend = strrchr (name, '/');

  if (dirend == 0)
    return dir_file_exists_p (".", name);

  slash = dirend;
  if (dirend == name)
    dirname = "/";
  else
    {
      char *p;
      p = alloca (dirend - name + 1);
      memcpy (p, name, dirend - name);
      p[dirend - name] = '\0';
      dirname = p;
    }
  slash++;

  return dir_file_exists_p (dirname, slash);
}

/* Mark FILENAME as 'impossible' for 'file_impossible_p'.
   This means an attempt has been made to search for FILENAME
   as an intermediate file, and it has failed.  */

void
file_impossible (const char *filename)
{
  const char *dirend;
  const char *p = filename;
  struct directory *dir;
  struct dirfile *new;

  dirend = strrchr (p, '/');
  if (dirend == 0)
    dir = find_directory (".");
  else
    {
      const char *dirname;
      const char *slash = dirend;
      if (dirend == p)
        dirname = "/";
      else
        {
          char *cp;
          cp = alloca (dirend - p + 1);
          memcpy (cp, p, dirend - p);
          cp[dirend - p] = '\0';
          dirname = cp;
        }
      dir = find_directory (dirname);
      filename = p = slash + 1;
    }

  if (dir->contents == 0)
    /* The directory could not be stat'd.  We allocate a contents
       structure for it, but leave it out of the contents hash table.  */
    dir->contents = xcalloc (sizeof (struct directory_contents));

  if (dir->contents->dirfiles.ht_vec == 0)
    {
      hash_init (&dir->contents->dirfiles, DIRFILE_BUCKETS,
                 dirfile_hash_1, dirfile_hash_2, dirfile_hash_cmp);
    }

  /* Make a new entry and put it in the table.  */

  new = xmalloc (sizeof (struct dirfile));
  new->length = strlen (filename);
  new->name = strcache_add_len (filename, new->length);
  new->impossible = 1;
  hash_insert (&dir->contents->dirfiles, new);
}

/* Return nonzero if FILENAME has been marked impossible.  */

int
file_impossible_p (const char *filename)
{
  const char *dirend;
  struct directory_contents *dir;
  struct dirfile *dirfile;
  struct dirfile dirfile_key;

  dirend = strrchr (filename, '/');

  if (dirend == 0)
    dir = find_directory (".")->contents;
  else
    {
      const char *dirname;
      const char *slash = dirend;
      if (dirend == filename)
        dirname = "/";
      else
        {
          char *cp;
          cp = alloca (dirend - filename + 1);
          memcpy (cp, filename, dirend - filename);
          cp[dirend - filename] = '\0';
          dirname = cp;
        }
      dir = find_directory (dirname)->contents;

      filename = slash + 1;
    }

  if (dir == 0 || dir->dirfiles.ht_vec == 0)
    /* There are no files entered for this directory.  */
    return 0;

  dirfile_key.name = filename;
  dirfile_key.length = strlen (filename);
  dirfile = hash_find_item (&dir->dirfiles, &dirfile_key);
  if (dirfile)
    return dirfile->impossible;

  return 0;
}

/* Return the already allocated name in the
   directory hash table that matches DIR.  */

const char *
dir_name (const char *dir)
{
  return find_directory (dir)->name;
}

/* Print the data base of directories.  */

void
print_dir_data_base (void)
{
  unsigned int files;
  unsigned int impossible;
  struct directory **dir_slot;
  struct directory **dir_end;

  puts (_("\n# Directories\n"));

  files = impossible = 0;

  dir_slot = (struct directory **) directories.ht_vec;
  dir_end = dir_slot + directories.ht_size;
  for ( ; dir_slot < dir_end; dir_slot++)
    {
      struct directory *dir = *dir_slot;
      if (! HASH_VACANT (dir))
        {
          if (dir->contents == 0)
            printf (_("# %s: could not be stat'd.\n"), dir->name);
          else if (dir->contents->dirfiles.ht_vec == 0)
            {
              printf (_("# %s (device %ld, inode %ld): could not be opened.\n"),
                      dir->name, (long int) dir->contents->dev,
                      (long int) dir->contents->ino);
            }
          else
            {
              unsigned int f = 0;
              unsigned int im = 0;
              struct dirfile **files_slot;
              struct dirfile **files_end;

              files_slot = (struct dirfile **) dir->contents->dirfiles.ht_vec;
              files_end = files_slot + dir->contents->dirfiles.ht_size;
              for ( ; files_slot < files_end; files_slot++)
                {
                  struct dirfile *df = *files_slot;
                  if (! HASH_VACANT (df))
                    {
                      if (df->impossible)
                        ++im;
                      else
                        ++f;
                    }
                }

              printf (_("# %s (device %ld, inode %ld): "),
                      dir->name,
                      (long)dir->contents->dev, (long)dir->contents->ino);

              if (f == 0)
                fputs (_("No"), stdout);
              else
                printf ("%u", f);
              fputs (_(" files, "), stdout);
              if (im == 0)
                fputs (_("no"), stdout);
              else
                printf ("%u", im);
              fputs (_(" impossibilities"), stdout);
              if (dir->contents->dirstream == 0)
                puts (".");
              else
                puts (_(" so far."));
              files += f;
              impossible += im;
            }
        }
    }

  fputs ("\n# ", stdout);
  if (files == 0)
    fputs (_("No"), stdout);
  else
    printf ("%u", files);
  fputs (_(" files, "), stdout);
  if (impossible == 0)
    fputs (_("no"), stdout);
  else
    printf ("%u", impossible);
  printf (_(" impossibilities in %lu directories.\n"), directories.ht_fill);
}

/* Hooks for globbing.  */

#include <glob.h>

/* Structure describing state of iterating through a directory hash table.  */

struct dirstream
  {
    struct directory_contents *contents; /* The directory being read.  */
    struct dirfile **dirfile_slot; /* Current slot in table.  */
  };

/* Forward declarations.  */
static __ptr_t open_dirstream (const char *);
static struct dirent *read_dirstream (__ptr_t);

static __ptr_t
open_dirstream (const char *directory)
{
  struct dirstream *new;
  struct directory *dir = find_directory (directory);

  if (dir->contents == 0 || dir->contents->dirfiles.ht_vec == 0)
    /* DIR->contents is nil if the directory could not be stat'd.
       DIR->contents->dirfiles is nil if it could not be opened.  */
    return 0;

  /* Read all the contents of the directory now.  There is no benefit
     in being lazy, since glob will want to see every file anyway.  */

  dir_contents_file_exists_p (dir->contents, 0);

  new = xmalloc (sizeof (struct dirstream));
  new->contents = dir->contents;
  new->dirfile_slot = (struct dirfile **) new->contents->dirfiles.ht_vec;

  return (__ptr_t) new;
}

static struct dirent *
read_dirstream (__ptr_t stream)
{
  static char *buf;
  static unsigned int bufsz;

  struct dirstream *const ds = (struct dirstream *) stream;
  struct directory_contents *dc = ds->contents;
  struct dirfile **dirfile_end = (struct dirfile **) dc->dirfiles.ht_vec + dc->dirfiles.ht_size;

  while (ds->dirfile_slot < dirfile_end)
    {
      struct dirfile *df = *ds->dirfile_slot++;
      if (! HASH_VACANT (df) && !df->impossible)
        {
          /* The glob interface wants a 'struct dirent', so mock one up.  */
          struct dirent *d;
          unsigned int len = df->length + 1;
          unsigned int sz = sizeof (*d) - sizeof (d->d_name) + len;
          if (sz > bufsz)
            {
              bufsz *= 2;
              if (sz > bufsz)
                bufsz = sz;
              buf = xrealloc (buf, bufsz);
            }
          d = (struct dirent *) buf;

          FAKE_DIR_ENTRY (d);
#ifdef _DIRENT_HAVE_D_NAMLEN
          d->d_namlen = len - 1;
#endif
#ifdef _DIRENT_HAVE_D_TYPE
          d->d_type = DT_UNKNOWN;
#endif
          memcpy (d->d_name, df->name, len);
          return d;
        }
    }

  return 0;
}

void
dir_setup_glob (glob_t *gl)
{
  gl->gl_opendir = open_dirstream;
  gl->gl_readdir = read_dirstream;
  gl->gl_closedir = free;
  gl->gl_stat = stat;
  /* We don't bother setting gl_lstat, since glob never calls it.
     The slot is only there for compatibility with 4.4 BSD.  */
}

void
hash_init_directories (void)
{
  hash_init (&directories, DIRECTORY_BUCKETS,
             directory_hash_1, directory_hash_2, directory_hash_cmp);
  hash_init (&directory_contents, DIRECTORY_BUCKETS,
             directory_contents_hash_1, directory_contents_hash_2,
             directory_contents_hash_cmp);
}
