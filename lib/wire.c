/*  $Id$
**
**  Wire format article utilities.
**
**  Originally written by Alex Kiernan (alex.kiernan@thus.net)
**
**  These routines manipulate wire format articles; in particular, they should
**  be safe in the presence of embedded NULs.  They assume wire format
**  conventions (\r\n as a line ending, in particular) and will not work with
**  articles in native format.
**
**  The functions in this file take const char * pointers and return char *
**  pointers so that they can work on both const char * and char * article
**  bodies without changing the const sense.  This unfortunately means that
**  the routines in this file will produce warnings about const being cast
**  away.  To avoid those, one would need to duplicate all the code in this
**  file or use C++.
*/

#include "config.h"
#include "clibrary.h"
#include <assert.h>

#include "inn/wire.h"
#include "libinn.h"

/*
**  Given a pointer to the start of an article, locate the first octet of the
**  body (which may be the octet beyond the end of the buffer if your article
**  is bodiless).
*/
char *
wire_findbody(const char *article, size_t length)
{
    char *p;
    const char *end;

    end = article + length;
    for (p = (char *) article; (p + 4) <= end; ++p) {
        p = memchr(p, '\r', end - p - 3);
        if (p == NULL)
            break;
        if (memcmp(p, "\r\n\r\n", 4) == 0) {
            p += 4;
            return p;
        }
    }
    return NULL;
}


/*
**  Given a pointer into an article and a pointer to the last octet of the
**  article, find the next line ending and return a pointer to the first
**  character after that line ending.  If no line ending is found in the
**  article or if it is at the end of the article, return NULL.
*/
char *
wire_nextline(const char *article, const char *end)
{
    char *p;

    for (p = (char *) article; (p + 2) <= end; ++p) {
        p = memchr(p, '\r', end - p - 2);
        if (p == NULL)
            break;
        if (p[1] == '\n') {
            p += 2;
            return p;
        }
    }
    return NULL;
}


/*
**  Returns true if line is the beginning of a valid header for header, also
**  taking the length of the header name as a third argument.  Assumes that
**  there is at least length + 2 bytes of data at line, and that the header
**  name doesn't contain nul.
*/
static bool
isheader(const char *line, const char *header, size_t length)
{
    if (line[length] != ':' || !ISWHITE(line[length + 1]))
        return false;
    return strncasecmp(line, header, length) == 0;
}


/*
**  Skip over folding whitespace, as defined by RFC 2822.  Takes a pointer to
**  where to start skipping and a pointer to the end of the data, and will not
**  return a pointer past the end pointer.  If skipping folding whitespace
**  takes us past the end of data, return NULL.
*/
static char *
skip_fws(char *text, const char *end)
{
    char *p;

    for (p = text; p <= end; p++) {
        if (p < end + 1 && p[0] == '\r' && p[1] == '\n' && ISWHITE(p[2]))
            p += 2;
        if (!ISWHITE(*p))
            return p;
    }
    return NULL;
}


/*
**  Given a pointer to the start of the article, the article length, and the
**  header to look for, find the first occurance of that header in the
**  article.  Skip over headers with no content, but allow for headers that
**  are folded before the first text in the header.  If no matching headers
**  with content other than spaces and tabs are found, return NULL.
*/
char *
wire_findheader(const char *article, size_t length, const char *header)
{
    char *p;
    const char *end;
    ptrdiff_t headerlen;

    headerlen = strlen(header);
    end = article + length - 1;

    /* There has to be enough space left in the article for at least the
       header, the colon, whitespace, and one non-whitespace character, hence
       3, minus 1 since the character pointed to by end is part of the
       article. */
    p = (char *) article;
    while (p != NULL && end - p > headerlen + 2) {
        if (p[0] == '\r' && p[1] == '\n')
            return NULL;
        else if (isheader(p, header, headerlen)) {
            p = skip_fws(p + headerlen + 2, end);
            if (p == NULL)
                return NULL;
            if (p >= end || p[0] != '\r' || p[1] != '\n')
                return p;
        }
        p = wire_nextline(p, end);
    }
    return NULL;
}


/*
**  Given a pointer to a header and a pointer to the last octet of the
**  article, find the end of the header (a pointer to the final \n of the
**  header value).  If the header contents don't end in \r\n, return NULL.
*/
char *
wire_endheader(const char *header, const char *end)
{
    char *p;

    p = wire_nextline(header, end);
    while (p != NULL) {
        if (!ISWHITE(*p))
            return p - 1;
        p = wire_nextline(p, end);
    }
    if (end - header >= 1 && *end == '\n' && *(end - 1) == '\r')
        return (char *) end;
    return NULL;
}