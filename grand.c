/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* Originally developed and coded by Makoto Matsumoto and Takuji
 * Nishimura.  Please mail <matumoto@math.keio.ac.jp>, if you're using
 * code from this file in your own programs or libraries.
 * Further information on the Mersenne Twister can be found at
 * http://www.math.keio.ac.jp/~matumoto/emt.html
 */

/*
 * Modified by the GLib Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.  
 */

#include <glib.h>
#include <math.h>
#include <stdio.h>

G_LOCK_DEFINE_STATIC (global_random);
static GRand* global_random = NULL;

/* Period parameters */  
#define N 624
#define M 397
#define MATRIX_A 0x9908b0df   /* constant vector a */
#define UPPER_MASK 0x80000000 /* most significant w-r bits */
#define LOWER_MASK 0x7fffffff /* least significant r bits */

/* Tempering parameters */   
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)
#define TEMPERING_SHIFT_T(y)  (y << 15)
#define TEMPERING_SHIFT_L(y)  (y >> 18)

struct _GRand
{
  guint32 mt[N]; /* the array for the state vector  */
  guint mti; 
  gboolean have_next_normal;
  gdouble next_normal;
};

GRand*
g_rand_new_with_seed (guint32 seed)
{
  GRand *rand = g_new0 (GRand, 1);
  g_rand_set_seed (rand, seed);
  return rand;
}

GRand* 
g_rand_new ()
{
  guint32 seed = 0;
  GTimeVal now;
  FILE* dev_random = fopen("/dev/random", "rb");

  if (dev_random)
    {
      if (fread (&seed, sizeof (seed), 1, dev_random) != 1)
	seed = 0;
      fclose (dev_random);
    }

  /* Using /dev/random alone makes the seed computable for the
     outside. This might pose security problems somewhere. This should
     yield better values */

  g_get_current_time (&now);
  seed ^= now.tv_sec ^ now.tv_usec;

  return g_rand_new_with_seed (seed);
}

void
g_rand_free (GRand* rand)
{
  g_return_if_fail (rand);

  g_free (rand);
}

void
g_rand_set_seed (GRand* rand, guint32 seed)
{
  g_return_if_fail (rand);

  /* setting initial seeds to mt[N] using         */
  /* the generator Line 25 of Table 1 in          */
  /* [KNUTH 1981, The Art of Computer Programming */
  /*    Vol. 2 (2nd Ed.), pp102]                  */
  rand->mt[0]= seed & 0xffffffff;
  for (rand->mti=1; rand->mti<N; rand->mti++)
    rand->mt[rand->mti] = (69069 * rand->mt[rand->mti-1]) & 0xffffffff;

  rand->have_next_normal = FALSE;
}

guint32
g_rand_int (GRand* rand)
{
  guint32 y;
  static const guint32 mag01[2]={0x0, MATRIX_A};
  /* mag01[x] = x * MATRIX_A  for x=0,1 */

  g_return_val_if_fail (rand, 0);

  if (rand->mti >= N) { /* generate N words at one time */
    int kk;
    
    for (kk=0;kk<N-M;kk++) {
      y = (rand->mt[kk]&UPPER_MASK)|(rand->mt[kk+1]&LOWER_MASK);
      rand->mt[kk] = rand->mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1];
    }
    for (;kk<N-1;kk++) {
      y = (rand->mt[kk]&UPPER_MASK)|(rand->mt[kk+1]&LOWER_MASK);
      rand->mt[kk] = rand->mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
    }
    y = (rand->mt[N-1]&UPPER_MASK)|(rand->mt[0]&LOWER_MASK);
    rand->mt[N-1] = rand->mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];
    
    rand->mti = 0;
  }
  
  y = rand->mt[rand->mti++];
  y ^= TEMPERING_SHIFT_U(y);
  y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
  y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
  y ^= TEMPERING_SHIFT_L(y);
  
  return y; 
}

gint32 
g_rand_int_range (GRand* rand, gint32 min, gint32 max)
{
  guint32 dist = max - min;
  guint32 random;

  g_return_val_if_fail (rand, min);
  g_return_val_if_fail (max > min, min);

  if (dist <= 0x10000L) /* 2^16 */
    {
      /* All tricks doing modulo calculations do not have a good
	 distribution -> We must use this slower method for maximal
	 quality, but this method is only good for (max - min) <= 2^16 */
      
      random = (gint32) g_rand_double_range (rand, 0, dist);
      /* we'd rather use the following, if -lm is allowed later on:
	 random = (gint32) floor (g_rand_double_range (rand, 0, dist));  */
    }
  else
    {
      /* Now it's harder to make it right. We calculate the smallest m,
         such that dist < 2 ^ m, then we calculate a random number in
         [1..2^32-1] and rightshift it by 32 - m. Then we test, if it
         is smaller than dist and if not, get a new number and so
         forth until we get a number smaller than dist. We just return
         this. */
      guint32 border = 0x20000L; /* 2^17 */
      guint right_shift = 15; /* 32 - 17 */

      if (dist >= 0x80000000) /* in the case of dist > 2^31 our loop
				below will be infinite */
	{
	  right_shift = 0;
	}
      else
	{
	  while (dist >= border) 
	    {
	      border <<= 1;
	      right_shift--;
	    }
	}
      do 
	{ 
	  random = g_rand_int (rand) >> right_shift; 
	} while (random >= dist);
    }
  return min + random;
}

/* transform [0..2^32-1] -> [0..1) */
#define G_RAND_DOUBLE_TRANSFORM 2.3283064365386963e-10

gdouble 
g_rand_double (GRand* rand)
{                            
  return g_rand_int (rand) * G_RAND_DOUBLE_TRANSFORM;
}

gdouble 
g_rand_double_range (GRand* rand, gdouble min, gdouble max)
{
  return g_rand_int (rand) * ((max - min) * G_RAND_DOUBLE_TRANSFORM)  + min;
}


#if WE_REALLY_WANT_HAVE_MATH_LIB_LINKED
gdouble
g_rand_normal (GRand* rand, gdouble mean, gdouble standard_deviation)
{
  /* For a description of the used algorithm see Knuth: "The Art of
     Computer Programming", Vol.2, Second Edition, Page 117: Polar
     method for normal deviates due to Box, Muller, Marsaglia */
  gdouble normal;
  g_return_val_if_fail (rand, 0);

  if (rand->have_next_normal) 
    {
      rand->have_next_normal = FALSE;
      normal = rand->next_normal;
    }
  else
    {
      gdouble u1;
      gdouble u2 = g_rand_double_range (rand, -1, 1); 
      gdouble s, f;
      do 
	{ 
	  u1 = u2;
	  u2 = g_rand_double_range (rand, -1, 1);
	  s = u1 * u1 + u2 * u2;
	} while (s >= 1.0);
      f = sqrt (-2 * log (s) / s);
      normal = u1 * f;
      rand->next_normal = u2 * f;
      rand->have_next_normal = TRUE;
    }
  return mean + normal * standard_deviation;
}
#endif

guint32
g_random_int (void)
{
  guint32 result;
  G_LOCK (global_random);
  if (!global_random)
    global_random = g_rand_new ();
  
  result = g_rand_int (global_random);
  G_UNLOCK (global_random);
  return result;
}

gint32 
g_random_int_range (gint32 min, gint32 max)
{
  gint32 result;
  G_LOCK (global_random);
  if (!global_random)
    global_random = g_rand_new ();
  
  result = g_rand_int_range (global_random, min, max);
  G_UNLOCK (global_random);
  return result;
}

gdouble 
g_random_double (void)
{
  double result;
  G_LOCK (global_random);
  if (!global_random)
    global_random = g_rand_new ();
  
  result = g_rand_double (global_random);
  G_UNLOCK (global_random);
  return result;
}

gdouble 
g_random_double_range (gdouble min, gdouble max)
{
  double result;
  G_LOCK (global_random);
  if (!global_random)
    global_random = g_rand_new ();
 
  result = g_rand_double_range (global_random, min, max);
  G_UNLOCK (global_random);
  return result;
}

#if WE_REALLY_WANT_HAVE_MATH_LIB_LINKED
gdouble
g_random_normal (gdouble mean, gdouble standard_deviation)
{
  double result;
  G_LOCK (global_random);
  if (!global_random)
    global_random = g_rand_new ();
 
  result = g_rand_normal (global_random, mean, standard_deviation);
  G_UNLOCK (global_random);
  return result;
}
#endif

void
g_random_set_seed (guint32 seed)
{
  G_LOCK (global_random);
  if (!global_random)
    global_random = g_rand_new_with_seed (seed);
  else
    g_rand_set_seed (global_random, seed);
  G_UNLOCK (global_random);
}

