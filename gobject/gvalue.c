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

/*
 * FIXME: MT-safety
 */

#include <string.h>

#include "gvalue.h"
#include "gvaluecollector.h"


/* --- typedefs & structures --- */
typedef struct
{
  GType          value_type1;
  GType          value_type2;
  GValueExchange func;
  GType          first_type;
} ExchangeEntry;


/* --- variables --- */
static GHashTable *param_exchange_ht = NULL;


/* --- functions --- */
void
g_value_init (GValue *value,
	      GType   g_type)
{
  GTypeValueTable *value_table = g_type_value_table_peek (g_type);
  
  g_return_if_fail (value != NULL);
  g_return_if_fail (G_VALUE_TYPE (value) == 0);
  
  if (value_table)
    {
      memset (value, 0, sizeof (*value));
      value->g_type = g_type;
      value_table->value_init (value);
    }
  else
    g_warning (G_STRLOC ": cannot initialize value of type `%s' which has no GTypeValueTable",
	       g_type_name (g_type));
}

void
g_value_copy (const GValue *src_value,
	      GValue       *dest_value)
{
  GTypeValueTable *value_table;
  
  g_return_if_fail (G_IS_VALUE (src_value));
  g_return_if_fail (G_IS_VALUE (dest_value));
  g_return_if_fail (g_type_is_a (G_VALUE_TYPE (src_value), G_VALUE_TYPE (dest_value)));
  
  value_table = g_type_value_table_peek (G_VALUE_TYPE (dest_value));
  if (!value_table)
    g_return_if_fail (g_type_value_table_peek (G_VALUE_TYPE (dest_value)) != NULL);
  
  if (src_value != dest_value)
    {
      /* make sure dest_value's value is free()d and zero initialized */
      g_value_reset (dest_value);
      
      value_table->value_copy (src_value, dest_value);
    }
}

gboolean
g_value_fits_pointer (const GValue *value)
{
  GTypeValueTable *value_table;

  g_return_val_if_fail (G_IS_VALUE (value), FALSE);

  value_table = g_type_value_table_peek (G_VALUE_TYPE (value));
  if (!value_table)
    g_return_val_if_fail (g_type_value_table_peek (G_VALUE_TYPE (value)) != NULL, FALSE);

  return value_table->value_peek_pointer != NULL;
}

gpointer
g_value_get_as_pointer (const GValue *value)
{
  GTypeValueTable *value_table;

  g_return_val_if_fail (G_IS_VALUE (value), NULL);

  value_table = g_type_value_table_peek (G_VALUE_TYPE (value));
  if (!value_table)
    g_return_val_if_fail (g_type_value_table_peek (G_VALUE_TYPE (value)) != NULL, NULL);
  if (!value_table->value_peek_pointer)
    g_return_val_if_fail (g_value_fits_pointer (value) == TRUE, NULL);

  return value_table->value_peek_pointer (value);
}

void
g_value_set_instance (GValue  *value,
		      gpointer instance)
{
  g_return_if_fail (G_IS_VALUE (value));
  
  g_value_reset (value);
  if (instance)
    {
      GType g_type = G_VALUE_TYPE (value);
      GTypeValueTable *value_table = g_type_value_table_peek (g_type);
      GTypeCValue cvalue = { 0, };
      guint nth_value = 0;
      guint collect_type = value_table->collect_type;
      gchar *error_msg;
      
      g_return_if_fail (G_TYPE_CHECK_INSTANCE (instance));
      g_return_if_fail (g_type_is_a (G_TYPE_FROM_INSTANCE (instance), G_VALUE_TYPE (value)));
      g_return_if_fail (value_table->collect_type == G_VALUE_COLLECT_POINTER);
      
      cvalue.v_pointer = instance;
      error_msg = value_table->collect_value (value, nth_value++, &collect_type, &cvalue);
      
      /* this shouldn't be triggered, instance types should collect just one pointer,
       * but since we have to follow the calling conventions for collect_value(),
       * we can attempt to feed them with 0s if they insist on extra args.
       */
      while (collect_type && !error_msg)
	{
	  memset (&cvalue, 0, sizeof (cvalue));
	  error_msg = value_table->collect_value (value, nth_value++, &collect_type, &cvalue);
	}
      
      if (error_msg)
	{
	  g_warning ("%s: %s", G_STRLOC, error_msg);
	  g_free (error_msg);
	  
	  /* we purposely leak the value here, it might not be
	   * in a sane state if an error condition occoured
	   */
	  memset (value, 0, sizeof (*value));
	  value->g_type = g_type;
	  value_table->value_init (value);
	}
    }
}

void
g_value_unset (GValue *value)
{
  GTypeValueTable *value_table = g_type_value_table_peek (G_VALUE_TYPE (value));
  
  g_return_if_fail (G_IS_VALUE (value));
  if (!value_table)
    g_return_if_fail (g_type_value_table_peek (G_VALUE_TYPE (value)) != NULL);
  
  if (value_table->value_free)
    value_table->value_free (value);
  memset (value, 0, sizeof (*value));
}

void
g_value_reset (GValue *value)
{
  GTypeValueTable *value_table;
  GType g_type;
  
  g_return_if_fail (G_IS_VALUE (value));
  
  value_table = g_type_value_table_peek (G_VALUE_TYPE (value));
  g_type = G_VALUE_TYPE (value);
  
  if (value_table->value_free)
    value_table->value_free (value);
  memset (value, 0, sizeof (*value));
  
  value->g_type = g_type;
  value_table->value_init (value);
}

static gboolean
exchange_entries_equal (gconstpointer v1,
			gconstpointer v2)
{
  const ExchangeEntry *entry1 = v1;
  const ExchangeEntry *entry2 = v2;
  
  return (entry1->value_type1 == entry2->value_type1 &&
	  entry1->value_type2 == entry2->value_type2);
}

static guint
exchange_entry_hash (gconstpointer key)
{
  const ExchangeEntry *entry = key;
  
  return entry->value_type1 ^ entry->value_type2;
}

static void
value_exchange_memcpy (GValue *value1,
		       GValue *value2)
{
  GValue tmp_value;
  
  memcpy (&tmp_value.data, &value1->data, sizeof (value1->data));
  memcpy (&value1->data, &value2->data, sizeof (value1->data));
  memcpy (&value2->data, &tmp_value.data, sizeof (value2->data));
}

static inline GValueExchange
exchange_func_lookup (GType     value_type1,
		      GType     value_type2,
		      gboolean *need_swap)
{
  if (value_type1 == value_type2)
    return value_exchange_memcpy;
  else
    {
      GType type1 = value_type1;
      
      do
	{
	  GType type2 = value_type2;
	  
	  do
	    {
	      ExchangeEntry entry, *ret;
	      
	      entry.value_type1 = MIN (type1, type2);
	      entry.value_type2 = MAX (type1, type2);
	      ret = g_hash_table_lookup (param_exchange_ht, &entry);
	      if (ret)
		{
		  if (need_swap)
		    *need_swap = ret->first_type == type2;
		  
		  return ret->func;
		}
	      
	      type2 = g_type_parent (type2);
	    }
	  while (type2);
	  
	  type1 = g_type_parent (type1);
	}
      while (type1);
    }
  
  return NULL;
}

void
g_value_register_exchange_func (GType          value_type1,
				GType          value_type2,
				GValueExchange func)
{
  ExchangeEntry entry;
  
  g_return_if_fail (g_type_name (value_type1) != NULL);
  g_return_if_fail (g_type_name (value_type2) != NULL);
  g_return_if_fail (func != NULL);
  
  entry.value_type1 = MIN (value_type1, value_type2);
  entry.value_type2 = MAX (value_type1, value_type2);
  if (param_exchange_ht && g_hash_table_lookup (param_exchange_ht, &entry))
    g_warning (G_STRLOC ": cannot re-register param value exchange function "
	       "for `%s' and `%s'",
	       g_type_name (value_type1),
	       g_type_name (value_type2));
  else
    {
      ExchangeEntry *entry = g_new (ExchangeEntry, 1);
      
      if (!param_exchange_ht)
	param_exchange_ht = g_hash_table_new (exchange_entry_hash, exchange_entries_equal);
      entry->value_type1 = MIN (value_type1, value_type2);
      entry->value_type2 = MAX (value_type1, value_type2);
      entry->func = func;
      entry->first_type = value_type1;
      g_hash_table_insert (param_exchange_ht, entry, entry);
    }
}

gboolean
g_value_types_exchangable (GType value_type1,
			   GType value_type2)
{
  g_return_val_if_fail (G_TYPE_IS_VALUE (value_type1), FALSE); /* these might bite us, think G_TYPE_ENUM */
  g_return_val_if_fail (G_TYPE_IS_VALUE (value_type2), FALSE);
  
  return exchange_func_lookup (value_type1, value_type2, NULL) != NULL;
}

gboolean
g_values_exchange (GValue *value1,
		   GValue *value2)
{
  g_return_val_if_fail (G_IS_VALUE (value1), FALSE);
  g_return_val_if_fail (G_IS_VALUE (value2), FALSE);
  
  if (value1 != value2)
    {
      gboolean need_swap;
      GValueExchange value_exchange = exchange_func_lookup (G_VALUE_TYPE (value1),
							    G_VALUE_TYPE (value2),
							    &need_swap);
      if (value_exchange)
	{
	  if (need_swap)
	    value_exchange (value2, value1);
	  else
	    value_exchange (value1, value2);
	}
      
      return value_exchange != NULL;
    }
  
  return TRUE;
}

gboolean
g_value_convert (const GValue *src_value,
		 GValue       *dest_value)
{
  gboolean success = TRUE;
  
  g_return_val_if_fail (G_IS_VALUE (src_value), FALSE);
  g_return_val_if_fail (G_IS_VALUE (dest_value), FALSE);
  
  if (src_value != dest_value)
    {
      GValue tmp_value = { 0, };
      
      g_value_init (&tmp_value, G_VALUE_TYPE (src_value));
      g_value_copy (src_value, &tmp_value);
      
      success = g_values_exchange (&tmp_value, dest_value);
      g_value_unset (&tmp_value);
    }
  
  return success;
}
