/*                         F I L E . H
 * BRL-CAD
 *
 * Copyright (c) 2004-2018 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */

#ifndef BU_FILE_H
#define BU_FILE_H

#include "common.h"

#include <stdio.h> /* For FILE */
#include <sys/types.h> /* for off_t */
#include <stddef.h> /* for size_t */
#include <stdlib.h> /* for getenv */

#include "bu/defines.h"
#include "bu/magic.h"
#include "bu/vls.h"

__BEGIN_DECLS

/** @addtogroup bu_file
 *
 * @brief
 * Support routines for identifying properties of files and
 * directories such as whether they exist or are the same as another
 * given file.
 *
 */
/** @{ */
/** @file bu/file.h */

/**
 * Returns truthfully whether the given file path exists or not.  An
 * empty or NULL path name is treated as a non-existent file and, as
 * such, will return false.  If fd is non-NULL, it will be set to an
 * open file descriptor for the provided path.
 *
 * @return >0 The given filename exists.
 * @return 0 The given filename does not exist.
 */
BU_EXPORT extern int bu_file_exists(const char *path, int *fd);

/**
 * Returns truthfully as to whether the two provided filenames are the
 * same file.  If either file does not exist, the result is false.  If
 * either filename is empty or NULL, it is treated as non-existent
 * and, as such, will also return false.
 */
BU_EXPORT extern int bu_same_file(const char *fn1, const char *fn2);

/**
 * returns truthfully as to whether or not the two provided file
 * descriptors are the same file.  if either file does not exist, the
 * result is false.
 */
BU_EXPORT extern int bu_same_fd(int fd1, int fd2);

/**
 * returns truthfully if current user can read the specified file or
 * directory.
 */
BU_EXPORT extern int bu_file_readable(const char *path);

/**
 * returns truthfully if current user can write to the specified file
 * or directory.
 */
BU_EXPORT extern int bu_file_writable(const char *path);

/**
 * returns truthfully if current user can run the specified file or
 * directory.
 */
BU_EXPORT extern int bu_file_executable(const char *path);

/**
 * Returns truthfully whether the given file path is a directory.  An
 * empty or NULL path name is treated as a non-existent directory and,
 * as such, will return false.
 *
 * @return >0 The given filename is a directory
 * @return 0 The given filename is not a directory.
 */
BU_EXPORT extern int bu_file_directory(const char *path);

/**
 * Returns truthfully whether the given file path is a symbolic link.
 * An empty or NULL path name is treated as a non-existent link and,
 * as such, will return false.
 *
 * @return >0 The given filename is a symbolic link
 * @return 0 The given filename is not a symbolic link.
 */
BU_EXPORT extern int bu_file_symbolic(const char *path);

/**
 * forcibly attempts to delete a specified file.  if the file can be
 * deleted by relaxing file permissions, they will be changed in order
 * to delete the file.
 *
 * returns truthfully if the specified file was deleted.
 */
BU_EXPORT extern int bu_file_delete(const char *path);


#if 0
/**
 * TODO - currently unimplemented
 *
 * matches a filepath pattern to directory entries.  if non-NULL,
 * matching paths are dynamically allocated, stored into the provided
 * 'matches' array, and followed by a terminating NULL entry.
 *
 * If '*matches' is NULL, the caller is expected to free the matches
 * array with bu_argv_free() If '*matches' is non-NULL (i.e., string
 * array is already allocated or on the stack), the caller is expected
 * to ensure adequate entries are allocated and call bu_free_array()
 * to clean up.  If 'matches' is NULL, no entries will be allocated or
 * stored, but the number of matches will still be returned.
 *
 * Example:
 *
 * char **my_matches = NULL;
 * bu_file_glob("src/libbu/[a-e]*.c", &my_matches);
 *
 * This will allocate an array for storing glob matches, filling in
 * the array with all of the directory entries starting with 'a'
 * through 'e' and ending with a '.c' suffix in the src/libbu
 * directory.
 *
 * returns the number of matches
 */
BU_EXPORT extern size_t bu_file_glob(const char *pattern, char ***matches);

#endif


/**
 * Returns the number of directory entries for a given path matching
 * an optional glob pattern.  If the caller provides a pointer to an
 * argv-style 'files' array, this function will dynamically allocate
 * an array of strings, filled with the sorted listing of matching
 * file(s).
 *
 * It is the caller's responsibility to free a non-NULL array with
 * bu_argv_free().
 */
BU_EXPORT extern size_t bu_file_list(const char *path, const char *pattern, char ***files);


/**
 * Call canonicalization routines to both expand and validate
 * a path name.
 *
 * Returns a pointer to the canonical path. If resolved_path is
 * NULL, caller is responsible for freeing the returned path
 * via bu_free.  If supplying a result string, the string must hold
 * at least MAXPATHLEN characters.
 */
BU_EXPORT extern char * bu_realpath(const char *path, char *resolved_path);


/**
 * @brief
 * Support routines to provide the executable code with information
 * about executable name used to invoke the current program.
 *
 */

/**
 * DEPRECATED: This routine is replaced by bu_getcwd().
 *             Do not use.
 *
 * returns the full path to argv0, regardless of how the application
 * was invoked.
 *
 * this routine will return "(BRL-CAD)" if argv[0] cannot be
 * identified but should never return NULL.  this routine is not
 * thread-safe.
 */
BU_EXPORT extern const char *bu_argv0_full_path(void);

/**
 * Get the name of the running application.  application codes should
 * call bu_setprogname() first to ensure that the program name is
 * stored appropriately on platforms that do not have an intrinsic
 * method for tracking the program name automatically.
 *
 * while this routine is thread-safe and reentrant, the static string
 * returned is shared amongst all threads.
 */
BU_EXPORT extern const char *bu_getprogname(void);

/**
 * Set the name of the running application.  This isn't strictly
 * necessary on platforms that have an intrinsic method for tracking
 * the program name automatically, but is still recommended for
 * portability and is necessary on some strict modes of compilation.
 *
 * while the implementation relies on a static string shared across
 * all threads, this routine is thread-safe and reentrant.
 */
BU_EXPORT extern void bu_setprogname(const char *path);

/**
 * Routine for getting the current working directory full pathname.
 */
BU_EXPORT extern char *bu_getcwd(char *buf, size_t size);

/**
 * returns the first USER path match to a given executable name.
 *
 * Routine to provide BSD "which" functionality, locating binaries of
 * specified programs from the user's PATH. This is useful to locate
 * binaries and resources at run-time.
 *
 * caller should not free the result, though it will not be preserved
 * between calls either.  the caller should strdup the result if they
 * need to keep it around.
 *
 * routine will return NULL if the executable command cannot be found.
 */
BU_EXPORT extern const char *bu_which(const char *cmd);

/**
 * returns the first SYSTEM path match to a given executable cmd name.
 *
 * Routine to provide BSD "whereis" functionality, locating binaries
 * of specified programs from the SYSTEM path.  This is useful to
 * locate binaries and resources at run-time.
 *
 * caller should not free the result, though it will not be preserved
 * between calls either.  the caller should strdup the result if they
 * need to keep it around.
 *
 * routine will return NULL if the executable command cannot be found.
 */
BU_EXPORT extern const char *bu_whereis(const char *cmd);


/**
 * @brief
 * Routine to open a temporary file.
 *
 * Create a temporary file.  The first readable/writable directory
 * will be used, searching TMPDIR/TEMP/TMP environment variable
 * directories followed by default system temp directories and
 * ultimately trying the current directory.
 *
 * This routine is guaranteed to return a new unique file or return
 * NULL on failure.  The temporary file will be automatically unlinked
 * on application exit.  It is the caller's responsibility to set file
 * access settings, preserve file contents, or destroy file contents
 * if the default behavior is non-optimal.
 *
 * The name of the temporary file will be copied into a user-provided
 * (filepath) buffer if it is a non-NULL pointer and of a sufficient
 * (len) length to contain the filename.
 *
 * This routine is NOT thread-safe.
 *
 * Typical Use:
 @code
  FILE *fp;
  char filename[MAXPATHLEN];
  fp = bu_temp_file(&filename, MAXPATHLEN); // get file name
  ...
  fclose(fp); // close the file when you're done
  ...
  fp = bu_temp_file(NULL, 0); // don't need file name
  bu_fchmod(fileno(fp), 0777);
  ...
  rewind(fp);
  while (fputc(0, fp) == 0);
  fclose(fp);
 @endcode
 */
BU_EXPORT extern FILE *bu_temp_file(char *filepath, size_t len);


/**
 * @brief Wrapper around fchmod.
 *
 * Portable wrapper for setting a file descriptor's permissions ala
 * fchmod().
 */
BU_EXPORT extern int bu_fchmod(int fd, unsigned long pmode);


/**@brief BRL-CAD specific path queries */

/**
 * @brief
 * Report the relative paths being used to hold BRL-CAD applications,
 * libraries, and data.
 *
 * Recognized keys include:
 *
 * Key     | Looks Up
 * ------- | -------------------------------------------
 * bin     | Directory containing applications
 * lib     | Directory containing libraries
 * include | Directory containing headers
 * data    | Directory containing shared data
 * share   | Directory containing shared data
 * doc     | Directory containing documentation
 * man     | Directory containing Unix man pages
 *
 * @return
 * A STATIC buffer is returned.  It is the caller's responsibility to
 * call bu_strdup() or make other provisions to save the returned
 * string, before calling again.
 */
BU_EXPORT extern const char *bu_brlcad_dir(const char *dirkey, int fail_quietly);

/**
 * @brief
 * Locate where the BRL-CAD applications and libraries are installed.
 *
 * The BRL-CAD root is searched for in the following order of
 * precedence by testing for the rhs existence if provided or the
 * directory existence otherwise:
 *
 *   BRLCAD_ROOT environment variable if set
 *   BRLCAD_ROOT compile-time path
 *   run-time path identification
 *   /usr/brlcad static path
 *   current directory
 *
 * @return
 * A STATIC buffer is returned.  It is the caller's responsibility to
 * call bu_strdup() or make other provisions to save the returned
 * string, before calling again.
 */
BU_EXPORT extern const char *bu_brlcad_root(const char *rhs, int fail_quietly);

/**
 * @brief
 * Locate where the BRL-CAD data resources are installed.
 *
 * The BRL-CAD data resources are searched for in the following order
 * of precedence by testing for the existence of rhs if provided or
 * the directory existence otherwise:
 *
 *   BRLCAD_DATA environment variable if set
 *   BRLCAD_DATA compile-time path
 *   bu_brlcad_root/DATA_DIR path
 *   bu_brlcad_root/share path
 *   current directory
 *
 * A STATIC buffer is returned.  It is the caller's responsibility to
 * call bu_strdup() or make other provisions to save the returned
 * string, before calling again.
 */
BU_EXPORT extern const char *bu_brlcad_data(const char *rhs, int fail_quietly);



/**
 *
 * Routines for managing signals.  In particular, provide a common
 * means to temporarily buffer processing a signal during critical
 * write operations.
 *
 */

/**
 * Defer signal processing and interrupts before critical sections.
 *
 * Signal processing for a variety of signals that would otherwise
 * disrupt the logic of an application are put on hold until
 * bu_restore_interrupts() is called.
 *
 * If an interrupt signal is received while suspended, it will be
 * raised when/if interrupts are restored.
 *
 * Returns 0 on success.
 * Returns non-zero on error (with perror set if signal() failure).
 */
BU_EXPORT extern int bu_suspend_interrupts(void);

/**
 * Resume signal processing and interrupts after critical sections.
 *
 * If a signal was raised since bu_suspend_interrupts() was called,
 * the previously installed signal handler will be immediately called
 * albeit only once even if multiple signals were received.
 *
 * Returns 0 on success.
 * Returns non-zero on error (with perror set if signal() failure).
 */
BU_EXPORT extern int bu_restore_interrupts(void);


/** NEW: Do not use. */
BU_EXPORT extern int bu_fseek(FILE *stream, off_t offset, int origin);
/** NEW: Do not use. */
BU_EXPORT extern off_t bu_ftell(FILE *stream);

/** @} */

__END_DECLS

#endif  /* BU_FILE_H */

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
