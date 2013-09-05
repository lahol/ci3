#include "gtk2-compat.h"
#include <string.h>
#include <errno.h>
#include <math.h>

int gtk_widget_get_allocated_height(GtkWidget *widget)
{
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    return alloc.height;
}

int gtk_widget_get_allocated_width(GtkWidget *widget)
{
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    return alloc.width;
}

/* shamelessly copied from gtk3 source */
#define SKIP_WHITESPACES(s) while (*(s) == ' ') (s)++;

/* Parses a single color component from a rgb() or rgba() specification
 * according to CSS3 rules. Compared to exact CSS3 parsing we are liberal
 * in what we accept as follows:
 *
 *  - For non-percentage values, we accept floats in the range 0-255
 *    not just [0-9]+ integers
 *  - For percentage values we accept any float, not just
 *     [ 0-9]+ | [0-9]* '.' [0-9]+
 *  - We accept mixed percentages and non-percentages in a single
 *    rgb() or rgba() specification.
 */
static gboolean
parse_rgb_value (const gchar  *str,
                 gchar       **endp,
                 gdouble      *number)
{
  const char *p;

  *number = g_ascii_strtod (str, endp);
  if (errno == ERANGE || *endp == str ||
      isinf (*number) || isnan (*number))
    return FALSE;

  p = *endp;

  SKIP_WHITESPACES (p);

  if (*p == '%')
    {
      *endp = (char *)(p + 1);
      *number = CLAMP(*number / 100., 0., 1.);
    }
  else
    {
      *number = CLAMP(*number / 255., 0., 1.);
    }

  return TRUE;
}

/**
 * gdk_rgba_parse:
 * @rgba: the #GdkRGBA struct to fill in
 * @spec: the string specifying the color
 *
 * Parses a textual representation of a color, filling in
 * the <structfield>red</structfield>, <structfield>green</structfield>,
 * <structfield>blue</structfield> and <structfield>alpha</structfield>
 * fields of the @rgba struct.
 *
 * The string can be either one of:
 * <itemizedlist>
 * <listitem>
 * A standard name (Taken from the X11 rgb.txt file).
 * </listitem>
 * <listitem>
 * A hex value in the form '#rgb' '#rrggbb' '#rrrgggbbb' or '#rrrrggggbbbb'
 * </listitem>
 * <listitem>
 * A RGB color in the form 'rgb(r,g,b)' (In this case the color will
 * have full opacity)
 * </listitem>
 * <listitem>
 * A RGBA color in the form 'rgba(r,g,b,a)'
 * </listitem>
 * </itemizedlist>
 *
 * Where 'r', 'g', 'b' and 'a' are respectively the red, green, blue and
 * alpha color values. In the last two cases, r g and b are either integers
 * in the range 0 to 255 or precentage values in the range 0% to 100%, and
 * a is a floating point value in the range 0 to 1.
 *
 * Returns: %TRUE if the parsing succeeded
 *
 * Since: 3.0
 */
gboolean
gdk_rgba_parse (GdkRGBA     *rgba,
                const gchar *spec)
{
  gboolean has_alpha;
  gdouble r, g, b, a;
  gchar *str = (gchar *) spec;
  gchar *p;

  if (strncmp (str, "rgba", 4) == 0)
    {
      has_alpha = TRUE;
      str += 4;
    }
  else if (strncmp (str, "rgb", 3) == 0)
    {
      has_alpha = FALSE;
      a = 1;
      str += 3;
    }
  else
    {
      PangoColor pango_color;

      /* Resort on PangoColor for rgb.txt color
       * map and '#' prefixed colors
       */
      if (pango_color_parse (&pango_color, str))
        {
          if (rgba)
            {
              rgba->red = pango_color.red / 65535.;
              rgba->green = pango_color.green / 65535.;
              rgba->blue = pango_color.blue / 65535.;
              rgba->alpha = 1;
            }

          return TRUE;
        }
      else
        return FALSE;
    }

  SKIP_WHITESPACES (str);

  if (*str != '(')
    return FALSE;

  str++;

  /* Parse red */
  SKIP_WHITESPACES (str);
  if (!parse_rgb_value (str, &str, &r))
    return FALSE;
  SKIP_WHITESPACES (str);

  if (*str != ',')
    return FALSE;

  str++;

  /* Parse green */
  SKIP_WHITESPACES (str);
  if (!parse_rgb_value (str, &str, &g))
    return FALSE;
  SKIP_WHITESPACES (str);

  if (*str != ',')
    return FALSE;

  str++;

  /* Parse blue */
  SKIP_WHITESPACES (str);
  if (!parse_rgb_value (str, &str, &b))
    return FALSE;
  SKIP_WHITESPACES (str);

  if (has_alpha)
    {
      if (*str != ',')
        return FALSE;

      str++;

      SKIP_WHITESPACES (str);
      a = g_ascii_strtod (str, &p);
      if (errno == ERANGE || p == str ||
          isinf (a) || isnan (a))
        return FALSE;
      str = p;
      SKIP_WHITESPACES (str);
    }

  if (*str != ')')
    return FALSE;

  str++;

  SKIP_WHITESPACES (str);

  if (*str != '\0')
    return FALSE;

  if (rgba)
    {
      rgba->red = CLAMP (r, 0, 1);
      rgba->green = CLAMP (g, 0, 1);
      rgba->blue = CLAMP (b, 0, 1);
      rgba->alpha = CLAMP (a, 0, 1);
    }

  return TRUE;
}

#undef SKIP_WHITESPACES



