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

#include "p_libiri.h"

static inline void
iri__dupcopy(const char **dest, const char *src, const char *srcbase, size_t buflen, const char *destbase)
{
	if(NULL == src)
	{
		*dest = NULL;
	}
	else if(src >= srcbase && src <= srcbase + buflen)
	{
		*dest = destbase + (src - srcbase);
	}
	else
	{
		*dest = strdup(src);
	}
}

iri_t *
iri_dup(iri_t *iri)
{
	iri_t *p;
	
	if(NULL == (p = (iri_t *) calloc(1, sizeof(iri_t))))
	{
		return NULL;
	}
	if(NULL == (p->base = (char *) calloc(1, iri->nbytes)))
	{
		free(p);
		return NULL;
	}
	p->nbytes = iri->nbytes;
	p->iri.port = iri->iri.port;
	memcpy(p->base, iri->base, p->nbytes);
	iri__dupcopy(&(p->iri.display), iri->iri.display, iri->base, iri->nbytes, p->base);
	iri__dupcopy(&(p->iri.scheme), iri->iri.scheme, iri->base, iri->nbytes, p->base);
	iri__dupcopy(&(p->iri.user), iri->iri.user, iri->base, iri->nbytes, p->base);
	iri__dupcopy(&(p->iri.auth), iri->iri.auth, iri->base, iri->nbytes, p->base);
	iri__dupcopy(&(p->iri.password), iri->iri.password, iri->base, iri->nbytes, p->base);
	iri__dupcopy(&(p->iri.host), iri->iri.host, iri->base, iri->nbytes, p->base);
	iri__dupcopy(&(p->iri.path), iri->iri.path, iri->base, iri->nbytes, p->base);
	iri__dupcopy(&(p->iri.query), iri->iri.query, iri->base, iri->nbytes, p->base);
	iri__dupcopy(&(p->iri.anchor), iri->iri.anchor, iri->base, iri->nbytes, p->base);
	return p;
}
