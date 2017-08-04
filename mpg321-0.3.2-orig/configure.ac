# mpg321 - a fully free clone of mpg123.
# Copyright (C) 2001 Joe Drew
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)
AC_INIT
AC_CONFIG_SRCDIR([mad.c])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE(mpg321, 0.3.2)

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

# Checks for libraries.
AC_CHECK_LIB(mad,mad_decoder_run,,AC_MSG_ERROR(libmad is required to compile mpg321. See http://www.mars.org/home/rob/proj/mpeg/))
AC_CHECK_LIB(z, compress2,,AC_MSG_ERROR("zlib is required to compile mpg321. Usually this is in a package called zlib-devel or zlib1g-dev."))
AC_CHECK_LIB(id3tag,id3_tag_new,,AC_MSG_ERROR("libid3tag is required to compile mpg321. Find it in the MAD distribution at http://www.mars.org/home/rob/proj/mpeg/ - version 0.14.1 or better."), -lz)
AC_CHECK_LIB(pthread, pthread_create,,AC_MSG_ERROR("POSIX Threads is required to compile mpg321."))

XIPH_PATH_AO(,AC_MSG_ERROR(libao needed!))

CFLAGS="$CFLAGS $AO_CFLAGS"
LIBS="$LIBS $AO_LIBS"

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h errno.h fcntl.h limits.h netdb.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h])

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h sys/ioctl.h sys/time.h unistd.h)
AC_HEADER_TIME

# Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN
AC_C_CONST
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_STAT
AC_FUNC_MMAP
AC_TYPE_SIGNAL

# From lsh-1.3.4:
# Some systems (in particular, Unixware) doesn't have socklen_t, but
# uses size_t. Falling back to int will cause some warnings.

# AC_CHECK_TYPE doesn't work, probably because the type in
# question is not defined in sys/types.h.

AC_TYPE_SOCKLEN_T


dnl Check for LFS
AC_SYS_LARGEFILE


AC_CHECK_FUNCS([gethostbyname memset munmap socket strchr strdup strerror strrchr strstr gettimeofday select getenv putenv setenv unsetenv strcasecmp])

AC_ARG_ENABLE(mpg123_symlink,
[  --enable-mpg123-symlink Enable symlink of mpg123 to mpg321 [[default=yes]] ],
,enable_mpg123_symlink=yes)
AM_CONDITIONAL(DO_SYMLINK, test x$enable_mpg123_symlink = xyes)

AC_ARG_ENABLE(ipv6,
[  --enable-ipv6           enable IPv6 support [[default: no]] ],
        ipv6=$enableval, ipv6=no)
AC_ARG_ENABLE(debug,
[  --enable-debug	Debug messages to stderr. Only for developers. [[default=no]] ],
		debug=$enableval, debug=no)


dnl get the flags
#CFLAGS="${CFLAGS=}"
dnl this macro is used to get the arguments supplied
dnl to the configure script (./configure --enable-debug)
dnl Check if we have enable debug support.
AC_MSG_CHECKING(whether to enable ALSA)
alsa_default="yes"
AC_ARG_ENABLE(alsa, [  --enable-alsa=[no/yes] enable ALSA
                       [default=yes]],, enable_alsa=$alsa_default)
dnl Yes, shell scripts can be used
if test "x$enable_alsa" = "xyes"; then
        CFLAGS="$CFLAGS -DHAVE_ALSA"
	check_alsa="yes"
        AC_MSG_RESULT(yes)
else
        CFLAGS="$CFLAGS"
	LIBS="$LIBS -lz -lpthread -lm"
	check_alsa="no"
        AC_MSG_RESULT(no)
fi


if test "x$check_alsa" = "xyes"; then
	AC_CHECK_LIB(asound, snd_mixer_selem_get_playback_volume,,AC_MSG_ERROR("ALSA lib is required to compile mpg321."))
	LIBS="$LIBS -lz -lpthread -lm -lasound"
fi

# borrowed from a mail from Russ Allbery - from INN
dnl The rat's nest of networking libraries.  The common cases are not to
dnl need any extra libraries, or to need -lsocket -lnsl.  We need to avoid
dnl linking with libnsl unless we need it, though, since on some OSes where
dnl it isn't necessary it will totally break networking.  Unisys also
dnl includes gethostbyname in libsocket but needs libnsl for socket().
AC_SEARCH_LIBS(gethostbyname, nsl)
AC_CHECK_FUNC(socket, , [
    AC_CHECK_LIB(socket, socket, LIBS="$LIBS -lsocket", [
        AC_CHECK_LIB(nsl, socket, LIBS="$LIBS -lsocket -lnsl", , -lsocket)
    ], "$LIBS")
])

AC_ARG_WITH(default_audio, 
[  --with-default-audio=DA Specify default libao output plugin to use 
                            [[oss,sun,alsa(09),esd,arts]] ])
                            
if test "x$with_default_audio" != "x" -a "x$with_default_audio" != "xyes"; then
    AC_DEFINE_UNQUOTED(AUDIO_DEFAULT, "$with_default_audio",[Define the default libao output device.])
else
# yes, literally 'the libao default', since this is used in output strings
    AC_DEFINE_UNQUOTED(AUDIO_DEFAULT, "the libao default", [Define the default libao output device.])
fi

dnl *********************************************************************
dnl ** IPv6 *************************************************************
dnl *********************************************************************

if test "$ipv6" = yes; then
        AC_CHECK_FUNCS(getaddrinfo, have_getaddrinfo=yes)
        AC_MSG_CHECKING(whether to enable IPv6 support)
        if test "$have_getaddrinfo" = yes; then
                AC_MSG_RESULT(yes)
                AC_DEFINE(USE_IPV6, 1, [Define if you want IPv6 support.])
        else
                ipv6=no
                AC_MSG_RESULT(no)
        fi
fi

dnl *********************************************************************
dnl ******** DEBUG VERSION **********************************************
dnl *********************************************************************
if test "$debug" = yes; then
	AC_DEFINE(DEBUG, 1, [Define to build debug version])
	CFLAGS="-ggdb $CFLAGS"
	AC_SUBST(CFLAGS)
	LIBS="-lefence $LIBS"
	AC_SUBST(LIBS)
fi

dnl *********************************************************************
dnl ** Installation path
dnl *********************************************************************
dnl Set PACKAGE PREFIX
if test "x${exec_prefix}" = "xNONE"; then
	if test "x${prefix}" = "xNONE"; then
		packageprefix=${ac_default_prefix}/bin
	else
		packageprefix=${prefix}/bin
	fi
else
	packageprefix=${exec_prefix}
fi
AC_DEFINE_UNQUOTED(MPG321_PATH, "${packageprefix}/mpg321",[The installation path])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
