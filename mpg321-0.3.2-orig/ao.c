/*
    mpg321 - a fully free clone of mpg123.
    ao.c: Copyright (C) 2001, 2002 Joe Drew
    
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

void set_play_device(char *devicename)
{
    if (strcmp(devicename, "oss") == 0)
    {
        options.opt |= MPG321_USE_OSS;
    }

    else if (strcmp(devicename, "sun") == 0)
    {
        options.opt |= MPG321_USE_SUN;
    }

    else if (strcmp(devicename, "alsa") == 0)
    {
        options.opt |= MPG321_USE_ALSA;
    }

    else if (strcmp(devicename, "esd") == 0)
    {
        options.opt |= MPG321_USE_ESD;
    }

    else if (strcmp(devicename, "arts") == 0)
    {
        options.opt |= MPG321_USE_ARTS;
    }

    else if (strcmp(devicename, "alsa09") == 0)
    {
        options.opt |= MPG321_USE_ALSA09;
    }
    
    else
    {
        options.opt |= MPG321_USE_USERDEF;
        options.devicetype = strdup(devicename);
    }
}

#if 0
/* we use this if the default ao device fails....maybe */
void check_ao_default_play_device()
{
    char *default_device;        
    ao_info *default_info;
        
    int driver_id;
            
    driver_id = ao_default_driver_id();

    if (driver_id < 0)
    {
        fprintf(stderr, "No default libao driver available.\n");
        exit(1);
    }

    
}
#endif

void check_default_play_device()
{
    /* check that no output devices are currently selected */
    if (!(options.opt & (MPG321_USE_OSS | MPG321_USE_STDOUT | MPG321_USE_ALSA | MPG321_USE_ESD 
                         | MPG321_USE_NULL | MPG321_USE_WAV | MPG321_USE_ARTS | MPG321_USE_AU 
                         | MPG321_USE_CDR | MPG321_USE_ALSA09 | MPG321_USE_USERDEF)))
    {
        ao_info *default_info;
        
        /* set default output device & various other bits. this is here
           so that the device-specific inits in mad.c: open_ao_playdevice 
           can do their dirty work all on their own */

        if (strcmp(AUDIO_DEFAULT, "the libao default")==0) /* just use the libao-specified default.
                                                            This is the default when compiling. */
        {
            int unset = 1;
            int driver_id;
            
            /* ESD is spawned when executing the ao_default_driver_id routine.
               This causes a delay and is rather annoying, so we'll disable it here
               for now. */

#ifdef HAS_GETENV
            if (getenv("ESD_NO_SPAWN")) unset = 0; /* only unset it later 
                                                      if it's not already set */
#endif

#ifdef HAS_PUTENV
            putenv("ESD_NO_SPAWN=1");
#else
#ifdef HAS_SETENV
            setenv("ESD_NO_SPAWN", "1", 0);
#endif
#endif
            driver_id = ao_default_driver_id();

#ifdef HAS_PUTENV
            if (unset) putenv("ESD_NO_SPAWN");
#else
#ifdef HAS_UNSETENV
            if (unset) unsetenv("ESD_NO_SPAWN");
#endif
#endif

            if (driver_id < 0) {
                fprintf(stderr, "No default libao driver available.\n");
                exit(1);
            }

            default_info = ao_driver_info(driver_id);

            set_play_device(default_info->short_name);
        }
        
        else
        {
            set_play_device(AUDIO_DEFAULT);
        }

    }
}        


int check_default_play_device_buffer()
{
    /* check that no output devices are currently selected */
    if (!(options.opt & (MPG321_USE_OSS | MPG321_USE_STDOUT | MPG321_USE_ALSA | MPG321_USE_ESD 
                         | MPG321_USE_NULL | MPG321_USE_WAV | MPG321_USE_ARTS | MPG321_USE_AU 
                         | MPG321_USE_CDR | MPG321_USE_ALSA09 | MPG321_USE_USERDEF)))
    {
        ao_info *default_info;
        
        /* set default output device & various other bits. this is here
           so that the device-specific inits in mad.c: open_ao_playdevice 
           can do their dirty work all on their own */

        if (strcmp(AUDIO_DEFAULT, "the libao default")==0) /* just use the libao-specified default.
                                                            This is the default when compiling. */
        {
            int unset = 1;
            int driver_id;
            
            /* ESD is spawned when executing the ao_default_driver_id routine.
               This causes a delay and is rather annoying, so we'll disable it here
               for now. */

#ifdef HAS_GETENV
            if (getenv("ESD_NO_SPAWN")) unset = 0; /* only unset it later 
                                                      if it's not already set */
#endif

#ifdef HAS_PUTENV
            putenv("ESD_NO_SPAWN=1");
#else
#ifdef HAS_SETENV
            setenv("ESD_NO_SPAWN", "1", 0);
#endif
#endif
            driver_id = ao_default_driver_id();

#ifdef HAS_PUTENV
            if (unset) putenv("ESD_NO_SPAWN");
#else
#ifdef HAS_UNSETENV
            if (unset) unsetenv("ESD_NO_SPAWN");
#endif
#endif

            if (driver_id < 0) {
                fprintf(stderr, "No default libao driver available.\n");
                return (-1);
            }

            default_info = ao_driver_info(driver_id);

            set_play_device(default_info->short_name);
        }
        
        else
        {
            set_play_device(AUDIO_DEFAULT);
        }

    }
    return 1;
}

int playdevice_is_live()
{
    int driver_id=0;

        if(options.opt & MPG321_USE_AU)
        {
            driver_id = ao_driver_id("au");
        }

        else if (options.opt & MPG321_USE_CDR)
        {
            driver_id = ao_driver_id("raw");
        }

        else if(options.opt & MPG321_USE_WAV)
        {
            driver_id = ao_driver_id("wav");
        }

        else if(options.opt & MPG321_USE_NULL)
        {
            driver_id = ao_driver_id("null"); 
        }

        else if (options.opt & MPG321_USE_STDOUT)
        {
            driver_id = ao_driver_id("raw");
        }

        else if(options.opt & MPG321_USE_ESD)
        {
            driver_id = ao_driver_id("esd");
        }        

        else if(options.opt & MPG321_USE_ARTS)
        {
            driver_id = ao_driver_id("arts");
        }

        else if(options.opt & MPG321_USE_ALSA)
        {
            driver_id = ao_driver_id("alsa");
        }

        else if(options.opt & MPG321_USE_OSS)
        {
            driver_id = ao_driver_id("oss");
        }

        else if(options.opt & MPG321_USE_SUN)
        {
            driver_id = ao_driver_id("sun");
        }
        
        else if (options.opt & MPG321_USE_ALSA09)
        {
            driver_id = ao_driver_id("alsa09");
        }
        
        else if (options.opt & MPG321_USE_USERDEF)
        {
            driver_id = ao_driver_id(options.devicetype);
        }

    return (ao_driver_info(driver_id)->type == AO_TYPE_LIVE);
}

void open_ao_playdevice(struct mad_header const *header)
{
        ao_sample_format format;

        /* Because these can sometimes block, we stop our custom signal handler,
           and restore it afterwards */
        signal(SIGINT, SIG_DFL);
        
        format.bits = 16;
        format.rate = header->samplerate;
        format.channels = (options.opt & MPG321_FORCE_STEREO) ? 2 : MAD_NCHANNELS(header);
	
	/* Add this element as an option to mpg321 */
	format.matrix = "L,R";

        /* mad gives us little-endian data; we swap it on big-endian targets, to
          big-endian format, because that's what most drivers expect. */
        format.byte_format = AO_FMT_NATIVE; 
        
        if(options.opt & MPG321_USE_AU)
        {
            int driver_id = ao_driver_id("au");
            ao_option *ao_options = NULL;

            /* Don't have to check options.device here: we only define
               MPG321_USE_AU when --au <aufile> is defined, and <aufile>
               is pointd to by options.device */
            if((playdevice=ao_open_file(driver_id, options.device, 1 /*overwrite*/,
                    &format, ao_options))==NULL)
            {
                fprintf(stderr, "Error opening libao file output driver to write AU data.\n");
                exit(1);
            }
        }

        else if (options.opt & MPG321_USE_CDR)
        {
            ao_option * ao_options = NULL;
            int driver_id = ao_driver_id("raw");

            /* because CDR is a special format, i.e. headerless PCM, big endian,
               this is a special case. */
            ao_append_option(&ao_options, "byteorder", "big");
        
            if((playdevice=ao_open_file(driver_id, options.device, 1 /*overwrite*/, 
                    &format, ao_options))==NULL)
            {
                fprintf(stderr, "Error opening libao file output driver to write CDR data.\n");
                exit(1);
            }
        }
        
        /* if the user specifies both au and wave, wav will be prefered, so testing
         * later */
        else if(options.opt & MPG321_USE_WAV)
        {
            int driver_id = ao_driver_id("wav");
            ao_option *ao_options = NULL;

            /* Don't have to check options.device here: we only define
               MPG321_USE_WAV when -w <wavfile> is defined, and <wavfile>
               is pointd to by options.device */
            if((playdevice=ao_open_file(driver_id, options.device, 1 /*overwrite*/,
                    &format, ao_options))==NULL)
            {
                fprintf(stderr, "Error opening libao wav file driver. (Do you have write permissions?)\n");
                exit(1);
            }
        }

        else if(options.opt & MPG321_USE_NULL)
        {
            int driver_id = ao_driver_id("null"); 
            /* null is dirty, create a proper options struct later */

            if((playdevice = ao_open_live(driver_id, &format, NULL)) == NULL)
            {
                fprintf(stderr, "Error opening libao null driver. (This shouldn't have happened.)\n");
                exit(1);
            }
        }
        
        else if (options.opt & MPG321_USE_STDOUT)
        {
            ao_option * ao_options = NULL;
            int driver_id = ao_driver_id("raw");

            /* stdout output is expected to be little-endian generally */
            ao_append_option(&ao_options, "byteorder", "little");
        
            if((playdevice=ao_open_file(driver_id, "-", 1 /*overwrite*/, 
                    &format, ao_options))==NULL)
            {
                fprintf(stderr, "Error opening libao raw output driver.\n");
                exit(1);
            }
        }
        
        else if (options.opt & MPG321_USE_USERDEF)
        {
            ao_option *ao_options = NULL;
            int driver_id = ao_driver_id(options.devicetype);
            
	    if (driver_id < 0)
            {
                fprintf(stderr, "Can't open unknown ao driver %s\n", options.devicetype);
                exit(1);
            }

            if (playdevice_is_live())
            {
                if (options.device)
                {
                    fprintf(stderr, "Can't set output device to %s for unknown ao plugin %s",
                            options.device, options.devicetype);
                }

                if((playdevice=ao_open_live(driver_id, &format, ao_options))==NULL)
                {
                    fprintf(stderr, "Error opening unknown libao %s driver. (Is device in use?)\n",
                        options.devicetype);
                    exit(1);
                }
            }

            else
            {
                if (options.device)
                {
                    /* Just assume that options.device is a filename. The user can shoot 
                       themselves in the foot all they like... */
                    if((playdevice=ao_open_file(driver_id, options.device, 1 /*overwrite*/,
                            &format, ao_options))==NULL)
                    {
                        fprintf(stderr, "Error opening unknown libao %s file driver for file %s. (Do you have write permissions?)\n",
                                options.devicetype, options.device);
                        exit(1);
                    }
                }
                
                else
                {
                    fprintf(stderr, "Filename must be specified (with -a filename) for unknown ao driver %s\n",
                            options.devicetype);
                }
            }
        } else {
            /* Hack-tacular. This code tries to the device as specified; if it can't, it'll
               fall through to the other devices, trying each in turn. If the user specified
               a device to use, though, it won't fall through: principle of least surprise */
        int opened = 0;

        if(!opened && options.opt & MPG321_USE_ALSA)
        {
            ao_option *ao_options = NULL;
            int driver_id = ao_driver_id("alsa");
            char *c;
            char *card=(char *)malloc((int)16);
            memset(card,0,16); 
	    strncat(card,"alsa:\0",6);
	    
	    if (options.device)
            {
	    	strcat(card,options.device);
                //if ((c = strchr(options.device, ':')) == NULL || strlen(c+1) < 1)
                if ((c = strchr(card, ':')) == NULL || strlen(c+1) < 1)
                {
                    fprintf(stderr, "Poorly formed ALSA card:device specification %s", options.device);
                    exit(1);
                }

                *(c++) = '\0'; /* change the : to a null to create two separate strings */
                //ao_append_option(&ao_options, "card", options.device);
                ao_append_option(&ao_options, "card", card);
                ao_append_option(&ao_options, "dev", c);
            }

            if((playdevice=ao_open_live(driver_id, &format, ao_options))==NULL)
            {
                if (options.device)
                {
                    fprintf(stderr, "Can't open libao driver with device %s (is device in use?)\n",
                        options.device);
                    exit(1);
                }
                else
                options.opt |= MPG321_USE_ALSA09;
            } else opened++;
        }

        if(!opened && options.opt & MPG321_USE_ALSA09)
        {
            ao_option *ao_options = NULL;
            int driver_id = ao_driver_id("alsa09");

            if(options.device)
                ao_append_option(&ao_options, "dev", options.device);

            if((playdevice=ao_open_live(driver_id, &format, ao_options))==NULL)
            {
                if (options.device)
                {
                    fprintf(stderr, "Can't open libao driver with device %s (is device in use?)\n",
                        options.device);
                    exit(1);
                }
                else
                options.opt |= MPG321_USE_OSS;
            } else opened++;
        }

        if(!opened &&  options.opt & MPG321_USE_OSS)
        {
            ao_option *ao_options = NULL;
            int driver_id = ao_driver_id("oss");

            if(options.device)
                ao_append_option(&ao_options, "dsp", options.device);

            if((playdevice=ao_open_live(driver_id, &format, ao_options))==NULL)
            {
                if (options.device)
                {
                    fprintf(stderr, "Can't open libao driver with device %s (is device in use?)\n",
                        options.device);
                    exit(1);
                }
                else
                options.opt |= MPG321_USE_SUN;
            } else opened++;
        }

        if(!opened && options.opt & MPG321_USE_SUN)
        {
            ao_option *ao_options = NULL;
            int driver_id = ao_driver_id("sun");

            if(options.device)
                ao_append_option(&ao_options, "dev", options.device);

            if((playdevice=ao_open_live(driver_id, &format, ao_options))==NULL)
            {
                if (options.device)
                {
                    fprintf(stderr, "Can't open libao driver with device %s (is device in use?)\n",
                        options.device);
                    exit(1);
                }
                else
                options.opt |= MPG321_USE_ESD;
            }
        }

        if(!opened && options.opt & MPG321_USE_ESD)
        {
            ao_option *ao_options = NULL;
            int driver_id = ao_driver_id("esd");

            if(options.device)
                ao_append_option(&ao_options, "host", options.device);

            if((playdevice=ao_open_live(driver_id, &format, ao_options))==NULL)
            {
                if (options.device)
                {
                    fprintf(stderr, "Can't open libao driver with device %s (is device in use?)\n",
                        options.device);
                    exit(1);
                }
                else
                options.opt |= MPG321_USE_ARTS;
            } else opened++;
        }

        if(!opened && options.opt & MPG321_USE_ARTS)
        {
            ao_option *ao_options = NULL;
            int driver_id = ao_driver_id("arts");
        
            if((playdevice=ao_open_live(driver_id, &format, ao_options))==NULL)
            {
                fprintf(stderr, "Can't find a suitable libao driver. (Is device in use?)\n");
                exit(1);
            }
        }
        
    }

    /* Restore signal handler */
    signal(SIGINT, handle_signals);
}



ao_device *open_ao_playdevice_buffer(struct mad_header const *header)
{
        ao_sample_format format;
	ao_device *pldev = NULL;

        /* Because these can sometimes block, we stop our custom signal handler,
           and restore it afterwards */
        signal(SIGINT, SIG_DFL);
        
        format.bits = 16;
        format.rate = header->samplerate;
        format.channels = (options.opt & MPG321_FORCE_STEREO) ? 2 : MAD_NCHANNELS(header);
	
	/* Add this element as an option to mpg321 */
	format.matrix = "L,R";

        /* mad gives us little-endian data; we swap it on big-endian targets, to
          big-endian format, because that's what most drivers expect. */
        format.byte_format = AO_FMT_NATIVE; 
        
        if(options.opt & MPG321_USE_AU)
        {
            int driver_id = ao_driver_id("au");
            ao_option *ao_options = NULL;

            /* Don't have to check options.device here: we only define
               MPG321_USE_AU when --au <aufile> is defined, and <aufile>
               is pointd to by options.device */
            if((pldev=ao_open_file(driver_id, options.device, 1 /*overwrite*/,
                    &format, ao_options))==NULL)
            {
                fprintf(stderr, "Error opening libao file output driver to write AU data.\n");
                return NULL;
            }
        }

        else if (options.opt & MPG321_USE_CDR)
        {
            ao_option * ao_options = NULL;
            int driver_id = ao_driver_id("raw");

            /* because CDR is a special format, i.e. headerless PCM, big endian,
               this is a special case. */
            ao_append_option(&ao_options, "byteorder", "big");
        
            if((pldev=ao_open_file(driver_id, options.device, 1 /*overwrite*/, 
                    &format, ao_options))==NULL)
            {
                fprintf(stderr, "Error opening libao file output driver to write CDR data.\n");
                return NULL;
            }
        }
        
        /* if the user specifies both au and wave, wav will be prefered, so testing
         * later */
        else if(options.opt & MPG321_USE_WAV)
        {
            int driver_id = ao_driver_id("wav");
            ao_option *ao_options = NULL;

            /* Don't have to check options.device here: we only define
               MPG321_USE_WAV when -w <wavfile> is defined, and <wavfile>
               is pointd to by options.device */
            if((pldev=ao_open_file(driver_id, options.device, 1 /*overwrite*/,
                    &format, ao_options))==NULL)
            {
                fprintf(stderr, "Error opening libao wav file driver. (Do you have write permissions?)\n");
                return NULL;
            }
        }

        else if(options.opt & MPG321_USE_NULL)
        {
            int driver_id = ao_driver_id("null"); 
            /* null is dirty, create a proper options struct later */

            if((pldev = ao_open_live(driver_id, &format, NULL)) == NULL)
            {
                fprintf(stderr, "Error opening libao null driver. (This shouldn't have happened.)\n");
                return NULL;
            }
        }
        
        else if (options.opt & MPG321_USE_STDOUT)
        {
            ao_option * ao_options = NULL;
            int driver_id = ao_driver_id("raw");

            /* stdout output is expected to be little-endian generally */
            ao_append_option(&ao_options, "byteorder", "little");
        
            if((pldev=ao_open_file(driver_id, "-", 1 /*overwrite*/, 
                    &format, ao_options))==NULL)
            {
                fprintf(stderr, "Error opening libao raw output driver.\n");
                return NULL;
            }
        }
        
        else if (options.opt & MPG321_USE_USERDEF)
        {
            ao_option *ao_options = NULL;
            int driver_id = ao_driver_id(options.devicetype);
            
	    if (driver_id < 0)
            {
                fprintf(stderr, "Can't open unknown ao driver %s\n", options.devicetype);
                exit(1);
            }

            if (playdevice_is_live())
            {
                if (options.device)
                {
                    fprintf(stderr, "Can't set output device to %s for unknown ao plugin %s",
                            options.device, options.devicetype);
                }

                if((pldev=ao_open_live(driver_id, &format, ao_options))==NULL)
                {
                    fprintf(stderr, "Error opening unknown libao %s driver. (Is device in use?)\n",
                        options.devicetype);
                    return NULL;
                }
            }

            else
            {
                if (options.device)
                {
                    /* Just assume that options.device is a filename. The user can shoot 
                       themselves in the foot all they like... */
                    if((pldev=ao_open_file(driver_id, options.device, 1 /*overwrite*/,
                            &format, ao_options))==NULL)
                    {
                        fprintf(stderr, "Error opening unknown libao %s file driver for file %s. (Do you have write permissions?)\n",
                                options.devicetype, options.device);
                        return NULL;
                    }
                }
                
                else
                {
                    fprintf(stderr, "Filename must be specified (with -a filename) for unknown ao driver %s\n",
                            options.devicetype);
                }
            }
        } else {
            /* Hack-tacular. This code tries to the device as specified; if it can't, it'll
               fall through to the other devices, trying each in turn. If the user specified
               a device to use, though, it won't fall through: principle of least surprise */
        int opened = 0;

        if(!opened && options.opt & MPG321_USE_ALSA)
        {
            ao_option *ao_options = NULL;
            int driver_id = ao_driver_id("alsa");
            char *c;
            char *card=(char *)malloc((int)16);
            memset(card,0,16); 
	    strncat(card,"alsa:\0",6);
	    
	    if (options.device)
            {
	    	strcat(card,options.device);
                //if ((c = strchr(options.device, ':')) == NULL || strlen(c+1) < 1)
                if ((c = strchr(card, ':')) == NULL || strlen(c+1) < 1)
                {
                    fprintf(stderr, "Poorly formed ALSA card:device specification %s", options.device);
                    exit(1);
                }

                *(c++) = '\0'; /* change the : to a null to create two separate strings */
                //ao_append_option(&ao_options, "card", options.device);
                ao_append_option(&ao_options, "card", card);
                ao_append_option(&ao_options, "dev", c);
            }

            if((pldev=ao_open_live(driver_id, &format, ao_options))==NULL)
            {
                if (options.device)
                {
                    fprintf(stderr, "Can't open libao driver with device %s (is device in use?)\n",
                        options.device);
                    return NULL;
                }
                else
                options.opt |= MPG321_USE_ALSA09;
            } else opened++;
        }

        if(!opened && options.opt & MPG321_USE_ALSA09)
        {
            ao_option *ao_options = NULL;
            int driver_id = ao_driver_id("alsa09");

            if(options.device)
                ao_append_option(&ao_options, "dev", options.device);

            if((pldev=ao_open_live(driver_id, &format, ao_options))==NULL)
            {
                if (options.device)
                {
                    fprintf(stderr, "Can't open libao driver with device %s (is device in use?)\n",
                        options.device);
                    return NULL;
                }
                else
                options.opt |= MPG321_USE_OSS;
            } else opened++;
        }

        if(!opened &&  options.opt & MPG321_USE_OSS)
        {
            ao_option *ao_options = NULL;
            int driver_id = ao_driver_id("oss");

            if(options.device)
                ao_append_option(&ao_options, "dsp", options.device);

            if((pldev=ao_open_live(driver_id, &format, ao_options))==NULL)
            {
                if (options.device)
                {
                    fprintf(stderr, "Can't open libao driver with device %s (is device in use?)\n",
                        options.device);
                    return NULL;
                }
                else
                options.opt |= MPG321_USE_SUN;
            } else opened++;
        }

        if(!opened && options.opt & MPG321_USE_SUN)
        {
            ao_option *ao_options = NULL;
            int driver_id = ao_driver_id("sun");

            if(options.device)
                ao_append_option(&ao_options, "dev", options.device);

            if((pldev=ao_open_live(driver_id, &format, ao_options))==NULL)
            {
                if (options.device)
                {
                    fprintf(stderr, "Can't open libao driver with device %s (is device in use?)\n",
                        options.device);
                    return NULL;
                }
                else
                options.opt |= MPG321_USE_ESD;
            }
        }

        if(!opened && options.opt & MPG321_USE_ESD)
        {
            ao_option *ao_options = NULL;
            int driver_id = ao_driver_id("esd");

            if(options.device)
                ao_append_option(&ao_options, "host", options.device);

            if((pldev=ao_open_live(driver_id, &format, ao_options))==NULL)
            {
                if (options.device)
                {
                    fprintf(stderr, "Can't open libao driver with device %s (is device in use?)\n",
                        options.device);
                    return NULL;
                }
                else
                options.opt |= MPG321_USE_ARTS;
            } else opened++;
        }

        if(!opened && options.opt & MPG321_USE_ARTS)
        {
            ao_option *ao_options = NULL;
            int driver_id = ao_driver_id("arts");
        
            if((pldev=ao_open_live(driver_id, &format, ao_options))==NULL)
            {
                fprintf(stderr, "Can't find a suitable libao driver. (Is device in use?)\n");
                return NULL;
            }
        }
        
    }

    /* Restore signal handler */
    signal(SIGINT, handle_signals);
    return pldev;
}
