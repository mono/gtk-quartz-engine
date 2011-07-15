/* GTK+ theme engine for the Quartz backend
 *
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


#import <Cocoa/Cocoa.h>
#include "nsNativeThemeColors.h"

@interface WindowGradientHelper : NSColor {
	NSWindow* wnd;
	CGFloat tbarHeight;
	CGFloat sbarHeight; // may be 0 if no status bar
}
- (id) initWithWindow: (NSWindow*)window;
- (void) hook;
- (void) setToolbarHeight: (CGFloat)height;
- (void) setStatusbarHeight: (CGFloat)height;
+ (float) titleBarHeight;
- (NSWindow*) window;
- (CGFloat) toolbarHeight;
- (CGFloat) statusbarHeight;
- (CGPatternRef) createUnifiedPattern;

+ (void) createGradients;
+ (CGGradientRef) activeTitle;
+ (CGGradientRef) inactiveTitle;
+ (CGGradientRef) activeStatus;
+ (CGGradientRef) inactiveStatus;
@end 
