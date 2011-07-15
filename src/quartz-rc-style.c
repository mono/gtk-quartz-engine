/* GTK+ theme engine for the Quartz backend
 *
 * Copyright (C) 2007-2008 Imendio AB
 * Copyright (C) 2011 Xamarin Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include "quartz-style.h"
#include "quartz-rc-style.h"

static void      quartz_rc_style_init         (QuartzRcStyle      *style);
static void      quartz_rc_style_class_init   (QuartzRcStyleClass *klass);
static GtkStyle *quartz_rc_style_create_style (GtkRcStyle         *rc_style);
static guint     quartz_rc_style_parse        (GtkRcStyle  *rc_style, GtkSettings *settings,
                                               GScanner    *scanner);
static void      quartz_rc_style_merge        (GtkRcStyle             *dest,
                                               GtkRcStyle             *src);

static GtkRcStyleClass *parent_class;

GType quartz_type_rc_style = 0;

enum
{
	TOKEN_BUTTONTYPE
};

struct
{
	const gchar *name;
	guint        token;
}
theme_symbols[] =
{
	{ "buttontype",           TOKEN_BUTTONTYPE }
};

const gchar* button_types[] =
{
	"",
	"aqua",
	"textured",
	"inset"
};


void
quartz_rc_style_register_type (GTypeModule * module)
{
    static const GTypeInfo object_info = {
	sizeof (QuartzRcStyleClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) quartz_rc_style_class_init,
	NULL,			/* class_finalize */
	NULL,			/* class_data */
	sizeof (QuartzRcStyle),
	0,			/* n_preallocs */
	(GInstanceInitFunc) quartz_rc_style_init,
    };

    quartz_type_rc_style = g_type_module_register_type (module,
						     GTK_TYPE_RC_STYLE,
						     "QuartzRcStyle",
						     &object_info, 0);
}

static void
quartz_rc_style_init (QuartzRcStyle * style)
{
	style->button_type = 0;
}

static void
quartz_rc_style_class_init (QuartzRcStyleClass * klass)
{
    GtkRcStyleClass *rc_style_class = GTK_RC_STYLE_CLASS (klass);

    parent_class = g_type_class_peek_parent (klass);

    rc_style_class->create_style = quartz_rc_style_create_style;
	rc_style_class->parse = quartz_rc_style_parse;
	rc_style_class->merge = quartz_rc_style_merge;
}

static GtkStyle *
quartz_rc_style_create_style (GtkRcStyle * rc_style)
{
    return GTK_STYLE (g_object_new (QUARTZ_TYPE_STYLE, NULL));
}

static guint
guartz_rc_parse_enum (GScanner *scanner, const gchar **choices, guint *selected)
{
	guint i, token;
	*selected = 0;

	token = g_scanner_get_next_token (scanner);

	token = g_scanner_get_next_token (scanner);
	if (token != G_TOKEN_EQUAL_SIGN)
		return G_TOKEN_EQUAL_SIGN;

	token = g_scanner_get_next_token (scanner);
	if (token != G_TOKEN_STRING)
		return G_TOKEN_STRING;

	for (i = 0; i < G_N_ELEMENTS (button_types); i++) {
		if (!g_ascii_strcasecmp (button_types [i], scanner->value.v_string)) {
			*selected = i;
			break;
		}
	}

	return G_TOKEN_NONE;
}

static guint
quartz_rc_style_parse (GtkRcStyle  *rc_style, GtkSettings *settings,
					   GScanner    *scanner)
{
	static GQuark scope_id = 0;
	QuartzRcStyle *quartz_style = QUARTZ_RC_STYLE (rc_style);

	guint i, old_scope, token;

	/* Set up a new scope in this scanner. */

	if (!scope_id)
		scope_id = g_quark_from_string ("quartz_theme_engine");

	/* If we bail out due to errors, we *don't* reset the scope, so the
	 * error messaging code can make sense of our tokens.
	 */
	old_scope = g_scanner_set_scope (scanner, scope_id);

	/* Now check if we already added our symbols to this scope
	 * (in some previous call to murrine_rc_style_parse for the
	 * same scanner.
	 */

	if (!g_scanner_lookup_symbol (scanner, theme_symbols[0].name))
	{
		g_scanner_freeze_symbol_table (scanner);
		for (i = 0; i < G_N_ELEMENTS (theme_symbols); i++)
			g_scanner_scope_add_symbol (scanner, scope_id, theme_symbols[i].name, GINT_TO_POINTER (theme_symbols[i].token));
		g_scanner_thaw_symbol_table (scanner);
	}

	token = g_scanner_peek_next_token (scanner);
	while (token != G_TOKEN_RIGHT_CURLY)
	{
		switch (token) {
			case TOKEN_BUTTONTYPE:
				token = guartz_rc_parse_enum (scanner, button_types, &quartz_style->button_type);
				break;

			default:
				g_scanner_get_next_token (scanner);
				token = G_TOKEN_RIGHT_CURLY;
				break;
		}

		if (token != G_TOKEN_NONE)
			return token;

		token = g_scanner_peek_next_token (scanner);
	}

	g_scanner_get_next_token (scanner);
	g_scanner_set_scope (scanner, old_scope);
	return G_TOKEN_NONE;
}

static void
quartz_rc_style_merge (GtkRcStyle *dest, GtkRcStyle *src)
{
	QuartzRcStyle *dest_w, *src_w;

	parent_class->merge (dest, src);

	if (!QUARTZ_IS_RC_STYLE (src))
		return;

	src_w = QUARTZ_RC_STYLE (src);
	dest_w = QUARTZ_RC_STYLE (dest);

	// for button type, higher numbers win?
	//if (src_w->button_type > dest_w->button_type)
	//	dest_w->button_type = src_w->button_type;
	if (!dest_w->button_type)
		dest_w->button_type = src_w->button_type;
}
