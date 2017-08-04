/*
    mpg321 - a fully free clone of mpg123.
    Copyright (C) 2001 Joe Drew
    
    Originally based heavily upon:
    plaympeg - Sample MPEG player using the SMPEG library
    Copyright (C) 1999 Loki Entertainment Software
    
    Also uses some code from
    mad - MPEG audio decoder
    Copyright (C) 2000-2001 Robert Leslie
    
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

#include "mpg321.h"

#include <sys/time.h>
#include <string.h>
#include <unistd.h>

char remote_input_buf[PATH_MAX + 5];
#ifdef HAVE_ALSA
extern long volume_max;
#endif

static
enum mad_flow remote_parse_input(buffer *buf, playlist *pl)
{
    char input[PATH_MAX + 5]; /* for filename as well as input and space */
    char new_remote_input_buf[PATH_MAX + 5];
    char *arg;
    int numread;
    int alreadyread;
    int linelen;

    fd_set fd;
    struct timeval tv = { 0, 0 };
    FD_ZERO(&fd);
    FD_SET(0,&fd);

    alreadyread = strlen (remote_input_buf);

    if (select (1, &fd, NULL, NULL, &tv))
    {
      if (!(numread = read(0, remote_input_buf + alreadyread, (sizeof(input)-1)-alreadyread)) > 0)
      {
        numread = 0;
        /* fprintf(stderr, "remote_parse_input() called with no input queued!");
        exit(1);
        This never happens.. (read blocks) */
      }
    } else {
      numread = 0;
    }

    remote_input_buf[numread+alreadyread] = '\0';
    
    if ((arg = strchr(remote_input_buf, '\n')))
    {
        *(arg) = '\0';
    }

    linelen = strlen (remote_input_buf);

    strcpy (input, remote_input_buf);
    /* input[linelen] = '\0'; */
    strcpy (new_remote_input_buf, remote_input_buf + linelen + 1);
    /* don't copy the \0... */
    strcpy (remote_input_buf, new_remote_input_buf);

    trim_whitespace(input); /* Trims on left and right side only */

    if (strlen(input) == 0)
        return 0;

    arg = strchr(input, ' ');

    if (arg)
    {
        *(arg++) = '\0';
        arg = strdup(arg);
        trim_whitespace(arg);
    }

    if (strcasecmp(input, "L") == 0 || strcasecmp(input, "LOAD") == 0)
    {
        if(arg)
        {
            status = MPG321_PLAYING;
            play_remote_file(pl, arg);
            current_frame = 0;
            options.seek = 0;
            pause_play(NULL, NULL); /* reset pause data */
            goto stop;
        }

        else
        {
            /* this works because if there's no argument, input is just
              'l' or 'load' */
            printf("@E Missing argument to '%s'\n",input);
            return 0;
        }
    }

    else if (strcasecmp(input, "J") == 0 || strcasecmp(input, "JUMP") == 0)
    {
        if (status == MPG321_PLAYING && arg)
        {
            /* relative seek */
            if (arg[0] == '-' || arg[0] == '+')
            {
                signed long toMove = atol(arg);
            
                /* on forward seeks we don't need to stop decoding */
                enum mad_flow toDo = move(buf, toMove);
                
                if (arg) 
                    free(arg);

                return toDo;
            }
            
            /* absolute seek */
            else
            {
                long toSeek = atol(arg);
                
                seek(buf, toSeek);
                goto stop;
            }
        }
        
        else
        {
            /* mpg123 does no error checking, so we should emulate them */
        }                        
    }
    
    else if (strcasecmp(input, "S") == 0 || strcasecmp(input, "STOP") == 0)
    {
        if (status != MPG321_STOPPED)
        {
            status = MPG321_STOPPED;
            clear_remote_file(pl);
            current_frame = 0;
            pause_play(NULL, NULL); /* reset pause data */
	    if(options.opt & MPG321_ENABLE_BUFFER) 
	    {
		    Decoded_Frames->total_played_frames = 0;
		    Decoded_Frames->stop_playing_file = 1;
	    }
            printf("@P 0\n");
        }
        
        goto stop;
    }
    
    else if (strcasecmp(input, "Q") == 0 || strcasecmp(input, "QUIT") == 0)
    {
        quit_now = 1;
	if(options.opt & MPG321_ENABLE_BUFFER) 
		    Decoded_Frames->quit_now = 1;
        goto stop;
    }
    
    else if (strcasecmp(input, "P") == 0 || strcasecmp(input, "PAUSE") == 0)
    {
        if (status == MPG321_PLAYING || status == MPG321_PAUSED)
        {
            pause_play(buf, pl);
        }

        goto stop;
    }

    else if (strcasecmp(input, "G") == 0 || strcasecmp(input, "GAIN") == 0)
    {
       if (arg)
       {
#ifdef HAVE_ALSA
	       if(options.opt & MPG321_ENABLE_BUFFER)
		       mpg321_alsa_set_volume((long)atol(arg)*volume_max/100);
	       else{
#endif
		       if(!(options.opt & MPG321_ENABLE_BUFFER))
		    	       options.volume = mad_f_tofixed(atoi(arg)/100.0);
#ifdef HAVE_ALSA
	       }
#endif
           free(arg);
       }
       else
       {
#ifdef HAVE_ALSA
	       if(options.opt & MPG321_ENABLE_BUFFER)
	           fprintf(stderr, "@G %lu\n", (int)(100*mpg321_alsa_get_volume()/volume_max));
	       else{
#endif
		       if(!(options.opt & MPG321_ENABLE_BUFFER))
		           fprintf(stderr, "@G %d\n", (int)((double)options.volume / (1 << MAD_F_FRACBITS) * 100.0));
#ifdef HAVE_ALSA
	       }
#endif
       }

       return MAD_FLOW_CONTINUE;
    }

    else 
    {
        fprintf(stderr, "@E Unknown command '%s'\n", input);

    }

    if (arg) 
        free(arg);
    return 0;

stop:
    if (arg)
        free(arg);
    return MAD_FLOW_STOP;    
}

void remote_get_input_wait(buffer *buf)
{
    fd_set fd;
    FD_ZERO(&fd);
    FD_SET(0,&fd);
    
    if (!strlen (remote_input_buf))
    {
      select(1, &fd, NULL, NULL, NULL);
    }

    remote_parse_input(buf, buf->pl);
}

enum mad_flow remote_get_input_nowait(buffer *buf)
{
    fd_set fd;
    struct timeval tv = { 0, 0 };
    FD_ZERO(&fd);
    FD_SET(0,&fd);

    if (!strlen (remote_input_buf))
    {
        if (select(1, &fd, NULL, NULL, &tv))
            return remote_parse_input(buf, buf->pl);

        else
            return 0;
    } 
     
    else 
    {
        return remote_parse_input(buf, buf->pl);
    }
}
