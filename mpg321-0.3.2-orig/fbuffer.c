/*
    mpg321 - a fully free clone of mpg123.
    fbuffer.c: Copyright (C) 2011-2012 Nanakos Chrysostomos
    
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

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <ao/ao.h>
#include <sys/sem.h>

#include "mpg321.h"

ao_device *play_device = NULL;
extern int quit_now;
extern int stop_playing_file;
extern int buffer_size;
signed long total_played_frames = 0;

void output_sighandler (int sig)	{
	ao_close (play_device);
	ao_shutdown ();
	exit (EXIT_SUCCESS);
}

void frame_buffer_p(){
	static unsigned int rate = 0;
	static int channels = 0;
	struct mad_header *header = NULL;
	output_frame *oframe = NULL;
	static struct sembuf read_sops = {0, 1, 0};
	static struct sembuf write_sops = {1, -1, 0};
	static struct sembuf end_sops = {2, 1, 0};
	decoded_frames *dframes = NULL;

	dframes = Decoded_Frames;
	dframes->total_played_frames = 0;

	pid_t ppid;

	ppid = getppid ();
	output_buffer_position = 0;
	signal (SIGUSR1, output_sighandler);
	ao_initialize();
	if (check_default_play_device_buffer() == -1)	{
		fprintf(stderr,"Unable to open play device");
		kill (ppid, SIGUSR1);
		handle_signals (SIGUSR1);
	}
	while (1)	{
		semop (semarray, &write_sops, 1);
		oframe = (Output_Queue+output_buffer_position);
		header = &(oframe->header);
		if (!play_device)	{
			channels = MAD_NCHANNELS(header);
			rate = header->samplerate;
			play_device = open_ao_playdevice_buffer(header);
			if (!play_device)	{
				fprintf(stderr,"Unable to open play device");
				kill (ppid, SIGUSR1);
				handle_signals (SIGUSR1);
			}
		} else if ((channels != MAD_NCHANNELS(header) || rate != header->samplerate) && playdevice_is_live()){
			ao_close(play_device);
			channels = MAD_NCHANNELS(header);
			rate = header->samplerate;
			play_device = open_ao_playdevice_buffer(header);
			if (!play_device)	{
				fprintf(stderr,"Unable to open play device");
				kill (ppid, SIGUSR1);
				handle_signals (SIGUSR1);
			}
		}
		if ((options.opt & MPG321_REMOTE_PLAY))
			fprintf(stderr,"%s",oframe->time);
		else if( options.opt & MPG321_VERBOSE_PLAY )
		{
			if(dframes->timer > 0)
				fprintf(stderr,"Volume: %lu%%  %s\r",dframes->bvolume,oframe->time);
			else
				fprintf(stderr,"%s",oframe->time);
		}

		if(oframe->num_frames <= 0 || dframes->quit_now || dframes->stop_playing_file || (dframes->is_http && dframes->done && (dframes->total_decoded_frames-total_played_frames)<=1) 
				|| (dframes->is_file && oframe->seconds <=0 && dframes->done) || (dframes->is_file && dframes->done && (dframes->total_decoded_frames-1 == total_played_frames)) 
				|| (dframes->is_http && dframes->done && (dframes->total_decoded_frames-1 == total_played_frames)) || (dframes->done && dframes->total_decoded_frames == 0) )
		{
			union semun sem_ops;
			sem_ops.val = buffer_size-1;
			semctl(semarray,0,SETVAL,sem_ops);
			sem_ops.val = 0;
			semctl(semarray,1,SETVAL,sem_ops);
			output_buffer_position = 0;
			if(stop_playing_file) stop_playing_file = 0;
			if(quit_now) quit_now = 0;
			dframes->total_decoded_frames = 0;
			dframes->done = 0;
			dframes->is_http = 0;
			dframes->is_file = 0;
			dframes->stop_playing_file = 0;
			total_played_frames = 0;
			semop(semarray,&end_sops,1);
			continue;
		}
		ao_play(play_device, oframe->data, oframe->length);
		total_played_frames+=1;
		dframes->total_played_frames += 1;
		output_buffer_position = getnext_place(output_buffer_position);
		semop (semarray, &read_sops, 1);
			
		if(dframes->timer > 0)
		{
			dframes->timer++;
			if(dframes->timer > 40)
			{
				
				if(!(options.opt & MPG321_VERBOSE_PLAY))
					fprintf(stderr,"                \r");
				dframes->timer = -1;
				fflush(stderr);
			}
		}

	}
}
