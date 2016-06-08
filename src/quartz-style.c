/* GTK+ theme engine for the Quartz backend
 *
 * Copyright (C) 2007-2008 Imendio AB
 * Copyright (C) 2009 Rob Caelers
 * Copyright (C) 2011-2013 Xamarin Inc.
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
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <gtk/gtk.h>

#include <Carbon/Carbon.h>
#include <AppKit/AppKit.h>

#include "quartz-rc-style.h"
#include "quartz-style.h"
#include "quartz-draw.h"
#include "WindowGradientHelper.h"

static GtkStyleClass *parent_class;

/* FIXME: Fix GTK+ to export those in a quartz header file. */
NSWindow *   gdk_quartz_window_get_nswindow (GdkWindow *window);

/* Private OSX APIs */
int _CGSDefaultConnection (void);
CGError CGSSetWindowShadowAndRimParameters (int cid, int wid, float standardDeviation, float density, int offsetX, int offsetY, unsigned int flags);
CGError CGSNewCIFilterByName (int cid, NSString* filterName, void** outFilter);
CGError CGSSetCIFilterValuesFromDictionary (int cid, void* filter, NSDictionary* filterValues);
CGError CGSAddWindowFilter (int cid, int wid, void* filter, NSInteger flags);
CGError CGSReleaseCIFilter (int cid, void* filter);

/* TODO:
 *
 * Put the button and frame draw functions in helper functions and handle
 * all the state/focus/active/etc stuff there.
 *
 * Tweak the spacing between check/radio and the label.
 */

static gboolean is_combo_box_child (GtkWidget *widget);

static gchar *debug = NULL;
#define DEBUG_DRAW if (debug && (strcmp (debug, "all") == 0 || strcmp (debug, G_OBJECT_TYPE_NAME (widget)) == 0)) \
    g_print ("%s, %s, %s\n", __PRETTY_FUNCTION__, G_OBJECT_TYPE_NAME (widget), detail);

#define IS_DETAIL(d,x) (d && strcmp (d, x) == 0)

static void
style_setup_settings (void)
{
  debug = g_strdup (g_getenv ("DEBUG_DRAW"));
}

static void
style_setup_system_font (GtkStyle *style)
{
  gchar *font;

  /* FIXME: This should be fetched from the correct preference value. See:
   * http://developer.apple.com/documentation/UserExperience/\
   * Conceptual/OSXHIGuidelines/XHIGText/chapter_13_section_2.html
   */
  font = "Lucida Grande 13";

  if (style->font_desc)
    pango_font_description_free (style->font_desc);

  style->font_desc = pango_font_description_from_string (font);
}

#if 0
static gchar *
style_get_system_font_string (NSControlSize size)
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSFont            *font;
  gchar             *str;

  font = [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize: size]];
  str = g_strdup_printf ("%s, size: %.2f", [[font familyName] UTF8String], [font pointSize]);

  [pool release];

  return str;
}
#endif

static void
style_setup_rc_styles (void)
{
    return;
}

static void
sanitize_size (GdkWindow *window,
               gint      *width,
               gint      *height)
{
  if ((*width == -1) && (*height == -1))
    gdk_drawable_get_size (window, width, height);
  else if (*width == -1)
    gdk_drawable_get_size (window, width, NULL);
  else if (*height == -1)
    gdk_drawable_get_size (window, NULL, height);
}

static void
draw_arrow (GtkStyle      *style,
            GdkWindow     *window,
            GtkStateType   state_type,
            GtkShadowType  shadow,
            GdkRectangle  *area,
            GtkWidget     *widget,
            const gchar   *detail,
            GtkArrowType   arrow_type,
            gboolean       fill,
            gint           x,
            gint           y,
            gint           width,
            gint           height)
{
  CGContextRef context;
  HIRect rect;
  HIThemePopupArrowDrawInfo arrow_info;

  DEBUG_DRAW;

  if (GTK_IS_SCROLLBAR (widget))
    return;
  else if (GTK_IS_SPIN_BUTTON (widget))
    return;
  else if (is_combo_box_child (widget))
    return;

  if (GTK_IS_MENU_ITEM(widget))
    {
      quartz_draw_menu_item (style,
                             window,
                             state_type,
                             widget);
      return;
    }

  context = get_context (window, area);
  if (!context)
    return;

  sanitize_size (window, &width, &height);
  rect = CGRectMake (x, y, width, height);

  arrow_info.version = 0;
  if (state_type == GTK_STATE_INSENSITIVE)
    arrow_info.state = kThemeStateUnavailable;
  else if (state_type == GTK_STATE_PRELIGHT)
    arrow_info.state = kThemeStatePressed;
  else
    arrow_info.state = kThemeStateActive;

  switch (arrow_type)
    {
    case GTK_ARROW_UP:
      arrow_info.orientation = kThemeArrowUp;
      break;

    default:
    case GTK_ARROW_DOWN:
      arrow_info.orientation = kThemeArrowDown;
      rect.origin.y += 4;
      break;

    case GTK_ARROW_RIGHT:
      arrow_info.orientation = kThemeArrowRight;
      rect.origin.x += 4;
      break;

    case GTK_ARROW_LEFT:
      arrow_info.orientation = kThemeArrowLeft;
      rect.origin.x -= 4;
      break;
    }

  arrow_info.size = kThemeArrow9pt;

  HIThemeDrawPopupArrow (&rect, &arrow_info, context, kHIThemeOrientationNormal);

  release_context (window, context);
}

static gboolean
is_combo_box_child (GtkWidget *widget)
{
  GtkWidget *tmp;

  for (tmp = widget->parent; tmp; tmp = tmp->parent)
    {
      if (GTK_IS_COMBO_BOX (tmp))
        return TRUE;
    }

  return FALSE;
}

static gboolean
is_tree_view_child (GtkWidget *widget)
{
  GtkWidget *tmp;

  for (tmp = widget->parent; tmp; tmp = tmp->parent)
    {
      if (GTK_IS_TREE_VIEW (tmp))
        return TRUE;
    }

  return FALSE;
}
static GtkWidget*
is_in_statusbar (GtkWidget *widget)
{
	GtkWidget *tmp;

	for (tmp = widget; tmp; tmp = tmp->parent)
    {
		if (GTK_IS_STATUSBAR (tmp))
			return tmp;
    }

	return NULL;
}
static gboolean
is_path_bar_button (GtkWidget *widget)
{
  GtkWidget *tmp;

  if (!GTK_IS_BUTTON (widget))
    return FALSE;

  for (tmp = widget->parent; tmp; tmp = tmp->parent)
    {
      if (strcmp (G_OBJECT_TYPE_NAME (tmp), "GtkPathBar") == 0)
        return TRUE;
    }

  return FALSE;
}

/* Checks if the button is displaying just an icon and no text, used to
 * decide whether to show a square or aqua button.
 */
static gboolean
is_icon_only_button (GtkWidget *widget)
{
  GtkWidget *child;

  if (!GTK_IS_BUTTON (widget))
    return FALSE;

  child = gtk_bin_get_child (GTK_BIN (widget));

  /* Very simplistic checks, could be made a bit smarter. */
  if (GTK_IS_IMAGE (child))
    return TRUE;

  if (GTK_IS_ALIGNMENT (child))
    {
      child = gtk_bin_get_child (GTK_BIN (child));
      if (GTK_IS_IMAGE (child))
        return TRUE;
    }

  return FALSE;
}

static void
draw_box (GtkStyle      *style,
          GdkWindow     *window,
          GtkStateType   state_type,
          GtkShadowType  shadow_type,
          GdkRectangle  *area,
          GtkWidget     *widget,
          const gchar   *detail,
          gint           x,
          gint           y,
          gint           width,
          gint           height)
{
  DEBUG_DRAW;

  sanitize_size (window, &width, &height);

	GtkWidget* statusbar = is_in_statusbar(widget);
	if (statusbar) // FIXME: ugly hack
	{
		GtkAllocation statusRect;
		gtk_widget_get_allocation (statusbar, &statusRect);
		quartz_draw_statusbar (style, gtk_widget_get_window (statusbar), state_type, statusbar, detail, x, statusRect.y, width, statusRect.height);
	}
  if (GTK_IS_BUTTON (widget) && is_tree_view_child (widget))
    {
      /* FIXME: Refactor and share with the rest of the button
       * drawing.
       */

      CGContextRef context;
      HIRect rect;
      HIThemeButtonDrawInfo draw_info;

      draw_info.version = 0;
      draw_info.kind = kThemeListHeaderButton;
      draw_info.adornment = kThemeAdornmentNone;
      draw_info.value = kThemeButtonOff;

      if (state_type == GTK_STATE_ACTIVE)
        draw_info.state = kThemeStatePressed;
      else if (state_type == GTK_STATE_INSENSITIVE)
        draw_info.state = kThemeStateInactive;
      else
        draw_info.state = kThemeStateActive;

      //if (GTK_WIDGET_HAS_FOCUS (widget))
      //  draw_info.adornment |= kThemeAdornmentFocus;

      /* We draw outside the allocation to cover the ugly frame from
       * the treeview.
       */
      rect = CGRectMake (x - 1, y - 1, width + 2, height + 2);

      context = get_context (window, area);
      if (!context)
        return;

      HIThemeDrawButton (&rect,
                         &draw_info,
                         context,
                         kHIThemeOrientationNormal,
                         NULL);

      release_context (window, context);

      return;
    }
  else if ( (GTK_IS_TOGGLE_BUTTON (widget) && is_combo_box_child (widget)) ||
            (IS_DETAIL (detail, "optionmenu")) )
    {
      /* FIXME: Support GtkComboBoxEntry too (using kThemeComboBox). */
      CGContextRef context;
      HIRect rect;
      HIThemeButtonDrawInfo draw_info;
      gint line_width;

      draw_info.version = 0;
      draw_info.kind = kThemePopupButton;
      draw_info.adornment = kThemeAdornmentNone;
      draw_info.value = kThemeButtonOff;

      if (state_type == GTK_STATE_ACTIVE)
        draw_info.state = kThemeStatePressed;
      else if (state_type == GTK_STATE_INSENSITIVE)
        draw_info.state = kThemeStateInactive;
      else
        draw_info.state = kThemeStateActive;

      //if (GTK_WIDGET_HAS_FOCUS (widget))
      //  draw_info.adornment |= kThemeAdornmentFocus;

      gtk_widget_style_get (widget,
                            "focus-line-width", &line_width,
                            NULL);

      rect = CGRectMake (x + line_width, y + line_width,
                         width - 2 * line_width, height - 2 * line_width - 1);

      context = get_context (window, area);
      if (!context)
        return;

      HIThemeDrawButton (&rect,
                         &draw_info,
                         context,
                         kHIThemeOrientationNormal,
                         NULL);

      release_context (window, context);

      return;
    }
  else if (IS_DETAIL (detail, "button") || IS_DETAIL (detail, "buttondefault"))
    {
      if (is_tree_view_child (widget) || GTK_IS_CLIST (widget->parent))
        {
          /* FIXME: refactor so that we can share this code with
           * normal buttons.
           */
          CGContextRef context;
          HIRect rect;
          HIThemeButtonDrawInfo draw_info;

          draw_info.version = 0;
          draw_info.kind = kThemeListHeaderButton;
          draw_info.adornment = kThemeAdornmentNone;
          draw_info.value = kThemeButtonOff;

          if (state_type == GTK_STATE_ACTIVE)
            draw_info.state = kThemeStatePressed;
          else if (state_type == GTK_STATE_INSENSITIVE)
            draw_info.state = kThemeStateInactive;
          else
            draw_info.state = kThemeStateActive;

          //if (GTK_WIDGET_HAS_FOCUS (widget))
          //  draw_info.adornment |= kThemeAdornmentFocus;

          if (IS_DETAIL (detail, "buttondefault"))
            draw_info.adornment |= kThemeAdornmentDefault;

          rect = CGRectMake (x, y, width, height);

          context = get_context (window, area);
          if (!context)
            return;

          HIThemeDrawButton (&rect,
                             &draw_info,
                             context,
                             kHIThemeOrientationNormal,
                             NULL);

          release_context (window, context);

          return;
        }
      else /* Normal button. */
        {
          quartz_draw_button (style, window, state_type, shadow_type,
                              widget, detail, area,
                              QUARTZ_STYLE (style)->theme_button_kind,
                              x, y,
                              width, height);
          return;
        }
    }
  else if (IS_DETAIL (detail, "toolbar"))
    {
		if ((height <= 1) || (y != 0))
			return;

		NSWindow* wnd  = gdk_quartz_window_get_nswindow (window);
		WindowGradientHelper* helper;

		if ([wnd backgroundColor] && [[wnd backgroundColor] isKindOfClass: [WindowGradientHelper class]]) {
			helper = (WindowGradientHelper*)[wnd backgroundColor];
		} else {
			helper = [[WindowGradientHelper alloc] initWithWindow: wnd];
		}

		/* horrible hack? */
		if (([wnd styleMask] & NSTexturedBackgroundWindowMask) != NSTexturedBackgroundWindowMask) {
			[helper performSelectorOnMainThread: @selector(hook) withObject: nil waitUntilDone: NO];
		}

		// we have to subtract 1 because this is clipped, and we need a pixel for the bottom line
		[helper setToolbarHeight: height - 1];
		[wnd setBackgroundColor: helper];

		CGContextRef context = get_context (window, area);
		if (!context)
			return;

		NSRect frame = [wnd frame];
		BOOL isMain = [wnd isMainWindow];

		float titlebarHeight = [WindowGradientHelper titleBarHeight];
		float gradientHeight = titlebarHeight + (height - 1);

		CGContextSaveGState (context);
		CGContextScaleCTM(context, 1.0f, -1.0f);
		CGContextTranslateCTM(context, 0.0f, -(frame.size.height - titlebarHeight));

		CGContextBeginPath (context);
		CGContextAddRect (context, CGRectMake (0.0f, frame.size.height - gradientHeight, frame.size.width, gradientHeight));
		CGContextDrawLinearGradient (context, isMain? [WindowGradientHelper activeTitle] : [WindowGradientHelper inactiveTitle], CGPointMake (0.0f, frame.size.height), CGPointMake (0.0f, frame.size.height - gradientHeight), 0);

		DrawNativeGreyColorInRect(context, headerBorderGrey, CGRectMake(0.0f, frame.size.height - gradientHeight - 1, frame.size.width, 1.0f), isMain);

		CGContextRestoreGState (context);
      release_context (window, context);
      return;
    }
  else if (IS_DETAIL (detail, "menubar"))
    {
      /* TODO: What about vertical menubars? */
      HIThemeMenuBarDrawInfo draw_info;
      HIThemePlacardDrawInfo bg_draw_info;
      HIRect rect;
      HIRect bg_rect;
      CGContextRef context;

      draw_info.version = 0;
      draw_info.state = kThemeMenuBarNormal;

      bg_draw_info.version = 0;
      bg_draw_info.state = kThemeStateActive;

      /* We paint the Rect with a shift of 10 pixels on both sides to avoid
       * round borders.
       */
      rect = CGRectMake (x-10, y, width+20, 22);

      /* FIXME?: We shift one pixel to avoid the grey border. */
      bg_rect = CGRectMake (x-1, y-1, width+2, height+2);

      context = get_context (window, area);
      if (!context)
        return;

      /* We fill the whole area with the background since the menubar has a
       * fixed height of 22 pixels. Ignore people with more than one text
       * line in menubars.
       */
      HIThemeDrawPlacard (&bg_rect, &bg_draw_info, context, kHIThemeOrientationNormal);
      HIThemeDrawMenuBarBackground (&rect, &draw_info, context, kHIThemeOrientationNormal);

      release_context (window, context);

      return;
    }
  else if (IS_DETAIL (detail, "menu"))
    {
      GtkWidget *toplevel;
      HIThemeMenuDrawInfo draw_info = { 0 };
      CGRect content_rect, window_rect;
      CGContextRef context;
      NSWindow* native;

      draw_info.version = kHIThemeMenuDrawInfoVersionOne;
      draw_info.menuType = kThemeMenuTypePopUp;

      toplevel = gtk_widget_get_toplevel (widget);
      native = gdk_quartz_window_get_nswindow (toplevel->window);

      window_rect = CGRectMake (x, y, width, height);

      // This has to be inset from window_rect because HIThemeDrawMenuBackground draws outside the passed rect
      content_rect = CGRectInset (window_rect, 0, 4);

      context = get_context (window, area);
      if (!context)
        return;

      CGContextClearRect (context, window_rect);
      HIThemeDrawMenuBackground (&content_rect, &draw_info, context, kHIThemeOrientationNormal);

      release_context (window, context);
      [native invalidateShadow];
      return;
    }
  else if (IS_DETAIL (detail, "menuitem"))
    {
      quartz_draw_menu_item (style,
                             window,
                             state_type,
                             widget);

      return;
    }
  else if (IS_DETAIL (detail, "spinbutton"))
    {
      CGContextRef context;
      HIRect rect;
      HIThemePlacardDrawInfo placard_info;
      HIThemeButtonDrawInfo draw_info;

      /* Draw the background texture to paint over the bg background
       * that the spinbutton draws.
       */
      placard_info.version = 0;
      placard_info.state = kThemeStateActive;

      rect = CGRectMake (x - 1, y - 1, width + 2, height + 2);

      context = get_context (window, area);
      if (!context)
        return;

      HIThemeDrawPlacard (&rect, &placard_info, context, kHIThemeOrientationNormal);

      /* And the arrows... */
      draw_info.version = 0;
      draw_info.kind = kThemeIncDecButton;
      draw_info.adornment = kThemeAdornmentNone;
      draw_info.value = kThemeButtonOff;

      if (state_type == GTK_STATE_INSENSITIVE)
        draw_info.state = kThemeStateInactive;
      else if (GTK_SPIN_BUTTON (widget)->click_child == GTK_ARROW_DOWN)
        draw_info.state = kThemeStatePressedDown;
      else if (GTK_SPIN_BUTTON (widget)->click_child == GTK_ARROW_UP)
        draw_info.state = kThemeStatePressedUp;
      else
        draw_info.state = kThemeStateActive;

      rect = CGRectMake (x-2, y+1, width, height);

      HIThemeDrawButton (&rect,
                         &draw_info,
                         context,
                         kHIThemeOrientationNormal,
                         NULL);

      release_context (window, context);

      return;
    }
  else if (IS_DETAIL (detail, "spinbutton_up") ||
           IS_DETAIL (detail, "spinbutton_down"))
    {
      return; /* Ignore. */
    }
  else if (GTK_IS_PROGRESS_BAR (widget) && IS_DETAIL (detail, "trough"))
    {
      CGContextRef context;
      HIRect rect;
      HIThemeTrackDrawInfo draw_info;

      draw_info.version = 0;
      draw_info.reserved = 0;
      draw_info.filler1 = 0;
      draw_info.kind = kThemeLargeProgressBar;

      if (state_type == GTK_STATE_INSENSITIVE)
        draw_info.enableState = kThemeTrackInactive;
      else
        draw_info.enableState = kThemeTrackActive;

      rect = CGRectMake (x, y, width, height);

      draw_info.bounds = rect;
      draw_info.min = 0;
      draw_info.max = 100;
      draw_info.value = gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (widget)) * 100;
      draw_info.trackInfo.progress.phase = 0; /* for indeterminate ones */

      switch (gtk_progress_bar_get_orientation (GTK_PROGRESS_BAR (widget)))
        {
        case GTK_PROGRESS_LEFT_TO_RIGHT:
        case GTK_PROGRESS_RIGHT_TO_LEFT:
          draw_info.attributes = kThemeTrackHorizontal;
          break;

        default:
          draw_info.attributes = 0;
          break;
        }

      context = get_context (window, area);
      if (!context)
        return;

      HIThemeDrawTrack (&draw_info, NULL, context, kHIThemeOrientationNormal);

      release_context (window, context);

      return;
    }
  else if (GTK_IS_PROGRESS_BAR (widget) && IS_DETAIL (detail, "bar"))
    {
      return; /* Ignore. */
    }
  else if (GTK_IS_SCALE (widget) && IS_DETAIL (detail, "trough"))
    {
      CGContextRef context;
      HIRect rect;
      HIThemeTrackDrawInfo draw_info;
      GtkAdjustment *adj;

      draw_info.version = 0;
      draw_info.reserved = 0;
      draw_info.filler1 = 0;
      draw_info.kind = kThemeSlider;

      if (state_type == GTK_STATE_INSENSITIVE)
        draw_info.enableState = kThemeTrackInactive;
      else
        draw_info.enableState = kThemeTrackActive;

      rect = CGRectMake (x, y, width, height);

      draw_info.bounds = rect;

      adj = gtk_range_get_adjustment (GTK_RANGE (widget));

      draw_info.min = adj->lower;
      draw_info.max = adj->upper;
      draw_info.value = adj->value;

      draw_info.attributes = kThemeTrackShowThumb | kThemeTrackThumbRgnIsNotGhost;

      if (GTK_IS_HSCALE (widget))
        draw_info.attributes |= kThemeTrackHorizontal;

      if (GTK_WIDGET_HAS_FOCUS (widget))
        draw_info.attributes |= kThemeTrackHasFocus;

      draw_info.trackInfo.slider.thumbDir = kThemeThumbPlain;

      context = get_context (window, area);
      if (!context)
        return;

      HIThemeDrawTrack (&draw_info, NULL, context, kHIThemeOrientationNormal);

      release_context (window, context);

      return;
    }
  else if (GTK_IS_SCALE (widget))
    {
      return; /* Ignore. */
    }
  else if (GTK_IS_SCROLLBAR (widget) && IS_DETAIL (detail, "trough"))
    {
      CGContextRef context;
      CGRect rect;
      HIThemeTrackDrawInfo draw_info;
      GtkAdjustment *adj;
      gint view_size;

      draw_info.version = 0;
      draw_info.reserved = 0;
      draw_info.filler1 = 0;
      draw_info.kind = kThemeScrollBarMedium;

      if (state_type == GTK_STATE_INSENSITIVE)
        draw_info.enableState = kThemeTrackInactive;
      else
        draw_info.enableState = kThemeTrackActive;

      rect = CGRectMake (x, y, width, height);

      draw_info.bounds = rect;

      adj = gtk_range_get_adjustment (GTK_RANGE (widget));

      /* NOTE: view_size != page_size, see:
       * http://lists.apple.com/archives/Carbon-development/2002/Sep/msg01922.html
       */
      view_size = (adj->page_size / (adj->upper - adj->lower) * INT_MAX);

      /* Hackery needed because min, max, and value only accept integers. */
      draw_info.min = 0;
      draw_info.max = INT_MAX - view_size;
      draw_info.value = ((adj->value - adj->lower) / adj->upper) * INT_MAX;

      draw_info.trackInfo.scrollbar.viewsize = view_size;
      draw_info.trackInfo.scrollbar.pressState = 0;

      draw_info.attributes = kThemeTrackShowThumb | kThemeTrackThumbRgnIsNotGhost;

      if (GTK_IS_HSCROLLBAR (widget))
        draw_info.attributes |= kThemeTrackHorizontal;

      if (GTK_WIDGET_HAS_FOCUS (widget))
        draw_info.attributes |= kThemeTrackHasFocus;

      //draw_info.trackInfo.slider.thumbDir = kThemeThumbPlain;

      context = get_context (window, area);
      if (!context)
        return;

      HIThemeDrawTrack (&draw_info, NULL, context, kHIThemeOrientationNormal);

      release_context (window, context);

      return;
    }
  else if (GTK_IS_SCROLLBAR (widget))
    {
      return; /* Ignore. */
    }

  const gchar *name = gtk_widget_get_name (widget);
  if (name && strcmp (name, "gtk-tooltips") == 0)
    {
      /* FIXME: draw tooltips. */
    }
/*
  parent_class->draw_box (style, window, state_type, shadow_type, area,
                          widget, detail, x, y, width, height);
*/
}

/* This hack is a fix for Xamarin bug #10021.
 *  It prevents the context menu from flickering by changing the window properties earlier
 *  than draw_box.
 */
static void
set_background (GtkStyle               *style,
                GdkWindow              *window,
                GtkStateType            state_type)
{
    GtkWidget *widget = NULL;
    NSWindow  *native = NULL;

    gdk_window_get_user_data (window, &widget);

    if (!GTK_IS_MENU (widget))
    {
        parent_class->set_background (style, window, state_type);
        return;
    }

    native = gdk_quartz_window_get_nswindow (window);

    if ([native isOpaque])
    {
        [native setOpaque:NO];
        [native setBackgroundColor:[NSColor clearColor]];

        // Round the menu corners and add blur- this trick "borrowed" from
        // http://hg.mozilla.org/mozilla-central/file/7a52ba9b1542/widget/cocoa/nsCocoaWindow.mm
        // http://git.chromium.org/gitweb/?p=git/chromium.git;a=blob;f=chrome/browser/ui/cocoa/info_bubble_window.mm;h=b04c1914a86812a80b1f2edfe2d6ecc53b84eba6;hb=4d86f67745b73fc9cc9bf53c74265d6a21903cf2

        int cid = _CGSDefaultConnection ();
        CGSSetWindowShadowAndRimParameters (cid, [native windowNumber], 10.0f, 0.44f, 0, 10, 512);
#if 0
        void* menu_blur;
        if (CGSNewCIFilterByName (cid, @"CIGaussianBlur", &menu_blur) == kCGErrorSuccess)
        {
            NSDictionary *options = [NSDictionary dictionaryWithObject:[NSNumber numberWithFloat:1.5] forKey:@"inputRadius"];
            if (CGSSetCIFilterValuesFromDictionary (cid, menu_blur, options) == kCGErrorSuccess)
                CGSAddWindowFilter (cid, [native windowNumber], menu_blur, 1);
            else
                [options release];
            CGSReleaseCIFilter (cid, menu_blur);
        }
#endif
    }
}

static void
draw_check (GtkStyle      *style,
            GdkWindow     *window,
            GtkStateType   state_type,
            GtkShadowType  shadow_type,
            GdkRectangle  *area,
            GtkWidget     *widget,
            const gchar   *detail,
            gint           x,
            gint           y,
            gint           width,
            gint           height)
{
  DEBUG_DRAW;

  /* FIXME: Refactor and share with the other button drawing
   * functions, and radiobuttons.
   */
  if (IS_DETAIL (detail, "checkbutton"))
    {
      CGContextRef context;
      HIRect rect;
      HIThemeButtonDrawInfo draw_info;

      draw_info.version = 0;
      draw_info.kind = kThemeCheckBox;
      draw_info.adornment = kThemeAdornmentNone;

      /* FIXME: might want this? kThemeAdornmentDrawIndicatorOnly */
      if (gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (widget)))
        draw_info.value = kThemeButtonMixed;
      else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
        draw_info.value = kThemeButtonOn;
      else
        draw_info.value = kThemeButtonOff;

      if (state_type == GTK_STATE_ACTIVE)
        draw_info.state = kThemeStatePressed;
      else if (state_type == GTK_STATE_INSENSITIVE)
        draw_info.state = kThemeStateInactive;
      else
        draw_info.state = kThemeStateActive;

      //if (GTK_WIDGET_HAS_FOCUS (widget))
      //  draw_info.adornment |= kThemeAdornmentFocus;

      if (IS_DETAIL (detail, "buttondefault"))
        draw_info.adornment |= kThemeAdornmentDefault;

      rect = CGRectMake (x, y, width, height);

      context = get_context (window, area);
      if (!context)
        return;

      HIThemeDrawButton (&rect,
                         &draw_info,
                         context,
                         kHIThemeOrientationNormal,
                         NULL);

      release_context (window, context);

      return;
    }
  else if (IS_DETAIL (detail, "cellcheck"))
    {
      CGContextRef context;
      HIRect rect;
      HIThemeButtonDrawInfo draw_info;

      // FIXME: check if this can be merged with detail==checkbutton

      draw_info.version = 0;
      draw_info.kind = kThemeCheckBox;
      draw_info.adornment = kThemeAdornmentNone;

      draw_info.value = kThemeButtonOff;
      draw_info.state = kThemeStateActive;

      if (shadow_type == GTK_SHADOW_IN)
        {
          draw_info.value = kThemeButtonOn;
        }

      switch (state_type)
        {
        case GTK_STATE_INSENSITIVE:
          draw_info.state = kThemeStateInactive;
          break;

        case GTK_STATE_SELECTED:
          draw_info.state = kThemeStatePressed;
          draw_info.adornment |= kThemeAdornmentFocus;
          break;

        case GTK_STATE_ACTIVE:
          draw_info.state = kThemeStatePressed;
          break;

        default:
          break;
        }

      rect = CGRectMake (x, y+1, width, height);

      context = get_context (window, area);
      if (!context)
        return;

      HIThemeDrawButton (&rect,
                         &draw_info,
                         context,
                         kHIThemeOrientationNormal,
                         NULL);

      release_context (window, context);

      return;
    }
  else if (GTK_IS_MENU(gtk_widget_get_parent (widget)))
    {
      quartz_draw_menu_checkmark (style,
                                  window,
                                  state_type,
                                  shadow_type,
                                  area,
                                  widget,
                                  detail,
                                  x,
                                  y,
                                  width,
                                  height);

      return;
    }
}

static void
draw_option (GtkStyle      *style,
             GdkWindow     *window,
             GtkStateType   state_type,
             GtkShadowType  shadow,
             GdkRectangle  *area,
             GtkWidget     *widget,
             const gchar   *detail,
             gint           x,
             gint           y,
             gint           width,
             gint           height)
{
  DEBUG_DRAW;

  /* FIXME: Refactor and share with the other button drawing
   * functions, and radiobuttons.
   */
  if (IS_DETAIL (detail, "radiobutton"))
    {
      CGContextRef context;
      HIRect rect;
      HIThemeButtonDrawInfo draw_info;

      draw_info.version = 0;
      draw_info.kind = kThemeRadioButton;
      draw_info.adornment = kThemeAdornmentNone;

      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
        draw_info.value = kThemeButtonOn;
      else
        draw_info.value = kThemeButtonOff;

      if (state_type == GTK_STATE_ACTIVE)
        draw_info.state = kThemeStatePressed;
      else if (state_type == GTK_STATE_INSENSITIVE)
        draw_info.state = kThemeStateInactive;
      else
        draw_info.state = kThemeStateActive;

      //if (GTK_WIDGET_HAS_FOCUS (widget))
      //  draw_info.adornment |= kThemeAdornmentFocus;

      if (IS_DETAIL (detail, "buttondefault"))
        draw_info.adornment |= kThemeAdornmentDefault;

      rect = CGRectMake (x, y-1, width, height);

      context = get_context (window, area);
      if (!context)
        return;

      HIThemeDrawButton (&rect,
                         &draw_info,
                         context,
                         kHIThemeOrientationNormal,
                         NULL);

      release_context (window, context);

      return;
    }
  else if (GTK_IS_MENU(gtk_widget_get_parent (widget)))
    {
      quartz_draw_menu_checkmark (style,
                                  window,
                                  state_type,
                                  shadow,
                                  area,
                                  widget,
                                  detail,
                                  x,
                                  y,
                                  width,
                                  height);

      return;
    }
}

static void
draw_tab (GtkStyle      *style,
          GdkWindow     *window,
          GtkStateType   state,
          GtkShadowType  shadow,
          GdkRectangle  *area,
          GtkWidget     *widget,
          const gchar   *detail,
          gint           x,
          gint           y,
          gint           width,
          gint           height)
{
  DEBUG_DRAW;
  return;
}

typedef struct {
  GtkWidget *notebook;
  gint       max_x;
} FindLastNotebookTabData;

static void
find_last_notebook_tab_forall (GtkWidget *widget,
                               gpointer   user_data)
{
  FindLastNotebookTabData *data = user_data;

  if (gtk_widget_get_parent (widget) != data->notebook)
    return;

  /* Filter out all the non-tab-label widgets by checking if the child has a
   * tab label in which case it isn't one... Very ugly.
   */
  if (gtk_notebook_get_tab_label (GTK_NOTEBOOK (data->notebook), widget))
    return;

  if (data->max_x < widget->allocation.x)
    data->max_x = widget->allocation.x;
}

static void
draw_extension (GtkStyle        *style,
                GdkWindow       *window,
                GtkStateType     state_type,
                GtkShadowType    shadow_type,
                GdkRectangle    *area,
                GtkWidget       *widget,
                const gchar     *detail,
                gint             x,
                gint             y,
                gint             width,
                gint             height,
                GtkPositionType  gap_side)
{
  DEBUG_DRAW;

  if (widget && GTK_IS_NOTEBOOK (widget) && IS_DETAIL (detail, "tab"))
    {
      HIRect rect, out_rect;
      HIThemeTabDrawInfo draw_info;
      CGContextRef context;

      context = get_context (window, area);
      if (!context)
        return;

      /* -1 to get the text label centered vertically. */
      rect = CGRectMake (x, y-1, width, height);

      draw_info.version = 1;
      draw_info.direction = kThemeTabNorth;
      draw_info.size = kHIThemeTabSizeNormal;
      draw_info.adornment = kHIThemeTabAdornmentTrailingSeparator;
      draw_info.kind = kHIThemeTabKindNormal;

#if 0
      /* This is something that we should do but first we have to make GTK+
       * invalidate the entire window when losing focus. Split the test out
       * and share it in all drawing functions.
       */
      if (!gtk_window_has_toplevel_focus (GTK_WINDOW (gtk_widget_get_toplevel (widget))))
        ...
#endif

      if (state_type == GTK_STATE_ACTIVE)
        draw_info.style = kThemeTabNonFront;
      else if (state_type == GTK_STATE_INSENSITIVE)
        draw_info.style = kThemeTabNonFrontInactive;
      else
        draw_info.style = kThemeTabFront;

      /* Try to draw notebook tabs okish. It's quite hacky but oh well... */
      if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (widget)) == 1)
        draw_info.position = kHIThemeTabPositionOnly;
      else
        {
          FindLastNotebookTabData data;
          gint border_width;
          gint extra_width;

          data.notebook = widget;
          data.max_x = 0;
          gtk_container_forall (GTK_CONTAINER (widget), find_last_notebook_tab_forall, &data);

          border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));
          extra_width = GTK_NOTEBOOK (widget)->tab_hborder + widget->style->xthickness;

          /* This might need some tweaking to work in all cases. */
          if (x == widget->allocation.x + border_width)
            draw_info.position = kHIThemeTabPositionFirst;
          else if (x == data.max_x - extra_width)
            draw_info.position = kHIThemeTabPositionLast;
          else
            draw_info.position = kHIThemeTabPositionMiddle;
        }

      HIThemeDrawTab (&rect,
                      &draw_info,
                      context,
                      kHIThemeOrientationNormal,
                      &out_rect);

      release_context (window, context);
    }

  parent_class->draw_extension (style, window, state_type,
                                shadow_type, area, widget, detail,
                                x, y, width, height, gap_side);

}

static void
draw_box_gap (GtkStyle        *style,
              GdkWindow       *window,
              GtkStateType     state_type,
              GtkShadowType    shadow_type,
              GdkRectangle    *area,
              GtkWidget       *widget,
              const gchar     *detail,
              gint             x,
              gint             y,
              gint             width,
              gint             height,
              GtkPositionType  gap_side,
              gint             gap_x,
              gint             gap_width)
{
  DEBUG_DRAW;


  parent_class->draw_box_gap (style, window, state_type, shadow_type,
                              area, widget, detail, x, y, width, height,
                              gap_side, gap_x, gap_width);

}

static void
draw_flat_box (GtkStyle      *style,
               GdkWindow     *window,
               GtkStateType   state_type,
               GtkShadowType  shadow_type,
               GdkRectangle  *area,
               GtkWidget     *widget,
               const gchar   *detail,
               gint           x,
               gint           y,
               gint           width,
               gint           height)
{
  DEBUG_DRAW;

  sanitize_size (window, &width, &height);

  GtkWidget* statusbar = is_in_statusbar(widget);
  if (statusbar) // FIXME: ugly hack
  {
	  GtkAllocation statusRect;
	  gtk_widget_get_allocation (statusbar, &statusRect);
	  quartz_draw_statusbar (style, gtk_widget_get_window (statusbar), state_type, statusbar, detail, x - 2, statusRect.y - 1, width + 4, statusRect.height + 4);

      return;
  }
  if (IS_DETAIL (detail, "base") ||
      IS_DETAIL (detail, "viewportbin") ||
      IS_DETAIL (detail, "eventbox"))
    {
      HIThemePlacardDrawInfo draw_info;
      HIRect rect;
      CGContextRef context;

      gdk_window_get_size (window, &width, &height);

      /* The background is not drawn to include the corners if we don't
       * pad with one pixel
       */
      rect = CGRectMake (-1, -1, width+2, height+2);

      draw_info.version = 0;
      draw_info.state = kThemeStateActive;

      context = get_context (window, area);
      if (!context)
        return;

      HIThemeDrawPlacard (&rect, &draw_info, context, kHIThemeOrientationNormal);

      release_context (window, context);

      return;
    }
  if (GTK_IS_PROGRESS_BAR (widget) && IS_DETAIL (detail, "trough"))
    {
      CGContextRef context;
      HIRect rect;
      HIThemePlacardDrawInfo placard_info;

      /* Draw the background texture to paint over the black bg that
       * the progressbar draws.
       */
      placard_info.version = 0;
      placard_info.state = kThemeStateActive;

      gdk_drawable_get_size (window, &width, &height);

      rect = CGRectMake (x - 2, y - 1, width + 4, height + 4);

      context = get_context (window, area);
      if (!context)
        return;

      HIThemeDrawPlacard (&rect, &placard_info, context, kHIThemeOrientationNormal);

      release_context (window, context);

      return;
    }
  if (IS_DETAIL (detail, "checkbutton"))
    {
      /* We don't want any background, no prelight etc. */
      return;
    }
  if (IS_DETAIL (detail, "cell_even") || IS_DETAIL (detail, "cell_odd") ||
      IS_DETAIL (detail, "cell_even_ruled") || IS_DETAIL (detail, "cell_odd_ruled"))
    {
      /* FIXME: Should draw using HITheme, or get the right selection
       * color.
       */
      parent_class->draw_flat_box (style, window, state_type, shadow_type,
                                   area, widget, detail, x, y, width, height);
      return;
    }
	/*
  parent_class->draw_flat_box (style, window, state_type, shadow_type,
			       area, widget, detail, x, y, width, height);
	 */
}

static void
draw_expander (GtkStyle         *style,
               GdkWindow        *window,
               GtkStateType      state,
               GdkRectangle     *area,
               GtkWidget        *widget,
               const gchar      *detail,
               gint              x,
               gint              y,
               GtkExpanderStyle  expander_style)
{
  DEBUG_DRAW;

  parent_class->draw_expander (style, window, state, area, widget,
                               detail, x, y, expander_style);
}

static void
draw_shadow (GtkStyle      *style,
             GdkWindow     *window,
             GtkStateType   state_type,
             GtkShadowType  shadow_type,
             GdkRectangle  *area,
             GtkWidget     *widget,
             const gchar   *detail,
             gint           x,
             gint           y,
             gint           width,
             gint           height)
{
  DEBUG_DRAW;

  sanitize_size (window, &width, &height);

  /* Handle shadow in and etched in for scrolled windows, frames and
   * entries.
   */
  if (IS_DETAIL (detail, "frame") && is_in_statusbar (widget))
    {
		if (height <= 1)
			return;

		quartz_draw_statusbar (style, window, state_type, widget, detail, x, y, width, height);

		return;
    }
  else if ((GTK_IS_SCROLLED_WINDOW (widget) && IS_DETAIL (detail, "scrolled_window")) ||
     /* (GTK_IS_FRAME (widget) && IS_DETAIL (detail, "frame")) || */
      (GTK_IS_ENTRY (widget) && IS_DETAIL (detail, "entry")))
    {
      GtkShadowType shadow_type = GTK_SHADOW_NONE;

       if (GTK_IS_SCROLLED_WINDOW (widget))
        shadow_type = gtk_scrolled_window_get_shadow_type (GTK_SCROLLED_WINDOW (widget));
      else if (GTK_IS_FRAME (widget))
        shadow_type = gtk_frame_get_shadow_type (GTK_FRAME (widget));
      else if (GTK_IS_ENTRY (widget))
        shadow_type = GTK_SHADOW_IN;

      if (shadow_type == GTK_SHADOW_IN || shadow_type == GTK_SHADOW_ETCHED_IN)
        {
          CGContextRef context;
          HIRect rect;
          HIThemeFrameDrawInfo draw_info;

          draw_info.version = 0;
          /* Could use ListBox for treeviews, but textframe looks good. */
          draw_info.kind = kHIThemeFrameTextFieldSquare;

          if (state_type == GTK_STATE_INSENSITIVE)
            draw_info.state = kThemeStateInactive;
          else
            draw_info.state = kThemeStateActive;

          draw_info.isFocused = GTK_WIDGET_HAS_FOCUS (widget);

          rect = CGRectMake (x+1, y+1, width-2, height-2);

          context = get_context (window, area);
          if (!context)
            return;

          HIThemeDrawFrame (&rect,
                            &draw_info,
                            context,
                            kHIThemeOrientationNormal);

          if (GTK_WIDGET_HAS_FOCUS (widget))
            HIThemeDrawFocusRect (&rect, true, context, kHIThemeOrientationNormal);

          release_context (window, context);
        }

      return;
    }

  else if (GTK_IS_SCALE (widget))
    return; /* Ignore. */

  return;
  g_print ("Missing implementation of draw_shadow for %s\n", detail);
  parent_class->draw_shadow (style, window, state_type, shadow_type, area,
                             widget, detail, x, y, width, height);
}

static void
draw_shadow_gap (GtkStyle        *style,
                 GdkWindow       *window,
                 GtkStateType     state_type,
                 GtkShadowType    shadow_type,
                 GdkRectangle    *area,
                 GtkWidget       *widget,
                 const gchar     *detail,
                 gint             x,
                 gint             y,
                 gint             width,
                 gint             height,
                 GtkPositionType  gap_side,
                 gint             gap_x,
                 gint             gap_width)
{
  DEBUG_DRAW;

  sanitize_size (window, &width, &height);

  g_print ("Missing implementation of draw_shadow_gap for %s\n", detail);

  parent_class->draw_shadow_gap (style, window, state_type, shadow_type, area,
                                 widget, detail, x, y, width, height,
                                 gap_side, gap_x, gap_width);

 }

static void
draw_hline (GtkStyle     *style,
            GdkWindow    *window,
            GtkStateType  state_type,
            GdkRectangle *area,
            GtkWidget    *widget,
            const gchar  *detail,
            gint          x1,
            gint          x2,
            gint          y)
{
  DEBUG_DRAW;

  if (IS_DETAIL (detail, "menuitem"))
    {
      CGRect menu_rect, item_rect;
      HIThemeMenuItemDrawInfo draw_info;
      CGContextRef context;
      GtkWidget *toplevel;
      gint width, height;

      // FIXME: refactor out and share with draw_box::menuitem

      draw_info.version = 0;
      draw_info.itemType = kThemeMenuItemPlain;
      draw_info.itemType |= kThemeMenuItemPopUpBackground;

      if (state_type == GTK_STATE_INSENSITIVE)
        draw_info.state = kThemeMenuDisabled;
      else if (state_type == GTK_STATE_PRELIGHT)
        draw_info.state = kThemeMenuSelected;
      else
        draw_info.state = kThemeMenuActive;

      toplevel = gtk_widget_get_toplevel (widget);
      gdk_window_get_size (toplevel->window, &width, &height);

      menu_rect = CGRectMake (0, 0, width, height);
      item_rect = CGRectMake (0, y + 3, width, 1);

      context = get_context (window, area);
      if (!context)
        return;

      HIThemeDrawMenuSeparator (&menu_rect,
                                &item_rect,
                                &draw_info,
                                context,
                                kHIThemeOrientationNormal);

      release_context (window, context);

      return;
    }
}

static void
draw_vline (GtkStyle     *style,
            GdkWindow    *window,
            GtkStateType  state_type,
            GdkRectangle *area,
            GtkWidget    *widget,
            const gchar  *detail,
            gint          y1,
            gint          y2,
            gint          x)
{
/*
	if (GTK_IS_COMBO_BOX (widget) || is_combo_box_child (widget))
		return;

	NSWindow* wnd;
	CGContextRef context;

	wnd     = gdk_quartz_window_get_nswindow (window);
	context = get_context (window, area);
	if (!wnd || !context)
        return;

	BOOL isActive = [wnd isMainWindow] && (state_type != GTK_STATE_INSENSITIVE);

	DrawNativeGreyColorInRect (context, headerBorderGrey, CGRectMake (x, y1, 1, y2 - y1), isActive);

	release_context (window, context);
*/
	return;
}

static void
draw_slider (GtkStyle       *style,
             GdkWindow      *window,
             GtkStateType    state_type,
             GtkShadowType   shadow_type,
             GdkRectangle   *area,
             GtkWidget      *widget,
             const gchar    *detail,
             gint            x,
             gint            y,
             gint            width,
             gint            height,
             GtkOrientation  orientation)
{
  DEBUG_DRAW;
#if 0
  if (0  && GTK_IS_SCROLLBAR (widget))
    {
      return; /* Ignore. */
    }

    parent_class->draw_slider (style, window, state_type, shadow_type, area,
      widget, detail, x, y, width, height,
      orientation);
#endif
}

static void
draw_resize_grip (GtkStyle      *style,
                  GdkWindow     *window,
                  GtkStateType   state_type,
                  GdkRectangle  *area,
                  GtkWidget     *widget,
                  const gchar   *detail,
                  GdkWindowEdge  edge,
                  gint           x,
                  gint           y,
                  gint           width,
                  gint           height)
{
  DEBUG_DRAW;

  sanitize_size (window, &width, &height);

  CGContextRef context;
/*
	HIPoint origin;
	HIThemeGrowBoxDrawInfo drawInfo;

	origin = CGPointMake (x, y);
	drawInfo.version = 0;
	drawInfo.kind = kHIThemeGrowBoxKindNormal;
	drawInfo.size = kHIThemeGrowBoxSizeNormal;

	if (state_type == GTK_STATE_INSENSITIVE)
		drawInfo.state = kThemeStateInactive;
	else
		drawInfo.state = kThemeStateActive;

	switch (edge) {

		case GDK_WINDOW_EDGE_NORTH:
			drawInfo.direction = kThemeGrowUp;
			break;

		case GDK_WINDOW_EDGE_NORTH_EAST:
			drawInfo.direction = kThemeGrowUp | kThemeGrowRight;
			break;

		case GDK_WINDOW_EDGE_EAST:
			drawInfo.direction = kThemeGrowRight;
			break;

		case GDK_WINDOW_EDGE_SOUTH_EAST:
			drawInfo.direction = kThemeGrowDown | kThemeGrowRight;
			break;

		case GDK_WINDOW_EDGE_SOUTH:
			drawInfo.direction = kThemeGrowDown;
			break;

		case GDK_WINDOW_EDGE_SOUTH_WEST:
			drawInfo.direction = kThemeGrowDown | kThemeGrowLeft;
			break;

		case GDK_WINDOW_EDGE_WEST:
			drawInfo.direction = kThemeGrowLeft;
			break;

		case GDK_WINDOW_EDGE_NORTH_WEST:
			drawInfo.direction = kThemeGrowUp | kThemeGrowLeft;
			break;
	}
*/
    GtkWidget* statusbar;
	if (IS_DETAIL(detail, "statusbar") && (statusbar = is_in_statusbar(widget))) {
		GtkAllocation statusRect;
		gtk_widget_get_allocation (statusbar, &statusRect);
		quartz_draw_statusbar (style, gtk_widget_get_window (statusbar), state_type, statusbar, detail, x, statusRect.y, width, statusRect.height);
	}

	context = get_context (window, area);
	if (!context)
        return;

	// This doesn't work well: it's always opaque and not the right grow box for "textured" areas -- i.e. status bar
	//HIThemeDrawGrowBox(&origin, &drawInfo, context, kHIThemeOrientationNormal);

	NSWindow* wnd = gdk_quartz_window_get_nswindow (window);
	if (!wnd)
		return;

	CGContextSaveGState (context);
	CGContextScaleCTM (context, 1.0f, -1.0f);
	CGContextTranslateCTM (context, 0.0f, -([wnd frame].size.height - [WindowGradientHelper titleBarHeight]));

	// HACK! Instead of using hitheme, we'll use some undocumented Cocoa apis (NSThemeFrame) to draw the right resize grip...

	id themeFrame = [[wnd contentView] superview];

	// ... but only if its supported!
	if ([themeFrame respondsToSelector:@selector(_drawGrowBoxWithClip:)]) {

		NSGraphicsContext* old = [NSGraphicsContext currentContext];
		NSGraphicsContext* ngctx = [NSGraphicsContext graphicsContextWithGraphicsPort: context flipped: NO];

		[NSGraphicsContext setCurrentContext: ngctx];
		[themeFrame _drawGrowBoxWithClip: NSMakeRect (x,  0, width, height)];
		[NSGraphicsContext setCurrentContext: old];

	}

	CGContextRestoreGState (context);
	release_context (window, context);
}

static void
draw_handle (GtkStyle       *style,
             GdkWindow      *window,
             GtkStateType    state_type,
             GtkShadowType   shadow_type,
             GdkRectangle   *area,
             GtkWidget      *widget,
             const gchar    *detail,
             gint            x,
             gint            y,
             gint            width,
             gint            height,
             GtkOrientation  orientation)
{
  DEBUG_DRAW;

  sanitize_size (window, &width, &height);

  if (GTK_IS_PANED (widget) && IS_DETAIL (detail, "paned"))
    {
      HIThemeSplitterDrawInfo draw_info;
      CGRect rect;
      CGContextRef context;

      draw_info.version = 0;
      draw_info.state = kThemeStateActive;
      draw_info.adornment = kHIThemeSplitterAdornmentNone;

      rect = CGRectMake (x, y, width, height);

      context = get_context (window, area);
      if (!context)
        return;

      HIThemeDrawPaneSplitter (&rect,
                               &draw_info,
                               context,
                               GTK_IS_HPANED (widget) ?
                               kHIThemeOrientationNormal :
                               kHIThemeOrientationInverted);

      release_context (window, context);

      return;
    }
	/*
	parent_class->draw_handle (style, window, state_type, shadow_type,
							   area, widget, detail, x, y, width, height, orientation);
	 */
}

static void
draw_focus (GtkStyle     *style,
            GdkWindow    *window,
            GtkStateType  state_type,
            GdkRectangle *area,
            GtkWidget    *widget,
            const gchar  *detail,
            gint          x,
            gint          y,
            gint          width,
            gint          height)
{
  DEBUG_DRAW;

#if 0
  CGRect rect;
  CGContext context;

  context = get_context (window, area);
  if (!context)
    return;

  sanitize_size (window, &width, &height);
  rect = CGRectMake (x, y, width, height);

  HIThemeDrawFocusRect (&rect, TRUE, context, kHIThemeOrientationNormal);

  release_context (window, context);
#endif
}

static void
draw_layout (GtkStyle     *style,
             GdkWindow    *window,
             GtkStateType  state_type,
             gboolean      use_text,
             GdkRectangle *area,
             GtkWidget    *widget,
             const gchar  *detail,
             gint          x,
             gint          y,
             PangoLayout  *layout)
{
  DEBUG_DRAW;

  if (state_type == GTK_STATE_PRELIGHT &&
      GTK_IS_PROGRESS_BAR (widget) && IS_DETAIL (detail, "progressbar"))
    {
      /* Ignore this, it looks very out of place on the mac (I think
       * the intention is to make the text clearly visible
       * independetly of the backgroun).
       */
      return;
    }

  parent_class->draw_layout (style, window, state_type, use_text,
                             area, widget, detail,
                             x, y, layout);

}

static void
quartz_style_init_from_rc (GtkStyle   *style,
                           GtkRcStyle *rc_style)
{
  QuartzStyle *quartz_style = QUARTZ_STYLE (style);
  parent_class->init_from_rc (style, rc_style);

  //style_setup_system_font (style);

  switch (QUARTZ_RC_STYLE (rc_style)->button_type) {

	case BUTTONTYPE_AQUA:
	  quartz_style->theme_button_kind = kThemePushButtonNormal;
	  break;

    case BUTTONTYPE_TEXTURED:
	  quartz_style->theme_button_kind = kThemePushButtonTextured;
	  break;

	case BUTTONTYPE_INSET:
	  quartz_style->theme_button_kind = kThemeBevelButtonInset;
	  break;

    default:
	  quartz_style->theme_button_kind = kThemePushButton;
	  break;
  }



}

static void
quartz_style_realize (GtkStyle *style)
{
  GdkGCValues gc_values;
  GdkGCValuesMask gc_values_mask;
  gint i;

  style->black.red = 0x0000;
  style->black.green = 0x0000;
  style->black.blue = 0x0000;
  gdk_colormap_alloc_color (style->colormap, &style->black, FALSE, TRUE);

  style->white.red = 0xffff;
  style->white.green = 0xffff;
  style->white.blue = 0xffff;
  gdk_colormap_alloc_color (style->colormap, &style->white, FALSE, TRUE);

  gc_values_mask = GDK_GC_FOREGROUND | GDK_GC_BACKGROUND;

  gc_values.foreground = style->black;
  gc_values.background = style->white;
  style->black_gc = gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);

  gc_values.foreground = style->white;
  gc_values.background = style->black;
  style->white_gc = gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);

  gc_values_mask = GDK_GC_FOREGROUND;

  for (i = 0; i < 5; i++)
    {
      /* FIXME: Implement bg pixmaps? */
      if (style->rc_style && style->rc_style->bg_pixmap_name[i])
        style->bg_pixmap[i] = (GdkPixmap *) GDK_PARENT_RELATIVE;

      gdk_colormap_alloc_color (style->colormap, &style->fg[i], FALSE, TRUE);
      gdk_colormap_alloc_color (style->colormap, &style->bg[i], FALSE, TRUE);
      gdk_colormap_alloc_color (style->colormap, &style->light[i], FALSE, TRUE);
      gdk_colormap_alloc_color (style->colormap, &style->dark[i], FALSE, TRUE);
      gdk_colormap_alloc_color (style->colormap, &style->mid[i], FALSE, TRUE);
      gdk_colormap_alloc_color (style->colormap, &style->text[i], FALSE, TRUE);
      gdk_colormap_alloc_color (style->colormap, &style->base[i], FALSE, TRUE);
      gdk_colormap_alloc_color (style->colormap, &style->text_aa[i], FALSE, TRUE);

      gc_values.foreground = style->fg[i];
      style->fg_gc[i] = gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);

      gc_values.foreground = style->bg[i];
      style->bg_gc[i] = gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);

      gc_values.foreground = style->light[i];
      style->light_gc[i] = gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);

      gc_values.foreground = style->dark[i];
      style->dark_gc[i] = gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);

      gc_values.foreground = style->mid[i];
      style->mid_gc[i] = gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);

      gc_values.foreground = style->text[i];
      style->text_gc[i] = gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);

      gc_values.foreground = style->base[i];
      style->base_gc[i] = gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);

      gc_values.foreground = style->text_aa[i];
      style->text_aa_gc[i] = gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);
    }
}

static void
quartz_style_unrealize (GtkStyle *style)
{
  parent_class->unrealize (style);
}

static void
quartz_style_class_init (QuartzStyleClass *klass)
{
  GtkStyleClass *style_class = GTK_STYLE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  style_class->draw_arrow = draw_arrow;
  style_class->draw_box = draw_box;
  style_class->draw_check = draw_check;
  style_class->draw_option = draw_option;
  style_class->draw_tab = draw_tab;
  style_class->draw_flat_box = draw_flat_box;
  style_class->draw_expander = draw_expander;
  style_class->draw_extension = draw_extension;
  style_class->draw_box_gap = draw_box_gap;
  style_class->draw_shadow = draw_shadow;
  style_class->draw_shadow_gap = draw_shadow_gap;
  style_class->draw_hline = draw_hline;
  style_class->draw_vline = draw_vline;
  style_class->draw_handle = draw_handle;
  style_class->draw_focus = draw_focus;
  style_class->draw_resize_grip = draw_resize_grip;
  style_class->draw_slider = draw_slider;
  style_class->draw_layout = draw_layout;
  style_class->set_background = set_background;

  style_class->init_from_rc = quartz_style_init_from_rc;
  style_class->realize = quartz_style_realize;
  style_class->unrealize = quartz_style_unrealize;
}

GType quartz_type_style = 0;

void
quartz_style_register_type (GTypeModule *module)
{
  static const GTypeInfo object_info = {
    sizeof (QuartzStyleClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) quartz_style_class_init,
    NULL,                       /* class_finalize */
    NULL,                       /* class_data */
    sizeof (QuartzStyle),
    0,                  /* n_preallocs */
    (GInstanceInitFunc) NULL,
  };

  quartz_type_style = g_type_module_register_type (module,
                                                   GTK_TYPE_STYLE,
                                                   "QuartzStyle",
                                                   &object_info, 0);
}

void
quartz_style_init (void)
{
  style_setup_settings ();
  style_setup_rc_styles ();
  [WindowGradientHelper createGradients];
}
