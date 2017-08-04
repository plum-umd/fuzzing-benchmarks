mpg321 now implements mpg123's 'Remote Control' interface, via option
-R. This is useful if, for example, you're writing a frontend to mpg321
which needs a consitent, reliable interface to control playback.

The Remote Control Interface has a small quirk, consistent with that of
mpg123: you must specify a dummy argument on the command-line. That
is to say, running `mpg321 -R' will bring up the usage message; running
`mpg321 -R abcd' (or anything else in place of 'abcd') will start the
Remote Control Interface.

Once you're running with the Remote Control Interface, there are 
several commands you can use:

COMMANDS:
--------
(All commands can be shortened to first character only)

LOAD <file>

Loads and starts playing <file>.

JUMP [+-]<frames>
If '+' or '-' is specified, jumps <frames> frames forward, or backwards,
respectively, in the the mp3 file.  If neither is specifies, jumps to
absolute frame <frames> in the mp3 file.

PAUSE
Pauses the playback of the mp3 file; if already paused, restarts playback.

STOP
Stops the playback of the mp3 file.

QUIT
Quits mpg321.

GAIN <0..100>
Sets sound volume, as a percentage. Analagous to -g.

There are also several outputs possible:

OUTPUT:
------

@R MPG123
mpg123 tagline. Output at startup.

@I mp3-filename
Prints out the filename of the mp3 file, minus the extension, or its ID3
informations if available, in the form <title> <artist> <album>
<year> <comment> <genre>.
Happens after an mp3 file has been loaded.

@S <a> <b> <c> <d> <e> <f> <g> <h> <i> <j> <k> <l>
Outputs information about the mp3 file after loading.
<a>: version of the mp3 file. Currently always 1.0 with madlib, but don't 
     depend on it, particularly if you intend portability to mpg123 as well.
     Float/string.
<b>: layer: 1, 2, or 3. Integer.
<c>: Samplerate. Integer.
<d>: Mode string. String.
<e>: Mode extension. Integer.
<f>: Bytes per frame (estimate, particularly if the stream is VBR). Integer.
<g>: Number of channels (1 or 2, usually). Integer.
<h>: Is stream copyrighted? (1 or 0). Integer.
<i>: Is stream CRC protected? (1 or 0). Integer.
<j>: Emphasis. Integer.
<k>: Bitrate, in kbps. (i.e., 128.) Integer.
<l>: Extension. Integer.

@F <current-frame> <frames-remaining> <current-time> <time-remaining>
Frame decoding status updates (once per frame).
Current-frame and frames-remaining are integers; current-time and
time-remaining floating point numbers with two decimal places.

@P {0, 1, 2, 3}
Stop/pause status.
0 - Playing has stopped. When 'STOP' is entered, or the mp3 file is finished.
1 - Playing is paused. Enter 'PAUSE' or 'P' to continue.
2 - Playing has begun again.
3 - Song has ended.

@E <message>
Unknown command or missing parameter.
