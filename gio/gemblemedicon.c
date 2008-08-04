/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* GIO - GLib Input, Output and Streaming Library
 * 
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 * Author: Matthias Clasen <mclasen@redhat.com>
 *         Clemens N. Buss <cebuzz@gmail.com>
 */

#include <config.h>

#include <string.h>

#include "gemblemedicon.h"
#include "glibintl.h"

#include "gioalias.h"

/**
 * SECTION:gemblemedicon
 * @short_description: Icon with emblems
 * @include: gio/gio.h
 * @see_also: #GIcon, #GLoadableIcon, #GThemedIcon, #GEmblem
 *
 * #GEmblemedIcon is an implementation of #GIcon that supports
 * adding an emblem to an icon. Adding multiple emblems to an
 * icon is ensured via g_emblemed_icon_add_emblem(). 
 *
 * Note that #GEmblemedIcon allows no control over the position
 * of the emblems. See also #GEmblem for more information.
 **/

static void g_emblemed_icon_icon_iface_init (GIconIface *iface);

struct _GEmblemedIcon
{
  GObject parent_instance;

  GIcon *icon;
  GList *emblems;
};

struct _GEmblemedIconClass
{
  GObjectClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (GEmblemedIcon, g_emblemed_icon, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ICON,
                         g_emblemed_icon_icon_iface_init))


static void
g_emblemed_icon_finalize (GObject *object)
{
  GEmblemedIcon *emblemed;

  emblemed = G_EMBLEMED_ICON (object);

  g_object_unref (emblemed->icon);
  g_list_foreach (emblemed->emblems, (GFunc) g_object_unref, NULL);

  (*G_OBJECT_CLASS (g_emblemed_icon_parent_class)->finalize) (object);
}

static void
g_emblemed_icon_class_init (GEmblemedIconClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = g_emblemed_icon_finalize;
}

static void
g_emblemed_icon_init (GEmblemedIcon *emblemed)
{
}

/**
 * g_emblemed_icon_new:
 * @icon: a #GIcon
 * @emblem: a #GEmblem
 *
 * Creates a new emblemed icon for @icon with the emblem @emblem.
 *
 * Returns: a new #GIcon
 *
 * Since: 2.18
 **/
GIcon *
g_emblemed_icon_new (GIcon   *icon,
                     GEmblem *emblem)
{
  GEmblemedIcon *emblemed;
  
  g_return_val_if_fail (G_IS_ICON (icon), NULL);
  g_return_val_if_fail (!G_IS_EMBLEM (icon), NULL);
  g_return_val_if_fail (G_IS_EMBLEM (emblem), NULL);

  emblemed = G_EMBLEMED_ICON (g_object_new (G_TYPE_EMBLEMED_ICON, NULL));
  emblemed->icon = g_object_ref (icon);
  
  g_emblemed_icon_add_emblem (emblemed, emblem);

  return G_ICON (emblemed);
}


/**
 * g_emblemed_icon_get_icon:
 * @emblemed: a #GEmblemedIcon
 *
 * Gets the main icon for @emblemed.
 *
 * Returns: a #GIcon that is owned by @emblemed
 *
 * Since: 2.18
 **/
GIcon *
g_emblemed_icon_get_icon (GEmblemedIcon *emblemed)
{
  g_return_val_if_fail (G_IS_EMBLEMED_ICON (emblemed), NULL);

  return emblemed->icon;
}

/**
 * g_emblemed_icon_get_emblems:
 * @emblemed: a #GEmblemedIcon
 *
 * Gets the list of emblems for the @icon.
 *
 * Returns: a #GList of #GEmblem <!-- -->s that is owned by @emblemed
 *
 * Since: 2.18
 **/

GList *
g_emblemed_icon_get_emblems (GEmblemedIcon *emblemed)
{
  g_return_val_if_fail (G_IS_EMBLEMED_ICON (emblemed), NULL);

  return emblemed->emblems;
}


/**
 * g_emblemed_icon_add_emblem:
 * @emblemed: a #GEmblemedIcon
 * @emblem: a #GEmblem
 *
 * Adds @emblem to the #GList of #GEmblem <!-- -->s.
 *
 * Returns: a #GList of #GEmblem <!-- -->s that is owned by @emblemed
 *
 * Since: 2.18
 **/
void 
g_emblemed_icon_add_emblem (GEmblemedIcon *emblemed,
                            GEmblem       *emblem)
{
  g_return_if_fail (G_IS_EMBLEMED_ICON (emblemed));
  g_return_if_fail (G_IS_EMBLEM (emblem));

  g_object_ref (emblem);
  emblemed->emblems = g_list_append (emblemed->emblems, emblem);
}

static guint
g_emblemed_icon_hash (GIcon *icon)
{
  GEmblemedIcon *emblemed = G_EMBLEMED_ICON (icon);
  GList *list;
  guint hash = g_icon_hash (emblemed->icon);

  for (list = emblemed->emblems; list != NULL; list = list->next)
    hash ^= g_icon_hash (G_ICON (list->data));

  return hash;
}

static gint
g_emblem_comp (GEmblem *a,
               GEmblem *b)
{
  guint hash_a = g_icon_hash (G_ICON (a));
  guint hash_b = g_icon_hash (G_ICON (b));

  if(hash_a < hash_b)
    return -1;

  if(hash_a == hash_b)
    return 0;

  return 1;
}

static gboolean
g_emblemed_icon_equal (GIcon *icon1,
                       GIcon *icon2)
{
  GEmblemedIcon *emblemed1 = G_EMBLEMED_ICON (icon1);
  GEmblemedIcon *emblemed2 = G_EMBLEMED_ICON (icon2);
  GList *list1, *list2;

  if (!g_icon_equal (emblemed1->icon, emblemed2->icon))
    return FALSE;

  list1 = emblemed1->emblems;
  list2 = emblemed2->emblems;

  list1 = g_list_sort (list1, (GCompareFunc) g_emblem_comp);
  list2 = g_list_sort (list2, (GCompareFunc) g_emblem_comp);

  while (list1 && list2)
  {
    if (!g_icon_equal (G_ICON (list1->data), G_ICON (list2->data)))
        return FALSE;
    
    list1 = list1->next;
    list2 = list2->next;
  }
  
  return list1 == NULL && list2 == NULL;
}

static void
g_emblemed_icon_icon_iface_init (GIconIface *iface)
{
  iface->hash = g_emblemed_icon_hash;
  iface->equal = g_emblemed_icon_equal;
}

#define __G_EMBLEMED_ICON_C__
#include "gioaliasdef.c"
