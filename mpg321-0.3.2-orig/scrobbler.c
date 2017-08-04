/*
    mpg321 - a fully free clone of mpg123.
    scrobbler.c: Copyright (C) 2005-2006 Peter Pentchev
    		 Copyright (C) 2010 Nanakos Chrysostomos
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "mpg321.h"

#define SCROBBLER_PLUGIN_NAME   "321"
#define SCROBBLER_PLUGIN_VER    "0.1"

#define SCROBBLER_HELPER_PATH   "scrobbler-helper"
#define SCROBBLER_CUTOFF        240
#define SCROBBLER_LOW_CUTOFF    30

char *scrobbler_args[6] = {
    "", "", "", "", "", "",
};
int scrobbler_time = -1;
static int scrobbler_track_length = -1;
static int scrobbler_verbose = 0;

void scrobbler_set_time(long seconds)
{
    if (seconds < SCROBBLER_LOW_CUTOFF)
        scrobbler_time = -1;
    else if (seconds < 2 * SCROBBLER_CUTOFF)
        scrobbler_time = seconds / 2;
    else
        scrobbler_time = SCROBBLER_CUTOFF;
    scrobbler_track_length = seconds;
    if (scrobbler_verbose || MPG321_USE_SCROBBLER)
        fprintf(stderr, "AudioScrobbler report at %d sec\n", scrobbler_time);
}

void scrobbler_report(void)
{
    char *args[] = {
        SCROBBLER_HELPER_PATH,
        "-P",
        SCROBBLER_PLUGIN_NAME,
        "-V",
        SCROBBLER_PLUGIN_VER,
        "--",
        "", "", "", "", "", "",
        "",
        NULL
    };
#define SARGS_SIZE      (sizeof(args) / sizeof(args[0]))
#define SARGS_POS       (SARGS_SIZE - (6 + 2))
    char lengthbuf[20];

    if (scrobbler_verbose) {
        /*printf("Reporting to AudioScrobbler!\n");*/
        args[1] = "-vP";
    }
#ifdef __uClinux__
    switch(vfork()) {
#else
    switch(fork()) {
#endif
        case -1:
            /* Error */
            mpg321_error("forking for the scrobbler helper");
            break;
            
        case 0:
            /* Child: execute the scrobbler plug-in */
            /* Do not close the AO fd*/
            
            /* Merge the ID3 info with the scrobbler helper execv() args */
	    memcpy(args + SARGS_POS, scrobbler_args,
                sizeof(scrobbler_args));
            snprintf(lengthbuf, sizeof(lengthbuf),
                "%d", scrobbler_track_length);
            args[SARGS_SIZE - 2] = lengthbuf;
            execvp(args[0], args);
            mpg321_error("scrobbler helper " SCROBBLER_HELPER_PATH);
            exit(1);
            
        default:
            /* Parent: nothing to do */
	    /* Continue playing the mp3 file or stream */
            break;
    }
}

void scrobbler_set_verbose(int v)
{

    scrobbler_verbose = v;
}
