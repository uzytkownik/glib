/* GObject - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 1997-1999, 2000-2001 Tim Janik and Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * gparamspecs.h: GLib default param specs
 */
#ifndef __G_PARAMSPECS_H__
#define __G_PARAMSPECS_H__

#if !defined (__GLIB_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#include        <gobject/gvalue.h>
#include        <gobject/genums.h>
#include        <gobject/gboxed.h>
#include        <gobject/gobject.h>

G_BEGIN_DECLS

/* --- type macros --- */
#define G_IS_PARAM_SPEC_CHAR(pspec)        (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), G_TYPE_PARAM_CHAR))
#define G_PARAM_SPEC_CHAR(pspec)           (G_TYPE_CHECK_INSTANCE_CAST ((pspec), G_TYPE_PARAM_CHAR, GParamSpecChar))
#define G_IS_PARAM_SPEC_UCHAR(pspec)       (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), G_TYPE_PARAM_UCHAR))
#define G_PARAM_SPEC_UCHAR(pspec)          (G_TYPE_CHECK_INSTANCE_CAST ((pspec), G_TYPE_PARAM_UCHAR, GParamSpecUChar))
#define G_IS_PARAM_SPEC_BOOLEAN(pspec)     (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), G_TYPE_PARAM_BOOLEAN))
#define G_PARAM_SPEC_BOOLEAN(pspec)        (G_TYPE_CHECK_INSTANCE_CAST ((pspec), G_TYPE_PARAM_BOOLEAN, GParamSpecBoolean))
#define G_IS_PARAM_SPEC_INT(pspec)         (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), G_TYPE_PARAM_INT))
#define G_PARAM_SPEC_INT(pspec)            (G_TYPE_CHECK_INSTANCE_CAST ((pspec), G_TYPE_PARAM_INT, GParamSpecInt))
#define G_IS_PARAM_SPEC_UINT(pspec)        (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), G_TYPE_PARAM_UINT))
#define G_PARAM_SPEC_UINT(pspec)           (G_TYPE_CHECK_INSTANCE_CAST ((pspec), G_TYPE_PARAM_UINT, GParamSpecUInt))
#define G_IS_PARAM_SPEC_LONG(pspec)        (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), G_TYPE_PARAM_LONG))
#define G_PARAM_SPEC_LONG(pspec)           (G_TYPE_CHECK_INSTANCE_CAST ((pspec), G_TYPE_PARAM_LONG, GParamSpecLong))
#define G_IS_PARAM_SPEC_ULONG(pspec)       (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), G_TYPE_PARAM_ULONG))
#define G_PARAM_SPEC_ULONG(pspec)          (G_TYPE_CHECK_INSTANCE_CAST ((pspec), G_TYPE_PARAM_ULONG, GParamSpecULong))
#define G_IS_PARAM_SPEC_INT64(pspec)        (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), G_TYPE_PARAM_INT64))
#define G_PARAM_SPEC_INT64(pspec)           (G_TYPE_CHECK_INSTANCE_CAST ((pspec), G_TYPE_PARAM_INT64, GParamSpecInt64))
#define G_IS_PARAM_SPEC_UINT64(pspec)       (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), G_TYPE_PARAM_UINT64))
#define G_PARAM_SPEC_UINT64(pspec)          (G_TYPE_CHECK_INSTANCE_CAST ((pspec), G_TYPE_PARAM_UINT64, GParamSpecUInt64))
#define G_PARAM_SPEC_UNICHAR(pspec)        (G_TYPE_CHECK_INSTANCE_CAST ((pspec), G_TYPE_PARAM_UNICHAR, GParamSpecUnichar))
#define G_IS_PARAM_SPEC_UNICHAR(pspec)     (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), G_TYPE_PARAM_UNICHAR))
#define G_IS_PARAM_SPEC_ENUM(pspec)        (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), G_TYPE_PARAM_ENUM))
#define G_PARAM_SPEC_ENUM(pspec)           (G_TYPE_CHECK_INSTANCE_CAST ((pspec), G_TYPE_PARAM_ENUM, GParamSpecEnum))
#define G_IS_PARAM_SPEC_FLAGS(pspec)       (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), G_TYPE_PARAM_FLAGS))
#define G_PARAM_SPEC_FLAGS(pspec)          (G_TYPE_CHECK_INSTANCE_CAST ((pspec), G_TYPE_PARAM_FLAGS, GParamSpecFlags))
#define G_IS_PARAM_SPEC_FLOAT(pspec)       (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), G_TYPE_PARAM_FLOAT))
#define G_PARAM_SPEC_FLOAT(pspec)          (G_TYPE_CHECK_INSTANCE_CAST ((pspec), G_TYPE_PARAM_FLOAT, GParamSpecFloat))
#define G_IS_PARAM_SPEC_DOUBLE(pspec)      (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), G_TYPE_PARAM_DOUBLE))
#define G_PARAM_SPEC_DOUBLE(pspec)         (G_TYPE_CHECK_INSTANCE_CAST ((pspec), G_TYPE_PARAM_DOUBLE, GParamSpecDouble))
#define G_IS_PARAM_SPEC_STRING(pspec)      (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), G_TYPE_PARAM_STRING))
#define G_PARAM_SPEC_STRING(pspec)         (G_TYPE_CHECK_INSTANCE_CAST ((pspec), G_TYPE_PARAM_STRING, GParamSpecString))
#define G_IS_PARAM_SPEC_PARAM(pspec)       (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), G_TYPE_PARAM_PARAM))
#define G_PARAM_SPEC_PARAM(pspec)          (G_TYPE_CHECK_INSTANCE_CAST ((pspec), G_TYPE_PARAM_PARAM, GParamSpecParam))
#define G_IS_PARAM_SPEC_BOXED(pspec)       (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), G_TYPE_PARAM_BOXED))
#define G_PARAM_SPEC_BOXED(pspec)          (G_TYPE_CHECK_INSTANCE_CAST ((pspec), G_TYPE_PARAM_BOXED, GParamSpecBoxed))
#define G_IS_PARAM_SPEC_POINTER(pspec)     (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), G_TYPE_PARAM_POINTER))
#define G_PARAM_SPEC_POINTER(pspec)        (G_TYPE_CHECK_INSTANCE_CAST ((pspec), G_TYPE_PARAM_POINTER, GParamSpecPointer))
#define G_IS_PARAM_SPEC_VALUE_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), G_TYPE_PARAM_VALUE_ARRAY))
#define G_PARAM_SPEC_VALUE_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), G_TYPE_PARAM_VALUE_ARRAY, GParamSpecValueArray))
#define G_IS_PARAM_SPEC_OBJECT(pspec)      (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), G_TYPE_PARAM_OBJECT))
#define G_PARAM_SPEC_OBJECT(pspec)         (G_TYPE_CHECK_INSTANCE_CAST ((pspec), G_TYPE_PARAM_OBJECT, GParamSpecObject))


/* --- typedefs & structures --- */
typedef struct _GParamSpecChar       GParamSpecChar;
typedef struct _GParamSpecUChar      GParamSpecUChar;
typedef struct _GParamSpecBoolean    GParamSpecBoolean;
typedef struct _GParamSpecInt        GParamSpecInt;
typedef struct _GParamSpecUInt       GParamSpecUInt;
typedef struct _GParamSpecLong       GParamSpecLong;
typedef struct _GParamSpecULong      GParamSpecULong;
typedef struct _GParamSpecInt64      GParamSpecInt64;
typedef struct _GParamSpecUInt64     GParamSpecUInt64;
typedef struct _GParamSpecUnichar    GParamSpecUnichar;
typedef struct _GParamSpecEnum       GParamSpecEnum;
typedef struct _GParamSpecFlags      GParamSpecFlags;
typedef struct _GParamSpecFloat      GParamSpecFloat;
typedef struct _GParamSpecDouble     GParamSpecDouble;
typedef struct _GParamSpecString     GParamSpecString;
typedef struct _GParamSpecParam      GParamSpecParam;
typedef struct _GParamSpecBoxed      GParamSpecBoxed;
typedef struct _GParamSpecPointer    GParamSpecPointer;
typedef struct _GParamSpecValueArray GParamSpecValueArray;
typedef struct _GParamSpecObject     GParamSpecObject;

struct _GParamSpecChar
{
  GParamSpec    parent_instance;
  
  gint8         minimum;
  gint8         maximum;
  gint8         default_value;
};
struct _GParamSpecUChar
{
  GParamSpec    parent_instance;
  
  guint8        minimum;
  guint8        maximum;
  guint8        default_value;
};
struct _GParamSpecBoolean
{
  GParamSpec    parent_instance;
  
  gboolean      default_value;
};
struct _GParamSpecInt
{
  GParamSpec    parent_instance;
  
  gint          minimum;
  gint          maximum;
  gint          default_value;
};
struct _GParamSpecUInt
{
  GParamSpec    parent_instance;
  
  guint         minimum;
  guint         maximum;
  guint         default_value;
};
struct _GParamSpecLong
{
  GParamSpec    parent_instance;
  
  glong         minimum;
  glong         maximum;
  glong         default_value;
};
struct _GParamSpecULong
{
  GParamSpec    parent_instance;
  
  gulong        minimum;
  gulong        maximum;
  gulong        default_value;
};
struct _GParamSpecInt64
{
  GParamSpec    parent_instance;
  
  gint64        minimum;
  gint64        maximum;
  gint64        default_value;
};
struct _GParamSpecUInt64
{
  GParamSpec    parent_instance;
  
  guint64       minimum;
  guint64       maximum;
  guint64       default_value;
};
struct _GParamSpecUnichar
{
  GParamSpec    parent_instance;
  
  gunichar      default_value;
};
struct _GParamSpecEnum
{
  GParamSpec    parent_instance;
  
  GEnumClass   *enum_class;
  gint          default_value;
};
struct _GParamSpecFlags
{
  GParamSpec    parent_instance;
  
  GFlagsClass  *flags_class;
  guint         default_value;
};
struct _GParamSpecFloat
{
  GParamSpec    parent_instance;
  
  gfloat        minimum;
  gfloat        maximum;
  gfloat        default_value;
  gfloat        epsilon;
};
struct _GParamSpecDouble
{
  GParamSpec    parent_instance;
  
  gdouble       minimum;
  gdouble       maximum;
  gdouble       default_value;
  gdouble       epsilon;
};
struct _GParamSpecString
{
  GParamSpec    parent_instance;
  
  gchar        *default_value;
  gchar        *cset_first;
  gchar        *cset_nth;
  gchar         substitutor;
  guint         null_fold_if_empty : 1;
  guint         ensure_non_null : 1;
};
struct _GParamSpecParam
{
  GParamSpec    parent_instance;
};
struct _GParamSpecBoxed
{
  GParamSpec    parent_instance;
};
struct _GParamSpecPointer
{
  GParamSpec    parent_instance;
};
struct _GParamSpecValueArray
{
  GParamSpec    parent_instance;
  GParamSpec   *element_spec;
  guint		fixed_n_elements;
};
struct _GParamSpecObject
{
  GParamSpec    parent_instance;
};

/* --- GParamSpec prototypes --- */
GParamSpec*	g_param_spec_char	 (const gchar	 *name,
					  const gchar	 *nick,
					  const gchar	 *blurb,
					  gint8		  minimum,
					  gint8		  maximum,
					  gint8		  default_value,
					  GParamFlags	  flags);
GParamSpec*	g_param_spec_uchar	 (const gchar	 *name,
					  const gchar	 *nick,
					  const gchar	 *blurb,
					  guint8	  minimum,
					  guint8	  maximum,
					  guint8	  default_value,
					  GParamFlags	  flags);
GParamSpec*	g_param_spec_boolean	 (const gchar	 *name,
					  const gchar	 *nick,
					  const gchar	 *blurb,
					  gboolean	  default_value,
					  GParamFlags	  flags);
GParamSpec*	g_param_spec_int	 (const gchar	 *name,
					  const gchar	 *nick,
					  const gchar	 *blurb,
					  gint		  minimum,
					  gint		  maximum,
					  gint		  default_value,
					  GParamFlags	  flags);
GParamSpec*	g_param_spec_uint	 (const gchar	 *name,
					  const gchar	 *nick,
					  const gchar	 *blurb,
					  guint		  minimum,
					  guint		  maximum,
					  guint		  default_value,
					  GParamFlags	  flags);
GParamSpec*	g_param_spec_long	 (const gchar	 *name,
					  const gchar	 *nick,
					  const gchar	 *blurb,
					  glong		  minimum,
					  glong		  maximum,
					  glong		  default_value,
					  GParamFlags	  flags);
GParamSpec*	g_param_spec_ulong	 (const gchar	 *name,
					  const gchar	 *nick,
					  const gchar	 *blurb,
					  gulong	  minimum,
					  gulong	  maximum,
					  gulong	  default_value,
					  GParamFlags	  flags);
GParamSpec*	g_param_spec_int64	 (const gchar	 *name,
					  const gchar	 *nick,
					  const gchar	 *blurb,
					  gint64       	  minimum,
					  gint64       	  maximum,
					  gint64       	  default_value,
					  GParamFlags	  flags);
GParamSpec*	g_param_spec_uint64	 (const gchar	 *name,
					  const gchar	 *nick,
					  const gchar	 *blurb,
					  guint64	  minimum,
					  guint64	  maximum,
					  guint64	  default_value,
					  GParamFlags	  flags);
GParamSpec*    g_param_spec_unichar      (const gchar    *name,
				          const gchar    *nick,
				          const gchar    *blurb,
				          gunichar	  default_value,
				          GParamFlags     flags);
GParamSpec*	g_param_spec_enum	 (const gchar	 *name,
					  const gchar	 *nick,
					  const gchar	 *blurb,
					  GType		  enum_type,
					  gint		  default_value,
					  GParamFlags	  flags);
GParamSpec*	g_param_spec_flags	 (const gchar	 *name,
					  const gchar	 *nick,
					  const gchar	 *blurb,
					  GType		  flags_type,
					  guint		  default_value,
					  GParamFlags	  flags);
GParamSpec*	g_param_spec_float	 (const gchar	 *name,
					  const gchar	 *nick,
					  const gchar	 *blurb,
					  gfloat	  minimum,
					  gfloat	  maximum,
					  gfloat	  default_value,
					  GParamFlags	  flags);
GParamSpec*	g_param_spec_double	 (const gchar	 *name,
					  const gchar	 *nick,
					  const gchar	 *blurb,
					  gdouble	  minimum,
					  gdouble	  maximum,
					  gdouble	  default_value,
					  GParamFlags	  flags);
GParamSpec*	g_param_spec_string	 (const gchar	 *name,
					  const gchar	 *nick,
					  const gchar	 *blurb,
					  const gchar	 *default_value,
					  GParamFlags	  flags);
GParamSpec*	g_param_spec_param	 (const gchar	 *name,
					  const gchar	 *nick,
					  const gchar	 *blurb,
					  GType		  param_type,
					  GParamFlags	  flags);
GParamSpec*	g_param_spec_boxed	 (const gchar	 *name,
					  const gchar	 *nick,
					  const gchar	 *blurb,
					  GType		  boxed_type,
					  GParamFlags	  flags);
GParamSpec*	g_param_spec_pointer	 (const gchar	 *name,
					  const gchar	 *nick,
					  const gchar	 *blurb,
					  GParamFlags	  flags);
GParamSpec*	g_param_spec_value_array (const gchar	 *name,
					  const gchar	 *nick,
					  const gchar	 *blurb,
					  GParamSpec	 *element_spec,
					  GParamFlags	  flags);
GParamSpec*	g_param_spec_object	 (const gchar	 *name,
					  const gchar	 *nick,
					  const gchar	 *blurb,
					  GType		  object_type,
					  GParamFlags	  flags);

G_END_DECLS

#endif /* __G_PARAMSPECS_H__ */
