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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include	"gparam.h"


#include	"gvaluecollector.h"
#include	<string.h>



/* --- defines --- */
#define G_PARAM_SPEC_CLASS(class)    (G_TYPE_CHECK_CLASS_CAST ((class), G_TYPE_PARAM, GParamSpecClass))
#define PSPEC_APPLIES_TO_VALUE(pspec, value)  (G_TYPE_CHECK_VALUE_TYPE ((value), G_PARAM_SPEC_VALUE_TYPE (pspec)))


/* --- prototypes --- */
static void	g_param_spec_class_base_init	 (GParamSpecClass	*class);
static void	g_param_spec_class_base_finalize (GParamSpecClass	*class);
static void	g_param_spec_class_init		 (GParamSpecClass	*class,
						  gpointer               class_data);
static void	g_param_spec_init		 (GParamSpec		*pspec,
						  GParamSpecClass	*class);
static void	g_param_spec_finalize		 (GParamSpec		*pspec);
static void	value_param_init		(GValue		*value);
static void	value_param_free_value		(GValue		*value);
static void	value_param_copy_value		(const GValue	*src_value,
						 GValue		*dest_value);
static gpointer	value_param_peek_pointer	(const GValue	*value);
static gchar*	value_param_collect_value	(GValue		*value,
						 guint		 nth_value,
						 GType		*collect_type,
						 GTypeCValue	*collect_value);
static gchar*	value_param_lcopy_value		(const GValue	*value,
						 guint		 nth_value,
						 GType		*collect_type,
						 GTypeCValue	*collect_value);


/* --- variables --- */
static GQuark quark_floating = 0;


/* --- functions --- */
void
g_param_type_init (void)	/* sync with gtype.c */
{
  static const GTypeFundamentalInfo finfo = {
    (G_TYPE_FLAG_CLASSED |
     G_TYPE_FLAG_INSTANTIATABLE |
     G_TYPE_FLAG_DERIVABLE |
     G_TYPE_FLAG_DEEP_DERIVABLE),
  };
  static const GTypeValueTable param_value_table = {
    value_param_init,           /* value_init */
    value_param_free_value,     /* value_free */
    value_param_copy_value,     /* value_copy */
    value_param_peek_pointer,   /* value_peek_pointer */
    G_VALUE_COLLECT_POINTER,    /* collect_type */
    value_param_collect_value,  /* collect_value */
    G_VALUE_COLLECT_POINTER,    /* lcopy_type */
    value_param_lcopy_value,    /* lcopy_value */
  };
  static const GTypeInfo param_spec_info = {
    sizeof (GParamSpecClass),

    (GBaseInitFunc) g_param_spec_class_base_init,
    (GBaseFinalizeFunc) g_param_spec_class_base_finalize,
    (GClassInitFunc) g_param_spec_class_init,
    (GClassFinalizeFunc) NULL,
    NULL,	/* class_data */

    sizeof (GParamSpec),
    0,		/* n_preallocs */
    (GInstanceInitFunc) g_param_spec_init,

    &param_value_table,
  };
  GType type;

  type = g_type_register_fundamental (G_TYPE_PARAM, "GParam", &param_spec_info, &finfo, G_TYPE_FLAG_ABSTRACT);
  g_assert (type == G_TYPE_PARAM);
}

static void
g_param_spec_class_base_init (GParamSpecClass *class)
{
}

static void
g_param_spec_class_base_finalize (GParamSpecClass *class)
{
}

static void
g_param_spec_class_init (GParamSpecClass *class,
			 gpointer         class_data)
{
  quark_floating = g_quark_from_static_string ("GParamSpec-floating");

  class->value_type = G_TYPE_NONE;
  class->finalize = g_param_spec_finalize;
  class->value_set_default = NULL;
  class->value_validate = NULL;
  class->values_cmp = NULL;
}

static void
g_param_spec_init (GParamSpec      *pspec,
		   GParamSpecClass *class)
{
  pspec->name = NULL;
  pspec->nick = NULL;
  pspec->blurb = NULL;
  pspec->flags = 0;
  pspec->value_type = class->value_type;
  pspec->owner_type = 0;
  pspec->qdata = NULL;
  pspec->ref_count = 1;
  g_datalist_id_set_data (&pspec->qdata, quark_floating, GUINT_TO_POINTER (TRUE));
}

static void
g_param_spec_finalize (GParamSpec *pspec)
{
  g_datalist_clear (&pspec->qdata);
  
  g_free (pspec->name);
  g_free (pspec->nick);
  g_free (pspec->blurb);

  g_type_free_instance ((GTypeInstance*) pspec);
}

GParamSpec*
g_param_spec_ref (GParamSpec *pspec)
{
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);
  g_return_val_if_fail (pspec->ref_count > 0, NULL);

  pspec->ref_count += 1;

  return pspec;
}

void
g_param_spec_unref (GParamSpec *pspec)
{
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (pspec->ref_count > 0);

  /* sync with _sink */
  pspec->ref_count -= 1;
  if (pspec->ref_count == 0)
    G_PARAM_SPEC_GET_CLASS (pspec)->finalize (pspec);
}

void
g_param_spec_sink (GParamSpec *pspec)
{
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (pspec->ref_count > 0);

  if (g_datalist_id_remove_no_notify (&pspec->qdata, quark_floating))
    {
      /* sync with _unref */
      if (pspec->ref_count > 1)
	pspec->ref_count -= 1;
      else
	g_param_spec_unref (pspec);
    }
}

gpointer
g_param_spec_internal (GType        param_type,
		       const gchar *name,
		       const gchar *nick,
		       const gchar *blurb,
		       GParamFlags  flags)
{
  GParamSpec *pspec;

  g_return_val_if_fail (G_TYPE_IS_PARAM (param_type) && param_type != G_TYPE_PARAM, NULL);
  g_return_val_if_fail (name != NULL, NULL);

  pspec = (gpointer) g_type_create_instance (param_type);
  pspec->name = g_strdup (name);
  g_strcanon (pspec->name, G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-", '-');
  pspec->nick = g_strdup (nick ? nick : pspec->name);
  pspec->blurb = g_strdup (blurb);
  pspec->flags = (flags & G_PARAM_USER_MASK) | (flags & G_PARAM_MASK);

  return pspec;
}

gpointer
g_param_spec_get_qdata (GParamSpec *pspec,
			GQuark      quark)
{
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);
  
  return quark ? g_datalist_id_get_data (&pspec->qdata, quark) : NULL;
}

void
g_param_spec_set_qdata (GParamSpec *pspec,
			GQuark      quark,
			gpointer    data)
{
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (quark > 0);

  g_datalist_id_set_data (&pspec->qdata, quark, data);
}

void
g_param_spec_set_qdata_full (GParamSpec    *pspec,
			     GQuark         quark,
			     gpointer       data,
			     GDestroyNotify destroy)
{
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (quark > 0);

  g_datalist_id_set_data_full (&pspec->qdata, quark, data, data ? destroy : (GDestroyNotify) NULL);
}

gpointer
g_param_spec_steal_qdata (GParamSpec *pspec,
			  GQuark      quark)
{
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);
  g_return_val_if_fail (quark > 0, NULL);
  
  return g_datalist_id_remove_no_notify (&pspec->qdata, quark);
}

void
g_param_value_set_default (GParamSpec *pspec,
			   GValue     *value)
{
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (G_IS_VALUE (value));
  g_return_if_fail (PSPEC_APPLIES_TO_VALUE (pspec, value));

  g_value_reset (value);
  G_PARAM_SPEC_GET_CLASS (pspec)->value_set_default (pspec, value);
}

gboolean
g_param_value_defaults (GParamSpec *pspec,
			GValue     *value)
{
  GValue dflt_value = { 0, };
  gboolean defaults;

  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);
  g_return_val_if_fail (G_IS_VALUE (value), FALSE);
  g_return_val_if_fail (PSPEC_APPLIES_TO_VALUE (pspec, value), FALSE);

  g_value_init (&dflt_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  G_PARAM_SPEC_GET_CLASS (pspec)->value_set_default (pspec, &dflt_value);
  defaults = G_PARAM_SPEC_GET_CLASS (pspec)->values_cmp (pspec, value, &dflt_value) == 0;
  g_value_unset (&dflt_value);

  return defaults;
}

gboolean
g_param_value_validate (GParamSpec *pspec,
			GValue     *value)
{
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);
  g_return_val_if_fail (G_IS_VALUE (value), FALSE);
  g_return_val_if_fail (PSPEC_APPLIES_TO_VALUE (pspec, value), FALSE);

  if (G_PARAM_SPEC_GET_CLASS (pspec)->value_validate)
    {
      GValue oval = *value;

      if (G_PARAM_SPEC_GET_CLASS (pspec)->value_validate (pspec, value) ||
	  memcmp (&oval.data, &value->data, sizeof (oval.data)))
	return TRUE;
    }

  return FALSE;
}

gint
g_param_values_cmp (GParamSpec   *pspec,
		    const GValue *value1,
		    const GValue *value2)
{
  gint cmp;

  /* param_values_cmp() effectively does: value1 - value2
   * so the return values are:
   * -1)  value1 < value2
   *  0)  value1 == value2
   *  1)  value1 > value2
   */
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), 0);
  g_return_val_if_fail (G_IS_VALUE (value1), 0);
  g_return_val_if_fail (G_IS_VALUE (value2), 0);
  g_return_val_if_fail (PSPEC_APPLIES_TO_VALUE (pspec, value1), 0);
  g_return_val_if_fail (PSPEC_APPLIES_TO_VALUE (pspec, value2), 0);

  cmp = G_PARAM_SPEC_GET_CLASS (pspec)->values_cmp (pspec, value1, value2);

  return CLAMP (cmp, -1, 1);
}

static void
value_param_init (GValue *value)
{
  value->data[0].v_pointer = NULL;
}

static void
value_param_free_value (GValue *value)
{
  if (value->data[0].v_pointer)
    g_param_spec_unref (value->data[0].v_pointer);
}

static void
value_param_copy_value (const GValue *src_value,
			GValue       *dest_value)
{
  dest_value->data[0].v_pointer = (src_value->data[0].v_pointer
				   ? g_param_spec_ref (src_value->data[0].v_pointer)
				   : NULL);
}

static gpointer
value_param_peek_pointer (const GValue *value)
{
  return value->data[0].v_pointer;
}

static gchar*
value_param_collect_value (GValue      *value,
			   guint        nth_value,
			   GType       *collect_type,
			   GTypeCValue *collect_value)
{
  if (collect_value->v_pointer)
    {
      GParamSpec *param = collect_value->v_pointer;

      if (param->g_type_instance.g_class == NULL)
	return g_strconcat ("invalid unclassed param spec pointer for value type `",
			    G_VALUE_TYPE_NAME (value),
			    "'",
			    NULL);
      else if (!g_type_is_a (G_PARAM_SPEC_TYPE (param), G_VALUE_TYPE (value)))
	return g_strconcat ("invalid param spec type `",
			    G_PARAM_SPEC_TYPE_NAME (param),
			    "' for value type `",
			    G_VALUE_TYPE_NAME (value),
			    "'",
			    NULL);
      value->data[0].v_pointer = g_param_spec_ref (param);
    }
  else
    value->data[0].v_pointer = NULL;

  *collect_type = 0;
  return NULL;
}

static gchar*
value_param_lcopy_value (const GValue *value,
			 guint         nth_value,
			 GType        *collect_type,
			 GTypeCValue  *collect_value)
{
  GParamSpec **param_p = collect_value->v_pointer;

  if (!param_p)
    return g_strdup_printf ("value location for `%s' passed as NULL", G_VALUE_TYPE_NAME (value));

  *param_p = value->data[0].v_pointer ? g_param_spec_ref (value->data[0].v_pointer) : NULL;

  *collect_type = 0;
  return NULL;
}


/* --- param spec pool --- */
struct _GParamSpecPool
{
  gboolean    type_prefixing;
  GHashTable *hash_table;
};

static guint
param_spec_pool_hash (gconstpointer key_spec)
{
  const GParamSpec *key = key_spec;
  const gchar *p;
  guint h = key->owner_type;

  for (p = key->name; *p; p++)
    h = (h << 5) - h + *p;

  return h;
}

static gboolean
param_spec_pool_equals (gconstpointer key_spec_1,
			gconstpointer key_spec_2)
{
  const GParamSpec *key1 = key_spec_1;
  const GParamSpec *key2 = key_spec_2;

  return (key1->owner_type == key2->owner_type &&
	  strcmp (key1->name, key2->name) == 0);
}

GParamSpecPool*
g_param_spec_pool_new (gboolean type_prefixing)
{
  GParamSpecPool *pool = g_new (GParamSpecPool, 1);

  pool->type_prefixing = type_prefixing != FALSE;
  pool->hash_table = g_hash_table_new (param_spec_pool_hash, param_spec_pool_equals);

  return pool;
}

void
g_param_spec_pool_insert (GParamSpecPool *pool,
			  GParamSpec     *pspec,
			  GType           owner_type)
{
  gchar *p;

  g_return_if_fail (pool != NULL);
  g_return_if_fail (pspec);
  g_return_if_fail (owner_type > 0);
  g_return_if_fail (pspec->owner_type == 0);

  for (p = pspec->name; *p; p++)
    {
      if (!strchr (G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-_", *p))
	{
	  g_warning (G_STRLOC ": pspec name \"%s\" contains invalid characters", pspec->name);
	  return;
	}
    }

  pspec->owner_type = owner_type;
  g_param_spec_ref (pspec);
  g_hash_table_insert (pool->hash_table, pspec, pspec);
}

void
g_param_spec_pool_remove (GParamSpecPool *pool,
			  GParamSpec     *pspec)
{
  g_return_if_fail (pool != NULL);
  g_return_if_fail (pspec);

  if (g_hash_table_remove (pool->hash_table, pspec))
    g_param_spec_unref (pspec);
  else
    g_warning (G_STRLOC ": attempt to remove unknown pspec `%s' from pool", pspec->name);
}

static inline GParamSpec*
param_spec_ht_lookup (GHashTable  *hash_table,
		      const gchar *param_name,
		      GType        owner_type,
		      gboolean     walk_ancestors)
{
  GParamSpec key, *pspec;

  key.owner_type = owner_type;
  key.name = (gchar*) param_name;
  if (walk_ancestors)
    do
      {
	pspec = g_hash_table_lookup (hash_table, &key);
	if (pspec)
	  return pspec;
	key.owner_type = g_type_parent (key.owner_type);
      }
    while (key.owner_type);
  else
    pspec = g_hash_table_lookup (hash_table, &key);

  if (!pspec)
    {
      /* sigh, try canonicalization */
      key.name = g_strdup (param_name);
      key.owner_type = owner_type;
      
      g_strcanon (key.name, G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-", '-');
      if (walk_ancestors)
	do
	  {
	    pspec = g_hash_table_lookup (hash_table, &key);
	    if (pspec)
	      {
		g_free (key.name);
		return pspec;
	      }
	    key.owner_type = g_type_parent (key.owner_type);
	  }
	while (key.owner_type);
      else
	pspec = g_hash_table_lookup (hash_table, &key);
      g_free (key.name);
    }

  return pspec;
}

GParamSpec*
g_param_spec_pool_lookup (GParamSpecPool *pool,
			  const gchar    *param_name,
			  GType           owner_type,
			  gboolean        walk_ancestors,
			  const gchar   **trailer_p)
{
  GParamSpec *pspec;
  gchar *delim;

  g_return_val_if_fail (pool != NULL, NULL);
  g_return_val_if_fail (param_name, NULL);

  delim = strchr (param_name, ':');

  /* try quick and away, i.e. no prefix, no trailer */
  if (!delim)
    {
      if (trailer_p)
	*trailer_p = NULL;
      return param_spec_ht_lookup (pool->hash_table, param_name, owner_type, walk_ancestors);
    }

  /* strip type prefix */
  if (pool->type_prefixing && delim[1] == ':')
    {
      guint l = delim - param_name;
      gchar stack_buffer[32], *buffer = l < 32 ? stack_buffer : g_new (gchar, l + 1);
      GType type;
      
      strncpy (buffer, param_name, delim - param_name);
      buffer[l] = 0;
      type = g_type_from_name (buffer);
      if (l >= 32)
	g_free (buffer);
      if (type)		/* type==0 isn't a valid type pefix */
	{
	  /* sanity check, these cases don't make a whole lot of sense */
	  if ((!walk_ancestors && type != owner_type) || !g_type_is_a (owner_type, type))
	    {
	      if (trailer_p)
		*trailer_p = NULL;
	      return NULL;
	    }
	  owner_type = type;
	  param_name += l + 2;
	  delim = strchr (param_name, ':');
	  if (!delim)		/* good, can still forget about trailer */
	    {
	      if (trailer_p)
		*trailer_p = NULL;
	      return param_spec_ht_lookup (pool->hash_table, param_name, owner_type, walk_ancestors);
	    }
	}
    }

  /* ok, no prefix, handle trailer */
  if (delim[1] == ':')
    {
      guint l = delim - param_name;
      gchar stack_buffer[32], *buffer = l < 32 ? stack_buffer : g_new (gchar, l + 1);
      
      strncpy (buffer, param_name, delim - param_name);
      buffer[l] = 0;
      pspec = param_spec_ht_lookup (pool->hash_table, buffer, owner_type, walk_ancestors);
      if (l >= 32)
	g_free (buffer);
      if (trailer_p)
	*trailer_p = pspec ? delim + 2 : NULL;
      return pspec;
    }

  /* malformed param_name */
  if (trailer_p)
    *trailer_p = NULL;
  return NULL;
}


/* --- auxillary functions --- */
typedef struct
{
  /* class portion */
  GType           value_type;
  void          (*finalize)             (GParamSpec   *pspec);
  void          (*value_set_default)    (GParamSpec   *pspec,
					 GValue       *value);
  gboolean      (*value_validate)       (GParamSpec   *pspec,
					 GValue       *value);
  gint          (*values_cmp)           (GParamSpec   *pspec,
					 const GValue *value1,
					 const GValue *value2);
} ParamSpecClassInfo;

static void
param_spec_generic_class_init (gpointer g_class,
			       gpointer class_data)
{
  GParamSpecClass *class = g_class;
  ParamSpecClassInfo *info = class_data;

  class->value_type = info->value_type;
  if (info->finalize)
    class->finalize = info->finalize;			/* optional */
  class->value_set_default = info->value_set_default;
  if (info->value_validate)
    class->value_validate = info->value_validate;	/* optional */
  class->values_cmp = info->values_cmp;
  g_free (class_data);
}

static void
default_value_set_default (GParamSpec *pspec,
			   GValue     *value)
{
  /* value is already zero initialized */
}

static gint
default_values_cmp (GParamSpec   *pspec,
		    const GValue *value1,
		    const GValue *value2)
{
  return memcmp (&value1->data, &value2->data, sizeof (value1->data));
}

GType
g_param_type_register_static (const gchar              *name,
			      const GParamSpecTypeInfo *pspec_info)
{
  GTypeInfo info = {
    sizeof (GParamSpecClass),      /* class_size */
    NULL,                          /* base_init */
    NULL,                          /* base_destroy */
    param_spec_generic_class_init, /* class_init */
    NULL,                          /* class_destroy */
    NULL,                          /* class_data */
    0,                             /* instance_size */
    16,                            /* n_preallocs */
    NULL,                          /* instance_init */
  };
  ParamSpecClassInfo *cinfo;

  g_return_val_if_fail (name != NULL, 0);
  g_return_val_if_fail (pspec_info != NULL, 0);
  g_return_val_if_fail (g_type_from_name (name) == 0, 0);
  g_return_val_if_fail (pspec_info->instance_size >= sizeof (GParamSpec), 0);
  g_return_val_if_fail (g_type_name (pspec_info->value_type) != NULL, 0);
  /* default: g_return_val_if_fail (pspec_info->value_set_default != NULL, 0); */
  /* optional: g_return_val_if_fail (pspec_info->value_validate != NULL, 0); */
  /* default: g_return_val_if_fail (pspec_info->values_cmp != NULL, 0); */

  info.instance_size = pspec_info->instance_size;
  info.n_preallocs = pspec_info->n_preallocs;
  info.instance_init = (GInstanceInitFunc) pspec_info->instance_init;
  cinfo = g_new (ParamSpecClassInfo, 1);
  cinfo->value_type = pspec_info->value_type;
  cinfo->finalize = pspec_info->finalize;
  cinfo->value_set_default = pspec_info->value_set_default ? pspec_info->value_set_default : default_value_set_default;
  cinfo->value_validate = pspec_info->value_validate;
  cinfo->values_cmp = pspec_info->values_cmp ? pspec_info->values_cmp : default_values_cmp;
  info.class_data = cinfo;

  return g_type_register_static (G_TYPE_PARAM, name, &info, 0);
}

void
g_value_set_param (GValue     *value,
		   GParamSpec *param)
{
  g_return_if_fail (G_IS_VALUE_PARAM (value));
  if (param)
    g_return_if_fail (G_IS_PARAM_SPEC (param));

  if (value->data[0].v_pointer)
    g_param_spec_unref (value->data[0].v_pointer);
  value->data[0].v_pointer = param;
  if (value->data[0].v_pointer)
    g_param_spec_ref (value->data[0].v_pointer);
}

GParamSpec*
g_value_get_param (const GValue *value)
{
  g_return_val_if_fail (G_IS_VALUE_PARAM (value), NULL);

  return value->data[0].v_pointer;
}

GParamSpec*
g_value_dup_param (const GValue *value)
{
  g_return_val_if_fail (G_IS_VALUE_PARAM (value), NULL);

  return value->data[0].v_pointer ? g_param_spec_ref (value->data[0].v_pointer) : NULL;
}
