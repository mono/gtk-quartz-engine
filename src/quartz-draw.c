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
#include <gtk/gtk.h>
#include <Carbon/Carbon.h>

#include "WindowGradientHelper.h"

#define IS_DETAIL(d,x) (d && strcmp (d, x) == 0)

/* FIXME: Fix GTK+ to export those in a quartz header file. */
CGContextRef gdk_quartz_drawable_get_context     (GdkDrawable  *drawable,
                                                  gboolean      antialias);
void         gdk_quartz_drawable_release_context (GdkDrawable  *drawable,
                                                  CGContextRef  context);
NSWindow *   gdk_quartz_window_get_nswindow (GdkWindow *window);

CGContextRef
get_context (GdkWindow    *window,
             GdkRectangle *area)
{
	GdkDrawable  *drawable;
	CGContextRef  context;

	gint x_delta = 0;
	gint y_delta = 0;

	if (GDK_IS_PIXMAP (window)) {
		drawable = GDK_PIXMAP_OBJECT (window)->impl;
	} else {
		gdk_window_get_internal_paint_info (window, &drawable, &x_delta, &y_delta);
		drawable = GDK_WINDOW_OBJECT (window)->impl;
	}

	context = gdk_quartz_drawable_get_context (drawable, FALSE);
	if (!context)
		return NULL;

	CGContextSaveGState (context);
	CGContextTranslateCTM (context, -x_delta, -y_delta);

	if (area)
		CGContextClipToRect (context, CGRectMake (area->x, area->y,
												  area->width, area->height));

	return context;
}

void
release_context (GdkWindow    *window,
                 CGContextRef  context)
{
	GdkDrawable *drawable;

	if (GDK_IS_PIXMAP (window))
		drawable = GDK_PIXMAP_OBJECT (window)->impl;
	else
		drawable = GDK_WINDOW_OBJECT (window)->impl;

	CGContextRestoreGState (context);
	gdk_quartz_drawable_release_context (drawable, context);
}

#if 0
static void
quartz_measure_button (HIThemeButtonDrawInfo *draw_info,
                       gint                   width,
                       gint                   height)
{
  HIRect in_rect, out_rect;
  HIShapeRef shape;

  in_rect.origin.x = 0;
  in_rect.origin.y = 0;
  in_rect.size.width = width;
  in_rect.size.height = height;

  HIThemeGetButtonShape (&in_rect, draw_info, &shape);
  HIShapeGetBounds (shape, &out_rect);
  NSLog (@"Shape: %d %d, %d %d  ",
           (int) out_rect.origin.x,
           (int) out_rect.origin.y,
           (int) out_rect.size.width - (int) out_rect.origin.x,
           (int) out_rect.size.height - (int) out_rect.origin.y);

  HIThemeGetButtonBackgroundBounds (&in_rect, draw_info, &out_rect);
  NSLog (@"Bounds: %d %d, %d %d  ",
           (int) out_rect.origin.x,
           (int) out_rect.origin.y,
           (int) out_rect.size.width - (int) out_rect.origin.x,
           (int) out_rect.size.height - (int) out_rect.origin.y);

  HIThemeGetButtonContentBounds (&in_rect, draw_info, &out_rect);
  NSLog (@"Content: %d %d, %d %d\n",
           (int) out_rect.origin.x,
           (int) out_rect.origin.y,
           (int) out_rect.size.width - (int) out_rect.origin.x,
           (int) out_rect.size.height - (int) out_rect.origin.y);
}
#endif


void
quartz_draw_button (GtkStyle        *style,
                    GdkWindow       *window,
                    GtkStateType     state_type,
                    GtkShadowType    shadow_type,
                    GtkWidget       *widget,
                    const gchar     *detail,
                    ThemeButtonKind  kind,
                    gint             x,
                    gint             y,
                    gint             width,
                    gint             height)
{
  CGContextRef context;
  HIRect rect;
  HIShapeRef shape;
  HIThemeButtonDrawInfo draw_info;
  gint line_width;
  SInt32 theme_height;

  draw_info.version = 0;
  draw_info.kind = kind;
  draw_info.adornment = kThemeAdornmentNone;
  draw_info.value = kThemeButtonOff;

  // if the button size is too small or too tall, force the button kind so it looks better..
	if (((width < 20) || (height > 29)) && (kind != kThemeBevelButtonInset)) { // FIXME: magic numbers. not sure if they're correct, just guesses
	  draw_info.kind = kThemeBevelButton;
	  rect = CGRectMake (x, y, width, height);

	} else {
	  gtk_widget_style_get (widget,
						    "focus-line-width", &line_width,
						    NULL);

	  rect = CGRectMake (x + line_width, y + line_width,
					     width - 2 * line_width, height - 2 * line_width - 2);
	}

  if (state_type == GTK_STATE_ACTIVE ||
      (GTK_IS_TOGGLE_BUTTON (widget) &&
       gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))))
    draw_info.state = kThemeStatePressed;
  else if (state_type == GTK_STATE_INSENSITIVE)
    draw_info.state = kThemeStateInactive;
  else
    draw_info.state = kThemeStateActive;

  //if (GTK_WIDGET_HAS_FOCUS (widget))
  //  draw_info.adornment |= kThemeAdornmentFocus;

  //if (IS_DETAIL (detail, "buttondefault"))
  //  draw_info.adornment |= kThemeAdornmentDefault;

  /* FIXME: Emulate default button pulsing. */

  context = get_context (window, NULL);
  if (!context)
    return;


  HIThemeDrawButton (&rect,
                     &draw_info,
                     context,
                     kHIThemeOrientationNormal,
                     NULL);


  release_context (window, context);
}


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
                            gint            height)
{
      /* based on http://lists.apple.com/archives/carbon-development/2004/Mar/msg00626.html */

      if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget)))
        return;

      CGContextRef context;
      HIRect rect;
      HIThemeTextInfo draw_info;

      UniChar uchCheck = kCheckUnicode;
      CFStringRef checkString = CFStringCreateWithCharacters (NULL, &uchCheck, 1);

      draw_info.version = 1;
      draw_info.fontID = kThemeMenuItemMarkFont;
      draw_info.horizontalFlushness = kHIThemeTextHorizontalFlushRight;
      draw_info.verticalFlushness = kHIThemeTextVerticalFlushCenter;
      draw_info.options = 0;
      draw_info.truncationPosition = kHIThemeTextTruncationNone;

      switch (state_type)
      {
        case GTK_STATE_INSENSITIVE:
          draw_info.state = kThemeStateInactive;
          break;

        case GTK_STATE_PRELIGHT:
          draw_info.state = kThemeStatePressed;
          break;

        default:
          draw_info.state = kThemeStateActive;
          break;
      }

      rect = CGRectMake (4, y, width, height);

      context = get_context (window, NULL);
      if (!context)
        return;

      HIThemeDrawTextBox (checkString,
                          &rect,
                          &draw_info,
                          context,
                          kHIThemeOrientationNormal);

      CFRelease (checkString);
      release_context (window, context);
}


void
quartz_draw_statusbar (GtkStyle        *style,
					   GdkWindow       *window,
					   GtkStateType     state_type,
					   GtkWidget       *widget,
					   const gchar     *detail,
					   gint             x,
					   gint             y,
					   gint             width,
					   gint             height)
{
	if (!window)
		return;

	CGContextRef context;
	context = gdk_quartz_drawable_get_context (GDK_WINDOW_OBJECT (window)->impl, FALSE);
	if (!context)
		return;

	NSWindow* wnd = gdk_quartz_window_get_nswindow (window);
	if (!wnd)
		return;

	if ([wnd backgroundColor] && [[wnd backgroundColor] isKindOfClass: [WindowGradientHelper class]]) {
		WindowGradientHelper* helper = (WindowGradientHelper*)[wnd backgroundColor];
		[helper setStatusbarHeight: height];
	}

	BOOL isMain = [wnd isMainWindow];
	NSRect frame = [wnd frame];

	float titlebarHeight = [WindowGradientHelper titleBarHeight];

	CGContextSaveGState (context);
	CGContextAddRect (context, CGRectMake (x, y, width, height));
	CGContextClip (context);

	CGContextScaleCTM(context, 1.0f, -1.0f);
	CGContextTranslateCTM(context, 0.0f, -(frame.size.height - titlebarHeight));

	CGContextBeginPath (context);
	CGContextAddRect (context, CGRectMake (0.0f, 0.0f, frame.size.width, height - 2));
	CGContextDrawLinearGradient (context, isMain? [WindowGradientHelper activeStatus] : [WindowGradientHelper inactiveStatus], CGPointMake (0.0f, height - 2), CGPointMake (0.0f, 0.0f), 0);

	DrawNativeGreyColorInRect(context, statusbarFirstTopBorderGrey, CGRectMake(0.0f, height - 1, frame.size.width, 0.5f), isMain);
	DrawNativeGreyColorInRect(context, statusbarSecondTopBorderGrey, CGRectMake(0.0f, height - 1.5, frame.size.width, 0.5f), isMain);

	CGContextRestoreGState (context);
	gdk_quartz_drawable_release_context (GDK_WINDOW_OBJECT (window)->impl, context);
}

