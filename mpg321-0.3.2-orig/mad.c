/*
    mpg321 - a fully free clone of mpg123.
    Copyright (C) 2001 Joe Drew
    
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


#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "mpg321.h"

unsigned long current_frame=0;

/* Basic control keys global variables */
extern int count, volume;
extern int http_file_length;


enum mad_flow read_from_mmap(void *data, struct mad_stream *stream)
{
    buffer *playbuf = (buffer *)data;
    void *mpegdata = NULL;
    
    /* libmad asks us for more data when it runs out. We don't have any more,
       so we want to quit here. */
    if (status != MPG321_REWINDING && playbuf->done)
    {
        status = MPG321_STOPPED;
        if (options.opt & MPG321_REMOTE_PLAY) printf("@P 3\n");
	if(options.opt & MPG321_ENABLE_BUFFER) Decoded_Frames->done = 1;
        return MAD_FLOW_STOP;
    }

    if(playbuf->fd != -1)
    {
        fprintf(stderr, "read_from_mmap called when not expected!\n");
        exit(1);
    }
    
    mpegdata = playbuf->buf;

    if(status == MPG321_REWINDING)
    {
        mpegdata = playbuf->frames[current_frame];
        options.seek = 0;
        status = MPG321_PLAYING;
    }

    if (status != MPG321_SEEKING) /* seeking goes to playing during the decoding process */
        status = MPG321_PLAYING;

    playbuf->done = 1;

    current_frame = 0;

    mad_stream_buffer(stream, mpegdata, playbuf->length - (mpegdata - playbuf->buf));
    
    return MAD_FLOW_CONTINUE;
}

/* assumes that playbuf->buf has been preallocated to BUF_SIZE */
enum mad_flow read_from_fd(void *data, struct mad_stream *stream)
{
    buffer *playbuf = data;
    int bytes_to_preserve = stream->bufend - stream->next_frame;
    int readbytes;
    
    if(playbuf->done)
    {
        status = MPG321_STOPPED;
	if(options.opt & MPG321_ENABLE_BUFFER) Decoded_Frames->done = 1;
	if((options.opt & MPG321_ENABLE_BUFFER) && Decoded_Frames->total_decoded_frames==0)
	{
            static struct sembuf release_sops = {1, 1, 0};
	    semop(semarray,&release_sops,1);
	
	}
        return MAD_FLOW_STOP;
    }

    if(playbuf->fd == -1)
    {
        fprintf(stderr, "read_from_fd called when not expected!\n");
        exit(1);
    }
    
    if(!playbuf->buf)
    {
        fprintf(stderr, "read_from_fd called with null buf!\n");
    }
    
    /* need to preserve the bytes which comprise an incomplete
       frame, that mad has passed back to us */
    if (bytes_to_preserve)
    {
        memmove(playbuf->buf, stream->next_frame, bytes_to_preserve);
    }


    readbytes = read(playbuf->fd, playbuf->buf + bytes_to_preserve,
		     BUF_SIZE - bytes_to_preserve);
    if (readbytes <= 0) {
        playbuf->done = 1;
	if(options.opt & MPG321_ENABLE_BUFFER) Decoded_Frames->done = 1;
    } else {
        mad_stream_buffer(stream, playbuf->buf, readbytes+bytes_to_preserve);
    }
    
    return MAD_FLOW_CONTINUE;
}    

char * layerstring(enum mad_layer layer)
{
    switch(layer)
    {
        case MAD_LAYER_I:
            return "I";
        case MAD_LAYER_II:
            return "II";
case MAD_LAYER_III:
    return "III";
        default:
            return "?";
    }
}
    
char * modestring(enum mad_mode mode)
{
    switch(mode)
    {
        case MAD_MODE_SINGLE_CHANNEL:
            return "mono";
        case MAD_MODE_DUAL_CHANNEL:
            return "dual-channel";
        case MAD_MODE_JOINT_STEREO:
            return "joint-stereo";
        case MAD_MODE_STEREO:
            return "stereo";
        default:
            return "?";
    }
}

char *versionstring(int flags)
{
	switch(flags&(MAD_FLAG_LSF_EXT | MAD_FLAG_MPEG_2_5_EXT))
	{
		case MAD_FLAG_LSF_EXT | MAD_FLAG_MPEG_2_5_EXT:
			return "2.5";
		case MAD_FLAG_LSF_EXT:
			return "2.0";
		case 0:
			return "1.0";
		default:
			return "?";
	}
}


char * modestringucase(enum mad_mode mode)
{
    switch(mode)
    {
        case MAD_MODE_SINGLE_CHANNEL:
            return "Single-Channel";
        case MAD_MODE_DUAL_CHANNEL:
            return "Dual-Channel";
        case MAD_MODE_JOINT_STEREO:
            return "Joint-Stereo";
        case MAD_MODE_STEREO:
            return "Stereo";
        default:
            return "?";
    }
}

enum mad_flow read_header(void *data, struct mad_header const * header)
{
    char long_currenttime_str[14]; /* this *will* fill if you're using 100000+ minute mp3s */
    char long_remaintime_str[14];
    static int frames_played = 0;
    
    buffer *playbuf = (buffer *)data;
    mad_timer_t time_remaining;
    char *ao_time;

    if (stop_playing_file)
    {
        stop_playing_file = 0;
        status = MPG321_STOPPED;
        return MAD_FLOW_STOP;
    }
    
    if(options.opt & MPG321_REMOTE_PLAY)
    {
        enum mad_flow mf;

        /* We might have to stop if the user inputs something */
        if ((mf = remote_get_input_nowait(playbuf)))
            return mf;
    }

    /* Stop playing if -n is used, and we're at the frame specified. */
    if ((playbuf->max_frames != -1) && (frames_played++ > playbuf->max_frames))
    {
        frames_played = 0;
        status = MPG321_STOPPED;
	if(options.opt & MPG321_ENABLE_BUFFER) Decoded_Frames->done = 1;
	//fprintf(stderr,"Total D: %d\n",Decoded_Frames->total_decoded_frames);
        return MAD_FLOW_STOP;
    }

    current_frame++;

    mad_timer_add(&current_time, header->duration);

    if(options.opt & MPG321_USE_SCROBBLER && scrobbler_time > 0 && scrobbler_time < current_time.seconds)
    {
	    scrobbler_time = -1;
	    scrobbler_report();
    }

    if(options.opt & (MPG321_VERBOSE_PLAY | MPG321_REMOTE_PLAY))
    {
        mad_timer_string(current_time, long_currenttime_str, "%.2u:%.2u.%.2u", MAD_UNITS_MINUTES,
                            MAD_UNITS_CENTISECONDS, 0);

        if (mad_timer_compare(playbuf->duration, mad_timer_zero) == 0)
            time_remaining = current_time;
        else
            time_remaining = playbuf->duration;


        mad_timer_negate(&current_time);

        mad_timer_add(&time_remaining, current_time);
        mad_timer_negate(&current_time);

        mad_timer_string(time_remaining, long_remaintime_str, "%.2u:%.2u.%.2u", MAD_UNITS_MINUTES,
                            MAD_UNITS_CENTISECONDS, 0);
    }
                        
    /* update cached table of frames & times */
    if (current_frame <= playbuf->num_frames) /* we only allocate enough for our estimate. */
    {
        playbuf->frames[current_frame] = playbuf->frames[current_frame-1] + (header->bitrate / 8 / 1000)
            * mad_timer_count(header->duration, MAD_UNITS_MILLISECONDS);
        playbuf->times[current_frame] = current_time;
    }
    
    if (file_change)
    {
        file_change = 0;
        if (options.opt & MPG321_REMOTE_PLAY)
        {
            printf("@S %s %d %d %s %d %ld %d %d %d %d %ld %d\n",versionstring(header->flags), header->layer, header->samplerate,
                modestringucase(header->mode), header->mode_extension, 
                (header->bitrate / 8 / 100) * mad_timer_count(header->duration, MAD_UNITS_CENTISECONDS),
                MAD_NCHANNELS(header), header->flags & MAD_FLAG_COPYRIGHT ? 1 : 0, 
                header->flags & MAD_FLAG_PROTECTION ? 1 : 0, header->emphasis,
                header->bitrate/1000, header->mode_extension);
        }    

        else if (options.opt & MPG321_VERBOSE_PLAY)/*zip it good*/
        {
            fprintf(stderr, "MPEG %s, Layer: %s, Freq: %d, mode: %s, modext: %d, BPF : %ld\n"
                    "Channels: %d, copyright: %s, original: %s, CRC: %s, emphasis: %d.\n"
                    "Bitrate: %ld Kbits/s, Extension value: %d\n"
                    "Audio: 1:1 conversion, rate: %d, encoding: signed 16 bit, channels: %d\n",
                    versionstring(header->flags),layerstring(header->layer), header->samplerate, modestringucase(header->mode), header->mode_extension, 
                    (header->bitrate / 100) * mad_timer_count(header->duration, MAD_UNITS_CENTISECONDS),
                    MAD_NCHANNELS(header), header->flags & MAD_FLAG_COPYRIGHT ? "Yes" : "No",
                    header->flags & MAD_FLAG_ORIGINAL ? "Yes" : "No", header->flags & MAD_FLAG_PROTECTION ? "Yes" : "No",
                    header->emphasis, header->bitrate/1000, header->mode_extension,
                    header->samplerate, MAD_NCHANNELS(header));
        }

        else if (!(options.opt & MPG321_QUIET_PLAY))/*I love Joey*/
        {
            fprintf(stderr, "MPEG %s layer %s, %ld kbit/s, %d Hz %s\n",
                versionstring(header->flags),layerstring(header->layer), header->bitrate/1000, header->samplerate, modestring(header->mode));
        }
    }
    
    if (status == MPG321_SEEKING && options.seek)
    {
        if (!--options.seek)
            status = MPG321_PLAYING;

        return MAD_FLOW_IGNORE;
    }
    else
    {
        status = MPG321_PLAYING;
    }

    if(!(options.opt & MPG321_ENABLE_BUFFER))
    {
	    if(count > 0)
	    {
		    count++;
		    if(count > 40)
		    {
			    if(!(options.opt & MPG321_VERBOSE_PLAY))
				    fprintf(stderr,"                \r");
			    count = -1;
			    fflush(stderr);
		    }
	    }
    }

    if(options.opt & MPG321_ENABLE_BUFFER)
    {
	    ao_time = (Output_Queue+mad_decoder_position)->time;
	    (Output_Queue+mad_decoder_position)->num_frames = playbuf->num_frames - current_frame;
	    if(Decoded_Frames->is_file) (Output_Queue+mad_decoder_position)->seconds = time_remaining.seconds;
    }

    if (options.opt & MPG321_VERBOSE_PLAY)
    {
        if (!options.skip_printing_frames 
            || (options.skip_printing_frames && !(current_frame % options.skip_printing_frames)))
   
	      	if(count > 0)
		{
/*			if(options.opt & MPG321_ENABLE_BUFFER)
			{	
				sprintf(ao_time, "Frame# %5lu [%5lu], Time: %s [%s], \r", current_frame,
					playbuf->num_frames > 0 ? playbuf->num_frames - current_frame : 0, long_currenttime_str, long_remaintime_str);
				//sprintf(ao_time, "Frame# %5lu [%5lu], Time: %s [%s], \r", current_frame,
				//	playbuf->num_frames > 0 ? playbuf->num_frames - current_frame : 0, long_currenttime_str, long_remaintime_str);
				//sprintf(ao_time, "Volume: %d%%  Frame# %5lu [%5lu], Time: %s [%s], \r",volume, current_frame,
				//	playbuf->num_frames > 0 ? playbuf->num_frames - current_frame : 0, long_currenttime_str, long_remaintime_str);
			}else*/
			{
				fprintf(stderr, "Volume: %d%%  Frame# %5lu [%5lu], Time: %s [%s], \r",volume, current_frame,
					playbuf->num_frames > 0 ? playbuf->num_frames - current_frame : 0, long_currenttime_str, long_remaintime_str);
			}
		}
		else if(count < 0)
		{
			if(options.opt & MPG321_ENABLE_BUFFER)
			{
//				sprintf(ao_time, "Frame# %5lu [%5lu], Time: %s [%s],                     \r", current_frame,
				sprintf(ao_time, "Frame# %5lu [%5lu], Time: %s [%s],                 \r", current_frame,
					playbuf->num_frames > 0 ? playbuf->num_frames - current_frame : 0, long_currenttime_str, long_remaintime_str);
			}
			else
			{
				
				fprintf(stderr, "Frame# %5lu [%5lu], Time: %s [%s],                      \r", current_frame,
					playbuf->num_frames > 0 ? playbuf->num_frames - current_frame : 0, long_currenttime_str, long_remaintime_str);
			}
		}
    }
    else if (options.opt & MPG321_REMOTE_PLAY)
    {
    
	    if (!options.skip_printing_frames 
            || (options.skip_printing_frames && !(current_frame % options.skip_printing_frames)))
	    {
		    if(options.opt & MPG321_ENABLE_BUFFER)
		    {
			    sprintf(ao_time,"@F %ld %ld %.2f %.2f\n", current_frame, playbuf->num_frames - current_frame,
				    	    ((double)mad_timer_count(current_time, MAD_UNITS_CENTISECONDS)/100.0),
			    		    ((double)mad_timer_count(time_remaining, MAD_UNITS_CENTISECONDS)/100.0));
		    }
		    else
		    {

			    fprintf(stderr,"@F %ld %ld %.2f %.2f\n", current_frame, playbuf->num_frames - current_frame,
				    	    ((double)mad_timer_count(current_time, MAD_UNITS_CENTISECONDS)/100.0),
			    		    ((double)mad_timer_count(time_remaining, MAD_UNITS_CENTISECONDS)/100.0));
		    }
	    }
    }
    return MAD_FLOW_CONTINUE;
}        

/* XING parsing is from the MAD winamp input plugin */

struct xing {
  int flags;
  unsigned long frames;
  unsigned long bytes;
  unsigned char toc[100];
  long scale;
};

enum {
  XING_FRAMES = 0x0001,
  XING_BYTES  = 0x0002,
  XING_TOC    = 0x0004,
  XING_SCALE  = 0x0008
};

# define XING_MAGIC     (('X' << 24) | ('i' << 16) | ('n' << 8) | 'g')

static
int parse_xing(struct xing *xing, struct mad_bitptr ptr, unsigned int bitlen)
{
  if (bitlen < 64 || mad_bit_read(&ptr, 32) != XING_MAGIC)
    goto fail;

  xing->flags = mad_bit_read(&ptr, 32);
  bitlen -= 64;

  if (xing->flags & XING_FRAMES) {
    if (bitlen < 32)
      goto fail;

    xing->frames = mad_bit_read(&ptr, 32);
    bitlen -= 32;
  }

  if (xing->flags & XING_BYTES) {
    if (bitlen < 32)
      goto fail;

    xing->bytes = mad_bit_read(&ptr, 32);
    bitlen -= 32;
  }

  if (xing->flags & XING_TOC) {
    int i;

    if (bitlen < 800)
      goto fail;

    for (i = 0; i < 100; ++i)
      xing->toc[i] = mad_bit_read(&ptr, 8);

    bitlen -= 800;
  }

  if (xing->flags & XING_SCALE) {
    if (bitlen < 32)
      goto fail;

    xing->scale = mad_bit_read(&ptr, 32);
    bitlen -= 32;
  }

  return 1;

 fail:
  xing->flags = 0;
  return 0;
}


/* Following two functions are adapted from mad_timer, from the 
   libmad distribution */
void scan(void const *ptr, ssize_t len, buffer *buf)
{
    struct mad_stream stream;
    struct mad_header header;
    struct xing xing;
    
    unsigned long bitrate = 0;
    int has_xing = 0;
    int is_vbr = 0;

    mad_stream_init(&stream);
    mad_header_init(&header);

    mad_stream_buffer(&stream, ptr, len);

    buf->num_frames = 0;

    /* There are three ways of calculating the length of an mp3:
      1) Constant bitrate: One frame can provide the information
         needed: # of frames and duration. Just see how long it
         is and do the division.
      2) Variable bitrate: Xing tag. It provides the number of 
         frames. Each frame has the same number of samples, so
         just use that.
      3) All: Count up the frames and duration of each frames
         by decoding each one. We do this if we've no other
         choice, i.e. if it's a VBR file with no Xing tag.
    */

    while (1)
    {
        if (mad_header_decode(&header, &stream) == -1)
        {
            if (MAD_RECOVERABLE(stream.error))
                continue;
            else
                break;
        }

        /* Limit xing testing to the first frame header */
        if (!buf->num_frames++)
        {
            if(parse_xing(&xing, stream.anc_ptr, stream.anc_bitlen))
            {
                is_vbr = 1;
                
                if (xing.flags & XING_FRAMES)
                {
                    /* We use the Xing tag only for frames. If it doesn't have that
                       information, it's useless to us and we have to treat it as a
                       normal VBR file */
                    has_xing = 1;
                    buf->num_frames = xing.frames;
                    break;
                }
            }
        }                

        /* Test the first n frames to see if this is a VBR file */
        if (!is_vbr && !(buf->num_frames > 20))
        {
            if (bitrate && header.bitrate != bitrate)
            {
                is_vbr = 1;
            }
            
            else
            {
                bitrate = header.bitrate;
            }
        }
        
        /* We have to assume it's not a VBR file if it hasn't already been
           marked as one and we've checked n frames for different bitrates */
        else if (!is_vbr)
        {
            break;
        }
            
        mad_timer_add(&buf->duration, header.duration);
    }

    if (!is_vbr)
    {
        double time = (len * 8.0) / (header.bitrate); /* time in seconds */
        double timefrac = (double)time - ((long)(time));
        long nsamples = 32 * MAD_NSBSAMPLES(&header); /* samples per frame */
        
        /* samplerate is a constant */
        buf->num_frames = (long) (time * header.samplerate / nsamples);

        mad_timer_set(&buf->duration, (long)time, (long)(timefrac*100), 100);
    }
        
    else if (has_xing)
    {
        /* modify header.duration since we don't need it anymore */
        mad_timer_multiply(&header.duration, buf->num_frames);
        buf->duration = header.duration;
    }

    else
    {
        /* the durations have been added up, and the number of frames
           counted. We do nothing here. */
    }
    
    mad_header_finish(&header);
    mad_stream_finish(&stream);
}

void pause_play(buffer *buf, playlist *pl)
{
    static char file[PATH_MAX] = "";
    static signed long seek = 0;
    
    if (buf == NULL && pl == NULL) /* reset */
    {
        strcpy(file, "");
        seek = 0;
        return;
    }
    
    /* pause */
    if (strlen(file) == 0)
    {
        status = MPG321_PAUSED;
        strncpy(file, buf->filename, PATH_MAX);
        file[PATH_MAX-1]='\0';
        clear_remote_file(pl);
	if(options.opt & MPG321_ENABLE_BUFFER)
		seek = Decoded_Frames->total_played_frames;
	else
		seek = current_frame;
	if(options.opt & MPG321_ENABLE_BUFFER) Decoded_Frames->stop_playing_file = 1;
        printf("@P 1\n");
    }
    
    /* unpause */
    else
    {
        status = MPG321_SEEKING;
        play_remote_file(pl, file);
        file[0] = '\0';
        options.seek = seek;
	//fprintf(stderr,"options.seek = %d\n",seek);
        current_frame = 0;
	mad_decoder_position = 0;
        printf("@P 2\n");
    }
}

/* seek to absolute frame frame */
void seek(buffer *buf, signed long frame)
{
    if (frame > buf->num_frames)
        options.seek = buf->num_frames;
    else
        options.seek = frame;
    current_frame = 0;
    status = MPG321_SEEKING;
}

enum mad_flow move(buffer *buf, signed long frames)
{
    if (frames == 0)
        return 0;
    
    /* Our normal skipping (for -k) code handles forward seeks.
       Rewinds are handled by a stop in decoding, a rewind, and
       a restart in decoding, implemented in the main loop and in the other
       functions */
    if ((frames + current_frame) > buf->num_frames)
        options.seek = buf->num_frames - current_frame;
    else
        options.seek = frames;

    status = MPG321_SEEKING;

    /* Rewinding doesn't correct for frames on its own, so we must do so */
    if (frames < 0)
    {
        status = MPG321_REWINDING;
        if (((signed long)current_frame + frames) < 0)
            current_frame = 0;
        else
            current_frame += frames;
        if (current_frame > 0)
            current_time = buf->times[current_frame];
        else
        {
            current_time.seconds = 0;
            current_time.fraction = 0;
        }
        return MAD_FLOW_STOP;
    }

    return 0;
}

int calc_http_length(buffer *buf)
{
	if(http_file_length)
		buf->length = http_file_length;

	return 0;
}

int calc_length(char *file, buffer *buf)
{
    int f;
    struct stat filestat;
    void *fdm = NULL;
    char buffer[3];
    
    //if(!strstr(file,".mp3") && !strstr(file,".MP3"))
   // {
//	    mpg321_error(file);
//	    return -1;
  //  }

    f = open(file, O_RDONLY);

    if (f < 0)
    {
        mpg321_error(file);
        return -1;
    }

    if (fstat(f, &filestat) < 0)
    {
        mpg321_error(file);
        close(f);
        return -1;
    }

    if (!S_ISREG(filestat.st_mode))
    {
        fprintf(stderr, "%s: Not a regular file\n", file);
        close(f);
        return -1;
    }

    /* TAG checking is adapted from XMMS */
    buf->length = filestat.st_size;

    if (lseek(f, -128, SEEK_END) < 0)
    {
        /* File must be very short or empty. Forget it. */
        close(f);
        return -1;
    }    

    if (read(f, buffer, 3) != 3)
    {
        close(f);
        return -1;
    }
    
    if (!strncmp(buffer, "TAG", 3))
    {
        buf->length -= 128; /* Correct for id3 tags */
    }
#ifdef __uClinux__
    fdm = mmap(0, buf->length, PROT_READ, MAP_PRIVATE, f, 0);
#else    
    fdm = mmap(0, buf->length, PROT_READ, MAP_SHARED, f, 0);
#endif    
    if (fdm == MAP_FAILED)
    {
        mpg321_error(file);
        close(f);
        return -1;
    }

    /* Scan the file for a XING header, or calculate the length,
       or just scan the whole file and add everything up. */
    scan(fdm, buf->length, buf);

    if (munmap(fdm, buf->length) == -1)
    {
        mpg321_error(file);
        close(f);
        return -1;
    }

    if (close(f) < 0)
    {
        mpg321_error(file);
        return -1;
    }

    return 0;
}

/* The following two routines and data structure are from the ever-brilliant
   Rob Leslie.
*/

struct audio_dither {
  mad_fixed_t error[3];
  mad_fixed_t random;
};

/*
* NAME:        prng()
* DESCRIPTION: 32-bit pseudo-random number generator
*/
static inline
unsigned long prng(unsigned long state)
{
  return (state * 0x0019660dL + 0x3c6ef35fL) & 0xffffffffL;
}

/*
* NAME:        audio_linear_dither()
* DESCRIPTION: generic linear sample quantize and dither routine
*/
inline
signed long audio_linear_dither(unsigned int bits, mad_fixed_t sample,
                                struct audio_dither *dither)
{
  unsigned int scalebits;
  mad_fixed_t output, mask, random;

  enum {
    MIN = -MAD_F_ONE,
    MAX =  MAD_F_ONE - 1
  };

  /* noise shape */
  sample += dither->error[0] - dither->error[1] + dither->error[2];

  dither->error[2] = dither->error[1];
  dither->error[1] = dither->error[0] / 2;

  /* bias */
  output = sample + (1L << (MAD_F_FRACBITS + 1 - bits - 1));

  scalebits = MAD_F_FRACBITS + 1 - bits;
  mask = (1L << scalebits) - 1;

  /* dither */
  random  = prng(dither->random);
  output += (random & mask) - (dither->random & mask);

  dither->random = random;

  /* clip */
  if (output > MAX) {
    output = MAX;

    if (sample > MAX)
      sample = MAX;
  }
  else if (output < MIN) {
    output = MIN;

    if (sample < MIN)
      sample = MIN;
  }

  /* quantize */
  output &= ~mask;

  /* error feedback */
  dither->error[0] = sample - output;

  /* scale */
  return output >> scalebits;
}

static void perform_and_print(char *samples,fft_state *state)
{
	static int xranges[] = {0, 1, 2, 3, 5, 7, 10, 14, 20, 28, 40, 54, 74, 101, 137, 187, 255};
	static double fftout[FFT_BUFFER_SIZE / 2 +1];

	printf("@I FFT:");
	
	fft_perform((sound_sample *)samples, fftout, state);

	int i;
	for(i = 0; i < 256; i++)
	{
		fftout[i] = (int)(sqrt(fftout[i + 1])) >> 8;
	}

	for(i = 0; i < 16; i++)
	{
		int val = 0;
		int j;
		for(j = xranges[i]; j < xranges[i + 1]; j++)
		{
			val += (fftout[j]) / 128;
		}
		if( val > 127) val = 127;
		printf("%03i ", val);
	}
	printf("\n");
}

static int process_fft( char *data, int size)
{
	static fft_state *left_fftstate;
	static int init=0;
	static char buff_samples[1024];
	static int buff_sample_count;

	if(!((options.opt & MPG321_PRINT_FFT) && (options.opt & MPG321_REMOTE_PLAY))) return 0;

	if(!init)
	{
		buff_sample_count = 0;
		left_fftstate = fft_init();
		init = 1;
	}

	if(buff_sample_count && 0)
	{
		memcpy(&buff_samples[buff_sample_count], data, 1024 - buff_sample_count);
		data = &data[buff_sample_count];
		size = size - buff_sample_count;
		perform_and_print(buff_samples, left_fftstate);
		buff_sample_count = 0;
	}

	int samples;
	for( samples = 0; (samples + 1030) < size; samples += 1024)
	{
		perform_and_print(&data[samples],left_fftstate);
	}

	if(samples < size && 0)
	{
		memcpy(buff_samples, &data[samples], size - samples);
		buff_sample_count = size - samples;
	}
}

enum mad_flow output(void *data,
                     struct mad_header const *header,
                     struct mad_pcm *pcm)
{
    register int nsamples = pcm->length;
    mad_fixed_t const *left_ch = pcm->samples[0], *right_ch = pcm->samples[1];
    
    static unsigned char stream[1152*4]; /* 1152 because that's what mad has as a max; *4 because
                                    there are 4 distinct bytes per sample (in 2 channel case) */
    static unsigned int rate = 0;
    static int channels = 0;
    static struct audio_dither dither;

    register char * ptr = stream;
    register signed int sample;
    register mad_fixed_t tempsample;


    /* Semaphore operations */
    static struct sembuf read_sops = {0, -1, 0};
    static struct sembuf write_sops = {1, 1, 0};

    if(options.opt & MPG321_ENABLE_BUFFER)
    {
	    semop(semarray,&read_sops,1);
	    ptr = (Output_Queue+mad_decoder_position)->data;
	    memcpy(&((Output_Queue+mad_decoder_position)->header),header,sizeof(struct mad_header));
			 
    }else{
	    /* We need to know information about the file before we can open the playdevice
	       in some cases. So, we do it here. */
	    if (!playdevice)
	    {
	    	    channels = MAD_NCHANNELS(header);
	    	    rate = header->samplerate;
	    	    open_ao_playdevice(header);        
	    }
	    else if ((channels != MAD_NCHANNELS(header) || rate != header->samplerate) && playdevice_is_live())
	    {
	    	    ao_close(playdevice);
	    	    channels = MAD_NCHANNELS(header);
	    	    rate = header->samplerate;
	    	    open_ao_playdevice(header);        
	    }        
    }

    static int peak_rate = 0;
    static unsigned short int peak = 0;


    if (pcm->channels == 2)
    {
        while (nsamples--)
        {
            tempsample = mad_f_mul(*left_ch++, options.volume);
            sample = (signed int) audio_linear_dither(16, tempsample, &dither);

	    peak = abs(sample) > peak ? abs(sample) : peak;

#ifndef WORDS_BIGENDIAN
            *ptr++ = (unsigned char) (sample >> 0);
            *ptr++ = (unsigned char) (sample >> 8);
#else
            *ptr++ = (unsigned char) (sample >> 8);
            *ptr++ = (unsigned char) (sample >> 0);
#endif
            
            tempsample = mad_f_mul(*right_ch++, options.volume);
            sample = (signed int) audio_linear_dither(16, tempsample, &dither);

	    peak = abs(sample) > peak ? abs(sample) : peak;

#ifndef WORDS_BIGENDIAN
            *ptr++ = (unsigned char) (sample >> 0);
            *ptr++ = (unsigned char) (sample >> 8);
#else
            *ptr++ = (unsigned char) (sample >> 8);
            *ptr++ = (unsigned char) (sample >> 0);
#endif
        }
	
	process_fft(stream, pcm->length * 4);
	if(options.opt & MPG321_ENABLE_BUFFER)
	{
		(Output_Queue+mad_decoder_position)->length = pcm->length * 4;
	}else
	{
		ao_play(playdevice, stream, pcm->length * 4);
	}
    }
    
    else if (options.opt & MPG321_FORCE_STEREO)
    {
        while (nsamples--)
        {
            tempsample = mad_f_mul(*left_ch++, options.volume);
            sample = (signed int) audio_linear_dither(16, tempsample, &dither);

	    peak = abs(sample) > peak ? abs(sample) : peak;
            
            /* Just duplicate the sample across both channels. */
#ifndef WORDS_BIGENDIAN
            *ptr++ = (unsigned char) (sample >> 0);
            *ptr++ = (unsigned char) (sample >> 8);
            *ptr++ = (unsigned char) (sample >> 0);
            *ptr++ = (unsigned char) (sample >> 8);
#else
            *ptr++ = (unsigned char) (sample >> 8);
            *ptr++ = (unsigned char) (sample >> 0);
            *ptr++ = (unsigned char) (sample >> 8);
            *ptr++ = (unsigned char) (sample >> 0);
#endif
        }
	process_fft(stream, pcm->length * 4);
	if(options.opt & MPG321_ENABLE_BUFFER)
	{
		(Output_Queue+mad_decoder_position)->length = pcm->length * 4;
	}else
	{
		ao_play(playdevice, stream, pcm->length * 4);
	}
    }
        
    else /* Just straight mono output */
    {
        while (nsamples--)
        {
            tempsample = mad_f_mul(*left_ch++, options.volume);
            sample = (signed int) audio_linear_dither(16, tempsample, &dither);

	    peak = abs(sample) > peak ? abs(sample) : peak;
            
#ifndef WORDS_BIGENDIAN
            *ptr++ = (unsigned char) (sample >> 0);
            *ptr++ = (unsigned char) (sample >> 8);
#else
            *ptr++ = (unsigned char) (sample >> 8);
            *ptr++ = (unsigned char) (sample >> 0);
#endif
        }

	process_fft(stream, pcm->length * 2);
	if(options.opt & MPG321_ENABLE_BUFFER)
	{
		(Output_Queue+mad_decoder_position)->length = pcm->length * 2;
	}else
	{
		ao_play(playdevice, stream, pcm->length * 2);
	}
    }

    if(options.opt & MPG321_REMOTE_PLAY && (options.opt & MPG321_PRINT_FFT))
    {
	    peak_rate = (peak_rate + 1) % 7;
	    if(!peak_rate)
	    {
		    int peak_scale = (int)round(10 * (peak / 32768.0));
		    printf("@I Peak:%d\n", peak_scale);
		    peak = 0;
	    }
    }
    

    if(options.opt & MPG321_ENABLE_BUFFER)
    {
	    mad_decoder_position = getnext_place(mad_decoder_position);
	    semop(semarray,&write_sops,1);
	    Decoded_Frames->total_decoded_frames+=1;
	    ///fprintf(stderr,"%d\n",Decoded_Frames->total_decoded_frames);
    }
    return MAD_FLOW_CONTINUE;        
}
