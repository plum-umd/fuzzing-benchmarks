/*
    mpg321 - a fully free clone of mpg123.
    Copyright (C) 2001 Joe Drew
    Copyright (C) 2010 Nanakos Chrysostomos, Giuseppe Scrivano <gscrivano@gnu.org>
    
    Originally based heavily upon:
    plaympeg - Sample MPEG player using the SMPEG library
    Copyright (C) 1999 Loki Entertainment Software
    
    Also uses some code from
    mad - MPEG audio decoder
    Copyright (C) 2000-2001 Robert Leslie
    
    Original playlist code contributed by Tobias Bengtsson <tobbe@tobbe.nu>

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "mpg321.h"

playlist * new_playlist()
{
    playlist *pl = (playlist *) malloc(sizeof(playlist));

    srand(time(NULL));
        
    pl->files = (char **) malloc(DEFAULT_PLAYLIST_SIZE * sizeof(char *));
    pl->files_size = DEFAULT_PLAYLIST_SIZE;
    pl->numfiles = 0;
    pl->random_play = 0;
    strcpy(pl->remote_file, "");
    
    return pl;
}

/* not needed - one static playlist object per instance 
void delete_playlist(playlist *pl)
{
    int i;
    for (i = 0; i < pl->numfiles; i++)
        free(pl->files[i]);

    free(pl->files);
    free(pl);
} */

void resize_playlist(playlist *pl)
{
    char ** temp = (char **) malloc ((pl->files_size *= 2) * sizeof(char *));
    register int i;
    
    for (i = 0; i < pl->numfiles; i++)
        temp [i] = pl->files[i];
        
    free (pl->files);
    
    pl->files = temp;
}

void set_random_play(playlist *pl)
{
    pl->random_play = 1;
}

void shuffle_files (playlist *pl)
{
    int i, r;
    char *swap;

    for (i = 0; i < pl->numfiles; i++)
    {
        /* find a random integer r with i<=r<numfile */
        r = i + (pl->numfiles-i-1) * ((double)rand()/RAND_MAX);

        /* now swap the current element with the random element */
        swap = pl->files[i];
        pl->files[i] = pl->files[r];
        pl->files[r] = swap;
    }
}

void trim_whitespace(char *str)
{
    char *ptr = str;
    register int pos = strlen(str)-1;

    while(isspace(ptr[pos]))
        ptr[pos--] = '\0';
    
    while(isspace(*ptr))
        ptr++;
    
    strncpy(str, ptr, pos+1);
}    
   
static inline double square(double a) {
	return a*a;
}

char * get_next_file(playlist *pl, buffer *buf)
{
    static int i = 0;
    
    char *swap;
    int j,n;

    if (options.opt & MPG321_REMOTE_PLAY)
    {
        while (strlen(pl->remote_file) == 0 && !quit_now)
            remote_get_input_wait(buf);
            
        return pl->remote_file;
    }    
    
    if (!pl->random_play)
    {
        if (i == pl->numfiles)
        {
            i = 0;
            if (loop_remaining != -1)
                loop_remaining--;
        }
        if (!loop_remaining)
            return NULL;
        
        return pl->files[i++];
    }
    
    else
    {
	    /* randomly choose a file from the first half of the playlist,
	       then append it to the end of the playlist. This way no file
	       gets repeated too early. We also give more priority to
	       files that were played less recently. If this option is
	       selected, we also shuffle ahead of time, so that the files
	       are random in the beginning. */
        if (!pl->numfiles)
            return NULL;

//        i = (pl->numfiles-1) * ((double)rand()/RAND_MAX);
	 /* determine the number of "eligible" files at the start of
	    +          the playlist. Special case: when there are only two files,
	    +          we must sometimes repeat, or else it will be A B A B A... */
       if (pl->numfiles == 2) {
	       n = 2;
       } else {
	       n = (pl->numfiles+1)/2;
       }
       /* the square of a random number is still in [0,1], but biased
          towards 0 */
       i = n * square((double)rand()/RAND_MAX);
       swap = pl->files[i];
       for (j=i; j<pl->numfiles-1; j++) {
	       pl->files[j] = pl->files[j+1];
       }
       pl->files[pl->numfiles-1] = swap;
//        return pl->files[i];
       return pl->files[pl->numfiles-1];
     }
}

void add_path(playlist *pl, char *path)
{
    struct stat sb;

    if(lstat(path,&sb))
	    return;
    if(S_ISREG(sb.st_mode))
    {
        if (pl->numfiles == pl->files_size)
            resize_playlist(pl);

        pl->files[(pl->numfiles)++] = path;
    } else {
	    DIR *dir = opendir(path);
	    struct dirent *entry;
	    if(dir == NULL)
		    return;

	    while((entry = readdir(dir)))
	    {
		    if(entry->d_name[0] != '.')
		    {
			    char *new_path = malloc(strlen(path)+strlen(entry->d_name)+2);
			    sprintf(new_path, "%s/%s", path, entry->d_name);
			    add_path(pl, new_path);
		    }
	    }
	    closedir(dir);
	    free(path);
    }
}

void add_cmdline_files_recursive_dir(playlist *pl, char *argv[])
{
	int i;
	for(i= optind; argv[i]; ++i)
		add_path(pl, strdup(argv[i]));
}

void add_cmdline_files(playlist *pl, char *argv[])
{

	int i;
	for (i = optind; argv[i]; ++i )
	{
		if (pl->numfiles == pl->files_size)
			resize_playlist(pl);
		pl->files[(pl->numfiles)++] = strdup(argv[i]);
	}
}

void play_remote_file(playlist *pl, char *file)
{
    strncpy(pl->remote_file, file, PATH_MAX);
    pl->remote_file[PATH_MAX-1]='\0';
}

void clear_remote_file(playlist *pl)
{
    pl->remote_file[0] = '\0';
}

void add_file(playlist *pl, char *file)
{
    if (pl->numfiles == pl->files_size)
         resize_playlist(pl);

    pl->files[pl->numfiles++] = strdup(file);
}

void load_playlist(playlist *pl, char *filename)
{
    FILE *plist;
    char playlist_buf[PATH_MAX];
    char *directory;
    int dirlen;

    if (strncmp(filename, "-", 1) == 0)
    {
        plist = stdin;
    }

    else if(!(plist = fopen(filename,"r")))
    {
        mpg321_error(filename);
        exit(1);
    }

    /* prepend the directory of the playlist file on to each filename. */
    directory = strdup(filename);
    directory = dirname(directory);

    dirlen = strlen(directory);

    while (fgets(playlist_buf, PATH_MAX, plist) != NULL)
    {
        trim_whitespace(playlist_buf);

        if (strlen(playlist_buf) == 0 || playlist_buf[0] == '#')
            continue;                

        if (pl->numfiles == pl->files_size)
            resize_playlist(pl);

        /* I hate special cases... */
        if(playlist_buf[0] != '/' && !strstr(playlist_buf, "://")) /* relative path  or network path */
        {
            /* +2 because of null terminating character plus / */
            pl->files[pl->numfiles] = malloc(strlen(playlist_buf) + dirlen + 2);
            strcpy(pl->files[pl->numfiles], directory);
            strcat(pl->files[pl->numfiles], "/");
            strcat(pl->files[pl->numfiles], playlist_buf);
        }

        else /* absolute path */
        {
            pl->files[pl->numfiles] = strdup(playlist_buf);
        }

        pl->numfiles++;
    }
    
    if (plist != stdin)
        fclose(plist);
}
