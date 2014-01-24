README for PortAudio version 14
Implementations for PC DirectSound and Mac SoundManager

This is a streamlined distribution of PortAudio v14. 
It contains only the minimum files necessary for building on a windows platform.
For a full distribution that includes docs and other platforms see PortAudio web site
http://portaudio.com/


/*
 * PortAudio Portable Real-Time Audio Library
 * Latest Version at: http://www.softsynth.com/portaudio/
 * DirectSound Implementation
 * Copyright (c) 1999-2000 Phil Burk and Ross Bencina
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

PortAudio is a portable audio I/O library designed for cross-platform
support of audio. It uses a callback mechanism to request audio processing.
Audio can be generated in various formats including 32 bit floating point
and will be converted to the native format internally.

Documentation:
    not included

Files:
    pa_common/              = platform independant code
    pa_common/portaudio.h   = header file for PortAudio API. Specifies API.
    pa_common/pa_lib.c      = host independant code for DirectSound and Macintosh.
    pa_common/pa_rbuf.c     = ring Buffer used by blocking read/write.
    pa_common/pa_rw.c       = blocking read/write tool.

DirectSound Files:
    pa_win_ds/pa_dsound.c   = implementation of PA for DirectSound on a PC
    pa_win_ds/dsound_wrapper.cpp = wrapper for DirectSound C++ calls

Macintosh Files:
    pa_mac/pa_mac.c         = implementation for Macintosh Soundmanager

Test Programs
    not included
    
Building PortAudio for DirectSound on PC

    PortAudio uses DirectSound so you must have a recent copy of DirectX
    from Microsoft installed on your computer.

    You must link your program with "dsound.lib" and "winmm.lib".

    Compile together:
        pa_win_ds\pa_dsound.c
        pa_win_ds\dsound_wrapper.c
        pa_common\pa_lib.c

Building PortAudio for SoundManager on Apple Macintosh

    Compile together:
        pa_mac\pa_mac.c
        pa_common\pa_lib.c

    Link with:
        SoundLib

