/*
 * Copyright (c) 2005-2006, Kohsuke Ohtani
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* modified by JiaoJinXing for SmileOS. */
#include "cmds/cmds.h"

#include <sys/stat.h>

#include <errno.h>
#include <err.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>

int
mv_main(int argc, char *argv[])
{
	char path[PATH_MAX];
	char *src, *dest, *p;
	struct stat st1, st2;
	int rc;

	if (argc != 3) {
		fprintf(stderr, "usage: mv src dest\n");
		return -1;
	}
	src = argv[1];
	dest = argv[2];

	/* Check if source exists and it's regular file. */
	if (stat(src, &st1) < 0) {
	    return -1;
	}

	if (!S_ISREG(st1.st_mode)) {
		fprintf(stderr, "mv: invalid file type\n");
		return -1;
	}

	/* Check if target is a directory. */
	rc = stat(dest, &st2);
	if (!rc && S_ISDIR(st2.st_mode)) {
		p = strrchr(src, '/');
		p = p ? p + 1 : src;
		strlcpy(path, dest, sizeof(path));
		if (strcmp(dest, "/"))
			strlcat(path, "/", sizeof(path));
		strlcat(path, p, sizeof(path));
		dest = path;
	}

	if (rename(src, dest) < 0) {
	    return -1;
	}

	return 0;
}

SMILEOS_CMD(
    mv,    mv_main,
    "bmp     - manipulate BMP image data\n",
    "info <imageAddr>          - display image info\n"
    "bmp display <imageAddr> [x y] - display image at x,y\n"
);
