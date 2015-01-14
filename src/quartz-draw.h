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

#ifndef QUARTZ_DRAW_H
#define QUARTZ_DRAW_H

CGContextRef
get_context (GdkWindow    *window,
             GdkRectangle *area);

void
release_context (GdkWindow    *window,
                 CGContextRef  context);


void quartz_draw_button (GtkStyle        *style,
                         GdkWindow       *window,
                         GtkStateType     state_type,
                         GtkShadowType    shadow_type,
                         GtkWidget       *widget,
                         const gchar     *detail,
                         GdkRectangle    *area,
                         ThemeButtonKind  kind,
                         gint             x,
                         gint             y,
                         gint             width,
                         gint             height);


void
quartz_draw_menu_checkmark (GtkStyle       *style,
                            GdkWindow      *window,
                            GtkStateType    state_type,
                            GtkShadowType   shadow_type,
                            GdkRectangle   *area,
                            GtkWidget      *widget,
                            const gchar    *detail,
                            gint            x,
                            gint            y,
                            gint            width,
                            gint            height);


void
quartz_draw_menu_item (GtkStyle       *style,
                       GdkWindow      *window,
                       GtkStateType    state_type,
                       GtkWidget      *widget);


void
quartz_draw_statusbar (GtkStyle        *style,
					   GdkWindow       *window,
					   GtkStateType     state_type,
					   GtkWidget       *widget,
					   const gchar     *detail,
					   gint             x,
					   gint             y,
					   gint             width,
					   gint             height);

#endif /* QUARTZ_DRAW_H */
