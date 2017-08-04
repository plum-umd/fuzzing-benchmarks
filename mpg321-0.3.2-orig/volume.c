/*
   mpg321 - a fully free clone of mpg123.
   Copyright (C) 2006-2012 Nanakos Chrysostomos
   volume.c: Copyright (C) 2011 Nanakos Chrysostomos
	
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
#ifdef HAVE_ALSA
#include "mpg321.h"
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

snd_mixer_t *mixer;
snd_mixer_elem_t *mixerelem;
long volume_min,volume_max;


int init_alsa_volume_control(char *name)
{
	char *elemnam;
	snd_mixer_open(&mixer,0);
	snd_mixer_attach(mixer,name);
	snd_mixer_selem_register(mixer,NULL,NULL);
	snd_mixer_load(mixer);

	mixerelem = snd_mixer_first_elem(mixer);

	while (mixerelem) {
	
		elemnam = snd_mixer_selem_get_name(mixerelem);
		/* It should work on most of the systems. Tested on Debian, Fedora, Gentoo, Ubuntu, RedHat, CentOS */
		if (strcasecmp(elemnam, "Master") == 0) {
			snd_mixer_selem_get_playback_volume_range(mixerelem,&volume_min,&volume_max);
			return 0;
		}
		mixerelem = snd_mixer_elem_next(mixerelem);
	}

 return 1;
}

void mpg321_alsa_set_volume(long value)
{
	if(mixerelem)
	      	snd_mixer_selem_set_playback_volume_all(mixerelem,value);
}

long mpg321_alsa_get_volume()
{
	long volume;
	if(mixerelem)
	      	snd_mixer_selem_get_playback_volume(mixerelem,SND_MIXER_SCHN_FRONT_RIGHT,&volume);
	return volume;
}
#endif
