/*
 * libcsync -- a library to sync a directory with another
 *
 * Copyright (c) 2006 by Andreas Schneider <mail@cynapses.org>
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

#ifndef _CSYNC_UTIL_H
#define _CSYNC_UTIL_H

#include <stdint.h>

#include "csync_private.h"

const char *csync_instruction_str(enum csync_instructions_e instr);

void csync_memstat_check(void);

int csync_merge_file_trees(CSYNC *ctx);

int csync_unix_extensions(CSYNC *ctx);

/* Normalize the uri to <host>/<path> */
uint64_t csync_create_statedb_hash(CSYNC *ctx);

/* Calculate the md5 sum for a file given by filename.
 * Caller has to free the memory. */
char* csync_file_md5(const char *filename);

/* Create an md5 sum from a data pointer with a given length.
 * Caller has to free the memory */
char* csync_buffer_md5(const char *str, int length);

#endif /* _CSYNC_UTIL_H */
/* vim: set ft=c.doxygen ts=8 sw=2 et cindent: */
