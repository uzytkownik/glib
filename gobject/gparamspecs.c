/* GObject - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 1997, 1998, 1999, 2000 Tim Janik and Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include	"gparamspecs.h"

#include	"gvaluecollector.h"
#include	<string.h>
#include	"../config.h"	/* for SIZEOF_LONG */

#define	G_FLOAT_EPSILON		(1e-30)
#define	G_DOUBLE_EPSILON	(1e-90)


/* --- prototypes --- */
extern void	g_param_spec_types_init	(void);


/* --- param spec functions --- */
static void
param_spec_char_init (GParamSpec *pspec)
{
  GParamSpecChar *cspec = G_PARAM_SPEC_CHAR (pspec);
  
  cspec->minimum = 0x7f;
  cspec->maximum = 0x80;
  cspec->default_value = 0;
}

static void
param_char_init (GValue	    *value,
		 GParamSpec *pspec)
{
  if (pspec)
    value->data[0].v_int = G_PARAM_SPEC_CHAR (pspec)->default_value;
}

static gboolean
param_char_validate (GValue	*value,
		     GParamSpec *pspec)
{
  GParamSpecChar *cspec = G_PARAM_SPEC_CHAR (pspec);
  gint oval = value->data[0].v_int;
  
  value->data[0].v_int = CLAMP (value->data[0].v_int, cspec->minimum, cspec->maximum);
  
  return value->data[0].v_int != oval;
}

static gchar*
param_char_lcopy_value (const GValue *value,
			GParamSpec   *pspec,
			guint	      nth_value,
			GType	     *collect_type,
			GParamCValue *collect_value)
{
  gint8 *int8_p = collect_value->v_pointer;
  
  if (!int8_p)
    return g_strdup_printf ("value location for `%s' passed as NULL",
			    g_type_name (pspec ? G_PARAM_SPEC_TYPE (pspec) : G_VALUE_TYPE (value)));
  
  *int8_p = value->data[0].v_int;
  
  *collect_type = 0;
  return NULL;
}

static void
param_spec_uchar_init (GParamSpec *pspec)
{
  GParamSpecUChar *uspec = G_PARAM_SPEC_UCHAR (pspec);
  
  uspec->minimum = 0;
  uspec->maximum = 0xff;
  uspec->default_value = 0;
}

static void
param_uchar_init (GValue     *value,
		  GParamSpec *pspec)
{
  if (pspec)
    value->data[0].v_uint = G_PARAM_SPEC_UCHAR (pspec)->default_value;
}

static gboolean
param_uchar_validate (GValue	 *value,
		      GParamSpec *pspec)
{
  GParamSpecUChar *uspec = G_PARAM_SPEC_UCHAR (pspec);
  guint oval = value->data[0].v_uint;
  
  value->data[0].v_uint = CLAMP (value->data[0].v_uint, uspec->minimum, uspec->maximum);
  
  return value->data[0].v_uint != oval;
}

static void
param_bool_init (GValue	    *value,
		 GParamSpec *pspec)
{
  if (pspec)
    value->data[0].v_int = G_PARAM_SPEC_BOOL (pspec)->default_value;
}

static gboolean
param_bool_validate (GValue	*value,
		     GParamSpec *pspec)
{
  gint oval = value->data[0].v_int;
  
  value->data[0].v_int = value->data[0].v_int != FALSE;
  
  return value->data[0].v_int != oval;
}

static gchar*
param_bool_lcopy_value (const GValue *value,
			GParamSpec   *pspec,
			guint	      nth_value,
			GType	     *collect_type,
			GParamCValue *collect_value)
{
  gboolean *bool_p = collect_value->v_pointer;
  
  if (!bool_p)
    return g_strdup_printf ("value location for `%s' passed as NULL",
			    g_type_name (pspec ? G_PARAM_SPEC_TYPE (pspec) : G_VALUE_TYPE (value)));
  
  *bool_p = value->data[0].v_int;
  
  *collect_type = 0;
  return NULL;
}

static void
param_spec_int_init (GParamSpec *pspec)
{
  GParamSpecInt *ispec = G_PARAM_SPEC_INT (pspec);
  
  ispec->minimum = 0x7fffffff;
  ispec->maximum = 0x80000000;
  ispec->default_value = 0;
}

static void
param_int_init (GValue	   *value,
		GParamSpec *pspec)
{
  if (pspec)
    value->data[0].v_int = G_PARAM_SPEC_INT (pspec)->default_value;
}

static gboolean
param_int_validate (GValue     *value,
		    GParamSpec *pspec)
{
  GParamSpecInt *ispec = G_PARAM_SPEC_INT (pspec);
  gint oval = value->data[0].v_int;
  
  value->data[0].v_int = CLAMP (value->data[0].v_int, ispec->minimum, ispec->maximum);
  
  return value->data[0].v_int != oval;
}

static gint
param_int_values_cmp (const GValue *value1,
		      const GValue *value2,
		      GParamSpec   *pspec)
{
  if (value1->data[0].v_int < value2->data[0].v_int)
    return -1;
  else
    return value1->data[0].v_int - value2->data[0].v_int;
}

static gchar*
param_int_collect_value (GValue	      *value,
			 GParamSpec   *pspec,
			 guint	       nth_value,
			 GType	      *collect_type,
			 GParamCValue *collect_value)
{
  value->data[0].v_int = collect_value->v_int;
  
  *collect_type = 0;
  return NULL;
}

static gchar*
param_int_lcopy_value (const GValue *value,
		       GParamSpec   *pspec,
		       guint	     nth_value,
		       GType	    *collect_type,
		       GParamCValue *collect_value)
{
  gint *int_p = collect_value->v_pointer;
  
  if (!int_p)
    return g_strdup_printf ("value location for `%s' passed as NULL",
			    g_type_name (pspec ? G_PARAM_SPEC_TYPE (pspec) : G_VALUE_TYPE (value)));
  
  *int_p = value->data[0].v_int;
  
  *collect_type = 0;
  return NULL;
}

static void
param_spec_uint_init (GParamSpec *pspec)
{
  GParamSpecUInt *uspec = G_PARAM_SPEC_UINT (pspec);
  
  uspec->minimum = 0;
  uspec->maximum = 0xffffffff;
  uspec->default_value = 0;
}

static void
param_uint_init (GValue	    *value,
		 GParamSpec *pspec)
{
  if (pspec)
    value->data[0].v_uint = G_PARAM_SPEC_UINT (pspec)->default_value;
}

static gboolean
param_uint_validate (GValue	*value,
		     GParamSpec *pspec)
{
  GParamSpecUInt *uspec = G_PARAM_SPEC_UINT (pspec);
  guint oval = value->data[0].v_uint;
  
  value->data[0].v_uint = CLAMP (value->data[0].v_uint, uspec->minimum, uspec->maximum);
  
  return value->data[0].v_uint != oval;
}

static gint
param_uint_values_cmp (const GValue *value1,
		       const GValue *value2,
		       GParamSpec   *pspec)
{
  if (value1->data[0].v_uint < value2->data[0].v_uint)
    return -1;
  else
    return value1->data[0].v_uint - value2->data[0].v_uint;
}

static void
param_spec_long_init (GParamSpec *pspec)
{
  GParamSpecLong *lspec = G_PARAM_SPEC_LONG (pspec);
  
#if SIZEOF_LONG == 4
  lspec->minimum = 0x7fffffff;
  lspec->maximum = 0x80000000;
#else /* SIZEOF_LONG != 4 (8) */
  lspec->minimum = 0x7fffffffffffffff;
  lspec->maximum = 0x8000000000000000;
#endif
  lspec->default_value = 0;
}

static void
param_long_init (GValue	    *value,
		 GParamSpec *pspec)
{
  if (pspec)
    value->data[0].v_long = G_PARAM_SPEC_LONG (pspec)->default_value;
}

static gboolean
param_long_validate (GValue	*value,
		     GParamSpec *pspec)
{
  GParamSpecLong *lspec = G_PARAM_SPEC_LONG (pspec);
  glong oval = value->data[0].v_long;
  
  value->data[0].v_long = CLAMP (value->data[0].v_long, lspec->minimum, lspec->maximum);
  
  return value->data[0].v_long != oval;
}

static gint
param_long_values_cmp (const GValue *value1,
		       const GValue *value2,
		       GParamSpec   *pspec)
{
  if (value1->data[0].v_long < value2->data[0].v_long)
    return -1;
  else
    return value1->data[0].v_long - value2->data[0].v_long;
}

static gchar*
param_long_collect_value (GValue       *value,
			  GParamSpec   *pspec,
			  guint		nth_value,
			  GType	       *collect_type,
			  GParamCValue *collect_value)
{
  value->data[0].v_long = collect_value->v_long;
  
  *collect_type = 0;
  return NULL;
}

static gchar*
param_long_lcopy_value (const GValue *value,
			GParamSpec   *pspec,
			guint	      nth_value,
			GType	     *collect_type,
			GParamCValue *collect_value)
{
  glong *long_p = collect_value->v_pointer;
  
  if (!long_p)
    return g_strdup_printf ("value location for `%s' passed as NULL",
			    g_type_name (pspec ? G_PARAM_SPEC_TYPE (pspec) : G_VALUE_TYPE (value)));
  
  *long_p = value->data[0].v_long;
  
  *collect_type = 0;
  return NULL;
}

static void
param_spec_ulong_init (GParamSpec *pspec)
{
  GParamSpecULong *uspec = G_PARAM_SPEC_ULONG (pspec);
  
  uspec->minimum = 0;
#if SIZEOF_LONG == 4
  uspec->maximum = 0xffffffff;
#else /* SIZEOF_LONG != 4 (8) */
  uspec->maximum = 0xffffffffffffffff;
#endif
  uspec->default_value = 0;
}

static void
param_ulong_init (GValue     *value,
		  GParamSpec *pspec)
{
  if (pspec)
    value->data[0].v_ulong = G_PARAM_SPEC_ULONG (pspec)->default_value;
}

static gboolean
param_ulong_validate (GValue	 *value,
		      GParamSpec *pspec)
{
  GParamSpecULong *uspec = G_PARAM_SPEC_ULONG (pspec);
  gulong oval = value->data[0].v_ulong;
  
  value->data[0].v_ulong = CLAMP (value->data[0].v_ulong, uspec->minimum, uspec->maximum);
  
  return value->data[0].v_ulong != oval;
}

static gint
param_ulong_values_cmp (const GValue *value1,
			const GValue *value2,
			GParamSpec   *pspec)
{
  if (value1->data[0].v_ulong < value2->data[0].v_ulong)
    return -1;
  else
    return value1->data[0].v_ulong - value2->data[0].v_ulong;
}

static void
param_spec_enum_init (GParamSpec *pspec)
{
  GParamSpecEnum *espec = G_PARAM_SPEC_ENUM (pspec);
  
  espec->enum_class = NULL;
  espec->default_value = 0;
}

static void
param_spec_enum_finalize (GParamSpec *pspec)
{
  GParamSpecEnum *espec = G_PARAM_SPEC_ENUM (pspec);
  GParamSpecClass *parent_class = g_type_class_peek (g_type_parent (G_TYPE_PARAM_ENUM));
  
  if (espec->enum_class)
    {
      g_type_class_unref (espec->enum_class);
      espec->enum_class = NULL;
    }
  
  parent_class->finalize (pspec);
}

static void
param_enum_init (GValue	    *value,
		 GParamSpec *pspec)
{
  if (pspec)
    value->data[0].v_long = G_PARAM_SPEC_ENUM (pspec)->default_value;
}

static gboolean
param_enum_validate (GValue	*value,
		     GParamSpec *pspec)
{
  GParamSpecEnum *espec = G_PARAM_SPEC_ENUM (pspec);
  glong oval = value->data[0].v_long;
  
  if (!espec->enum_class ||
      !g_enum_get_value (espec->enum_class, value->data[0].v_long))
    value->data[0].v_long = espec->default_value;
  
  return value->data[0].v_long != oval;
}

static gchar*
param_enum_collect_value (GValue       *value,
			  GParamSpec   *pspec,
			  guint		nth_value,
			  GType	       *collect_type,
			  GParamCValue *collect_value)
{
  value->data[0].v_long = collect_value->v_int;
  
  *collect_type = 0;
  return NULL;
}

static gchar*
param_enum_lcopy_value (const GValue *value,
			GParamSpec   *pspec,
			guint	      nth_value,
			GType	     *collect_type,
			GParamCValue *collect_value)
{
  gint *int_p = collect_value->v_pointer;
  
  if (!int_p)
    return g_strdup_printf ("value location for `%s' passed as NULL",
			    g_type_name (pspec ? G_PARAM_SPEC_TYPE (pspec) : G_VALUE_TYPE (value)));
  
  *int_p = value->data[0].v_long;
  
  *collect_type = 0;
  return NULL;
}

static void
param_spec_flags_init (GParamSpec *pspec)
{
  GParamSpecFlags *fspec = G_PARAM_SPEC_FLAGS (pspec);
  
  fspec->flags_class = NULL;
  fspec->default_value = 0;
}

static void
param_spec_flags_finalize (GParamSpec *pspec)
{
  GParamSpecFlags *fspec = G_PARAM_SPEC_FLAGS (pspec);
  GParamSpecClass *parent_class = g_type_class_peek (g_type_parent (G_TYPE_PARAM_FLAGS));
  
  if (fspec->flags_class)
    {
      g_type_class_unref (fspec->flags_class);
      fspec->flags_class = NULL;
    }
  
  parent_class->finalize (pspec);
}

static void
param_flags_init (GValue     *value,
		  GParamSpec *pspec)
{
  if (pspec)
    value->data[0].v_ulong = G_PARAM_SPEC_FLAGS (pspec)->default_value;
}

static gboolean
param_flags_validate (GValue	 *value,
		      GParamSpec *pspec)
{
  GParamSpecFlags *fspec = G_PARAM_SPEC_FLAGS (pspec);
  gulong oval = value->data[0].v_ulong;
  
  if (fspec->flags_class)
    value->data[0].v_ulong &= fspec->flags_class->mask;
  else
    value->data[0].v_ulong = fspec->default_value;
  
  return value->data[0].v_ulong != oval;
}

static void
param_spec_float_init (GParamSpec *pspec)
{
  GParamSpecFloat *fspec = G_PARAM_SPEC_FLOAT (pspec);
  
  fspec->minimum = G_MINFLOAT;
  fspec->maximum = G_MAXFLOAT;
  fspec->default_value = 0;
  fspec->epsilon = G_FLOAT_EPSILON;
}

static void
param_float_init (GValue     *value,
		  GParamSpec *pspec)
{
  if (pspec)
    value->data[0].v_float = G_PARAM_SPEC_FLOAT (pspec)->default_value;
}

static gboolean
param_float_validate (GValue	 *value,
		      GParamSpec *pspec)
{
  GParamSpecFloat *fspec = G_PARAM_SPEC_FLOAT (pspec);
  gfloat oval = value->data[0].v_float;
  
  value->data[0].v_float = CLAMP (value->data[0].v_float, fspec->minimum, fspec->maximum);
  
  return value->data[0].v_float != oval;
}

static gint
param_float_values_cmp (const GValue *value1,
			const GValue *value2,
			GParamSpec   *pspec)
{
  gfloat epsilon = pspec ? G_PARAM_SPEC_FLOAT (pspec)->epsilon : G_FLOAT_EPSILON;
  
  if (value1->data[0].v_float < value2->data[0].v_float)
    return - (value2->data[0].v_float - value1->data[0].v_float > epsilon);
  else
    return value1->data[0].v_float - value2->data[0].v_float > epsilon;
}

static gchar*
param_float_collect_value (GValue	*value,
			   GParamSpec	*pspec,
			   guint	 nth_value,
			   GType	*collect_type,
			   GParamCValue *collect_value)
{
  value->data[0].v_float = collect_value->v_double;
  
  *collect_type = 0;
  return NULL;
}

static gchar*
param_float_lcopy_value (const GValue *value,
			 GParamSpec   *pspec,
			 guint	       nth_value,
			 GType	      *collect_type,
			 GParamCValue *collect_value)
{
  gfloat *float_p = collect_value->v_pointer;
  
  if (!float_p)
    return g_strdup_printf ("value location for `%s' passed as NULL",
			    g_type_name (pspec ? G_PARAM_SPEC_TYPE (pspec) : G_VALUE_TYPE (value)));
  
  *float_p = value->data[0].v_float;
  
  *collect_type = 0;
  return NULL;
}

static void
param_spec_double_init (GParamSpec *pspec)
{
  GParamSpecDouble *dspec = G_PARAM_SPEC_DOUBLE (pspec);
  
  dspec->minimum = G_MINDOUBLE;
  dspec->maximum = G_MAXDOUBLE;
  dspec->default_value = 0;
  dspec->epsilon = G_DOUBLE_EPSILON;
}

static void
param_double_init (GValue     *value,
		   GParamSpec *pspec)
{
  if (pspec)
    value->data[0].v_double = G_PARAM_SPEC_DOUBLE (pspec)->default_value;
}

static gboolean
param_double_validate (GValue	  *value,
		       GParamSpec *pspec)
{
  GParamSpecDouble *dspec = G_PARAM_SPEC_DOUBLE (pspec);
  gdouble oval = value->data[0].v_double;
  
  value->data[0].v_double = CLAMP (value->data[0].v_double, dspec->minimum, dspec->maximum);
  
  return value->data[0].v_double != oval;
}

static gint
param_double_values_cmp (const GValue *value1,
			 const GValue *value2,
			 GParamSpec   *pspec)
{
  gdouble epsilon = pspec ? G_PARAM_SPEC_DOUBLE (pspec)->epsilon : G_DOUBLE_EPSILON;
  
  if (value1->data[0].v_double < value2->data[0].v_double)
    return - (value2->data[0].v_double - value1->data[0].v_double > epsilon);
  else
    return value1->data[0].v_double - value2->data[0].v_double > epsilon;
}

static gchar*
param_double_collect_value (GValue	 *value,
			    GParamSpec	 *pspec,
			    guint	  nth_value,
			    GType	 *collect_type,
			    GParamCValue *collect_value)
{
  value->data[0].v_double = collect_value->v_double;
  
  *collect_type = 0;
  return NULL;
}

static gchar*
param_double_lcopy_value (const GValue *value,
			  GParamSpec   *pspec,
			  guint		nth_value,
			  GType	       *collect_type,
			  GParamCValue *collect_value)
{
  gdouble *double_p = collect_value->v_pointer;
  
  if (!double_p)
    return g_strdup_printf ("value location for `%s' passed as NULL",
			    g_type_name (pspec ? G_PARAM_SPEC_TYPE (pspec) : G_VALUE_TYPE (value)));
  
  *double_p = value->data[0].v_double;
  
  *collect_type = 0;
  return NULL;
}

static void
param_spec_string_init (GParamSpec *pspec)
{
  GParamSpecString *sspec = G_PARAM_SPEC_STRING (pspec);
  
  sspec->default_value = NULL;
  sspec->cset_first = NULL;
  sspec->cset_nth = NULL;
  sspec->substitutor = '_';
  sspec->null_fold_if_empty = FALSE;
  sspec->ensure_non_null = FALSE;
}

static void
param_spec_string_finalize (GParamSpec *pspec)
{
  GParamSpecString *sspec = G_PARAM_SPEC_STRING (pspec);
  GParamSpecClass *parent_class = g_type_class_peek (g_type_parent (G_TYPE_PARAM_STRING));
  
  g_free (sspec->default_value);
  g_free (sspec->cset_first);
  g_free (sspec->cset_nth);
  sspec->default_value = NULL;
  sspec->cset_first = NULL;
  sspec->cset_nth = NULL;
  
  parent_class->finalize (pspec);
}

static void
param_string_init (GValue     *value,
		   GParamSpec *pspec)
{
  if (pspec)
    value->data[0].v_pointer = g_strdup (G_PARAM_SPEC_STRING (pspec)->default_value);
}

static void
param_string_free_value (GValue *value)
{
  g_free (value->data[0].v_pointer);
}

static gboolean
param_string_validate (GValue	  *value,
		       GParamSpec *pspec)
{
  GParamSpecString *sspec = G_PARAM_SPEC_STRING (pspec);
  gchar *string = value->data[0].v_pointer;
  guint changed = 0;
  
  if (string && string[0])
    {
      gchar *s;
      
      if (sspec->cset_first && !strchr (sspec->cset_first, string[0]))
	{
	  string[0] = sspec->substitutor;
	  changed++;
	}
      if (sspec->cset_nth)
	for (s = string + 1; *s; s++)
	  if (!strchr (sspec->cset_nth, *s))
	    {
	      *s = sspec->substitutor;
	      changed++;
	    }
    }
  if (sspec->null_fold_if_empty && string && string[0] == 0)
    {
      g_free (value->data[0].v_pointer);
      value->data[0].v_pointer = NULL;
      changed++;
      string = value->data[0].v_pointer;
    }
  if (sspec->ensure_non_null && !string)
    {
      value->data[0].v_pointer = g_strdup ("");
      changed++;
      string = value->data[0].v_pointer;
    }
  
  return changed;
}

static gint
param_string_values_cmp (const GValue *value1,
			 const GValue *value2,
			 GParamSpec   *pspec)
{
  if (!value1->data[0].v_pointer)
    return value2->data[0].v_pointer != NULL ? -1 : 0;
  else if (!value2->data[0].v_pointer)
    return value1->data[0].v_pointer != NULL;
  else
    return strcmp (value1->data[0].v_pointer, value2->data[0].v_pointer);
}

static void
param_string_copy_value (const GValue *src_value,
			 GValue       *dest_value)
{
  dest_value->data[0].v_pointer = g_strdup (src_value->data[0].v_pointer);
}

static gchar*
param_string_collect_value (GValue	 *value,
			    GParamSpec	 *pspec,
			    guint	  nth_value,
			    GType	 *collect_type,
			    GParamCValue *collect_value)
{
  value->data[0].v_pointer = g_strdup (collect_value->v_pointer);
  
  *collect_type = 0;
  return NULL;
}

static gchar*
param_string_lcopy_value (const GValue *value,
			  GParamSpec   *pspec,
			  guint		nth_value,
			  GType	       *collect_type,
			  GParamCValue *collect_value)
{
  gchar **string_p = collect_value->v_pointer;
  
  if (!string_p)
    return g_strdup_printf ("value location for `%s' passed as NULL",
			    g_type_name (pspec ? G_PARAM_SPEC_TYPE (pspec) : G_VALUE_TYPE (value)));
  
  *string_p = g_strdup (value->data[0].v_pointer);
  
  *collect_type = 0;
  return NULL;
}

static void
param_spec_object_init (GParamSpec *pspec)
{
  GParamSpecObject *ospec = G_PARAM_SPEC_OBJECT (pspec);
  
  ospec->object_type = G_TYPE_OBJECT;
}

static void
param_object_init (GValue     *value,
		   GParamSpec *pspec)
{
  value->data[0].v_pointer = NULL;
}

static void
param_object_free_value (GValue *value)
{
  if (value->data[0].v_pointer)
    g_object_unref (value->data[0].v_pointer);
}

static gboolean
param_object_validate (GValue	  *value,
		       GParamSpec *pspec)
{
  GParamSpecObject *ospec = G_PARAM_SPEC_OBJECT (pspec);
  GObject *object = value->data[0].v_pointer;
  guint changed = 0;
  
  if (object && !g_type_is_a (G_OBJECT_TYPE (object), ospec->object_type))
    {
      g_object_unref (object);
      value->data[0].v_pointer = NULL;
      changed++;
    }
  
  return changed;
}

static gint
param_object_values_cmp (const GValue *value1,
			 const GValue *value2,
			 GParamSpec   *pspec)
{
  return value1->data[0].v_pointer != value2->data[0].v_pointer;
}

static void
param_object_copy_value (const GValue *src_value,
			 GValue       *dest_value)
{
  if (src_value->data[0].v_pointer)
    dest_value->data[0].v_pointer = g_object_ref (src_value->data[0].v_pointer);
  else
    dest_value->data[0].v_pointer = NULL;
}

static gchar*
param_object_collect_value (GValue	 *value,
			    GParamSpec	 *pspec,
			    guint	  nth_value,
			    GType	 *collect_type,
			    GParamCValue *collect_value)
{
  if (collect_value->v_pointer)
    {
      GObject *object = collect_value->v_pointer;
      
      if (object->g_type_instance.g_class == NULL)
	return g_strconcat ("invalid unclassed object pointer for param type `",
			    g_type_name (pspec ? G_PARAM_SPEC_TYPE (pspec) : G_VALUE_TYPE (value)),
			    "'",
			    NULL);
      else if (pspec && !g_type_is_a (G_OBJECT_TYPE (object), G_PARAM_SPEC_OBJECT (pspec)->object_type))
	return g_strconcat ("invalid object `",
			    G_OBJECT_TYPE_NAME (object),
			    "' for param type `",
			    g_type_name (G_PARAM_SPEC_TYPE (pspec)),
			    "' which requires `",
			    g_type_name (G_PARAM_SPEC_OBJECT (pspec)->object_type),
			    "'",
			    NULL);
      value->data[0].v_pointer = g_object_ref (object);
    }
  else
    value->data[0].v_pointer = NULL;
  
  *collect_type = 0;
  return NULL;
}

static gchar*
param_object_lcopy_value (const GValue *value,
			  GParamSpec   *pspec,
			  guint		nth_value,
			  GType	       *collect_type,
			  GParamCValue *collect_value)
{
  GObject **object_p = collect_value->v_pointer;
  
  if (!object_p)
    return g_strdup_printf ("value location for `%s' passed as NULL",
			    g_type_name (pspec ? G_PARAM_SPEC_TYPE (pspec) : G_VALUE_TYPE (value)));
  
  *object_p = value->data[0].v_pointer ? g_object_ref (value->data[0].v_pointer) : NULL;
  
  *collect_type = 0;
  return NULL;
}

static void
value_exch_memcpy (GValue *value1,
		   GValue *value2)
{
  GValue tmp_value;
  memcpy (&tmp_value.data, &value1->data, sizeof (value1->data));
  memcpy (&value1->data, &value2->data, sizeof (value1->data));
  memcpy (&value2->data, &tmp_value.data, sizeof (value2->data));
}

static void
value_exch_long_int (GValue *value1,
		     GValue *value2)
{
  glong tmp = value1->data[0].v_long;
  value1->data[0].v_long = value2->data[0].v_int;
  value2->data[0].v_int = tmp;
}

static void
value_exch_long_uint (GValue *value1,
		      GValue *value2)
{
  glong tmp = value1->data[0].v_long;
  value1->data[0].v_long = value2->data[0].v_uint;
  value2->data[0].v_uint = tmp;
}

static void
value_exch_ulong_int (GValue *value1,
		      GValue *value2)
{
  gulong tmp = value1->data[0].v_ulong;
  value1->data[0].v_ulong = value2->data[0].v_int;
  value2->data[0].v_int = tmp;
}

static void
value_exch_ulong_uint (GValue *value1,
		       GValue *value2)
{
  gulong tmp = value1->data[0].v_ulong;
  value1->data[0].v_ulong = value2->data[0].v_uint;
  value2->data[0].v_uint = tmp;
}

static void
value_exch_float_int (GValue *value1,
		      GValue *value2)
{
  gfloat tmp = value1->data[0].v_float;
  value1->data[0].v_float = value2->data[0].v_int;
  value2->data[0].v_int = 0.5 + tmp;
}

static void
value_exch_float_uint (GValue *value1,
		       GValue *value2)
{
  gfloat tmp = value1->data[0].v_float;
  value1->data[0].v_float = value2->data[0].v_uint;
  value2->data[0].v_uint = 0.5 + tmp;
}

static void
value_exch_float_long (GValue *value1,
		       GValue *value2)
{
  gfloat tmp = value1->data[0].v_float;
  value1->data[0].v_float = value2->data[0].v_long;
  value2->data[0].v_long = 0.5 + tmp;
}

static void
value_exch_float_ulong (GValue *value1,
			GValue *value2)
{
  gfloat tmp = value1->data[0].v_float;
  value1->data[0].v_float = value2->data[0].v_ulong;
  value2->data[0].v_ulong = 0.5 + tmp;
}

static void
value_exch_double_int (GValue *value1,
		       GValue *value2)
{
  gdouble tmp = value1->data[0].v_double;
  value1->data[0].v_double = value2->data[0].v_int;
  value2->data[0].v_int = 0.5 + tmp;
}

static void
value_exch_double_uint (GValue *value1,
			GValue *value2)
{
  gdouble tmp = value1->data[0].v_double;
  value1->data[0].v_double = value2->data[0].v_uint;
  value2->data[0].v_uint = 0.5 + tmp;
}

static void
value_exch_double_long (GValue *value1,
			GValue *value2)
{
  gdouble tmp = value1->data[0].v_double;
  value1->data[0].v_double = value2->data[0].v_long;
  value2->data[0].v_long = 0.5 + tmp;
}

static void
value_exch_double_ulong (GValue *value1,
			 GValue *value2)
{
  gdouble tmp = value1->data[0].v_double;
  value1->data[0].v_double = value2->data[0].v_ulong;
  value2->data[0].v_ulong = 0.5 + tmp;
}

static void
value_exch_double_float (GValue *value1,
			 GValue *value2)
{
  gdouble tmp = value1->data[0].v_double;
  value1->data[0].v_double = value2->data[0].v_float;
  value2->data[0].v_float = tmp;
}


/* --- type initialization --- */
typedef struct {
  void		(*finalize)		(GParamSpec   *pspec);
  void		(*param_init)		(GValue	      *value,
					 GParamSpec   *pspec);
  void		(*param_free_value)	(GValue	      *value);
  gboolean	(*param_validate)	(GValue	      *value,
					 GParamSpec   *pspec);
  gint		(*param_values_cmp)	(const GValue *value1,
					 const GValue *value2,
					 GParamSpec   *pspec);
  void		(*param_copy_value)	(const GValue *src_value,
					 GValue	      *dest_value);
  guint		  collect_type;
  gchar*	(*param_collect_value)	(GValue	      *value,
					 GParamSpec   *pspec,
					 guint	       nth_value,
					 GType	      *collect_type,
					 GParamCValue *collect_value);
  guint		  lcopy_type;
  gchar*	(*param_lcopy_value)	(const GValue *value,
					 GParamSpec   *pspec,
					 guint	       nth_value,
					 GType	      *collect_type,
					 GParamCValue *collect_value);
} ParamSpecClassInfo;

static void
param_spec_class_init (gpointer g_class,
		       gpointer class_data)
{
  GParamSpecClass *class = g_class;
  ParamSpecClassInfo *info = class_data;
  
  if (info->finalize)
    class->finalize = info->finalize;
  if (info->param_init)
    class->param_init = info->param_init;
  if (info->param_free_value)
    class->param_free_value = info->param_free_value;
  if (info->param_validate)
    class->param_validate = info->param_validate;
  if (info->param_values_cmp)
    class->param_values_cmp = info->param_values_cmp;
  if (info->param_copy_value)
    class->param_copy_value = info->param_copy_value;
  class->collect_type = info->collect_type;
  class->param_collect_value = info->param_collect_value;
  class->lcopy_type = info->lcopy_type;
  class->param_lcopy_value = info->param_lcopy_value;
}

void
g_param_spec_types_init (void)	/* sync with glib-gparam.c */
{
  GTypeInfo info = {
    sizeof (GParamSpecClass),	/* class_size */
    NULL,			/* base_init */
    NULL,			/* base_destroy */
    param_spec_class_init,	/* class_init */
    NULL,			/* class_destroy */
    NULL,			/* class_data */
    0,				/* instance_size */
    16,				/* n_preallocs */
    NULL,			/* instance_init */
  };
  GType type;
  
  /* G_TYPE_PARAM_CHAR
   */
  {
    static const ParamSpecClassInfo class_info = {
      NULL,			/* finalize */
      param_char_init,		/* param_init */
      NULL,			/* param_free_value */
      param_char_validate,	/* param_validate */
      param_int_values_cmp,	/* param_values_cmp */
      NULL,			/* param_copy_value */
      G_VALUE_COLLECT_INT,	/* collect_type */
      param_int_collect_value,	/* param_collect_value */
      G_VALUE_COLLECT_POINTER,	/* lcopy_type */
      param_char_lcopy_value,	/* param_lcopy_value */
    };
    info.class_data = &class_info;
    info.instance_size = sizeof (GParamSpecChar);
    info.instance_init = (GInstanceInitFunc) param_spec_char_init;
    type = g_type_register_static (G_TYPE_PARAM, "GParamChar", &info);
    g_assert (type == G_TYPE_PARAM_CHAR);
  }
  
  /* G_TYPE_PARAM_UCHAR
   */
  {
    static const ParamSpecClassInfo class_info = {
      NULL,			/* finalize */
      param_uchar_init,		/* param_init */
      NULL,			/* param_free_value */
      param_uchar_validate,	/* param_validate */
      param_uint_values_cmp,	/* param_values_cmp */
      NULL,			/* param_copy_value */
      G_VALUE_COLLECT_INT,	/* collect_type */
      param_int_collect_value,	/* param_collect_value */
      G_VALUE_COLLECT_POINTER,	/* lcopy_type */
      param_char_lcopy_value,	/* param_lcopy_value */
    };
    info.class_data = &class_info;
    info.instance_size = sizeof (GParamSpecUChar);
    info.instance_init = (GInstanceInitFunc) param_spec_uchar_init;
    type = g_type_register_static (G_TYPE_PARAM, "GParamUChar", &info);
    g_assert (type == G_TYPE_PARAM_UCHAR);
  }
  
  /* G_TYPE_PARAM_BOOL
   */
  {
    static const ParamSpecClassInfo class_info = {
      NULL,			/* finalize */
      param_bool_init,		/* param_init */
      NULL,			/* param_free_value */
      param_bool_validate,	/* param_validate */
      param_int_values_cmp,	/* param_values_cmp */
      NULL,			/* param_copy_value */
      G_VALUE_COLLECT_INT,	/* collect_type */
      param_int_collect_value,	/* param_collect_value */
      G_VALUE_COLLECT_POINTER,	/* lcopy_type */
      param_bool_lcopy_value,	/* param_lcopy_value */
    };
    info.class_data = &class_info;
    info.instance_size = sizeof (GParamSpecBool);
    info.instance_init = (GInstanceInitFunc) NULL;
    type = g_type_register_static (G_TYPE_PARAM, "GParamBool", &info);
    g_assert (type == G_TYPE_PARAM_BOOL);
  }
  
  /* G_TYPE_PARAM_INT
   */
  {
    static const ParamSpecClassInfo class_info = {
      NULL,			/* finalize */
      param_int_init,		/* param_init */
      NULL,			/* param_free_value */
      param_int_validate,	/* param_validate */
      param_int_values_cmp,	/* param_values_cmp */
      NULL,			/* param_copy_value */
      G_VALUE_COLLECT_INT,	/* collect_type */
      param_int_collect_value,	/* param_collect_value */
      G_VALUE_COLLECT_POINTER,	/* lcopy_type */
      param_int_lcopy_value,	/* param_lcopy_value */
    };
    info.class_data = &class_info;
    info.instance_size = sizeof (GParamSpecInt);
    info.instance_init = (GInstanceInitFunc) param_spec_int_init;
    type = g_type_register_static (G_TYPE_PARAM, "GParamInt", &info);
    g_assert (type == G_TYPE_PARAM_INT);
  }
  
  /* G_TYPE_PARAM_UINT
   */
  {
    static const ParamSpecClassInfo class_info = {
      NULL,			/* finalize */
      param_uint_init,		/* param_init */
      NULL,			/* param_free_value */
      param_uint_validate,	/* param_validate */
      param_uint_values_cmp,	/* param_values_cmp */
      NULL,			/* param_copy_value */
      G_VALUE_COLLECT_INT,	/* collect_type */
      param_int_collect_value,	/* param_collect_value */
      G_VALUE_COLLECT_POINTER,	/* lcopy_type */
      param_int_lcopy_value,	/* param_lcopy_value */
    };
    info.class_data = &class_info;
    info.instance_size = sizeof (GParamSpecUInt);
    info.instance_init = (GInstanceInitFunc) param_spec_uint_init;
    type = g_type_register_static (G_TYPE_PARAM, "GParamUInt", &info);
    g_assert (type == G_TYPE_PARAM_UINT);
  }
  
  /* G_TYPE_PARAM_LONG
   */
  {
    static const ParamSpecClassInfo class_info = {
      NULL,			/* finalize */
      param_long_init,		/* param_init */
      NULL,			/* param_free_value */
      param_long_validate,	/* param_validate */
      param_long_values_cmp,	/* param_values_cmp */
      NULL,			/* param_copy_value */
      G_VALUE_COLLECT_LONG,	/* collect_type */
      param_long_collect_value,	/* param_collect_value */
      G_VALUE_COLLECT_POINTER,	/* lcopy_type */
      param_long_lcopy_value,	/* param_lcopy_value */
    };
    info.class_data = &class_info;
    info.instance_size = sizeof (GParamSpecLong);
    info.instance_init = (GInstanceInitFunc) param_spec_long_init;
    type = g_type_register_static (G_TYPE_PARAM, "GParamLong", &info);
    g_assert (type == G_TYPE_PARAM_LONG);
  }
  
  /* G_TYPE_PARAM_ULONG
   */
  {
    static const ParamSpecClassInfo class_info = {
      NULL,			/* finalize */
      param_ulong_init,		/* param_init */
      NULL,			/* param_free_value */
      param_ulong_validate,	/* param_validate */
      param_ulong_values_cmp,	/* param_values_cmp */
      NULL,			/* param_copy_value */
      G_VALUE_COLLECT_LONG,	/* collect_type */
      param_long_collect_value,	/* param_collect_value */
      G_VALUE_COLLECT_POINTER,	/* lcopy_type */
      param_long_lcopy_value,	/* param_lcopy_value */
    };
    info.class_data = &class_info;
    info.instance_size = sizeof (GParamSpecULong);
    info.instance_init = (GInstanceInitFunc) param_spec_ulong_init;
    type = g_type_register_static (G_TYPE_PARAM, "GParamULong", &info);
    g_assert (type == G_TYPE_PARAM_ULONG);
  }
  
  /* G_TYPE_PARAM_ENUM
   */
  {
    static const ParamSpecClassInfo class_info = {
      param_spec_enum_finalize,	/* finalize */
      param_enum_init,		/* param_init */
      NULL,			/* param_free_value */
      param_enum_validate,	/* param_validate */
      param_long_values_cmp,	/* param_values_cmp */
      NULL,			/* param_copy_value */
      G_VALUE_COLLECT_INT,	/* collect_type */
      param_enum_collect_value,	/* param_collect_value */
      G_VALUE_COLLECT_POINTER,	/* lcopy_type */
      param_enum_lcopy_value,	/* param_lcopy_value */
    };
    info.class_data = &class_info;
    info.instance_size = sizeof (GParamSpecEnum);
    info.instance_init = (GInstanceInitFunc) param_spec_enum_init;
    type = g_type_register_static (G_TYPE_PARAM, "GParamEnum", &info);
    g_assert (type == G_TYPE_PARAM_ENUM);
  }
  
  /* G_TYPE_PARAM_FLAGS
   */
  {
    static const ParamSpecClassInfo class_info = {
      param_spec_flags_finalize,/* finalize */
      param_flags_init,		/* param_init */
      NULL,			/* param_free_value */
      param_flags_validate,	/* param_validate */
      param_ulong_values_cmp,	/* param_values_cmp */
      NULL,			/* param_copy_value */
      G_VALUE_COLLECT_INT,	/* collect_type */
      param_enum_collect_value,	/* param_collect_value */
      G_VALUE_COLLECT_POINTER,	/* lcopy_type */
      param_enum_lcopy_value,	/* param_lcopy_value */
    };
    info.class_data = &class_info;
    info.instance_size = sizeof (GParamSpecFlags);
    info.instance_init = (GInstanceInitFunc) param_spec_flags_init;
    type = g_type_register_static (G_TYPE_PARAM, "GParamFlags", &info);
    g_assert (type == G_TYPE_PARAM_FLAGS);
  }
  
  /* G_TYPE_PARAM_FLOAT
   */
  {
    static const ParamSpecClassInfo class_info = {
      NULL,			/* finalize */
      param_float_init,		/* param_init */
      NULL,			/* param_free_value */
      param_float_validate,	/* param_validate */
      param_float_values_cmp,	/* param_values_cmp */
      NULL,			/* param_copy_value */
      G_VALUE_COLLECT_DOUBLE,	/* collect_type */
      param_float_collect_value,/* param_collect_value */
      G_VALUE_COLLECT_POINTER,	/* lcopy_type */
      param_float_lcopy_value,	/* param_lcopy_value */
    };
    info.class_data = &class_info;
    info.instance_size = sizeof (GParamSpecFloat);
    info.instance_init = (GInstanceInitFunc) param_spec_float_init;
    type = g_type_register_static (G_TYPE_PARAM, "GParamFloat", &info);
    g_assert (type == G_TYPE_PARAM_FLOAT);
  }
  
  /* G_TYPE_PARAM_DOUBLE
   */
  {
    static const ParamSpecClassInfo class_info = {
      NULL,			  /* finalize */
      param_double_init,	  /* param_init */
      NULL,			  /* param_free_value */
      param_double_validate,	  /* param_validate */
      param_double_values_cmp,	  /* param_values_cmp */
      NULL,			  /* param_copy_value */
      G_VALUE_COLLECT_DOUBLE,	  /* collect_type */
      param_double_collect_value, /* param_collect_value */
      G_VALUE_COLLECT_POINTER,	  /* lcopy_type */
      param_double_lcopy_value,	  /* param_lcopy_value */
    };
    info.class_data = &class_info;
    info.instance_size = sizeof (GParamSpecDouble);
    info.instance_init = (GInstanceInitFunc) param_spec_double_init;
    type = g_type_register_static (G_TYPE_PARAM, "GParamDouble", &info);
    g_assert (type == G_TYPE_PARAM_DOUBLE);
  }
  
  /* G_TYPE_PARAM_STRING
   */
  {
    static const ParamSpecClassInfo class_info = {
      param_spec_string_finalize, /* finalize */
      param_string_init,	  /* param_init */
      param_string_free_value,	  /* param_free_value */
      param_string_validate,	  /* param_validate */
      param_string_values_cmp,	  /* param_values_cmp */
      param_string_copy_value,	  /* param_copy_value */
      G_VALUE_COLLECT_POINTER,	  /* collect_type */
      param_string_collect_value, /* param_collect_value */
      G_VALUE_COLLECT_POINTER,	  /* lcopy_type */
      param_string_lcopy_value,	  /* param_lcopy_value */
    };
    info.class_data = &class_info;
    info.instance_size = sizeof (GParamSpecString);
    info.instance_init = (GInstanceInitFunc) param_spec_string_init;
    type = g_type_register_static (G_TYPE_PARAM, "GParamString", &info);
    g_assert (type == G_TYPE_PARAM_STRING);
  }
  
  /* G_TYPE_PARAM_OBJECT
   */
  {
    static const ParamSpecClassInfo class_info = {
      NULL,			  /* finalize */
      param_object_init,	  /* param_init */
      param_object_free_value,	  /* param_free_value */
      param_object_validate,	  /* param_validate */
      param_object_values_cmp,	  /* param_values_cmp */
      param_object_copy_value,	  /* param_copy_value */
      G_VALUE_COLLECT_POINTER,	  /* collect_type */
      param_object_collect_value, /* param_collect_value */
      G_VALUE_COLLECT_POINTER,	  /* lcopy_type */
      param_object_lcopy_value,	  /* param_lcopy_value */
    };
    info.class_data = &class_info;
    info.instance_size = sizeof (GParamSpecObject);
    info.instance_init = (GInstanceInitFunc) param_spec_object_init;
    type = g_type_register_static (G_TYPE_PARAM, "GParamObject", &info);
    g_assert (type == G_TYPE_PARAM_OBJECT);
  }
  
  g_value_register_exchange_func (G_TYPE_PARAM_CHAR,   G_TYPE_PARAM_UCHAR,  value_exch_memcpy);
  g_value_register_exchange_func (G_TYPE_PARAM_CHAR,   G_TYPE_PARAM_BOOL,   value_exch_memcpy);
  g_value_register_exchange_func (G_TYPE_PARAM_CHAR,   G_TYPE_PARAM_INT,    value_exch_memcpy);
  g_value_register_exchange_func (G_TYPE_PARAM_CHAR,   G_TYPE_PARAM_UINT,   value_exch_memcpy);
  g_value_register_exchange_func (G_TYPE_PARAM_CHAR,   G_TYPE_PARAM_ENUM,   value_exch_memcpy);
  g_value_register_exchange_func (G_TYPE_PARAM_CHAR,   G_TYPE_PARAM_FLAGS,  value_exch_memcpy); 
  g_value_register_exchange_func (G_TYPE_PARAM_UCHAR,  G_TYPE_PARAM_BOOL,   value_exch_memcpy);
  g_value_register_exchange_func (G_TYPE_PARAM_UCHAR,  G_TYPE_PARAM_INT,    value_exch_memcpy);
  g_value_register_exchange_func (G_TYPE_PARAM_UCHAR,  G_TYPE_PARAM_UINT,   value_exch_memcpy);
  g_value_register_exchange_func (G_TYPE_PARAM_UCHAR,  G_TYPE_PARAM_ENUM,   value_exch_memcpy);
  g_value_register_exchange_func (G_TYPE_PARAM_UCHAR,  G_TYPE_PARAM_FLAGS,  value_exch_memcpy); 
  g_value_register_exchange_func (G_TYPE_PARAM_BOOL,   G_TYPE_PARAM_INT,    value_exch_memcpy);
  g_value_register_exchange_func (G_TYPE_PARAM_BOOL,   G_TYPE_PARAM_UINT,   value_exch_memcpy);
  g_value_register_exchange_func (G_TYPE_PARAM_BOOL,   G_TYPE_PARAM_ENUM,   value_exch_memcpy);
  g_value_register_exchange_func (G_TYPE_PARAM_BOOL,   G_TYPE_PARAM_FLAGS,  value_exch_memcpy); 
  g_value_register_exchange_func (G_TYPE_PARAM_INT,    G_TYPE_PARAM_UINT,   value_exch_memcpy); 
  g_value_register_exchange_func (G_TYPE_PARAM_INT,    G_TYPE_PARAM_ENUM,   value_exch_memcpy); 
  g_value_register_exchange_func (G_TYPE_PARAM_INT,    G_TYPE_PARAM_FLAGS,  value_exch_memcpy); 
  g_value_register_exchange_func (G_TYPE_PARAM_UINT,   G_TYPE_PARAM_ENUM,   value_exch_memcpy); 
  g_value_register_exchange_func (G_TYPE_PARAM_UINT,   G_TYPE_PARAM_FLAGS,  value_exch_memcpy); 
  g_value_register_exchange_func (G_TYPE_PARAM_LONG,   G_TYPE_PARAM_CHAR,   value_exch_long_int); 
  g_value_register_exchange_func (G_TYPE_PARAM_LONG,   G_TYPE_PARAM_UCHAR,  value_exch_long_uint); 
  g_value_register_exchange_func (G_TYPE_PARAM_LONG,   G_TYPE_PARAM_BOOL,   value_exch_long_int); 
  g_value_register_exchange_func (G_TYPE_PARAM_LONG,   G_TYPE_PARAM_INT,    value_exch_long_int); 
  g_value_register_exchange_func (G_TYPE_PARAM_LONG,   G_TYPE_PARAM_UINT,   value_exch_long_uint); 
  g_value_register_exchange_func (G_TYPE_PARAM_LONG,   G_TYPE_PARAM_ULONG,  value_exch_memcpy); 
  g_value_register_exchange_func (G_TYPE_PARAM_LONG,   G_TYPE_PARAM_ENUM,   value_exch_long_int); 
  g_value_register_exchange_func (G_TYPE_PARAM_LONG,   G_TYPE_PARAM_FLAGS,  value_exch_long_uint); 
  g_value_register_exchange_func (G_TYPE_PARAM_ULONG,  G_TYPE_PARAM_CHAR,   value_exch_ulong_int); 
  g_value_register_exchange_func (G_TYPE_PARAM_ULONG,  G_TYPE_PARAM_UCHAR,  value_exch_ulong_uint); 
  g_value_register_exchange_func (G_TYPE_PARAM_ULONG,  G_TYPE_PARAM_BOOL,   value_exch_ulong_int); 
  g_value_register_exchange_func (G_TYPE_PARAM_ULONG,  G_TYPE_PARAM_INT,    value_exch_ulong_int); 
  g_value_register_exchange_func (G_TYPE_PARAM_ULONG,  G_TYPE_PARAM_UINT,   value_exch_ulong_uint); 
  g_value_register_exchange_func (G_TYPE_PARAM_ULONG,  G_TYPE_PARAM_ENUM,   value_exch_ulong_int); 
  g_value_register_exchange_func (G_TYPE_PARAM_ULONG,  G_TYPE_PARAM_FLAGS,  value_exch_ulong_uint); 
  g_value_register_exchange_func (G_TYPE_PARAM_ENUM,   G_TYPE_PARAM_FLAGS,  value_exch_memcpy); 
  g_value_register_exchange_func (G_TYPE_PARAM_FLOAT,  G_TYPE_PARAM_CHAR,   value_exch_float_int); 
  g_value_register_exchange_func (G_TYPE_PARAM_FLOAT,  G_TYPE_PARAM_UCHAR,  value_exch_float_uint); 
  g_value_register_exchange_func (G_TYPE_PARAM_FLOAT,  G_TYPE_PARAM_BOOL,   value_exch_float_int); 
  g_value_register_exchange_func (G_TYPE_PARAM_FLOAT,  G_TYPE_PARAM_INT,    value_exch_float_int); 
  g_value_register_exchange_func (G_TYPE_PARAM_FLOAT,  G_TYPE_PARAM_UINT,   value_exch_float_uint); 
  g_value_register_exchange_func (G_TYPE_PARAM_FLOAT,  G_TYPE_PARAM_LONG,   value_exch_float_long); 
  g_value_register_exchange_func (G_TYPE_PARAM_FLOAT,  G_TYPE_PARAM_ULONG,  value_exch_float_ulong);
  g_value_register_exchange_func (G_TYPE_PARAM_FLOAT,  G_TYPE_PARAM_ENUM,   value_exch_float_int); 
  g_value_register_exchange_func (G_TYPE_PARAM_FLOAT,  G_TYPE_PARAM_FLAGS,  value_exch_float_uint); 
  g_value_register_exchange_func (G_TYPE_PARAM_DOUBLE, G_TYPE_PARAM_CHAR,   value_exch_double_int); 
  g_value_register_exchange_func (G_TYPE_PARAM_DOUBLE, G_TYPE_PARAM_UCHAR,  value_exch_double_uint); 
  g_value_register_exchange_func (G_TYPE_PARAM_DOUBLE, G_TYPE_PARAM_BOOL,   value_exch_double_int); 
  g_value_register_exchange_func (G_TYPE_PARAM_DOUBLE, G_TYPE_PARAM_INT,    value_exch_double_int); 
  g_value_register_exchange_func (G_TYPE_PARAM_DOUBLE, G_TYPE_PARAM_UINT,   value_exch_double_uint); 
  g_value_register_exchange_func (G_TYPE_PARAM_DOUBLE, G_TYPE_PARAM_LONG,   value_exch_double_long);
  g_value_register_exchange_func (G_TYPE_PARAM_DOUBLE, G_TYPE_PARAM_ULONG,  value_exch_double_ulong);
  g_value_register_exchange_func (G_TYPE_PARAM_DOUBLE, G_TYPE_PARAM_ENUM,   value_exch_double_int); 
  g_value_register_exchange_func (G_TYPE_PARAM_DOUBLE, G_TYPE_PARAM_FLAGS,  value_exch_double_uint);
  g_value_register_exchange_func (G_TYPE_PARAM_DOUBLE, G_TYPE_PARAM_FLOAT,  value_exch_double_float); 
}


/* --- GValue functions --- */
void
g_value_set_char (GValue *value,
		  gint8	  v_char)
{
  g_return_if_fail (G_IS_VALUE_CHAR (value));
  
  value->data[0].v_int = v_char;
}

gint8
g_value_get_char (GValue *value)
{
  g_return_val_if_fail (G_IS_VALUE_CHAR (value), 0);
  
  return value->data[0].v_int;
}

void
g_value_set_uchar (GValue *value,
		   guint8  v_uchar)
{
  g_return_if_fail (G_IS_VALUE_UCHAR (value));
  
  value->data[0].v_uint = v_uchar;
}

guint8
g_value_get_uchar (GValue *value)
{
  g_return_val_if_fail (G_IS_VALUE_UCHAR (value), 0);
  
  return value->data[0].v_uint;
}

void
g_value_set_bool (GValue  *value,
		  gboolean v_bool)
{
  g_return_if_fail (G_IS_VALUE_BOOL (value));
  
  value->data[0].v_int = v_bool;
}

gboolean
g_value_get_bool (GValue *value)
{
  g_return_val_if_fail (G_IS_VALUE_BOOL (value), 0);
  
  return value->data[0].v_int;
}

void
g_value_set_int (GValue *value,
		 gint	 v_int)
{
  g_return_if_fail (G_IS_VALUE_INT (value));
  
  value->data[0].v_int = v_int;
}

gint
g_value_get_int (GValue *value)
{
  g_return_val_if_fail (G_IS_VALUE_INT (value), 0);
  
  return value->data[0].v_int;
}

void
g_value_set_uint (GValue *value,
		  guint	  v_uint)
{
  g_return_if_fail (G_IS_VALUE_UINT (value));
  
  value->data[0].v_uint = v_uint;
}

guint
g_value_get_uint (GValue *value)
{
  g_return_val_if_fail (G_IS_VALUE_UINT (value), 0);
  
  return value->data[0].v_uint;
}

void
g_value_set_long (GValue *value,
		  glong	  v_long)
{
  g_return_if_fail (G_IS_VALUE_LONG (value));
  
  value->data[0].v_long = v_long;
}

glong
g_value_get_long (GValue *value)
{
  g_return_val_if_fail (G_IS_VALUE_LONG (value), 0);
  
  return value->data[0].v_long;
}

void
g_value_set_ulong (GValue *value,
		   gulong  v_ulong)
{
  g_return_if_fail (G_IS_VALUE_ULONG (value));
  
  value->data[0].v_ulong = v_ulong;
}

gulong
g_value_get_ulong (GValue *value)
{
  g_return_val_if_fail (G_IS_VALUE_ULONG (value), 0);
  
  return value->data[0].v_ulong;
}

void
g_value_set_enum (GValue *value,
		  gint	  v_enum)
{
  g_return_if_fail (G_IS_VALUE_ENUM (value));
  
  value->data[0].v_long = v_enum;
}

gint
g_value_get_enum (GValue *value)
{
  g_return_val_if_fail (G_IS_VALUE_ENUM (value), 0);
  
  return value->data[0].v_long;
}

void
g_value_set_flags (GValue *value,
		   guint   v_flags)
{
  g_return_if_fail (G_IS_VALUE_FLAGS (value));
  
  value->data[0].v_ulong = v_flags;
}

guint
g_value_get_flags (GValue *value)
{
  g_return_val_if_fail (G_IS_VALUE_FLAGS (value), 0);
  
  return value->data[0].v_ulong;
}

void
g_value_set_float (GValue *value,
		   gfloat  v_float)
{
  g_return_if_fail (G_IS_VALUE_FLOAT (value));
  
  value->data[0].v_float = v_float;
}

gfloat
g_value_get_float (GValue *value)
{
  g_return_val_if_fail (G_IS_VALUE_FLOAT (value), 0);
  
  return value->data[0].v_float;
}

void
g_value_set_double (GValue *value,
		    gdouble v_double)
{
  g_return_if_fail (G_IS_VALUE_DOUBLE (value));
  
  value->data[0].v_double = v_double;
}

gdouble
g_value_get_double (GValue *value)
{
  g_return_val_if_fail (G_IS_VALUE_DOUBLE (value), 0);
  
  return value->data[0].v_double;
}

void
g_value_set_string (GValue	*value,
		    const gchar *v_string)
{
  g_return_if_fail (G_IS_VALUE_STRING (value));
  
  g_free (value->data[0].v_pointer);
  value->data[0].v_pointer = g_strdup (v_string);
}

gchar*
g_value_get_string (GValue *value)
{
  g_return_val_if_fail (G_IS_VALUE_STRING (value), NULL);
  
  return value->data[0].v_pointer;
}

gchar*
g_value_dup_string (GValue *value)
{
  g_return_val_if_fail (G_IS_VALUE_STRING (value), NULL);
  
  return g_strdup (value->data[0].v_pointer);
}

void
g_value_set_object (GValue  *value,
		    GObject *v_object)
{
  g_return_if_fail (G_IS_VALUE_OBJECT (value));
  if (v_object)
    g_return_if_fail (G_IS_OBJECT (v_object));
  
  if (value->data[0].v_pointer)
    g_object_unref (value->data[0].v_pointer);
  value->data[0].v_pointer = v_object;
  if (value->data[0].v_pointer)
    g_object_ref (value->data[0].v_pointer);
}

GObject*
g_value_get_object (GValue *value)
{
  g_return_val_if_fail (G_IS_VALUE_OBJECT (value), NULL);
  
  return value->data[0].v_pointer;
}

GObject*
g_value_dup_object (GValue *value)
{
  g_return_val_if_fail (G_IS_VALUE_OBJECT (value), NULL);
  
  return value->data[0].v_pointer ? g_object_ref (value->data[0].v_pointer) : NULL;
}


/* --- GParamSpec initialization --- */
GParamSpec*
g_param_spec_char (const gchar *name,
		   const gchar *nick,
		   const gchar *blurb,
		   gint8	minimum,
		   gint8	maximum,
		   gint8	default_value,
		   GParamFlags	flags)
{
  GParamSpecChar *cspec = g_param_spec_internal (G_TYPE_PARAM_CHAR,
						 name,
						 nick,
						 blurb,
						 flags);
  
  cspec->minimum = minimum;
  cspec->maximum = maximum;
  cspec->default_value = default_value;
  
  return G_PARAM_SPEC (cspec);
}

GParamSpec*
g_param_spec_uchar (const gchar *name,
		    const gchar *nick,
		    const gchar *blurb,
		    guint8	 minimum,
		    guint8	 maximum,
		    guint8	 default_value,
		    GParamFlags	 flags)
{
  GParamSpecUChar *uspec = g_param_spec_internal (G_TYPE_PARAM_UCHAR,
						  name,
						  nick,
						  blurb,
						  flags);
  
  uspec->minimum = minimum;
  uspec->maximum = maximum;
  uspec->default_value = default_value;
  
  return G_PARAM_SPEC (uspec);
}

GParamSpec*
g_param_spec_bool (const gchar *name,
		   const gchar *nick,
		   const gchar *blurb,
		   gboolean	default_value,
		   GParamFlags	flags)
{
  GParamSpecBool *bspec = g_param_spec_internal (G_TYPE_PARAM_BOOL,
						 name,
						 nick,
						 blurb,
						 flags);
  
  bspec->default_value = default_value;
  
  return G_PARAM_SPEC (bspec);
}

GParamSpec*
g_param_spec_int (const gchar *name,
		  const gchar *nick,
		  const gchar *blurb,
		  gint	       minimum,
		  gint	       maximum,
		  gint	       default_value,
		  GParamFlags  flags)
{
  GParamSpecInt *ispec = g_param_spec_internal (G_TYPE_PARAM_INT,
						name,
						nick,
						blurb,
						flags);
  
  ispec->minimum = minimum;
  ispec->maximum = maximum;
  ispec->default_value = default_value;
  
  return G_PARAM_SPEC (ispec);
}

GParamSpec*
g_param_spec_uint (const gchar *name,
		   const gchar *nick,
		   const gchar *blurb,
		   guint	minimum,
		   guint	maximum,
		   guint	default_value,
		   GParamFlags	flags)
{
  GParamSpecUInt *uspec = g_param_spec_internal (G_TYPE_PARAM_UINT,
						 name,
						 nick,
						 blurb,
						 flags);
  
  uspec->minimum = minimum;
  uspec->maximum = maximum;
  uspec->default_value = default_value;
  
  return G_PARAM_SPEC (uspec);
}

GParamSpec*
g_param_spec_long (const gchar *name,
		   const gchar *nick,
		   const gchar *blurb,
		   glong	minimum,
		   glong	maximum,
		   glong	default_value,
		   GParamFlags	flags)
{
  GParamSpecLong *lspec = g_param_spec_internal (G_TYPE_PARAM_LONG,
						 name,
						 nick,
						 blurb,
						 flags);
  
  lspec->minimum = minimum;
  lspec->maximum = maximum;
  lspec->default_value = default_value;
  
  return G_PARAM_SPEC (lspec);
}

GParamSpec*
g_param_spec_ulong (const gchar *name,
		    const gchar *nick,
		    const gchar *blurb,
		    gulong	 minimum,
		    gulong	 maximum,
		    gulong	 default_value,
		    GParamFlags	 flags)
{
  GParamSpecULong *uspec = g_param_spec_internal (G_TYPE_PARAM_ULONG,
						  name,
						  nick,
						  blurb,
						  flags);
  
  uspec->minimum = minimum;
  uspec->maximum = maximum;
  uspec->default_value = default_value;
  
  return G_PARAM_SPEC (uspec);
}

GParamSpec*
g_param_spec_enum (const gchar *name,
		   const gchar *nick,
		   const gchar *blurb,
		   GType	enum_type,
		   gint		default_value,
		   GParamFlags	flags)
{
  GParamSpecEnum *espec;
  
  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);
  
  espec = g_param_spec_internal (G_TYPE_PARAM_ENUM,
				 name,
				 nick,
				 blurb,
				 flags);
  
  espec->enum_class = g_type_class_ref (enum_type);
  espec->default_value = default_value;
  
  return G_PARAM_SPEC (espec);
}

GParamSpec*
g_param_spec_flags (const gchar *name,
		    const gchar *nick,
		    const gchar *blurb,
		    GType	 flags_type,
		    guint	 default_value,
		    GParamFlags	 flags)
{
  GParamSpecFlags *fspec;
  
  g_return_val_if_fail (G_TYPE_IS_FLAGS (flags_type), NULL);
  
  fspec = g_param_spec_internal (G_TYPE_PARAM_FLAGS,
				 name,
				 nick,
				 blurb,
				 flags);
  
  fspec->flags_class = g_type_class_ref (flags_type);
  fspec->default_value = default_value;
  
  return G_PARAM_SPEC (fspec);
}

GParamSpec*
g_param_spec_float (const gchar *name,
		    const gchar *nick,
		    const gchar *blurb,
		    gfloat	 minimum,
		    gfloat	 maximum,
		    gfloat	 default_value,
		    GParamFlags	 flags)
{
  GParamSpecFloat *fspec = g_param_spec_internal (G_TYPE_PARAM_FLOAT,
						  name,
						  nick,
						  blurb,
						  flags);
  
  fspec->minimum = minimum;
  fspec->maximum = maximum;
  fspec->default_value = default_value;
  
  return G_PARAM_SPEC (fspec);
}

GParamSpec*
g_param_spec_double (const gchar *name,
		     const gchar *nick,
		     const gchar *blurb,
		     gdouble	  minimum,
		     gdouble	  maximum,
		     gdouble	  default_value,
		     GParamFlags  flags)
{
  GParamSpecDouble *dspec = g_param_spec_internal (G_TYPE_PARAM_DOUBLE,
						   name,
						   nick,
						   blurb,
						   flags);
  
  dspec->minimum = minimum;
  dspec->maximum = maximum;
  dspec->default_value = default_value;
  
  return G_PARAM_SPEC (dspec);
}

GParamSpec*
g_param_spec_string (const gchar *name,
		     const gchar *nick,
		     const gchar *blurb,
		     const gchar *default_value,
		     GParamFlags  flags)
{
  GParamSpecString *sspec = g_param_spec_internal (G_TYPE_PARAM_STRING,
						   name,
						   nick,
						   blurb,
						   flags);
  g_free (sspec->default_value);
  sspec->default_value = g_strdup (default_value);
  
  return G_PARAM_SPEC (sspec);
}

GParamSpec*
g_param_spec_string_c (const gchar *name,
		       const gchar *nick,
		       const gchar *blurb,
		       const gchar *default_value,
		       GParamFlags  flags)
{
  GParamSpecString *sspec = g_param_spec_internal (G_TYPE_PARAM_STRING,
						   name,
						   nick,
						   blurb,
						   flags);
  g_free (sspec->default_value);
  sspec->default_value = g_strdup (default_value);
  g_free (sspec->cset_first);
  sspec->cset_first = g_strdup (G_CSET_a_2_z "_" G_CSET_A_2_Z);
  g_free (sspec->cset_nth);
  sspec->cset_nth = g_strdup (G_CSET_a_2_z
			      "_0123456789"
			      /* G_CSET_LATINS G_CSET_LATINC */
			      G_CSET_A_2_Z);
  
  return G_PARAM_SPEC (sspec);
}

GParamSpec*
g_param_spec_object (const gchar *name,
		     const gchar *nick,
		     const gchar *blurb,
		     GType	  object_type,
		     GParamFlags  flags)
{
  GParamSpecObject *ospec;
  
  g_return_val_if_fail (G_TYPE_IS_OBJECT (object_type), NULL);
  
  ospec = g_param_spec_internal (G_TYPE_PARAM_OBJECT,
				 name,
				 nick,
				 blurb,
				 flags);
  ospec->object_type = object_type;
  
  return G_PARAM_SPEC (ospec);
}
