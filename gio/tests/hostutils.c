/* 
 * Copyright (C) 2008 Red Hat, Inc
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

#include <glib/glib.h>
#include <gio/gio.h>

#include <stdlib.h>
#include <string.h>

static const struct {
  const char *ascii_name, *unicode_name;
} idn_test_domains[] = {
  /* "example.test" in various languages */
  { "xn--mgbh0fb.xn--kgbechtv", "\xd9\x85\xd8\xab\xd8\xa7\xd9\x84.\xd8\xa5\xd8\xae\xd8\xaa\xd8\xa8\xd8\xa7\xd8\xb1" },
  { "xn--fsqu00a.xn--0zwm56d", "\xe4\xbe\x8b\xe5\xad\x90.\xe6\xb5\x8b\xe8\xaf\x95" },
  { "xn--fsqu00a.xn--g6w251d", "\xe4\xbe\x8b\xe5\xad\x90.\xe6\xb8\xac\xe8\xa9\xa6" },
  { "xn--hxajbheg2az3al.xn--jxalpdlp", "\xcf\x80\xce\xb1\xcf\x81\xce\xac\xce\xb4\xce\xb5\xce\xb9\xce\xb3\xce\xbc\xce\xb1.\xce\xb4\xce\xbf\xce\xba\xce\xb9\xce\xbc\xce\xae" },
  { "xn--p1b6ci4b4b3a.xn--11b5bs3a9aj6g", "\xe0\xa4\x89\xe0\xa4\xa6\xe0\xa4\xbe\xe0\xa4\xb9\xe0\xa4\xb0\xe0\xa4\xa3.\xe0\xa4\xaa\xe0\xa4\xb0\xe0\xa5\x80\xe0\xa4\x95\xe0\xa5\x8d\xe0\xa4\xb7\xe0\xa4\xbe" },
  { "xn--r8jz45g.xn--zckzah", "\xe4\xbe\x8b\xe3\x81\x88.\xe3\x83\x86\xe3\x82\xb9\xe3\x83\x88" },
  { "xn--9n2bp8q.xn--9t4b11yi5a", "\xec\x8b\xa4\xeb\xa1\x80.\xed\x85\x8c\xec\x8a\xa4\xed\x8a\xb8" },
  { "xn--mgbh0fb.xn--hgbk6aj7f53bba", "\xd9\x85\xd8\xab\xd8\xa7\xd9\x84.\xd8\xa2\xd8\xb2\xd9\x85\xd8\xa7\xdb\x8c\xd8\xb4\xdb\x8c" },
  { "xn--e1afmkfd.xn--80akhbyknj4f", "\xd0\xbf\xd1\x80\xd0\xb8\xd0\xbc\xd0\xb5\xd1\x80.\xd0\xb8\xd1\x81\xd0\xbf\xd1\x8b\xd1\x82\xd0\xb0\xd0\xbd\xd0\xb8\xd0\xb5" },
  { "xn--zkc6cc5bi7f6e.xn--hlcj6aya9esc7a", "\xe0\xae\x89\xe0\xae\xa4\xe0\xae\xbe\xe0\xae\xb0\xe0\xae\xa3\xe0\xae\xae\xe0\xaf\x8d.\xe0\xae\xaa\xe0\xae\xb0\xe0\xae\xbf\xe0\xae\x9f\xe0\xaf\x8d\xe0\xae\x9a\xe0\xaf\x88" },
  { "xn--fdbk5d8ap9b8a8d.xn--deba0ad", "\xd7\x91\xd7\xb2\xd6\xb7\xd7\xa9\xd7\xa4\xd6\xbc\xd7\x99\xd7\x9c.\xd7\x98\xd7\xa2\xd7\xa1\xd7\x98" },

  /* further examples without their own IDN-ized TLD */
  { "xn--1xd0bwwra.idn.icann.org", "\xe1\x8a\xa0\xe1\x88\x9b\xe1\x88\xad\xe1\x8a\x9b.idn.icann.org" },
  { "xn--54b7fta0cc.idn.icann.org", "\xe0\xa6\xac\xe0\xa6\xbe\xe0\xa6\x82\xe0\xa6\xb2\xe0\xa6\xbe.idn.icann.org" },
  { "xn--5dbqzzl.idn.icann.org", "\xd7\xa2\xd7\x91\xd7\xa8\xd7\x99\xd7\xaa.idn.icann.org" },
  { "xn--j2e7beiw1lb2hqg.idn.icann.org", "\xe1\x9e\x97\xe1\x9e\xb6\xe1\x9e\x9f\xe1\x9e\xb6\xe1\x9e\x81\xe1\x9f\x92\xe1\x9e\x98\xe1\x9f\x82\xe1\x9e\x9a.idn.icann.org" },
  { "xn--o3cw4h.idn.icann.org", "\xe0\xb9\x84\xe0\xb8\x97\xe0\xb8\xa2.idn.icann.org" },
  { "xn--mgbqf7g.idn.icann.org", "\xd8\xa7\xd8\xb1\xd8\xaf\xd9\x88.idn.icann.org" }
};
static const int num_idn_test_domains = G_N_ELEMENTS (idn_test_domains);

static void
test_to_ascii (void)
{
  int i;
  char *ascii;

  for (i = 0; i < num_idn_test_domains; i++)
    {
      g_assert (g_hostname_is_non_ascii (idn_test_domains[i].unicode_name));
      ascii = g_hostname_to_ascii (idn_test_domains[i].unicode_name);
      g_assert_cmpstr (idn_test_domains[i].ascii_name, ==, ascii);
      g_free (ascii);

      ascii = g_hostname_to_ascii (idn_test_domains[i].ascii_name);
      g_assert_cmpstr (idn_test_domains[i].ascii_name, ==, ascii);
      g_free (ascii);
    }
}

static void
test_to_unicode (void)
{
  int i;
  char *unicode;

  for (i = 0; i < num_idn_test_domains; i++)
    {
      g_assert (g_hostname_is_ascii_encoded (idn_test_domains[i].ascii_name));
      unicode = g_hostname_to_unicode (idn_test_domains[i].ascii_name);
      g_assert_cmpstr (idn_test_domains[i].unicode_name, ==, unicode);
      g_free (unicode);

      unicode = g_hostname_to_unicode (idn_test_domains[i].unicode_name);
      g_assert_cmpstr (idn_test_domains[i].unicode_name, ==, unicode);
      g_free (unicode);
    }
}

/* FIXME: test g_hostname_is_ip_addr() */
/* FIXME: test names with both unicode and ACE-encoded labels */
/* FIXME: test invalid unicode names */

int
main (int   argc,
      char *argv[])
{
  g_type_init ();
  g_test_init (&argc, &argv, NULL);
  
  g_test_add_func ("/hostutils/to_ascii", test_to_ascii);
  g_test_add_func ("/hostutils/to_unicode", test_to_unicode);

  return g_test_run ();
}
