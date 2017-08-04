/*
 *     mpg321 - a fully free clone of mpg123.
 *     Proxy Server Authentication Mechanism
 *     Copyright (C) 2006 Nanakos Chrysostomos <nanakos@wired-net.gr>
 *           
 *                                
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *     You should have received a copy of the GNU General Public License
 *     along with this program; if not, write to the Free Software
 *     Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *                                                                         
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>


/* base64 encoding... */
static const char base64digits[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#define ERRB     -1
static const char base64val[] = {
	    ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,
	    ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,ERRB,
	    ERRB,ERRB,ERRB,ERRB, ERRB,ERRB,ERRB,ERRB, ERRB,ERRB,ERRB, 62, ERRB,ERRB,ERRB, 63,
	    52, 53, 54, 55,  56, 57, 58, 59,  60, 61,ERRB,ERRB, ERRB,ERRB,ERRB,ERRB,
	    ERRB,  0,  1,  2,   3,  4,  5,  6,   7,  8,  9, 10,  11, 12, 13, 14,
	    15, 16, 17, 18,  19, 20, 21, 22,  23, 24, 25,ERRB, ERRB,ERRB,ERRB,ERRB,
	    ERRB, 26, 27, 28,  29, 30, 31, 32,  33, 34, 35, 36,  37, 38, 39, 40,
	    41, 42, 43, 44,  45, 46, 47, 48,  49, 50, 51,ERRB, ERRB,ERRB,ERRB,ERRB
};

#define DECODE64(c) (isascii(c) ? base64val[c] : ERRB)


unsigned char authstring[80];
extern char *basic_auth;
extern int auth_enable;
extern int auth_enable_var;

void do_base64(unsigned char *out, const unsigned char *in, int len)
{

	while (len >= 3) {
		*out++ = base64digits[in[0] >> 2];
		*out++ = base64digits[((in[0] << 4) & 0x30) | (in[1] >> 4)];
		*out++ = base64digits[((in[1] << 2) & 0x3c) | (in[2] >> 6)];
		*out++ = base64digits[in[2] & 0x3f];
		len -= 3;
		in += 3;
	}

	/* clean up remainder */
	if (len > 0) {
		unsigned char fragment;
		*out++ = base64digits[in[0] >> 2];
		fragment = (in[0] << 4) & 0x30;
		if (len > 1)
			fragment |= in[1] >> 4;
		*out++ = base64digits[fragment];
		*out++ = (len < 2) ? '=' : base64digits[(in[1] << 2) & 0x3c];
		*out++ = '=';
	}
	*out = '\0';
}

void do_basicauth() 
{

	char user_arg[16],pass_arg[16],*t,*p=NULL;
	char *user_arg_var,*pass_arg_var;
	int len;
	t=strchr(basic_auth,':');
	sscanf(t,":%s",(char *)&pass_arg);
	snprintf(user_arg,(int)(t-basic_auth)+1,"%s",basic_auth);	

	if(auth_enable && !auth_enable_var)
	{
		len=strlen(user_arg)+strlen(pass_arg)+2;
		p=(char *)malloc(len);
		sprintf(p,"%s:%s",user_arg,pass_arg);
	}else if(!auth_enable && auth_enable_var)
	{
		user_arg_var = getenv(user_arg);
		pass_arg_var = getenv(pass_arg);
		len=strlen(user_arg_var)+strlen(pass_arg_var)+2;
		p=(char *)malloc(len);
		sprintf(p,"%s:%s",user_arg_var,pass_arg_var);
	}
	
	do_base64(authstring,(unsigned char *)p,strlen(p));
	free(p);
}

