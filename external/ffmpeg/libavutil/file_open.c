/*
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#ifdef WIN32
#pragma warning(disable:4244 4018 4101 4996 4554 4090 4005 4305 4146)
#endif
#include "config.h"
#include "internal.h"
#include "mem.h"
//#include <fcntl.h>
//#include <sys/stat.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_IO_H
#include <io.h>
#endif

#if defined(_WIN32) && !defined(__MINGW32CE__)
#undef open
#undef lseek
#undef stat
#undef fstat
#include <windows.h>
#include <share.h>
#include <errno.h>
//#include "wchar_filename.h"
#if 0
static int win32_open(const char *filename_utf8, int oflag, int pmode)
{
    int fd;
    wchar_t *filename_w;

    /* convert UTF-8 to wide chars */
    if (utf8towchar(filename_utf8, &filename_w))
        return -1;
    if (!filename_w)
        goto fallback;

    fd = _wsopen(filename_w, oflag, SH_DENYNO, pmode);
    av_freep(&filename_w);

    if (fd != -1 || (oflag & O_CREAT))
        return fd;

fallback:
    /* filename may be in CP_ACP */
    return _sopen(filename_utf8, oflag, SH_DENYNO, pmode);
}
#define open win32_open
#endif
#endif
int avpriv_open(const char *filename, int flags, ...)
{
  return -1;
}

/*FILE *av_fopen_utf8(const char *path, const char *mode)
{
    int fd;
    int access;
    const char *m = mode;

    switch (*m++) {
    case 'r': access = O_RDONLY; break;
    case 'w': access = O_CREAT|O_WRONLY|O_TRUNC; break;
    case 'a': access = O_CREAT|O_WRONLY|O_APPEND; break;
    default :
        errno = EINVAL;
        return NULL;
    }
    while (*m) {
        if (*m == '+') {
            access &= ~(O_RDONLY | O_WRONLY);
            access |= O_RDWR;
        } else if (*m == 'b') {
#ifdef O_BINARY
            access |= O_BINARY;
#endif
        } else if (*m) {
            errno = EINVAL;
            return NULL;
        }
        m++;
    }
    fd = avpriv_open(path, access, 0666);
    if (fd == -1)
        return NULL;
    return fdopen(fd, mode);
}*/
