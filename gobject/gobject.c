/* GObject - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 1998, 1999, 2000 Tim Janik and Red Hat, Inc.
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
#include	"gobject.h"


#include	"gvaluecollector.h"
#include	"gsignal.h"
#include	"gparamspecs.h"
#include	"gvaluetypes.h"
#include	<string.h>


#define	PREALLOC_CPARAMS	(8)
#define	DEBUG_OBJECTS


/* --- macros --- */
#define PARAM_SPEC_PARAM_ID(pspec)	(GPOINTER_TO_UINT (g_param_spec_get_qdata ((pspec), quark_property_id)))


/* --- signals --- */
enum {
  PROPERTIES_CHANGED,
  NOTIFY,
  LAST_SIGNAL
};


/* --- properties --- */
enum {
  PROP_NONE,
  PROP_DATA,
  PROP_SIGNAL,
  PROP_SWAPPED_SIGNAL,
  PROP_SIGNAL_AFTER,
  PROP_SWAPPED_SIGNAL_AFTER
};


/* --- typedefs --- */
typedef struct _NotifyQueue NotifyQueue;


/* --- prototypes --- */
static void	g_object_base_class_init		(GObjectClass	*class);
static void	g_object_base_class_finalize		(GObjectClass	*class);
static void	g_object_do_class_init			(GObjectClass	*class);
static void	g_object_init				(GObject	*object);
static GObject*	g_object_constructor			(GType                  type,
							 guint                  n_construct_properties,
							 GObjectConstructParam *construct_params);
static void	g_object_last_unref			(GObject	*object);
static void	g_object_shutdown			(GObject	*object);
static void	g_object_finalize			(GObject	*object);
static void	g_object_do_set_property		(GObject        *object,
							 guint           property_id,
							 const GValue   *value,
							 GParamSpec     *pspec,
							 const gchar    *trailer);
static void	g_object_do_get_property		(GObject        *object,
							 guint           property_id,
							 GValue         *value,
							 GParamSpec     *pspec,
							 const gchar    *trailer);
static void	g_value_object_init			(GValue		*value);
static void	g_value_object_free_value		(GValue		*value);
static void	g_value_object_copy_value		(const GValue	*src_value,
							 GValue		*dest_value);
static gpointer g_value_object_peek_pointer             (const GValue   *value);
static gchar*	g_value_object_collect_value		(GValue		*value,
							 guint		 nth_value,
							 GType		*collect_type,
							 GTypeCValue	*collect_value);
static gchar*	g_value_object_lcopy_value		(const GValue	*value,
							 guint		 nth_value,
							 GType		*collect_type,
							 GTypeCValue	*collect_value);
static void	g_object_dispatch_properties_changed	(GObject	*object,
							 guint		 n_pspecs,
							 GParamSpec    **pspecs);
static void	g_object_properties_changed		(GObject	*object,
							 guint		 n_pspecs,
							 GParamSpec    **pspecs);
static void	g_object_notify_property_changed	(GObject	*object,
							 GParamSpec	*pspec);
static inline NotifyQueue* object_freeze_notifies    	(GObject        *object);
static inline void	   object_queue_property	(GObject        *object,
							 GParamSpec     *pspec,
							 NotifyQueue    *nqueue);
static inline void	   object_thaw_notifies		(GObject        *object,
							 NotifyQueue    *nqueue);
static inline void         object_get_property		(GObject        *object,
							 GValue         *value,
							 GParamSpec     *pspec,
							 const gchar    *trailer);
static inline void	   object_set_property		(GObject        *object,
							 GValue         *value,
							 GParamSpec     *pspec,
							 const gchar    *trailer,
							 NotifyQueue    *nqueue);


/* --- structures --- */
struct _NotifyQueue
{
  GSList *pspecs;
  guint   n_pspecs;
  guint   freeze_count;
};


/* --- variables --- */
static GQuark	       quark_notify_queue = 0;
static GQuark	       quark_property_id = 0;
static GQuark	       quark_closure_array = 0;
static GParamSpecPool *pspec_pool = NULL;
static gulong	       gobject_signals[LAST_SIGNAL] = { 0, };


/* --- functions --- */
#ifdef	DEBUG_OBJECTS

/* We need an actual method for handling debug keys in GLib.
 * For now, we'll simply use, as a method
 * 'extern gboolean glib_debug_objects'
 */
gboolean glib_debug_objects = FALSE;

static guint		  debug_objects_count = 0;
static GHashTable	 *debug_objects_ht = NULL;
static void
debug_objects_foreach (gpointer key,
		       gpointer value,
		       gpointer user_data)
{
  GObject *object = value;
  
  g_message ("[%p] stale %s\tref_count=%u",
	     object,
	     G_OBJECT_TYPE_NAME (object),
	     object->ref_count);
}
static void
debug_objects_atexit (void)
{
  if (glib_debug_objects)
    {
      if (debug_objects_ht)
	{
	  g_message ("stale GObjects: %u", debug_objects_count);
	  g_hash_table_foreach (debug_objects_ht, debug_objects_foreach, NULL);
	}
    }
}
#endif /* DEBUG_OBJECTS */

void
g_object_type_init (void)	/* sync with gtype.c */
{
  static gboolean initialized = FALSE;
  static const GTypeFundamentalInfo finfo = {
    G_TYPE_FLAG_CLASSED | G_TYPE_FLAG_INSTANTIATABLE | G_TYPE_FLAG_DERIVABLE | G_TYPE_FLAG_DEEP_DERIVABLE,
  };
  static GTypeInfo info = {
    sizeof (GObjectClass),
    (GBaseInitFunc) g_object_base_class_init,
    (GBaseFinalizeFunc) g_object_base_class_finalize,
    (GClassInitFunc) g_object_do_class_init,
    NULL	/* class_destroy */,
    NULL	/* class_data */,
    sizeof (GObject),
    0		/* n_preallocs */,
    (GInstanceInitFunc) g_object_init,
    NULL,	/* value_table */
  };
  static const GTypeValueTable value_table = {
    g_value_object_init,	  /* value_init */
    g_value_object_free_value,	  /* value_free */
    g_value_object_copy_value,	  /* value_copy */
    g_value_object_peek_pointer,  /* value_peek_pointer */
    G_VALUE_COLLECT_POINTER,	  /* collect_type */
    g_value_object_collect_value, /* collect_value */
    G_VALUE_COLLECT_POINTER,	  /* lcopy_type */
    g_value_object_lcopy_value,	  /* lcopy_value */
  };
  GType type;
  
  g_return_if_fail (initialized == FALSE);
  initialized = TRUE;
  
  /* G_TYPE_OBJECT
   */
  info.value_table = &value_table;
  type = g_type_register_fundamental (G_TYPE_OBJECT, "GObject", &info, &finfo, 0);
  g_assert (type == G_TYPE_OBJECT);
  
#ifdef	DEBUG_OBJECTS
  g_atexit (debug_objects_atexit);
#endif /* DEBUG_OBJECTS */
}

static void
g_object_base_class_init (GObjectClass *class)
{
  GObjectClass *pclass = g_type_class_peek_parent (class);

  /* reset instance specific fields and methods that don't get inherited */
  class->n_property_specs = 0;
  class->property_specs = NULL;
  class->construct_properties = pclass ? g_slist_copy (pclass->construct_properties) : NULL;
  class->get_property = NULL;
  class->set_property = NULL;
}

static void
g_object_base_class_finalize (GObjectClass *class)
{
  guint i;
  
  g_message ("finallizing base class of %s", G_OBJECT_CLASS_NAME (class));

  _g_signals_destroy (G_OBJECT_CLASS_TYPE (class));

  g_slist_free (class->construct_properties);
  class->construct_properties = NULL;
  for (i = 0; i < class->n_property_specs; i++)
    {
      GParamSpec *pspec = class->property_specs[i];
      
      g_param_spec_pool_remove (pspec_pool, pspec);
      g_param_spec_set_qdata (pspec, quark_property_id, NULL);
      g_param_spec_unref (pspec);
    }
  class->n_property_specs = 0;
  g_free (class->property_specs);
  class->property_specs = NULL;
}

static void
g_object_do_class_init (GObjectClass *class)
{
  quark_notify_queue = g_quark_from_static_string ("GObject-notify-queue");
  quark_property_id = g_quark_from_static_string ("GObject-property-id");
  quark_closure_array = g_quark_from_static_string ("GObject-closure-array");
  pspec_pool = g_param_spec_pool_new (TRUE);
  
  class->constructor = g_object_constructor;
  class->set_property = g_object_do_set_property;
  class->get_property = g_object_do_get_property;
  class->shutdown = g_object_shutdown;
  class->finalize = g_object_finalize;
  class->dispatch_properties_changed = g_object_dispatch_properties_changed;
  class->properties_changed = g_object_properties_changed;
  class->notify = g_object_notify_property_changed;

  g_object_class_install_property (class,
				   PROP_DATA,
				   g_param_spec_pointer ("data", "Named Data",
							 "Named anonymous pointers",
							 G_PARAM_READABLE | G_PARAM_WRITABLE));
  g_object_class_install_property (class,
				   PROP_SIGNAL,
				   g_param_spec_ccallback ("signal", "Signal Connection",
							   "Signal connection consisting of a callback function "
							   "and a data pointer",
							   G_PARAM_WRITABLE));
  g_object_class_install_property (class,
				   PROP_SWAPPED_SIGNAL,
				   g_param_spec_ccallback ("swapped_signal", "Swapped Signal Connection",
							   "Signal connection consisting of a callback function "
							   "and a data pointer",
							   G_PARAM_WRITABLE));
  g_object_class_install_property (class,
				   PROP_SIGNAL_AFTER,
				   g_param_spec_ccallback ("signal_after", "Signal After Connection",
							   "Signal connection consisting of a callback function "
							   "and a data pointer",
							   G_PARAM_WRITABLE));
  g_object_class_install_property (class,
				   PROP_SWAPPED_SIGNAL_AFTER,
				   g_param_spec_ccallback ("swapped_signal_after", "Swapped Signal After Connection",
							   "Signal connection consisting of a callback function "
							   "and a data pointer",
							   G_PARAM_WRITABLE));
  gobject_signals[PROPERTIES_CHANGED] =
    g_signal_newc ("properties_changed",
                   G_TYPE_FROM_CLASS (class),
                   G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
                   G_STRUCT_OFFSET (GObjectClass, properties_changed),
                   NULL, /* accumulator */
                   g_cclosure_marshal_VOID__UINT_POINTER,
                   G_TYPE_NONE,
                   2, G_TYPE_UINT, G_TYPE_POINTER);
  gobject_signals[NOTIFY] =
    g_signal_newc ("notify",
                   G_TYPE_FROM_CLASS (class),
                   G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | G_SIGNAL_DETAILED | G_SIGNAL_NO_HOOKS,
                   G_STRUCT_OFFSET (GObjectClass, notify),
                   NULL, /* accumulator */
                   g_cclosure_marshal_VOID__PARAM,
                   G_TYPE_NONE,
                   1, G_TYPE_PARAM);
}

void
g_object_class_install_property (GObjectClass *class,
				 guint	       property_id,
				 GParamSpec   *pspec)
{
  guint i;
  
  g_return_if_fail (G_IS_OBJECT_CLASS (class));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  if (pspec->flags & G_PARAM_WRITABLE)
    g_return_if_fail (class->set_property != NULL);
  if (pspec->flags & G_PARAM_READABLE)
    g_return_if_fail (class->get_property != NULL);
  g_return_if_fail (property_id > 0);
  g_return_if_fail (PARAM_SPEC_PARAM_ID (pspec) == 0);	/* paranoid */
  if (pspec->flags & G_PARAM_CONSTRUCT)
    g_return_if_fail ((pspec->flags & G_PARAM_CONSTRUCT_ONLY) == 0);
  if (pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY))
    g_return_if_fail (pspec->flags & G_PARAM_WRITABLE);

  /* expensive paranoia checks ;( */
  for (i = 0; i < class->n_property_specs; i++)
    if (PARAM_SPEC_PARAM_ID (class->property_specs[i]) == property_id)
      {
	g_warning (G_STRLOC ": class `%s' already contains a property `%s' with id %u, "
		   "cannot install property `%s'",
		   G_OBJECT_CLASS_NAME (class),
		   class->property_specs[i]->name,
		   property_id,
		   pspec->name);
	return;
      }
  if (g_param_spec_pool_lookup (pspec_pool, pspec->name, G_OBJECT_CLASS_TYPE (class), FALSE, NULL))
    {
      g_warning (G_STRLOC ": class `%s' already contains a property named `%s'",
		 G_OBJECT_CLASS_NAME (class),
		 pspec->name);
      return;
    }

  g_param_spec_ref (pspec);
  g_param_spec_sink (pspec);
  g_param_spec_set_qdata (pspec, quark_property_id, GUINT_TO_POINTER (property_id));
  g_param_spec_pool_insert (pspec_pool, pspec, G_OBJECT_CLASS_TYPE (class));
  i = class->n_property_specs++;
  class->property_specs = g_renew (GParamSpec*, class->property_specs, class->n_property_specs);
  class->property_specs[i] = pspec;
  if (pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY))
    class->construct_properties = g_slist_prepend (class->construct_properties, pspec);

  /* for property overrides of construct poperties, we have to get rid
   * of the overidden inherited construct property
   */
  pspec = g_param_spec_pool_lookup (pspec_pool, pspec->name, g_type_parent (G_OBJECT_CLASS_TYPE (class)), TRUE, NULL);
  if (pspec && pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY))
    class->construct_properties = g_slist_remove (class->construct_properties, pspec);
}

GParamSpec*
g_object_class_find_property (GObjectClass *class,
			      const gchar  *property_name)
{
  g_return_val_if_fail (G_IS_OBJECT_CLASS (class), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);
  
  return g_param_spec_pool_lookup (pspec_pool,
				   property_name,
				   G_OBJECT_CLASS_TYPE (class),
				   TRUE, NULL);
}

static void
free_notify_queue (gpointer data)
{
  NotifyQueue *nqueue = data;
  
  g_slist_free (nqueue->pspecs);
  g_free (nqueue);
}

static inline NotifyQueue*
object_freeze_notifies (GObject *object)
{
  NotifyQueue *nqueue;

  nqueue = g_object_get_qdata (object, quark_notify_queue);
  if (!nqueue)
    {
      nqueue = g_new0 (NotifyQueue, 1);
      g_object_set_qdata_full (object, quark_notify_queue, nqueue, free_notify_queue);
    }
  nqueue->freeze_count++;

  return nqueue;
}

static inline void
object_queue_property (GObject     *object,
		       GParamSpec  *pspec,
		       NotifyQueue *nqueue)
{
  /* we will dedup later */
  nqueue->pspecs = g_slist_prepend (nqueue->pspecs, pspec);
  nqueue->n_pspecs++;
}

static void
g_object_init (GObject *object)
{
  object->ref_count = 1;
  g_datalist_init (&object->qdata);
  
  /* freeze object's notification queue, g_object_new_valist() takes care of that */
  object_freeze_notifies (object);

#ifdef	DEBUG_OBJECTS
  if (glib_debug_objects)
    {
      if (!debug_objects_ht)
	debug_objects_ht = g_hash_table_new (g_direct_hash, NULL);
      debug_objects_count++;
      g_hash_table_insert (debug_objects_ht, object, object);
    }
#endif /* DEBUG_OBJECTS */
}

static void
g_object_do_set_property (GObject      *object,
			  guint         property_id,
			  const GValue *value,
			  GParamSpec   *pspec,
			  const gchar  *trailer)
{
  guint i = 0;
  
  switch (property_id)
    {
      gboolean swapped, after;
      gpointer callback, data;
    case PROP_DATA:
      g_return_if_fail (trailer != NULL);

      g_object_set_data (object, trailer, g_value_get_pointer (value));
      break;
    case PROP_SWAPPED_SIGNAL_AFTER:
      i++;
    case PROP_SIGNAL_AFTER:
      i++;
    case PROP_SWAPPED_SIGNAL:
      i++;
    case PROP_SIGNAL:
      after = i > 2;
      swapped = i & 1;
      g_return_if_fail (trailer != NULL);

      g_value_get_ccallback (value, &callback, &data);
      g_signal_connect_data (object, trailer,
			     callback, data, NULL,
			     swapped, after);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
g_object_do_get_property (GObject     *object,
			  guint        property_id,
			  GValue      *value,
			  GParamSpec  *pspec,
			  const gchar *trailer)
{
  switch (property_id)
    {
    case PROP_DATA:
      g_return_if_fail (trailer != NULL);

      g_value_set_pointer (value, g_object_get_data (object, trailer));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
g_object_last_unref (GObject *object)
{
  g_return_if_fail (object->ref_count > 0);
  
  if (object->ref_count == 1)	/* may have been re-referenced meanwhile */
    G_OBJECT_GET_CLASS (object)->shutdown (object);
  
  object->ref_count -= 1;
  
  if (object->ref_count == 0)	/* may have been re-referenced meanwhile */
    {
      G_OBJECT_GET_CLASS (object)->finalize (object);
#ifdef  DEBUG_OBJECTS
      if (glib_debug_objects && debug_objects_ht)
	g_assert (g_hash_table_lookup (debug_objects_ht, object) == NULL);
#endif /* DEBUG_OBJECTS */
      g_type_free_instance ((GTypeInstance*) object);
    }
}

static void
g_object_shutdown (GObject *object)
{
  /* this function needs to be always present for unconditional
   * chaining, we also might add some code here later.
   * beware though, subclasses may invoke shutdown() arbitrarily.
   */
}

static void
g_object_finalize (GObject *object)
{
  g_signal_handlers_destroy (object);
  g_datalist_clear (&object->qdata);
  
#ifdef	DEBUG_OBJECTS
  if (glib_debug_objects)
    {
      g_assert (g_hash_table_lookup (debug_objects_ht, object) == object);
      
      g_hash_table_remove (debug_objects_ht, object);
      debug_objects_count--;
    }
#endif /* DEBUG_OBJECTS */
}

static inline void
object_thaw_notifies (GObject     *object,
		      NotifyQueue *nqueue)
{
  GParamSpec **pspecs;
  GSList *slist;
  guint n_pspecs = 0;
  
  nqueue->freeze_count--;
  if (nqueue->freeze_count)
    return;
  g_return_if_fail (object->ref_count > 0);
  
  pspecs = g_new (GParamSpec*, nqueue->n_pspecs);
  for (slist = nqueue->pspecs; slist; slist = slist->next)
    {
      GParamSpec *pspec = slist->data;
      gint i = 0;
      
      /* dedup, make pspecs in the list unique */
    redo_dedup_check:
      if (pspecs[i] == pspec)
	continue;
      if (++i < n_pspecs)
	goto redo_dedup_check;
      
      pspecs[n_pspecs++] = pspec;
    }
  g_object_set_qdata (object, quark_notify_queue, NULL);
  
  if (n_pspecs)
    G_OBJECT_GET_CLASS (object)->dispatch_properties_changed (object, n_pspecs, pspecs);
  
  g_free (pspecs);
}

static void
g_object_dispatch_properties_changed (GObject     *object,
				      guint        n_pspecs,
				      GParamSpec **pspecs)
{
  g_signal_emit (object, gobject_signals[PROPERTIES_CHANGED], 0, n_pspecs, pspecs);
}

static void
g_object_properties_changed (GObject     *object,
			     guint        n_pspecs,
			     GParamSpec **pspecs)
{
  guint i;

  for (i = 0; i < n_pspecs; i++)
    g_signal_emit (object, gobject_signals[NOTIFY], g_quark_from_string (pspecs[i]->name), pspecs[i]);
}

static void
g_object_notify_property_changed (GObject    *object,
				  GParamSpec *pspec)
{
  if (0) /* FIXME */
    g_message ("NOTIFICATION: property `%s' changed on object `%s'",
	       pspec->name,
	       G_OBJECT_TYPE_NAME (object));
}

void
g_object_freeze_notify (GObject *object)
{
  g_return_if_fail (G_IS_OBJECT (object));
  if (!object->ref_count)
    return;

  g_object_ref (object);
  object_freeze_notifies (object);
  g_object_unref (object);
}

void
g_object_notify (GObject     *object,
		 const gchar *property_name)
{
  GParamSpec *pspec;
  
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (property_name != NULL);
  if (!object->ref_count)
    return;
  
  g_object_ref (object);
  pspec = g_param_spec_pool_lookup (pspec_pool,
				    property_name,
				    G_OBJECT_TYPE (object),
				    TRUE, NULL);
  if (!pspec)
    g_warning ("%s: object class `%s' has no property named `%s'",
	       G_STRLOC,
	       G_OBJECT_TYPE_NAME (object),
	       property_name);
  else
    {
      NotifyQueue *nqueue = object_freeze_notifies (object);

      object_queue_property (object, pspec, nqueue);
      object_thaw_notifies (object, nqueue);
    }
  g_object_unref (object);
}

void
g_object_thaw_notify (GObject *object)
{
  NotifyQueue *nqueue;
  
  g_return_if_fail (G_IS_OBJECT (object));
  if (!object->ref_count)
    return;
  
  g_object_ref (object);
  nqueue = g_object_get_qdata (object, quark_notify_queue);
  if (!nqueue || !nqueue->freeze_count)
    g_warning (G_STRLOC ": property-changed notification for %s(%p) is not frozen",
	       G_OBJECT_TYPE_NAME (object), object);
  else
    object_thaw_notifies (object, nqueue);
  g_object_unref (object);
}

static inline void
object_get_property (GObject     *object,
		     GValue      *value,
		     GParamSpec  *pspec,
		     const gchar *trailer)
{
  GObjectClass *class;
  
  g_return_if_fail (g_type_is_a (G_OBJECT_TYPE (object), pspec->owner_type));	/* paranoid */
  
  class = g_type_class_peek (pspec->owner_type);
  
  class->get_property (object, PARAM_SPEC_PARAM_ID (pspec), value, pspec, trailer);
}

static inline void
object_set_property (GObject     *object,
		     GValue      *value,
		     GParamSpec  *pspec,
		     const gchar *trailer,
		     NotifyQueue *nqueue)
{
  GObjectClass *class;
  
  g_return_if_fail (g_type_is_a (G_OBJECT_TYPE (object), pspec->owner_type));	/* paranoid */
  
  class = g_type_class_peek (pspec->owner_type);
  
  class->set_property (object, PARAM_SPEC_PARAM_ID (pspec), value, pspec, trailer);
  object_queue_property (object, pspec, nqueue);
}

gpointer
g_object_new (GType	   object_type,
	      const gchar *first_property_name,
	      ...)
{
  GObject *object;
  va_list var_args;
  
  g_return_val_if_fail (G_TYPE_IS_OBJECT (object_type), NULL);
  
  va_start (var_args, first_property_name);
  object = g_object_new_valist (object_type, first_property_name, var_args);
  va_end (var_args);
  
  return object;
}

gpointer
g_object_new_valist (GType	  object_type,
		     const gchar *first_property_name,
		     va_list	  var_args)
{
  NotifyQueue *nqueue;
  GObject *object;
  GObjectClass *class;
  const gchar *name;
  GObjectConstructParam *cparams = NULL, *nparams = NULL;
  guint n_cparams = 0, n_nparams = 0;
  GSList *clist;
  
  g_return_val_if_fail (G_TYPE_IS_OBJECT (object_type), NULL);

  class = g_type_class_ref (object_type);
  clist = g_slist_copy (class->construct_properties);

  /* collect parameters, sort into construction and normal ones */
  name = first_property_name;
  while (name)
    {
      const gchar *trailer = NULL;
      GValue *value;
      GParamSpec *pspec;
      gchar *error = NULL;
      
      pspec = g_param_spec_pool_lookup (pspec_pool,
					name,
					object_type,
					TRUE,
					&trailer);
      if (!pspec)
	{
	  g_warning ("%s: object class `%s' has no property named `%s'",
		     G_STRLOC,
		     g_type_name (object_type),
		     name);
	  break;
	}
      if (!(pspec->flags & G_PARAM_WRITABLE))
	{
	  g_warning ("%s: property `%s' of object class `%s' is not writable",
		     G_STRLOC,
		     pspec->name,
		     g_type_name (object_type));
	  break;
	}

      value = g_new (GValue, 1);
      value->g_type = 0;
      g_value_init (value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      G_VALUE_COLLECT (value, var_args, &error);
      if (error)
	{
	  g_warning ("%s: %s", G_STRLOC, error);
	  g_free (error);

	  /* we purposely leak the value here, it might not be
	   * in a sane state if an error condition occoured
	   */
	  break;
	}
      if (pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY))
	{
	  guint i;

	  if (!n_cparams || n_cparams >= PREALLOC_CPARAMS)
	    cparams = g_renew (GObjectConstructParam, cparams, MAX (n_cparams + 1, PREALLOC_CPARAMS));
	  cparams[n_cparams].pspec = pspec;
	  cparams[n_cparams].value = value;
	  cparams[n_cparams].trailer = trailer;
	  for (i = 0; i < n_cparams; i++)	/* picky, aren't we? ;) */
	    if (cparams[i].pspec == pspec)
	      g_warning (G_STRLOC ": construct property \"%s\" for object `%s' is being set twice",
			 pspec->name, g_type_name (object_type));
	  n_cparams++;
	  clist = g_slist_remove (clist, pspec);   /* FIXME: unique */
	}
      else
	{
	  if (!n_nparams || n_nparams >= PREALLOC_CPARAMS)
	    nparams = g_renew (GObjectConstructParam, nparams, MAX (n_nparams + 1, PREALLOC_CPARAMS));
	  nparams[n_nparams].pspec = pspec;
	  nparams[n_nparams].value = value;
	  nparams[n_nparams].trailer = trailer;
	  n_nparams++;
	}

      name = va_arg (var_args, gchar*);
    }

  /* construct object from construction parameters */
  while (clist)
    {
      GSList *tmp = clist->next;
      GParamSpec *pspec = clist->data;
      GValue *value = g_new (GValue, 1);

      value->g_type = 0;
      g_value_init (value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      g_param_value_set_default (pspec, value);

      if (!n_cparams || n_cparams >= PREALLOC_CPARAMS)
	cparams = g_renew (GObjectConstructParam, cparams, MAX (n_cparams + 1, PREALLOC_CPARAMS));
      cparams[n_cparams].pspec = pspec;
      cparams[n_cparams].value = value;
      cparams[n_cparams].trailer = NULL;
      n_cparams++;

      g_slist_free_1 (clist);
      clist = tmp;
    }
  object = class->constructor (object_type, n_cparams, cparams);

  /* free construction values */
  while (n_cparams--)
    {
      g_value_unset (cparams[n_cparams].value);
      g_free (cparams[n_cparams].value);
    }
  g_free (cparams);
  
  /* release g_object_init() notification queue freeze_count */
  nqueue = object_freeze_notifies (object);
  nqueue->freeze_count--;
  
  /* set remaining properties */
  cparams = nparams;
  while (n_nparams--)
    {
      GValue *value = nparams->value;
      GParamSpec *pspec = nparams->pspec;
      const gchar *trailer = nparams++->trailer;

      /* convert if necessary */
      if (!g_type_is_a (G_VALUE_TYPE (value), G_PARAM_SPEC_VALUE_TYPE (pspec)))
	{
	  GValue tmp_value = { 0, };

	  g_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
	  if (!g_value_convert (value, &tmp_value) ||
	      g_param_value_validate (pspec, &tmp_value))
	    g_warning ("%s: cannot convert `%s' value to property `%s' value of type `%s'",
		       G_STRLOC,
		       G_VALUE_TYPE_NAME (value),
		       pspec->name,
		       g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
	  else
	    object_set_property (object, &tmp_value, pspec, trailer, nqueue);
	  g_value_unset (&tmp_value);
	}
      else
	object_set_property (object, value, pspec, trailer, nqueue);
      
      g_value_unset (value);
      g_free (value);
    }
  g_free (cparams);

  g_type_class_unref (class);

  /* release our own freeze count and handle notifications */
  object_thaw_notifies (object, nqueue);
  
  return object;
}

static GObject*
g_object_constructor (GType                  type,
		      guint                  n_construct_properties,
		      GObjectConstructParam *construct_params)
{
  GObject *object;

  /* create object */
  object = (GObject*) g_type_create_instance (type);

  /* set construction parameters */
  if (n_construct_properties)
    {
      NotifyQueue *nqueue = object_freeze_notifies (object);
      
      /* set construct properties */
      while (n_construct_properties--)
	{
	  GValue *value = construct_params->value;
	  GParamSpec *pspec = construct_params->pspec;
	  const gchar *trailer = construct_params++->trailer;
	  
	  /* convert if necessary */
	  if (!g_type_is_a (G_VALUE_TYPE (value), G_PARAM_SPEC_VALUE_TYPE (pspec)))
	    {
	      GValue tmp_value = { 0, };
	      
	      g_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
	      if (!g_value_convert (value, &tmp_value) ||
		  g_param_value_validate (pspec, &tmp_value))
		g_warning ("%s: cannot convert `%s' value to property `%s' value of type `%s'",
			   G_STRLOC,
			   G_VALUE_TYPE_NAME (value),
			   pspec->name,
			   g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
	      else
		object_set_property (object, &tmp_value, pspec, trailer, nqueue);
	      g_value_unset (&tmp_value);
	    }
	  else
	    object_set_property (object, value, pspec, trailer, nqueue);
	}
      nqueue->freeze_count--;
      /* the notification queue is still frozen from g_object_init(), so
       * we don't need to handle it here, g_object_new_valist() takes
       * care of that
       */
    }

  return object;
}

void
g_object_set_valist (GObject	 *object,
		     const gchar *first_property_name,
		     va_list	  var_args)
{
  NotifyQueue *nqueue;
  const gchar *name;
  
  g_return_if_fail (G_IS_OBJECT (object));
  
  g_object_ref (object);
  nqueue = object_freeze_notifies (object);
  
  name = first_property_name;
  while (name)
    {
      const gchar *trailer = NULL;
      GValue value = { 0, };
      GParamSpec *pspec;
      gchar *error = NULL;
      
      pspec = g_param_spec_pool_lookup (pspec_pool,
					name,
					G_OBJECT_TYPE (object),
					TRUE,
					&trailer);
      if (!pspec)
	{
	  g_warning ("%s: object class `%s' has no property named `%s'",
		     G_STRLOC,
		     G_OBJECT_TYPE_NAME (object),
		     name);
	  break;
	}
      if (!(pspec->flags & G_PARAM_WRITABLE))
	{
	  g_warning ("%s: property `%s' of object class `%s' is not writable",
		     G_STRLOC,
		     pspec->name,
		     G_OBJECT_TYPE_NAME (object));
	  break;
	}
      
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      
      G_VALUE_COLLECT (&value, var_args, &error);
      if (error)
	{
	  g_warning ("%s: %s", G_STRLOC, error);
	  g_free (error);
	  
	  /* we purposely leak the value here, it might not be
	   * in a sane state if an error condition occoured
	   */
	  break;
	}
      
      object_set_property (object, &value, pspec, trailer, nqueue);
      
      g_value_unset (&value);
      
      name = va_arg (var_args, gchar*);
    }

  object_thaw_notifies (object, nqueue);
  g_object_unref (object);
}

void
g_object_get_valist (GObject	 *object,
		     const gchar *first_property_name,
		     va_list	  var_args)
{
  const gchar *name;
  
  g_return_if_fail (G_IS_OBJECT (object));
  
  g_object_ref (object);
  
  name = first_property_name;
  
  while (name)
    {
      const gchar *trailer = NULL;
      GValue value = { 0, };
      GParamSpec *pspec;
      gchar *error;
      
      pspec = g_param_spec_pool_lookup (pspec_pool,
					name,
					G_OBJECT_TYPE (object),
					TRUE,
					&trailer);
      if (!pspec)
	{
	  g_warning ("%s: object class `%s' has no property named `%s'",
		     G_STRLOC,
		     G_OBJECT_TYPE_NAME (object),
		     name);
	  break;
	}
      if (!(pspec->flags & G_PARAM_READABLE))
	{
	  g_warning ("%s: property `%s' of object class `%s' is not readable",
		     G_STRLOC,
		     pspec->name,
		     G_OBJECT_TYPE_NAME (object));
	  break;
	}
      
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      
      object_get_property (object, &value, pspec, trailer);
      
      G_VALUE_LCOPY (&value, var_args, &error);
      if (error)
	{
	  g_warning ("%s: %s", G_STRLOC, error);
	  g_free (error);
	  
	  /* we purposely leak the value here, it might not be
	   * in a sane state if an error condition occoured
	   */
	  break;
	}
      
      g_value_unset (&value);
      
      name = va_arg (var_args, gchar*);
    }
  
  g_object_unref (object);
}

void
g_object_set (gpointer     _object,
	      const gchar *first_property_name,
	      ...)
{
  GObject *object = _object;
  va_list var_args;
  
  g_return_if_fail (G_IS_OBJECT (object));
  
  va_start (var_args, first_property_name);
  g_object_set_valist (object, first_property_name, var_args);
  va_end (var_args);
}

void
g_object_get (gpointer     _object,
	      const gchar *first_property_name,
	      ...)
{
  GObject *object = _object;
  va_list var_args;
  
  g_return_if_fail (G_IS_OBJECT (object));
  
  va_start (var_args, first_property_name);
  g_object_get_valist (object, first_property_name, var_args);
  va_end (var_args);
}

void
g_object_set_property (GObject	    *object,
		       const gchar  *property_name,
		       const GValue *value)
{
  NotifyQueue *nqueue;
  GParamSpec *pspec;
  const gchar *trailer;
  
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));
  
  g_object_ref (object);
  nqueue = object_freeze_notifies (object);
  
  pspec = g_param_spec_pool_lookup (pspec_pool,
				    property_name,
				    G_OBJECT_TYPE (object),
				    TRUE,
				    &trailer);
  if (!pspec)
    g_warning ("%s: object class `%s' has no property named `%s'",
	       G_STRLOC,
	       G_OBJECT_TYPE_NAME (object),
	       property_name);
  else
    {
      GValue tmp_value = { 0, };
      
      /* provide a copy to work from and convert if necessary */
      g_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      
      if (!g_value_convert (value, &tmp_value) ||
	  g_param_value_validate (pspec, &tmp_value))
	g_warning ("%s: cannot convert `%s' value to property `%s' value of type `%s'",
		   G_STRLOC,
		   G_VALUE_TYPE_NAME (value),
		   pspec->name,
		   g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
      else
	object_set_property (object, &tmp_value, pspec, trailer, nqueue);
      
      g_value_unset (&tmp_value);
    }
  
  object_thaw_notifies (object, nqueue);
  g_object_unref (object);
}

void
g_object_get_property (GObject	   *object,
		       const gchar *property_name,
		       GValue	   *value)
{
  GParamSpec *pspec;
  const gchar *trailer;
  
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));
  
  g_object_ref (object);
  
  pspec = g_param_spec_pool_lookup (pspec_pool,
				    property_name,
				    G_OBJECT_TYPE (object),
				    TRUE,
				    &trailer);
  if (!pspec)
    g_warning ("%s: object class `%s' has no property named `%s'",
	       G_STRLOC,
	       G_OBJECT_TYPE_NAME (object),
	       property_name);
  else
    {
      GValue tmp_value = { 0, };
      
      /* provide a copy to work from and later convert if necessary, so
       * _get_property() implementations need *not* care about freeing values
       * that might be already set in the property to get.
       * (though, at this point, GValue should exclusively be modified
       * through the accessor functions anyways)
       */
      g_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      
      if (!g_value_types_exchangable (G_VALUE_TYPE (value), G_PARAM_SPEC_VALUE_TYPE (pspec)))
	g_warning ("%s: can't retrive property `%s' value of type `%s' as value of type `%s'",
		   G_STRLOC,
		   pspec->name,
		   g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
		   G_VALUE_TYPE_NAME (value));
      else
	{
	  object_get_property (object, &tmp_value, pspec, trailer);
	  g_value_convert (&tmp_value, value);
	  /* g_value_validate (value, pspec); */
	}
      
      g_value_unset (&tmp_value);
    }
  
  g_object_unref (object);
}

gpointer
g_object_ref (gpointer _object)
{
  GObject *object = _object;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (object->ref_count > 0, NULL);
  
  object->ref_count += 1;
  
  return object;
}

void
g_object_unref (gpointer _object)
{
  GObject *object = _object;

  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (object->ref_count > 0);
  
  if (object->ref_count > 1)
    object->ref_count -= 1;
  else
    g_object_last_unref (object);
}

gpointer
g_object_get_qdata (GObject *object,
		    GQuark   quark)
{
  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  
  return quark ? g_datalist_id_get_data (&object->qdata, quark) : NULL;
}

void
g_object_set_qdata (GObject *object,
		    GQuark   quark,
		    gpointer data)
{
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (quark > 0);
  
  g_datalist_id_set_data (&object->qdata, quark, data);
}

void
g_object_set_qdata_full (GObject       *object,
			 GQuark		quark,
			 gpointer	data,
			 GDestroyNotify destroy)
{
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (quark > 0);
  
  g_datalist_id_set_data_full (&object->qdata, quark, data, data ? destroy : NULL);
}

gpointer
g_object_steal_qdata (GObject *object,
		      GQuark   quark)
{
  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (quark > 0, NULL);
  
  return g_datalist_id_remove_no_notify (&object->qdata, quark);
}

gpointer
g_object_get_data (GObject     *object,
                   const gchar *key)
{
  GQuark quark;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (key != NULL, NULL);

  quark = g_quark_try_string (key);

  return quark ? g_datalist_id_get_data (&object->qdata, quark) : NULL;
}

void
g_object_set_data (GObject     *object,
                   const gchar *key,
                   gpointer     data)
{
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (key != NULL);

  g_datalist_id_set_data (&object->qdata, g_quark_from_string (key), data);
}

void
g_object_set_data_full (GObject       *object,
                        const gchar   *key,
                        gpointer       data,
                        GDestroyNotify destroy)
{
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (key != NULL);

  g_datalist_id_set_data_full (&object->qdata, g_quark_from_string (key), data, data ? destroy : NULL);
}

gpointer
g_object_steal_data (GObject     *object,
                     const gchar *key)
{
  GQuark quark;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (key != NULL, NULL);

  quark = g_quark_try_string (key);

  return quark ? g_datalist_id_remove_no_notify (&object->qdata, quark) : NULL;
}

static void
g_value_object_init (GValue *value)
{
  value->data[0].v_pointer = NULL;
}

static void
g_value_object_free_value (GValue *value)
{
  if (value->data[0].v_pointer)
    g_object_unref (value->data[0].v_pointer);
}

static void
g_value_object_copy_value (const GValue *src_value,
			   GValue	*dest_value)
{
  if (src_value->data[0].v_pointer)
    dest_value->data[0].v_pointer = g_object_ref (src_value->data[0].v_pointer);
  else
    dest_value->data[0].v_pointer = NULL;
}

static gpointer
g_value_object_peek_pointer (const GValue *value)
{
  return value->data[0].v_pointer;
}

static gchar*
g_value_object_collect_value (GValue	  *value,
			      guint	   nth_value,
			      GType	  *collect_type,
			      GTypeCValue *collect_value)
{
  if (collect_value->v_pointer)
    {
      GObject *object = collect_value->v_pointer;
      
      if (object->g_type_instance.g_class == NULL)
	return g_strconcat ("invalid unclassed object pointer for value type `",
			    G_VALUE_TYPE_NAME (value),
			    "'",
			    NULL);
      else if (!g_type_is_a (G_OBJECT_TYPE (object), G_VALUE_TYPE (value)))
	return g_strconcat ("invalid object type `",
			    G_OBJECT_TYPE_NAME (object),
			    "' for value type `",
			    G_VALUE_TYPE_NAME (value),
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
g_value_object_lcopy_value (const GValue *value,
			    guint	  nth_value,
			    GType	 *collect_type,
			    GTypeCValue  *collect_value)
{
  GObject **object_p = collect_value->v_pointer;
  
  if (!object_p)
    return g_strdup_printf ("value location for `%s' passed as NULL", G_VALUE_TYPE_NAME (value));
  
  *object_p = value->data[0].v_pointer ? g_object_ref (value->data[0].v_pointer) : NULL;
  
  *collect_type = 0;
  return NULL;
}

void
g_value_set_object (GValue  *value,
		    GObject *v_object)
{
  g_return_if_fail (G_IS_VALUE_OBJECT (value));
  
  if (value->data[0].v_pointer)
    {
      g_object_unref (value->data[0].v_pointer);
      value->data[0].v_pointer = NULL;
    }

  if (v_object)
    {
      g_return_if_fail (G_IS_OBJECT (v_object));
      g_return_if_fail (g_type_is_a (G_OBJECT_TYPE (v_object), G_VALUE_TYPE (value)));

      value->data[0].v_pointer = v_object;
      g_object_ref (value->data[0].v_pointer);
    }
}

GObject*
g_value_get_object (const GValue *value)
{
  g_return_val_if_fail (G_IS_VALUE_OBJECT (value), NULL);
  
  return value->data[0].v_pointer;
}

GObject*
g_value_dup_object (const GValue *value)
{
  g_return_val_if_fail (G_IS_VALUE_OBJECT (value), NULL);
  
  return value->data[0].v_pointer ? g_object_ref (value->data[0].v_pointer) : NULL;
}

guint
g_signal_connect_object (gpointer     instance,
			 const gchar *detailed_signal,
			 GCallback    c_handler,
			 gpointer     gobject,
			 gboolean     swapped,
			 gboolean     after)
{
  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE (instance), 0);
  g_return_val_if_fail (detailed_signal != NULL, 0);
  g_return_val_if_fail (c_handler != NULL, 0);

  if (gobject)
    {
      GClosure *closure;

      g_return_val_if_fail (G_IS_OBJECT (gobject), 0);

      closure = (swapped ? g_cclosure_new_object_swap : g_cclosure_new_object) (c_handler, gobject);

      return g_signal_connect_closure (instance, detailed_signal, closure, after);
    }
  else
    return g_signal_connect_data (instance, detailed_signal, c_handler, NULL, NULL, swapped, after);
}

typedef struct {
  GObject  *object;
  guint     n_closures;
  GClosure *closures[1]; /* flexible array */
} CArray;

static void
object_remove_closure (gpointer  data,
		       GClosure *closure)
{
  GObject *object = data;
  CArray *carray = g_object_get_qdata (object, quark_closure_array);
  guint i;
  
  for (i = 0; i < carray->n_closures; i++)
    if (carray->closures[i] == closure)
      {
	carray->n_closures--;
	if (i < carray->n_closures)
	  carray->closures[i] = carray->closures[carray->n_closures];
	return;
      }
  g_assert_not_reached ();
}

static void
destroy_closure_array (gpointer data)
{
  CArray *carray = data;
  GObject *object = carray->object;
  guint i, n = carray->n_closures;
  
  for (i = 0; i < n; i++)
    {
      GClosure *closure = carray->closures[i];
      
      /* removing object_remove_closure() upfront is probably faster than
       * letting it fiddle with quark_closure_array which is empty anyways
       */
      g_closure_remove_inotify (closure, object, object_remove_closure);
      g_closure_invalidate (closure);
    }
  g_free (carray);
}

void
g_object_watch_closure (GObject  *object,
			GClosure *closure)
{
  CArray *carray;
  
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (closure != NULL);
  g_return_if_fail (closure->is_invalid == FALSE);
  g_return_if_fail (closure->in_marshal == FALSE);
  g_return_if_fail (object->ref_count > 0);	/* this doesn't work on finalizing objects */
  
  g_closure_add_inotify (closure, object, object_remove_closure);
  g_closure_add_marshal_guards (closure,
				object, (GClosureNotify) g_object_ref,
				object, (GClosureNotify) g_object_unref);
  carray = g_object_steal_qdata (object, quark_closure_array);
  if (!carray)
    {
      carray = g_renew (CArray, NULL, 1);
      carray->object = object;
      carray->n_closures = 1;
      carray->closures[0] = closure;
      g_object_set_qdata_full (object, quark_closure_array, carray, destroy_closure_array);
    }
  else
    {
      guint i = carray->n_closures++;
      
      carray = g_realloc (carray, sizeof (*carray) + sizeof (carray->closures[0]) * i);
      carray->closures[i] = closure;
      g_object_set_qdata_full (object, quark_closure_array, carray, destroy_closure_array);
    }
}

GClosure*
g_closure_new_object (guint    sizeof_closure,
		      GObject *object)
{
  GClosure *closure;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (object->ref_count > 0, NULL);     /* this doesn't work on finalizing objects */

  closure = g_closure_new_simple (sizeof_closure, object);
  g_object_watch_closure (object, closure);

  return closure;
}

GClosure*
g_cclosure_new_object (GCallback callback_func,
		       gpointer  _object)
{
  GObject *object = _object;
  GClosure *closure;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (object->ref_count > 0, NULL);     /* this doesn't work on finalizing objects */
  g_return_val_if_fail (callback_func != NULL, NULL);

  closure = g_cclosure_new (callback_func, object, NULL);
  g_object_watch_closure (object, closure);

  return closure;
}

GClosure*
g_cclosure_new_object_swap (GCallback callback_func,
			    gpointer  _object)
{
  GObject *object = _object;
  GClosure *closure;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (object->ref_count > 0, NULL);     /* this doesn't work on finalizing objects */
  g_return_val_if_fail (callback_func != NULL, NULL);

  closure = g_cclosure_new_swap (callback_func, object, NULL);
  g_object_watch_closure (object, closure);

  return closure;
}
