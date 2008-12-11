/*
 * libiri: An IRI/URI/URL parsing library
 * @(#) $Id$
 */

/*
 * Copyright (c) 2005, 2008 Mo McRoberts.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. The names of the author(s) of this software may not be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * AUTHORS OF THIS SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>

#include "p_libiri.h"

#undef ALIGNMENT
#define ALIGNMENT 8
#undef ALIGN
#define _ALIGN(x) ((((x)+(ALIGNMENT-1))&~(ALIGNMENT-1)))
#define ALIGN(x) (char *) _ALIGN((size_t) x)

static inline int
iri__hexnibble(char c)
{
	if(c >= '0' && c <= '9')
	{
		return c - '0';
	}
	if(c >= 'A' && c <= 'F')
	{
		return c - 'A' + 10;
	}
	if(c >= 'a' && c <= 'f')
	{
		return c - 'a' + 10;
	}
}

static inline const char *
iri__copychar(char **dest, const char *src)
{
	**dest = *src;
	(*dest)++;
	src++;
	return src;
}

/* TODO: Punycode decoding for the host part */
static inline const char *
iri__copychar_decode(char **dest, const char *src, int convert_space)
{
	unsigned char *p = (unsigned char *) (*dest);
	
	if(1 == convert_space && '+' == *src)
	{
		**dest = ' ';
	}
	else if('%' == *src)
	{
		if(0 == isxdigit(src[1]) || 0 == isxdigit(src[2]))
		{
			/* TODO: Deal with %u<nnnn> non-standard encoding - be liberal in
			 * what you accept, etc.
			 */
			**dest = '%';
		}
		else
		{
			*p = (iri__hexnibble(src[1]) << 4) | iri__hexnibble(src[2]);
			src += 2;
		}
	}
	else
	{
		**dest = *src;
	}
	src++;
	(*dest)++;
	return src;
}

static inline char *
iri__allocbuf(const char *src, size_t *len)
{
	size_t sc;
	const char *p, *c;
	
	/* Calculate the size of the buffer required to hold a decoded version of
	 * src, including enough breathing space for null bytes.
	 */
	/* XXX: This is way too much; we need to actually count it */
	*len = (strlen(src) * 4) + 16;
	/* Determine how much space we need for the scheme list */
	if(NULL != (c = strchr(src, ':')))
	{
		sc = 1;
		for(p = src; p < c; p++)
		{
			if(*p == '+')
			{
				sc++;
			}
		}
		/* Ensure we can align each element on an 8-byte boundary */
		*len = (src - c) + 1 + sc + ((sc + 1) * (sizeof(char *) + 7));
		*len += (7 * 11);
	}
	return (char *) calloc(1, *len);
}

iri_t *
iri_parse(const char *src)
{
	iri_t *p;
	char *bufstart, *endp, *bufp, **sl;
	const char *at, *colon, *slash, *t;
	size_t buflen, sc, cp;
	
	if(NULL == (p = (iri_t *) calloc(1, sizeof(iri_t))))
	{
		return NULL;
	}
	if(NULL == (bufstart = iri__allocbuf(src, &buflen)))
	{
		free(p);
		return NULL;
	}
	p->base = bufp = bufstart;
	p->nbytes = buflen;
	at = strchr(src, '@');
	slash = strchr(src, '/');
	colon = strchr(src, ':');
	if(slash && colon && slash < colon)
	{
		/* We can disregard the colon if a slash appears before it */
		colon = NULL;
	}
	if(colon && !at)
	{
		/* Definitely a scheme */
		bufp = ALIGN(bufp);
		p->iri.scheme = bufp;
		while(*src && *src != ':')
		{
			src = iri__copychar_decode(&bufp, src, 0);
		}
		*bufp = 0;
		bufp++;
		src++;
		/* src[0-1] SHOULD == '/' */
		if(src[0] == '/') src++;
		if(src[0] == '/') src++;
	}
	else if(colon && at && colon < at)
	{
		fprintf(stderr, "Colon occurs before at\n");
		/* This could be scheme://user[;auth][:password]@host or [scheme:]user[;auth][:password]@host (urgh) */
		if(colon[1] == '/' && colon[2] == '/' && colon[3] != '/')
		{
			bufp = ALIGN(bufp);
			p->iri.scheme = bufp;
			while(*src && *src != ':')
			{
				src = iri__copychar_decode(&bufp, src, 0);
			}
			*bufp = 0;
			bufp++;
			src++;
			/* src[0-1] SHOULD == '/' */
			for(; *src == '/'; src++);
			bufp = ALIGN(bufp);
			p->iri.user = bufp;
			fprintf(stderr, "Found user\n");
		}
		else
		{
			fprintf(stderr, "Matched scheme\n");
			bufp = ALIGN(bufp);
			p->iri.scheme = bufp;
		}
		while(*src && *src != ':' && *src != '@' && *src != ';')
		{
			src = iri__copychar_decode(&bufp, src, 0);
		}
		*bufp = 0;
		bufp++;
		if(*src == ';')
		{
			/* Following authentication parameters */
			src++;
			bufp = ALIGN(bufp);
			p->iri.auth = bufp;
			while(*src && *src != ':' && *src != '@')
			{
				/* Don't decode, so it can be extracted properly */
				src = iri__copychar(&bufp, src);
			}
			*bufp = 0;
			bufp++;
		}
		if(*src == ':')
		{
			/* Following password data */
			src++;
			bufp = ALIGN(bufp);
			p->iri.password = bufp;
			while(*src && *src != ':' && *src != '@')
			{
				src = iri__copychar_decode(&bufp, src, 0);
			}
			*bufp = 0;
			bufp++;
			if(*src == ':')
			{
				src++;
				/* It was actually scheme:user:auth@host */
				p->iri.user = p->iri.auth;
				bufp = ALIGN(bufp);
				p->iri.password = bufp;
				while(*src && *src != '@')
				{
					src = iri__copychar_decode(&bufp, src, 0);
				}
				*bufp = 0;
				bufp++;
			}
		}
		if(!*src)
		{
			/* No host part */
			return p;
		}
		if(*src == '@')
		{
			src++;
		}
	}
	else if(at)
	{
		/* user[;auth]@host[/path...] */
		bufp = ALIGN(bufp);
		p->iri.user = bufp;
		while(*src != '@' && *src != ';')
		{
			src = iri__copychar_decode(&bufp, src, 0);
		}
		*bufp = 0;
		bufp++;
		if(*src == ';')
		{
			src++;
			bufp = ALIGN(bufp);
			p->iri.auth = bufp;
			while(*src && *src != '@')
			{
				/* Don't decode, so it can be extracted properly */
				src = iri__copychar(&bufp, src);
			}
			*bufp = 0;
			bufp++;
		}
		else
		{
			src++;
		}
	}
	if(NULL != p->iri.scheme)
	{
		sc = 1;
		for(t = p->iri.scheme; *t; t++)
		{
			if('+' == *t)
			{
				sc++;
			}
		}
		bufp = ALIGN(bufp);
		sl = (char **) (void *) bufp;
		bufp += (sc + 1) * sizeof(char *);
		sc = 0;
		cp = 0;
		bufp = ALIGN(bufp);
		sl[0] = bufp;
		for(t = p->iri.scheme; *t; t++)
		{
			if('+' == *t)
			{
				if(sl[sc][0])
				{
					sl[sc][cp] = 0;
					bufp++;
					sc++;
					bufp = ALIGN(bufp);
					sl[sc] = bufp;
					cp = 0;
				}
			}
			else
			{
				sl[sc][cp] = *t;
				bufp++;
				cp++;
			}
		}
		if(sl[sc][0])
		{
			sl[sc][cp] = 0;
			sc++;
			bufp++;
		}
		sl[sc] = NULL;
		p->iri.schemelist = (const char **) sl;
		p->iri.nschemes = sc;
		bufp++;
	}
	bufp = ALIGN(bufp);
	p->iri.host = bufp;
	while(*src && *src != ':' && *src != '/' && *src != '?' && *src != '#')
	{
		src = iri__copychar_decode(&bufp, src, 0);
	}
	*bufp = 0;
	bufp++;
	if(*src == ':')
	{
		/* Port part */
		src++;
		endp = (char *) src;
		p->iri.port = strtol(src, &endp, 10);
		src = endp;
	}
	if(*src == '/')
	{
		bufp = ALIGN(bufp);
		p->iri.path = bufp;
		while(*src && *src != '?' && *src != '#')
		{
			src = iri__copychar_decode(&bufp, src, 0);
		}
		*bufp = 0;
		bufp++;
	}
	if(*src == '?')
	{
		bufp = ALIGN(bufp);
		p->iri.query = bufp;
		src++;
		while(*src && *src != '#')
		{
			/* Don't actually decode the query itself, otherwise it
			 * can't be reliably split */
			src = iri__copychar(&bufp, src);
		}
		*bufp = 0;
		bufp++;
	}
	if(*src == '#')
	{
		bufp = ALIGN(bufp);
		p->iri.anchor = bufp; 
		while(*src)
		{
			src = iri__copychar_decode(&bufp, src, 0);
		}
		*bufp = 0;
		bufp++;
	}
	if(*src)
	{
		/* Still stuff left? It must be a path... of sorts */
		bufp = ALIGN(bufp);
		p->iri.path = bufp; 
		while(*src && *src != '?' && *src != '#')
		{
			src = iri__copychar_decode(&bufp, src, 0);
		}
		*bufp = 0;
		bufp++;
	}
	return p;
}
