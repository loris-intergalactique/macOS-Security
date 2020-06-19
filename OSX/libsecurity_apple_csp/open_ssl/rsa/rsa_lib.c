/*
 * Copyright (c) 2000-2001,2011,2014 Apple Inc. All Rights Reserved.
 * 
 * The contents of this file constitute Original Code as defined in and are
 * subject to the Apple Public Source License Version 1.2 (the 'License').
 * You may not use this file except in compliance with the License. Please obtain
 * a copy of the License at http://www.apple.com/publicsource and read it before
 * using this file.
 * 
 * This Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS
 * OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES, INCLUDING WITHOUT
 * LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. Please see the License for the
 * specific language governing rights and limitations under the License.
 */


/* crypto/rsa/rsa_lib.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#include <stdio.h>
#include <openssl/crypto_legacy.h>
#include "cryptlib.h"
#include <openssl/lhash_legacy.h>
#include <openssl/bn_legacy.h>
#include <openssl/rsa_legacy.h>
#include <pthread.h>

const char *RSA_version="RSA" OPENSSL_VERSION_PTEXT;

static const RSA_METHOD *default_RSA_meth=NULL;
static int rsa_meth_num=0;
static STACK_OF(CRYPTO_EX_DATA_FUNCS) *rsa_meth=NULL;

RSA *RSA_new(void)
	{
	return(RSA_new_method(NULL));
	}

void RSA_set_default_method(const RSA_METHOD *meth)
	{
	default_RSA_meth=meth;
	}

const RSA_METHOD *RSA_get_default_method(void)
{
	return default_RSA_meth;
}

const RSA_METHOD *RSA_get_method(RSA *rsa)
{
	return rsa->meth;
}

const RSA_METHOD *RSA_set_method(RSA *rsa, const RSA_METHOD *meth)
{
	const RSA_METHOD *mtmp;
	mtmp = rsa->meth;
	if (mtmp->finish) mtmp->finish(rsa);
	rsa->meth = meth;
	if (meth->init) meth->init(rsa);
	return mtmp;
}

RSA *RSA_new_method(const RSA_METHOD *meth)
	{
	RSA *ret;

	if (default_RSA_meth == NULL)
		{
#ifdef RSA_NULL
		default_RSA_meth=RSA_null_method();
#else
#ifdef RSAref
		default_RSA_meth=RSA_PKCS1_RSAref();
#else
		default_RSA_meth=RSA_PKCS1_SSLeay();
#endif
#endif
		}
	ret=(RSA *)Malloc(sizeof(RSA));
	if (ret == NULL)
		{
		RSAerr(RSA_F_RSA_NEW_METHOD,ERR_R_MALLOC_FAILURE);
		return(NULL);
		}

	if (meth == NULL)
		ret->meth=default_RSA_meth;
	else
		ret->meth=meth;

	ret->pad=0;
	ret->version=0;
	ret->n=NULL;
	ret->e=NULL;
	ret->d=NULL;
	ret->p=NULL;
	ret->q=NULL;
	ret->dmp1=NULL;
	ret->dmq1=NULL;
	ret->iqmp=NULL;
	ret->references=1;
	ret->_method_mod_n=NULL;
	ret->_method_mod_p=NULL;
	ret->_method_mod_q=NULL;
	
	// make blinding per thread
	ret->num_blinding_threads = 0;
	pthread_mutex_init(&ret->blinding_mutex, NULL);
	ret->blinding_array = NULL;
	
	ret->bignum_data=NULL;
	ret->flags=ret->meth->flags;
	if ((ret->meth->init != NULL) && !ret->meth->init(ret))
		{
		Free(ret);
		ret=NULL;
		}
	else
		CRYPTO_new_ex_data(rsa_meth,ret,&ret->ex_data);
	return(ret);
	}

void RSA_free(RSA *r)
	{
	int i;

	if (r == NULL) return;

	i=CRYPTO_add(&r->references,-1,CRYPTO_LOCK_RSA);
#ifdef REF_PRINT
	REF_PRINT("RSA",r);
#endif
	if (i > 0) return;
#ifdef REF_CHECK
	if (i < 0)
		{
		fprintf(stderr,"RSA_free, bad reference count\n");
		abort();
		}
#endif

	CRYPTO_free_ex_data(rsa_meth,r,&r->ex_data);

	if (r->meth->finish != NULL)
		r->meth->finish(r);

	if (r->n != NULL) BN_clear_free(r->n);
	if (r->e != NULL) BN_clear_free(r->e);
	if (r->d != NULL) BN_clear_free(r->d);
	if (r->p != NULL) BN_clear_free(r->p);
	if (r->q != NULL) BN_clear_free(r->q);
	if (r->dmp1 != NULL) BN_clear_free(r->dmp1);
	if (r->dmq1 != NULL) BN_clear_free(r->dmq1);
	if (r->iqmp != NULL) BN_clear_free(r->iqmp);
	RSA_free_thread_blinding_ptr(r);
	if (r->bignum_data != NULL) Free_locked(r->bignum_data);
	Free(r);
	}

int RSA_get_ex_new_index(long argl, void *argp, CRYPTO_EX_new *new_func,
	     CRYPTO_EX_dup *dup_func, CRYPTO_EX_free *free_func)
        {
	rsa_meth_num++;
	return(CRYPTO_get_ex_new_index(rsa_meth_num-1,
		&rsa_meth,argl,argp,new_func,dup_func,free_func));
        }

int RSA_set_ex_data(RSA *r, int idx, void *arg)
	{
	return(CRYPTO_set_ex_data(&r->ex_data,idx,arg));
	}

void *RSA_get_ex_data(RSA *r, int idx)
	{
	return(CRYPTO_get_ex_data(&r->ex_data,idx));
	}

int RSA_size(RSA *r)
	{
	return(BN_num_bytes(r->n));
	}

int RSA_public_encrypt(int flen, unsigned char *from, unsigned char *to,
	     RSA *rsa, int padding)
	{
	return(rsa->meth->rsa_pub_enc(flen, from, to, rsa, padding));
	}

int RSA_private_encrypt(int flen, unsigned char *from, unsigned char *to,
	     RSA *rsa, int padding)
	{
	return(rsa->meth->rsa_priv_enc(flen, from, to, rsa, padding));
	}

int RSA_private_decrypt(int flen, unsigned char *from, unsigned char *to,
	     RSA *rsa, int padding)
	{
	return(rsa->meth->rsa_priv_dec(flen, from, to, rsa, padding));
	}

int RSA_public_decrypt(int flen, unsigned char *from, unsigned char *to,
	     RSA *rsa, int padding)
	{
	return(rsa->meth->rsa_pub_dec(flen, from, to, rsa, padding));
	}

int RSA_flags(RSA *r)
	{
	return((r == NULL)?0:r->meth->flags);
	}

void RSA_blinding_off(RSA *rsa)
	{
	RSA_free_thread_blinding_ptr(rsa);
	rsa->flags&= ~RSA_FLAG_BLINDING;
	}

int RSA_blinding_on(RSA *rsa, BN_CTX *p_ctx)
	{
	BIGNUM *A,*Ai;
	BN_CTX *ctx;
	int ret=0;

	if (p_ctx == NULL)
		{
		if ((ctx=BN_CTX_new()) == NULL) goto err;
		}
	else
		ctx=p_ctx;

	RSA_free_thread_blinding_ptr(rsa);

	BN_CTX_start(ctx);
	A = BN_CTX_get(ctx);
	if (!BN_rand(A,BN_num_bits(rsa->n)-1,1,0)) goto err;
	if ((Ai=BN_mod_inverse(NULL,A,rsa->n,ctx)) == NULL) goto err;

	if (!rsa->meth->bn_mod_exp(A,A,rsa->e,rsa->n,ctx,rsa->_method_mod_n))
	    goto err;
	RSA_set_thread_blinding_ptr(rsa, BN_BLINDING_new(A,Ai,rsa->n));
	rsa->flags|=RSA_FLAG_BLINDING;
	BN_free(Ai);
	ret=1;
err:
	BN_CTX_end(ctx);
	if (ctx != p_ctx) BN_CTX_free(ctx);
	return(ret);
	}

int RSA_memory_lock(RSA *r)
	{
	int i,j,k,off;
	char *p;
	BIGNUM *bn,**t[6],*b;
	BN_ULONG *ul;

	if (r->d == NULL) return(1);
	t[0]= &r->d;
	t[1]= &r->p;
	t[2]= &r->q;
	t[3]= &r->dmp1;
	t[4]= &r->dmq1;
	t[5]= &r->iqmp;
	k=sizeof(BIGNUM)*6;
	off=k/sizeof(BN_ULONG)+1;
	j=1;
	for (i=0; i<6; i++)
		j+= (*t[i])->top;
	if ((p=Malloc_locked((off+j)*sizeof(BN_ULONG))) == NULL)
		{
		RSAerr(RSA_F_MEMORY_LOCK,ERR_R_MALLOC_FAILURE);
		return(0);
		}
	bn=(BIGNUM *)p;
	ul=(BN_ULONG *)&(p[off]);
	for (i=0; i<6; i++)
		{
		b= *(t[i]);
		*(t[i])= &(bn[i]);
		memcpy((char *)&(bn[i]),(char *)b,sizeof(BIGNUM));
		bn[i].flags=BN_FLG_STATIC_DATA;
		bn[i].d=ul;
		memcpy((char *)ul,b->d,sizeof(BN_ULONG)*b->top);
		ul+=b->top;
		BN_clear_free(b);
		}
	
	/* I should fix this so it can still be done */
	r->flags&= ~(RSA_FLAG_CACHE_PRIVATE|RSA_FLAG_CACHE_PUBLIC);

	r->bignum_data=p;
	return(1);
	}

struct BN_BLINDING_STRUCT
{
	pthread_t thread_ID;
	BN_BLINDING* blinding;
};



static struct BN_BLINDING_STRUCT* RSA_get_blinding_struct(RSA *r)
{
	// look for storage for the current thread
	pthread_t current = pthread_self();
	
	int i;
	for (i = 0; i < r->num_blinding_threads; ++i)
	{
		if (pthread_equal(current, r->blinding_array[i].thread_ID)) // do we have storage for this thread?
		{
			return &(r->blinding_array[i]);
		}
	}
	
	return NULL;
}

typedef struct BN_BLINDING_STRUCT BN_BLINDING_STRUCT;


BN_BLINDING* RSA_get_thread_blinding_ptr(RSA *r)
{
	// lock down the structure
	pthread_mutex_lock(&r->blinding_mutex);
	
	BN_BLINDING* result = NULL;
	BN_BLINDING_STRUCT *st = RSA_get_blinding_struct(r);
	if (st != NULL)
	{
		result = st->blinding;
	}
	
	pthread_mutex_unlock(&r->blinding_mutex);
	
	return result;
}



void RSA_set_thread_blinding_ptr(RSA *r, BN_BLINDING* bnb)
{
	// lock down the structure
	pthread_mutex_lock(&r->blinding_mutex);
	
	// see if there is an existing record for this thread
	BN_BLINDING_STRUCT *st = RSA_get_blinding_struct(r);
	if (st == NULL)
	{
		// add a new blinding struct
		int last_member = r->num_blinding_threads;
		r->num_blinding_threads += 1;
		r->blinding_array = (BN_BLINDING_STRUCT*) realloc(r->blinding_array, sizeof(BN_BLINDING_STRUCT) * r->num_blinding_threads);
		st = &r->blinding_array[last_member];
	}
	
	st->thread_ID = pthread_self();
	st->blinding = bnb;
	
	pthread_mutex_unlock(&r->blinding_mutex);
}



void RSA_free_thread_blinding_ptr(RSA *r)
{
	// look for storage for the current thread
	pthread_t current = pthread_self();
	
	int i;
	for (i = 0; i < r->num_blinding_threads; ++i)
	{
		if (pthread_equal(current, r->blinding_array[i].thread_ID)) // do we have storage for this thread?
		{
			BN_BLINDING_free(r->blinding_array[i].blinding);
			
			int new_count = r->num_blinding_threads - 1;
			if (new_count == 0)
			{
				// no more thread storage, just blow our array away
				free(r->blinding_array);
				r->blinding_array = NULL;
				r->num_blinding_threads = 0;
			}
			else
			{
				r->blinding_array[i] = r->blinding_array[new_count];
				r->num_blinding_threads = new_count;
			}
			break;
		}
	}
}