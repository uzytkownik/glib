/* GMODULE - GLIB wrapper code for dynamic module loading
 * Copyright (C) 1998 Tim Janik  
 *
 * BeOS GMODULE implementation
 * Copyright (C) 1999 Richard Offer and Shawn T. Amundson (amundson@gtk.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* 
 * MT safe
 */

#include <be/kernel/image.h> /* image (aka DSO) handling functions... */

/*
 * The BeOS doesn't use the same symantics as Unix's dlopen....
 * 
 */
#ifndef	RTLD_GLOBAL
#define	RTLD_GLOBAL	0
#endif	/* RTLD_GLOBAL */
#ifndef	RTLD_LAZY
#define	RTLD_LAZY	1
#endif	/* RTLD_LAZY */
#ifndef	RTLD_NOW
#define	RTLD_NOW	0
#endif	/* RTLD_NOW */


/*
 * Points to Ponder
 * 
 * You can load the same DSO more than once, in which case you'll have 
 * different image_id's. While this means that we don't have to worry about 
 * reference counts, it could lead to problems in the future....
 * richard.
 */

#include <Errors.h>
#include <stdio.h>

/* --- functions --- */
static gpointer
_g_module_open (const gchar    *file_name,
		gboolean	bind_lazy)
{
  image_id handle;
  
  handle = load_add_on (file_name);
  if (handle < B_OK) {
    g_module_set_error (g_strdup_printf("failed to load_add_on(%s), reason: %s", 
		        (gchar *) file_name, strerror(handle)));
    return NULL;
  }

  return (gpointer) handle;
}

static gpointer
_g_module_self (void)
{
  image_info info;
  int32 cookie = 0;
  status_t status;

  /* Is it always the first one?  I'm guessing yes. */
  if ((status = get_next_image_info(0, &cookie, &info)) == B_OK)
    return (gpointer) info.id;
  else
    {
      g_module_set_error (g_strdup_printf("get_next_image_info() for self failed, reason: %s", strerror(status)));
      return NULL;
    }
}

static void
_g_module_close (gpointer	  handle,
		 gboolean	  is_unref)
{
   image_info info;
   gchar *name;

   if (unload_add_on((image_id) handle) != B_OK)
     {
       /* Try and get the name of the image. */
       if (get_image_info((image_id) handle, &info) != B_OK)
         name = g_strdup("(unknown)");
       else
         name = g_strdup (info.name);

       g_module_set_error (g_strdup_printf("failed to unload_add_on(%s)", 
                           name));
       g_free (name);
     }
}

static gpointer
_g_module_symbol (gpointer	  handle,
		  const gchar	 *symbol_name)
{
  image_id id;
  gpointer p;
  status_t status;
  image_info info;
  gchar name[256];
  int32 name_len;
  int32 type;
  int32 n;

  id = (image_id) handle;

  if ((status = get_image_info(id, &info)) != B_OK)
    {
      g_module_set_error (g_strdup_printf("failed get_image_info(), reason: %s", strerror(status)));
      return NULL;
    }

  name_len = 256;
  type = B_SYMBOL_TYPE_ANY;
  n = 0;
  while ((status = get_nth_image_symbol(id, n, name, &name_len, &type, (void **)&p)) == B_OK)
    {
      if (!strncmp (name, symbol_name, strlen(symbol_name)))
        {
          return p;
        }

      if (!strcmp (name, "_end"))
        {
          g_module_set_error (g_strdup_printf("g_module_symbol(): no symbol matching '%s'", symbol_name));
          return NULL;
        }

      name_len = 256;
      type = B_SYMBOL_TYPE_ANY;
      n++;
    }

  g_module_set_error (g_strdup_printf("failed get_image_symbol(%s), reason: %s", symbol_name, strerror(status)));
  return NULL;
}

static gchar*
_g_module_build_path (const gchar *directory,
		      const gchar *module_name)
{
  printf("WARNING: _g_module_build_path() untested!\n");
  if (directory && *directory) {
    if (strncmp (module_name, "lib", 3) == 0)
      return g_strconcat (directory, "/", module_name, NULL);
    else
      return g_strconcat (directory, "/lib", module_name, ".so", NULL);
  } else if (strncmp (module_name, "lib", 3) == 0)
    return g_strdup (module_name);
  else
    return g_strconcat ("lib", module_name, ".so", NULL);
}
