#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024

gint main (gint argc, gchar * argv[])
{
    GIOChannel *gio_r, *gio_w ;
    GError *gerr = NULL;
    GString *buffer;
    gint rlength = 0, wlength = 0, length_out, line_term_len;
    gboolean block;
    const gchar encoding[] = "EUC-JP", line_term[] = "\n";
    GIOStatus status;
    GIOFlags flags;

    setbuf(stdout, NULL); /* For debugging */

    gio_r = g_io_channel_new_file ("iochannel-test-infile", "r", &gerr);
    if (gerr)
      {
        g_warning(gerr->message);
        g_warning("Unable to open file %s", "iochannel-test-infile");
        g_free(gerr);
        return 0;
      }
    gio_w = g_io_channel_new_file( "iochannel-test-outfile", "w", &gerr);
    if (gerr)
      {
        g_warning(gerr->message);
        g_warning("Unable to open file %s", "iochannel-test-outfile");
        g_free(gerr);
        return 0;
      }

    g_io_channel_set_encoding (gio_r, encoding, &gerr);
    if (gerr)
      {
        g_warning(gerr->message);
        g_free(gerr);
        return 0;
      }
    
    g_io_channel_set_buffer_size (gio_r, BUFFER_SIZE);

    status = g_io_channel_set_flags (gio_r, G_IO_FLAG_NONBLOCK, &gerr);
    if (status == G_IO_STATUS_ERROR)
      {
        g_warning(gerr->message);
        g_error_free(gerr);
        gerr = NULL;
      }
    flags = g_io_channel_get_flags (gio_r);
    block = ! (flags & G_IO_FLAG_NONBLOCK);
    if (block)
        g_print (" BLOCKING TRUE \n\n");
    else
        g_print (" BLOCKING FALSE \n\n");

    line_term_len = strlen (line_term);
    buffer = g_string_sized_new (BUFFER_SIZE);

    while (TRUE)
    {
        do
          status = g_io_channel_read_line_string (gio_r, buffer, NULL, &gerr);
        while (status == G_IO_STATUS_AGAIN);
        if (status != G_IO_STATUS_NORMAL)
          break;

        rlength += buffer->len;

        do
          status = g_io_channel_write_chars (gio_w, buffer->str, buffer->len,
            &length_out, &gerr);
        while (status == G_IO_STATUS_AGAIN);
        if (status != G_IO_STATUS_NORMAL)
          break;

        wlength += length_out;

        if (length_out < buffer->len)
          g_warning ("Only wrote part of the line.");

        g_print ("%s", buffer->str);
        g_string_truncate (buffer, 0);
    }

    switch (status)
      {
        case G_IO_STATUS_EOF:
          break;
        case G_IO_STATUS_ERROR:
          g_warning (gerr->message);
          g_error_free (gerr);
          gerr = NULL;
          break;
        default:
          g_warning ("Abnormal exit from write loop.");
          break;
      }

    do
      status = g_io_channel_flush (gio_w, &gerr);
    while (status == G_IO_STATUS_AGAIN);

    if (status == G_IO_STATUS_ERROR)
      {
        g_warning(gerr->message);
        g_error_free(gerr);
        gerr = NULL;
      }

    g_print ("read %d bytes, wrote %d bytes\n", rlength, wlength);

    g_io_channel_unref(gio_r);
    g_io_channel_unref(gio_w);
    
    return 0;
}
