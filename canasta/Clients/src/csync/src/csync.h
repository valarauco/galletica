/*
 * libcsync -- a library to sync a directory with another
 *
 * Copyright (c) 2006-2008 by Andreas Schneider <mail@cynapses.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @file csync.h
 *
 * @brief Application developer interface for csync.
 *
 * @defgroup csyncPublicAPI csync public API
 *
 * @{
 */

#ifndef _CSYNC_H
#define _CSYNC_H

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CSYNC_STRINGIFY(s) CSYNC_TOSTRING(s)
#define CSYNC_TOSTRING(s) #s

/* csync version macros */
#define CSYNC_VERSION_INT(a, b, c) ((a) << 16 | (b) << 8 | (c))
#define CSYNC_VERSION_DOT(a, b, c) a ##.## b ##.## c
#define CSYNC_VERSION(a, b, c) CSYNC_VERSION_DOT(a, b, c)

/* csync version */
#define LIBCSYNC_VERSION_MAJOR  0
#define LIBCSYNC_VERSION_MINOR  50 
#define LIBCSYNC_VERSION_MICRO  8

#define LIBCSYNC_VERSION_INT CSYNC_VERSION_INT(LIBCSYNC_VERSION_MAJOR, \
                                           LIBCSYNC_VERSION_MINOR, \
                                           LIBCSYNC_VERSION_MICRO)
#define LIBCSYNC_VERSION     CSYNC_VERSION(LIBCSYNC_VERSION_MAJOR, \
                                           LIBCSYNC_VERSION_MINOR, \
                                           LIBCSYNC_VERSION_MICRO)

/*
 * csync file declarations
 */
#define CSYNC_CONF_DIR ".csync"
#define CSYNC_CONF_FILE "csync.conf"
#define CSYNC_LOG_FILE "csync_log.conf"
#define CSYNC_EXCLUDE_FILE "csync_exclude.conf"
#define CSYNC_LOCK_FILE "lock"

typedef int (*csync_auth_callback) (const char *prompt, char *buf, size_t len,
    int echo, int verify, void *userdata);

enum csync_error_codes_e {
  CSYNC_ERR_NONE          = 0,
  CSYNC_ERR_LOG,
  CSYNC_ERR_LOCK,
  CSYNC_ERR_STATEDB_LOAD,
  CSYNC_ERR_MODULE,
  CSYNC_ERR_TIMESKEW,
  CSYNC_ERR_FILESYSTEM,
  CSYNC_ERR_TREE,
  CSYNC_ERR_MEM,
  CSYNC_ERR_PARAM,
  CSYNC_ERR_RECONCILE,
  CSYNC_ERR_PROPAGATE,
  CSYNC_ERR_ACCESS_FAILED,
  CSYNC_ERR_REMOTE_CREATE,
  CSYNC_ERR_REMOTE_STAT,
  CSYNC_ERR_LOCAL_CREATE,
  CSYNC_ERR_LOCAL_STAT,
  CSYNC_ERR_PROXY,
  CSYNC_ERR_UNSPEC
};
typedef enum csync_error_codes_e CSYNC_ERROR_CODE;

/**
  * Instruction enum. In the file traversal structure, it describes
  * the csync state of a file.
  */
enum csync_instructions_e {
  CSYNC_INSTRUCTION_NONE       = 0x00000000,
  CSYNC_INSTRUCTION_EVAL       = 0x00000001,
  CSYNC_INSTRUCTION_REMOVE     = 0x00000002,
  CSYNC_INSTRUCTION_RENAME     = 0x00000004,
  CSYNC_INSTRUCTION_NEW        = 0x00000008,
  CSYNC_INSTRUCTION_CONFLICT   = 0x00000010,
  CSYNC_INSTRUCTION_IGNORE     = 0x00000020,
  CSYNC_INSTRUCTION_SYNC       = 0x00000040,
  CSYNC_INSTRUCTION_STAT_ERROR = 0x00000080,
  CSYNC_INSTRUCTION_ERROR      = 0x00000100,
  /* instructions for the propagator */
  CSYNC_INSTRUCTION_DELETED    = 0x00000200,
  CSYNC_INSTRUCTION_UPDATED    = 0x00000400
};

/**
 * CSync File Traversal structure.
 *
 * This structure is passed to the visitor function for every file
 * which is seen.
 * Note: The file size is missing here because type off_t is depending
 *       on the large file support in your build. Make sure to check
 *       that cmake and the callback app are compiled with the same
 *       setting for it, such as:
 *       -D_LARGEFILE64_SOURCE or -D_LARGEFILE_SOURCE
 *
 */
struct csync_tree_walk_file_s {
    const char *path;
    /* off_t       size; */
    time_t      modtime;
#ifdef _WIN32
    uint32_t    uid;
    uint32_t    gid;
#else
    uid_t       uid;
    gid_t       gid;
#endif
    mode_t      mode;
    int         type;
    enum csync_instructions_e instruction;
};
typedef struct csync_tree_walk_file_s TREE_WALK_FILE;

/**
 * csync handle
 */
typedef struct csync_s CSYNC;

/**
 * @brief Allocate a csync context.
 *
 * @param csync  The context variable to allocate.
 *
 * @return  0 on success, less than 0 if an error occured.
 */
int csync_create(CSYNC **csync, const char *local, const char *remote);

/**
 * @brief Initialize the file synchronizer.
 *
 * This function loads the configuration, the statedb and locks the client.
 *
 * @param ctx  The context to initialize.
 *
 * @return  0 on success, less than 0 if an error occured.
 */
int csync_init(CSYNC *ctx);

/**
 * @brief Update detection
 *
 * @param ctx  The context to run the update detection on.
 *
 * @return  0 on success, less than 0 if an error occured.
 */
int csync_update(CSYNC *ctx);

/**
 * @brief Reconciliation
 *
 * @param ctx  The context to run the reconciliation on.
 *
 * @return  0 on success, less than 0 if an error occured.
 */
int csync_reconcile(CSYNC *ctx);

/**
 * @brief Propagation
 *
 * @param ctx  The context to run the propagation on.
 *
 * @return  0 on success, less than 0 if an error occured.
 */
int csync_propagate(CSYNC *ctx);

/**
 * @brief Destroy the csync context
 *
 * Writes the statedb, unlocks csync and frees the memory.
 *
 * @param ctx  The context to destroy.
 *
 * @return  0 on success, less than 0 if an error occured.
 */
int csync_destroy(CSYNC *ctx);

/**
 * @brief Check if csync is the required version or get the version
 * string.
 *
 * @param req_version   The version required.
 *
 * @return              If the version of csync is newer than the version
 *                      required it will return a version string.
 *                      NULL if the version is older.
 *
 * Example:
 *
 * @code
 *  if (csync_version(CSYNC_VERSION_INT(0,42,1)) == NULL) {
 *    fprintf(stderr, "libcsync version is too old!\n");
 *    exit(1);
 *  }
 *
 *  if (debug) {
 *    printf("csync %s\n", csync_version(0));
 *  }
 * @endcode
 */
const char *csync_version(int req_version);

/**
 * @brief Add an additional exclude list.
 *
 * @param ctx           The context to add the exclude list.
 *
 * @param path          The path pointing to the file.
 *
 * @return              0 on success, less than 0 if an error occured.
 */
int csync_add_exclude_list(CSYNC *ctx, const char *path);

/**
 * @brief Get the config directory.
 *
 * @param ctx          The csync context.
 *
 * @return             The path of the config directory or NULL on error.
 */
const char *csync_get_config_dir(CSYNC *ctx);

/**
 * @brief Change the config directory.
 *
 * @param ctx           The csync context.
 *
 * @param path          The path to the new config directory.
 *
 * @return              0 on success, less than 0 if an error occured.
 */
int csync_set_config_dir(CSYNC *ctx, const char *path);

/**
 * @brief Remove the complete config directory.
 *
 * @param ctx           The csync context.
 *
 * @return              0 on success, less than 0 if an error occured.
 */
int csync_remove_config_dir(CSYNC *ctx);

/**
 * @brief Enable the usage of the statedb. It is enabled by default.
 *
 * @param ctx           The csync context.
 *
 * @return              0 on success, less than 0 if an error occured.
 */
int csync_enable_statedb(CSYNC *ctx);

/**
 * @brief Disable the usage of the statedb. It is enabled by default.
 *
 * @param ctx           The csync context.
 *
 * @return              0 on success, less than 0 if an error occured.
 */
int csync_disable_statedb(CSYNC *ctx);

/**
 * @brief Check if the statedb usage is enabled.
 *
 * @param ctx           The csync context.
 *
 * @return              1 if it is enabled, 0 if it is disabled.
 */
int csync_is_statedb_disabled(CSYNC *ctx);

/**
 * @brief Get the userdata saved in the context.
 *
 * @param ctx           The csync context.
 *
 * @return              The userdata saved in the context, NULL if an error
 *                      occured.
 */
void *csync_get_userdata(CSYNC *ctx);

/**
 * @brief Save userdata to the context which is passed to the auth
 * callback function.
 *
 * @param ctx           The csync context.
 *
 * @param userdata      The userdata to be stored in the context.
 *
 * @return              0 on success, less than 0 if an error occured.
 */
int csync_set_userdata(CSYNC *ctx, void *userdata);

/**
 * @brief Get the authentication callback set.
 *
 * @param ctx           The csync context.
 *
 * @return              The authentication callback set or NULL if an error
 *                      occured.
 */
csync_auth_callback csync_get_auth_callback(CSYNC *ctx);

/**
 * @brief Set the authentication callback.
 *
 * @param ctx           The csync context.
 *
 * @param cb            The authentication callback.
 *
 * @return              0 on success, less than 0 if an error occured.
 */
int csync_set_auth_callback(CSYNC *ctx, csync_auth_callback cb);

/**
 * @brief Get the path of the statedb file used.
 *
 * @param ctx           The csync context.
 *
 * @return              The path to the statedb file, NULL if an error occured.
 */
const char *csync_get_statedb_file(CSYNC *ctx);

/**
 * @brief Enable the creation of backup copys if files are changed on both sides
 *
 * @param ctx           The csync context.
 *
 * @return              0 on success, less than 0 if an error occured.
 */
int csync_enable_conflictcopys(CSYNC *ctx);

/**
  * @brief Flag to tell csync that only a local run is intended. Call before csync_init
  *
  * @param local_only   Bool flag to indicate local only mode.
  *
  * @return             0 on success, less than 0 if an error occured.
  */
int csync_set_local_only( CSYNC *ctx, bool local_only );

/**
  * @brief Retrieve the flag to tell csync that only a local run is intended.
  *
  * @return             1: stay local only, 0: local and remote mode
  */
bool csync_get_local_only( CSYNC *ctx );

/* Used for special modes or debugging */
int csync_get_status(CSYNC *ctx);

/* Used for special modes or debugging */
int csync_set_status(CSYNC *ctx, int status);

typedef int csync_treewalk_visit_func(TREE_WALK_FILE* ,void*);

/**
 * @brief Walk the local file tree and call a visitor function for each file.
 *
 * @param ctx           The csync context.
 * @param visitor       A callback function to handle the file info.
 * @param filter        A filter, built from or'ed csync_instructions_e
 *
 * @return              0 on success, less than 0 if an error occured.
 */
int csync_walk_local_tree(CSYNC *ctx, csync_treewalk_visit_func *visitor, int filter);

/**
 * @brief Walk the remote file tree and call a visitor function for each file.
 *
 * @param ctx           The csync context.
 * @param visitor       A callback function to handle the file info.
 * @param filter        A filter, built from and'ed csync_instructions_e
 *
 * @return              0 on success, less than 0 if an error occured.
 */
int csync_walk_remote_tree(CSYNC *ctx, csync_treewalk_visit_func *visitor, int filter);

/**
 * @brief Get the error code from the last operation.
 * 
 * @return              An error code defined by structure CSYNC_ERROR_CODE
 */
CSYNC_ERROR_CODE csync_get_error(CSYNC *ctx);

#ifdef LOG_TO_CALLBACK

typedef void (*csync_log_callback)(const char *msg);

void csync_set_log_callback( csync_log_callback );

void csync_log_cb(char *catName, int a_priority,
		  const char* a_format,...);
#endif


#ifdef __cplusplus
}
#endif

/**
 * }@
 */
#endif /* _CSYNC_H */
/* vim: set ft=c.doxygen ts=8 sw=2 et cindent: */
