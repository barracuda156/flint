/*
    Copyright (C) 2017 William Hart

    This file is part of FLINT.

    FLINT is free software: you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.  See <http://www.gnu.org/licenses/>.
*/

#include <gmp.h>
#include <stdlib.h>
#include "flint.h"
#include "fmpz.h"
#include "fmpz_mpoly.h"
#include "longlong.h"

slong _fmpz_mpoly_divrem_monagan_pearce1(slong * lenr,
   fmpz ** polyq, ulong ** expq, slong * allocq, fmpz ** polyr,
  ulong ** expr, slong * allocr, const fmpz * poly2, const ulong * exp2,
            slong len2, const fmpz * poly3, const ulong * exp3, slong len3,
                                                        ulong maxn, slong bits)
{
   slong i, k, l, s;
   slong next_free, Q_len = 0, reuse_len = 0, heap_len = 2; /* heap zero index unused */
   mpoly_heap1_s * heap;
   mpoly_heap_t * chain;
   mpoly_heap_t ** Q, ** reuse;
   mpoly_heap_t * x, * x2;
   fmpz * p1 = *polyq;
   fmpz * p2 = *polyr;
   ulong * e1 = *expq;
   ulong * e2 = *expr;
   ulong exp;
   ulong c[3], p[2]; /* for accumulating coefficients */
   ulong mask = 0, ub;
   fmpz_t mb, qc, r;
   int small;
   slong bits2, bits3;
   int d1;
   TMP_INIT;

   TMP_START;

   fmpz_init(mb);
   fmpz_init(qc);
   fmpz_init(r);

   bits2 = _fmpz_vec_max_bits(poly2, len2);
   bits3 = _fmpz_vec_max_bits(poly3, len3);
   /* allow one bit for sign, one bit for subtraction */
   small = FLINT_ABS(bits2) <= (FLINT_ABS(bits3) + FLINT_BIT_COUNT(len3) + FLINT_BITS - 2) &&
           FLINT_ABS(bits3) <= FLINT_BITS - 2;

   heap = (mpoly_heap1_s *) TMP_ALLOC((len3 + 1)*sizeof(mpoly_heap1_s));
   chain = (mpoly_heap_t *) TMP_ALLOC(len3*sizeof(mpoly_heap_t));
   Q = (mpoly_heap_t **) TMP_ALLOC(len3*sizeof(mpoly_heap_t *));
   reuse = (mpoly_heap_t **) TMP_ALLOC(len3*sizeof(mpoly_heap_t *));

   next_free = 0;

   for (i = 0; i < FLINT_BITS/bits; i++)
      mask = (mask << bits) + (UWORD(1) << (bits - 1));

   k = -WORD(1);
   l = -WORD(1);
   s = len3;
   
   x = chain + next_free++;
   x->i = -WORD(1);
   x->j = 0;
   x->next = NULL;

   HEAP_ASSIGN(heap[1], maxn - exp2[len2 - 1], x);

   fmpz_neg(mb, poly3 + len3 - 1);

   ub = ((ulong) FLINT_ABS(*mb)) >> 1; /* abs(poly3[0])/2 */
   
   while (heap_len > 1)
   {
      exp = heap[1].exp;
      k++;

      if (k >= *allocq)
      {
         p1 = (fmpz *) flint_realloc(p1, 2*sizeof(fmpz)*(*allocq));
         e1 = (ulong *) flint_realloc(e1, 2*sizeof(ulong)*(*allocq));
         flint_mpn_zero(p1 + *allocq, *allocq);
         (*allocq) *= 2;
      }

      c[0] = c[1] = c[2] = 0;

      while (heap_len > 1 && heap[1].exp == exp)
      {
         x = _mpoly_heap_pop1(heap, &heap_len);
         
         if (small)
         {
            fmpz fc = poly2[len2 - x->j - 1];

            if (x->i == -WORD(1))
            {
               if (!COEFF_IS_MPZ(fc))
               {
                  if (fc >= 0)
                     sub_dddmmmsss(c[2], c[1], c[0], c[2], c[1], c[0], 0, 0, fc);
                  else
                     add_sssaaaaaa(c[2], c[1], c[0], c[2], c[1], c[0], 0, 0, -fc);
               } else
               {
                  slong size = fmpz_size(poly2 + len2 - x->j - 1);
                  __mpz_struct * m = COEFF_TO_PTR(fc);
                  if (fmpz_sgn(poly2 + len2 - x->j - 1) < 0)
                     mpn_add(c, c, 3, m->_mp_d, size);
                  else
                     mpn_sub(c, c, 3, m->_mp_d, size);
               }
            } else
            {
               smul_ppmm(p[1], p[0], poly3[len3 - x->i - 1], p1[x->j]);
               if (0 > (slong) p[1])
                  add_sssaaaaaa(c[2], c[1], c[0], c[2], c[1], c[0], ~WORD(0), p[1], p[0]);
               else
                  add_sssaaaaaa(c[2], c[1], c[0], c[2], c[1], c[0], 0, p[1], p[0]);
            }
         } else
         {
            if (x->i == -WORD(1))
               fmpz_sub(qc, qc, poly2 + len2 - x->j - 1);
            else
               fmpz_addmul(qc, poly3 + len3 - x->i - 1, p1 + x->j);
         }

         if (x->i != -WORD(1) || x->j < len2 - 1)
            Q[Q_len++] = x;
         else
            reuse[reuse_len++] = x;

         while ((x = x->next) != NULL)
         {
            if (small)
            {
               fmpz fc = poly2[len2 - x->j - 1];

               if (x->i == -WORD(1))
               {
                  if (!COEFF_IS_MPZ(fc))
                  {
                     if (fc >= 0)
                        sub_dddmmmsss(c[2], c[1], c[0], c[2], c[1], c[0], 0, 0, fc);
                     else
                        add_sssaaaaaa(c[2], c[1], c[0], c[2], c[1], c[0], 0, 0, -fc);
                  } else
                  {
                     slong size = fmpz_size(poly2 + len2 - x->j - 1);
                     __mpz_struct * m = COEFF_TO_PTR(fc);
                     if (fmpz_sgn(poly2 + len2 - x->j - 1) < 0)
                        mpn_add(c, c, 3, m->_mp_d, size);
                     else
                        mpn_sub(c, c, 3, m->_mp_d, size);
                  }
               } else
               {
                  smul_ppmm(p[1], p[0], poly3[len3 - x->i - 1], p1[x->j]);
                  if (0 > (slong) p[1])
                     add_sssaaaaaa(c[2], c[1], c[0], c[2], c[1], c[0], ~WORD(0), p[1], p[0]);
                  else
                     add_sssaaaaaa(c[2], c[1], c[0], c[2], c[1], c[0], 0, p[1], p[0]);
               }
            } else
            {
               if (x->i == -WORD(1))
                  fmpz_sub(qc, qc, poly2 + len2 - x->j - 1);
               else
                  fmpz_addmul(qc, poly3 + len3 - x->i - 1, p1 + x->j);
            }

            if (x->i != -WORD(1) || x->j < len2 - 1)
               Q[Q_len++] = x;
            else
               reuse[reuse_len++] = x;
         }
      }

      while (Q_len > 0)
      {
         x = Q[--Q_len];
     
         if (x->i == -WORD(1))
         {
            x->j++;
            x->next = NULL;

            _mpoly_heap_insert1(heap, maxn - exp2[len2 - x->j - 1], x, &heap_len);
         } else if (x->j < k - 1)
         {
            x->j++;
            x->next = NULL;

            _mpoly_heap_insert1(heap, maxn - exp3[len3 - x->i - 1] - e1[x->j], x, &heap_len);
         } else if (x->j == k - 1)
         {
            s++;
            reuse[reuse_len++] = x;
         }
      }

      if ((small && (c[2] == 0 && c[1] == 0 && c[0] == 0)) || (!small && fmpz_is_zero(qc)))
         k--;
      else
      {
         d1 = mpoly_monomial_divides1(e1 + k, maxn - exp3[len3 - 1], exp, mask);

         if (!d1)
         {
            l++;

            if (l >= *allocr)
            {
               p2 = (fmpz *) flint_realloc(p2, 2*sizeof(fmpz)*(*allocr));
               e2 = (ulong *) flint_realloc(e2, 2*sizeof(ulong)*(*allocr));
               flint_mpn_zero(p2 + *allocr, *allocr);
               (*allocr) *= 2;
            }

            if (small)
            {
               fmpz_set_signed_uiuiui(p2 + l, c[2], c[1], c[0]);
               fmpz_neg(p2 + l, p2 + l);
            } else
               fmpz_neg(p2 + l, qc); 
 
            e2[l] = maxn - exp;

            k--;
         } else
         {
            if (small)
            {
               ulong d[3];

               if (0 > (slong) c[2])
                  mpn_neg(d, c, 3);
               else
                  flint_mpn_copyi(d, c, 3);

               if (d[2] != 0 || ub < d[1] || (ub == 0 && 0 > (slong) d[0])) /* quotient not a small */
               {
                  fmpz_set_signed_uiuiui(qc, c[2], c[1], c[0]);

                  small = 0;
               } else /* quotient fits a small */
               {
                  ulong r1;

                  sdiv_qrnnd(p1[k], r1, c[1], c[0], *mb);

                  if (r1 != 0)
                  {
                     l++;

                     if (l >= *allocr)
                     {
                        p2 = (fmpz *) flint_realloc(p2, 2*sizeof(fmpz)*(*allocr));
                        e2 = (ulong *) flint_realloc(e2, 2*sizeof(ulong)*(*allocr));
                        flint_mpn_zero(p2 + *allocr, *allocr);
                        (*allocr) *= 2;
                     }

                     p2[l] = -r1;

                     e2[l] = maxn - exp;
                  }
               }
            } 

            /* quotient non-small case */
            if (!small)
            {
               fmpz_fdiv_qr(p1 + k, r, qc, mb);

               if (!fmpz_is_zero(r))
               {
                  l++;

                  if (l >= *allocr)
                  {
                     p2 = (fmpz *) flint_realloc(p2, 2*sizeof(fmpz)*(*allocr));
                     e2 = (ulong *) flint_realloc(e2, 2*sizeof(ulong)*(*allocr));
                     flint_mpn_zero(p2 + *allocr, *allocr);
                     (*allocr) *= 2;
                  }

                  fmpz_neg(p2 + l, r);
                     
                  e2[l] = maxn - exp;
               }
            }

            if (!fmpz_is_zero(p1 + k))
            {
               for (i = 1; i < s; i++)
               {
                  if (reuse_len != 0)
                     x2 = reuse[--reuse_len];
                  else
                     x2 = chain + next_free++;
            
                  x2->i = i;
                  x2->j = k;
                  x2->next = NULL;

                  _mpoly_heap_insert1(heap, maxn - exp3[len3 - i - 1] - e1[k], x2, &heap_len);
               }
               s = 1;
            } else
               k--;
         }
      } 
      
      fmpz_zero(qc);  
   }

   k++;
   l++;

   fmpz_clear(mb);
   fmpz_clear(qc);
   fmpz_clear(r);

   (*polyq) = p1;
   (*expq) = e1;
   (*polyr) = p2;
   (*expr) = e2;
   
   (*lenr) = l;

   TMP_END;

   return k;
}

slong _fmpz_mpoly_divrem_monagan_pearce(slong * lenr,
  fmpz ** polyq, ulong ** expq, slong * allocq, fmpz ** polyr,
                  ulong ** expr, slong * allocr, const fmpz * poly2,
   const ulong * exp2, slong len2, const fmpz * poly3, const ulong * exp3, 
                                 slong len3, ulong * maxn, slong bits, slong N)
{
   slong i, k, l, s;
   slong next_free, Q_len = 0, reuse_len = 0, heap_len = 2; /* heap zero index unused */
   mpoly_heap_s * heap;
   mpoly_heap_t * chain;
   mpoly_heap_t ** Q, ** reuse;
   mpoly_heap_t * x, * x2;
   fmpz * p1 = *polyq;
   fmpz * p2 = *polyr;
   ulong * e1 = *expq;
   ulong * e2 = *expr;
   ulong * exp, * exps, * texp;
   ulong ** exp_list;
   ulong c[3], p[2]; /* for accumulating coefficients */
   slong exp_next;
   ulong mask = 0, ub;
   fmpz_t mb, qc, r;
   int small;
   slong bits2, bits3;
   int d1;
   TMP_INIT;

   if (N == 1)
      return _fmpz_mpoly_divrem_monagan_pearce1(lenr, polyq, expq, allocq,
       polyr, expr, allocr, poly2, exp2, len2, poly3, exp3, len3, *maxn, bits);

   TMP_START;

   fmpz_init(mb);
   fmpz_init(qc);
   fmpz_init(r);

   bits2 = _fmpz_vec_max_bits(poly2, len2);
   bits3 = _fmpz_vec_max_bits(poly3, len3);
   /* allow one bit for sign, one bit for subtraction */
   small = FLINT_ABS(bits2) <= (FLINT_ABS(bits3) + FLINT_BIT_COUNT(len3) + FLINT_BITS - 2) &&
           FLINT_ABS(bits3) <= FLINT_BITS - 2;

   heap = (mpoly_heap_s *) TMP_ALLOC((len3 + 1)*sizeof(mpoly_heap_s));
   chain = (mpoly_heap_t *) TMP_ALLOC(len3*sizeof(mpoly_heap_t));
   Q = (mpoly_heap_t **) TMP_ALLOC(len3*sizeof(mpoly_heap_t *));
   reuse = (mpoly_heap_t **) TMP_ALLOC(len3*sizeof(mpoly_heap_t *));
   exps = (ulong *) TMP_ALLOC(len3*N*sizeof(ulong));
   exp_list = (ulong **) TMP_ALLOC(len3*sizeof(ulong *));
   texp = (ulong *) TMP_ALLOC(N*sizeof(ulong));
   exp = (ulong *) TMP_ALLOC(N*sizeof(ulong));

   for (i = 0; i < len3; i++)
      exp_list[i] = exps + i*N;

   next_free = 0;
   exp_next = 0;

   for (i = 0; i < FLINT_BITS/bits; i++)
      mask = (mask << bits) + (UWORD(1) << (bits - 1));

   k = -WORD(1);
   l = -WORD(1);
   s = len3;
   
   x = chain + next_free++;
   x->i = -WORD(1);
   x->j = 0;
   x->next = NULL;

   heap[1].next = x;
   heap[1].exp = exp_list[exp_next++];

   mpoly_monomial_sub(heap[1].exp, maxn, exp2 + (len2 - 1)*N, N);

   fmpz_neg(mb, poly3 + len3 - 1);

   ub = ((ulong) FLINT_ABS(*mb)) >> 1; /* abs(poly3[0])/2 */
   
   while (heap_len > 1)
   {
      mpoly_monomial_set(exp, heap[1].exp, N);
      k++;

      if (k >= *allocq)
      {
         p1 = (fmpz *) flint_realloc(p1, 2*sizeof(fmpz)*(*allocq));
         e1 = (ulong *) flint_realloc(e1, 2*N*sizeof(ulong)*(*allocq));
         flint_mpn_zero(p1 + *allocq, *allocq);
         (*allocq) *= 2;
      }

      c[0] = c[1] = c[2] = 0;

      while (heap_len > 1 && mpoly_monomial_equal(heap[1].exp, exp, N))
      {
         exp_list[--exp_next] = heap[1].exp;

         x = _mpoly_heap_pop(heap, &heap_len, N);

         if (small)
         {
            fmpz fc = poly2[len2 - x->j - 1];

            if (x->i == -WORD(1))
            {
               if (!COEFF_IS_MPZ(fc))
               {
                  if (fc >= 0)
                     sub_dddmmmsss(c[2], c[1], c[0], c[2], c[1], c[0], 0, 0, fc);
                  else
                     add_sssaaaaaa(c[2], c[1], c[0], c[2], c[1], c[0], 0, 0, -fc);
               } else
               {
                  slong size = fmpz_size(poly2 + len2 - x->j - 1);
                  __mpz_struct * m = COEFF_TO_PTR(fc);
                  if (fmpz_sgn(poly2 + len2 - x->j - 1) < 0)
                     mpn_add(c, c, 3, m->_mp_d, size);
                  else
                     mpn_sub(c, c, 3, m->_mp_d, size);
               }
            } else
            {
               smul_ppmm(p[1], p[0], poly3[len3 - x->i - 1], p1[x->j]);
               if (0 > (slong) p[1])
                  add_sssaaaaaa(c[2], c[1], c[0], c[2], c[1], c[0], ~WORD(0), p[1], p[0]);
               else
                  add_sssaaaaaa(c[2], c[1], c[0], c[2], c[1], c[0], 0, p[1], p[0]);
            }
         } else
         {
            if (x->i == -WORD(1))
               fmpz_sub(qc, qc, poly2 + len2 - x->j - 1);
            else
               fmpz_addmul(qc, poly3 + len3 - x->i - 1, p1 + x->j);
         }

         if (x->i != -WORD(1) || x->j < len2 - 1)
            Q[Q_len++] = x;
         else
            reuse[reuse_len++] = x;

         while ((x = x->next) != NULL)
         {
            if (small)
            {
               fmpz fc = poly2[len2 - x->j - 1];

               if (x->i == -WORD(1))
               {
                  if (!COEFF_IS_MPZ(fc))
                  {
                     if (fc >= 0)
                        sub_dddmmmsss(c[2], c[1], c[0], c[2], c[1], c[0], 0, 0, fc);
                     else
                        add_sssaaaaaa(c[2], c[1], c[0], c[2], c[1], c[0], 0, 0, -fc);
                  } else
                  {
                     slong size = fmpz_size(poly2 + len2 - x->j - 1);
                     __mpz_struct * m = COEFF_TO_PTR(fc);
                     if (fmpz_sgn(poly2 + len2 - x->j - 1) < 0)
                        mpn_add(c, c, 3, m->_mp_d, size);
                     else
                        mpn_sub(c, c, 3, m->_mp_d, size);
                  }
               } else
               {
                  smul_ppmm(p[1], p[0], poly3[len3 - x->i - 1], p1[x->j]);
                  if (0 > (slong) p[1])
                     add_sssaaaaaa(c[2], c[1], c[0], c[2], c[1], c[0], ~WORD(0), p[1], p[0]);
                  else
                     add_sssaaaaaa(c[2], c[1], c[0], c[2], c[1], c[0], 0, p[1], p[0]);
               }
            } else
            {
               if (x->i == -WORD(1))
                  fmpz_sub(qc, qc, poly2 + len2 - x->j - 1);
               else
                  fmpz_addmul(qc, poly3 + len3 - x->i - 1, p1 + x->j);
            }

            if (x->i != -WORD(1) || x->j < len2 - 1)
               Q[Q_len++] = x;
            else
               reuse[reuse_len++] = x;
         }
      }

      while (Q_len > 0)
      {
         x = Q[--Q_len];
     
         if (x->i == -WORD(1))
         {
            x->j++;
            x->next = NULL;

            mpoly_monomial_sub(exp_list[exp_next], maxn, exp2 + (len2 - x->j - 1)*N, N);

            if (!_mpoly_heap_insert(heap, exp_list[exp_next++], x, &heap_len, N))
               exp_next--;
         } else if (x->j < k - 1)
         {
            x->j++;
            x->next = NULL;

            mpoly_monomial_sub(texp, maxn, exp3 + (len3 - x->i - 1)*N, N);
            mpoly_monomial_sub(exp_list[exp_next], texp, e1 + x->j*N, N);

            if (!_mpoly_heap_insert(heap, exp_list[exp_next++], x, &heap_len, N))
               exp_next--;
         } else if (x->j == k - 1)
         {
            s++;
            reuse[reuse_len++] = x;
         }
      }

      if ((small && (c[2] == 0 && c[1] == 0 && c[0] == 0)) || (!small && fmpz_is_zero(qc)))
         k--;
      else
      {
         mpoly_monomial_sub(texp, maxn, exp3 + (len3 - 1)*N, N);

         d1 = mpoly_monomial_divides(e1 + k*N, texp, exp, N, mask);

         if (!d1)
         {
            l++;

            if (l >= *allocr)
            {
               p2 = (fmpz *) flint_realloc(p2, 2*sizeof(fmpz)*(*allocr));
               e2 = (ulong *) flint_realloc(e2, 2*N*sizeof(ulong)*(*allocr));
               flint_mpn_zero(p2 + *allocr, *allocr);
               (*allocr) *= 2;
            }

            if (small)
            {
               fmpz_set_signed_uiuiui(p2 + l, c[2], c[1], c[0]);
               fmpz_neg(p2 + l, p2 + l);
            } else
               fmpz_neg(p2 + l, qc); 

            mpoly_monomial_sub(e2 + l*N, maxn, exp, N);

            k--;
         } else
         {
            if (small)
            {
               ulong d[3];

               if (0 > (slong) c[2])
                  mpn_neg(d, c, 3);
               else
                  flint_mpn_copyi(d, c, 3);

               if (d[2] != 0 || ub < d[1] || (ub == 0 && 0 > (slong) d[0])) /* quotient not a small */
               {
                  fmpz_set_signed_uiuiui(qc, c[2], c[1], c[0]);

                  small = 0;
               } else /* quotient fits a small */
               {
                  ulong r1;

                  sdiv_qrnnd(p1[k], r1, c[1], c[0], *mb);

                  if (r1 != 0)
                  {
                     l++;

                     if (l >= *allocr)
                     {
                        p2 = (fmpz *) flint_realloc(p2, 2*sizeof(fmpz)*(*allocr));
                        e2 = (ulong *) flint_realloc(e2, 2*N*sizeof(ulong)*(*allocr));
                        flint_mpn_zero(p2 + *allocr, *allocr);
                        (*allocr) *= 2;
                     }

                     p2[l] = -r1;

                     mpoly_monomial_sub(e2 + l*N, maxn, exp, N);
                  }
               }
            } 

            /* quotient non-small case */
            if (!small)
            {
               fmpz_fdiv_qr(p1 + k, r, qc, mb);

               if (!fmpz_is_zero(r))
               {
                  l++;

                  if (l >= *allocr)
                  {
                     p2 = (fmpz *) flint_realloc(p2, 2*sizeof(fmpz)*(*allocr));
                     e2 = (ulong *) flint_realloc(e2, 2*N*sizeof(ulong)*(*allocr));
                     flint_mpn_zero(p2 + *allocr, *allocr);
                     (*allocr) *= 2;
                  }

                  fmpz_neg(p2 + l, r);
                     
                  mpoly_monomial_sub(e2 + l*N, maxn, exp, N);
               }
            }

            if (!fmpz_is_zero(p1 + k))
            {
               for (i = 1; i < s; i++)
               {
                  if (reuse_len != 0)
                     x2 = reuse[--reuse_len];
                  else
                     x2 = chain + next_free++;
            
                  x2->i = i;
                  x2->j = k;
                  x2->next = NULL;

                  mpoly_monomial_sub(texp, maxn, exp3 + (len3 - i - 1)*N, N);
                  mpoly_monomial_sub(exp_list[exp_next], texp, e1 + k*N, N);

                  if (!_mpoly_heap_insert(heap, exp_list[exp_next++], x2, &heap_len, N))
                     exp_next--;
               }
               s = 1;
            } else
               k--;
         }
      } 
      
      fmpz_zero(qc);  
   }

   k++;
   l++;

   fmpz_clear(mb);
   fmpz_clear(qc);
   fmpz_clear(r);

   (*polyq) = p1;
   (*expq) = e1;
   (*polyr) = p2;
   (*expr) = e2;
   
   (*lenr) = l;

   TMP_END;

   return k;
}

void fmpz_mpoly_divrem_monagan_pearce(fmpz_mpoly_t q, fmpz_mpoly_t r,
                  const fmpz_mpoly_t poly2, const fmpz_mpoly_t poly3,
                                                    const fmpz_mpoly_ctx_t ctx)
{
   slong i, exp_bits, N, lenq = 0, lenr = 0;
   ulong * exp2, * exp3;
   ulong * max_degs2, * max_degs3;
   int free2 = 0, free3 = 0;
   fmpz_mpoly_t temp1, temp2;
   fmpz_mpoly_struct * tq, * tr;
   ulong * maxn, * maxexp;
   int deg, rev;
   TMP_INIT;

   if (poly3->length == 0)
      flint_throw(FLINT_DIVZERO, "Divide by zero in fmpz_mpoly_divrem_monagan_pearce");

   if (poly2->length == 0)
   {
      fmpz_mpoly_zero(q, ctx);
      fmpz_mpoly_zero(r, ctx);

      return;
   }

   TMP_START;

   degrev_from_ord(deg, rev, ctx->ord);

   maxexp = (ulong *) TMP_ALLOC(ctx->n*sizeof(ulong));

   max_degs2 = (ulong *) TMP_ALLOC(ctx->n*sizeof(ulong));
   max_degs3 = (ulong *) TMP_ALLOC(ctx->n*sizeof(ulong));

   fmpz_mpoly_max_degrees(max_degs2, poly2, ctx);
   fmpz_mpoly_max_degrees(max_degs3, poly3, ctx);

   for (i = 0; i < ctx->n; i++)
      maxexp[i] = FLINT_MAX(max_degs2[i], max_degs3[i]);

   exp_bits = FLINT_MAX(poly2->bits, poly3->bits);

   N = (exp_bits*ctx->n - 1)/FLINT_BITS + 1;

   maxn = (ulong *) TMP_ALLOC(N*sizeof(ulong));

   mpoly_set_monomial(maxn, maxexp, exp_bits, ctx->n, deg, rev);

   exp2 = mpoly_unpack_monomials(exp_bits, poly2->exps, 
                                           poly2->length, ctx->n, poly2->bits);

   free2 = exp2 != poly2->exps;

   exp3 = mpoly_unpack_monomials(exp_bits, poly3->exps, 
                                           poly3->length, ctx->n, poly3->bits);
   
   free3 = exp3 != poly3->exps;

   if (q == poly2 || q == poly3)
   {
      fmpz_mpoly_init2(temp1, FLINT_MAX(poly2->length/poly3->length + 1, 1),
                                                                          ctx);
      fmpz_mpoly_fit_bits(temp1, exp_bits, ctx);

      tq = temp1;
   } else
   {
      fmpz_mpoly_fit_length(q, FLINT_MAX(poly2->length/poly3->length + 1, 1),
                                                                          ctx);
      fmpz_mpoly_fit_bits(q, exp_bits, ctx);

      tq = q;
   }

   if (r == poly2 || r == poly3)
   {
      fmpz_mpoly_init2(temp2, poly3->length, ctx);
      fmpz_mpoly_fit_bits(temp2, exp_bits, ctx);

      tr = temp2;
   } else
   {
      fmpz_mpoly_fit_length(r, poly3->length, ctx);
      fmpz_mpoly_fit_bits(r, exp_bits, ctx);

      tr = r;
   }

   lenq = _fmpz_mpoly_divrem_monagan_pearce(&lenr, &tq->coeffs, &tq->exps,
         &tq->alloc, &tr->coeffs, &tr->exps, &tr->alloc, poly2->coeffs, exp2, 
         poly2->length, poly3->coeffs, exp3, poly3->length, maxn, exp_bits, N);

   if (q == poly2 || q == poly3)
   {
      fmpz_mpoly_swap(temp1, q, ctx);

      fmpz_mpoly_clear(temp1, ctx);
   } 

   if (r == poly2 || r == poly3)
   {
      fmpz_mpoly_swap(temp2, r, ctx);

      fmpz_mpoly_clear(temp2, ctx);
   } 

   _fmpz_mpoly_set_length(q, lenq, ctx);
   _fmpz_mpoly_set_length(r, lenr, ctx);

   fmpz_mpoly_reverse(q, q, ctx);
   fmpz_mpoly_reverse(r, r, ctx);

   if (free2)
      flint_free(exp2);

   if (free3)
      flint_free(exp3);

   TMP_END;
}
