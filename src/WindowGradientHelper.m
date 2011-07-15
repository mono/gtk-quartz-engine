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


/*
 This approach is based on the Firefox approach to drawing unified toolbar/titlebar/statusbar gradients.
 See http://hg.mozilla.org/mozilla-central/raw-file/d3537bbc4e6a/widget/src/cocoa/nsCocoaWindow.mm
 */

#import "WindowGradientHelper.h"

static CGGradientRef aTitle, iTitle, aStatus, iStatus;

@implementation WindowGradientHelper

- (id) initWithWindow: (NSWindow*)window {
	self = [super init];
	wnd = window;
	tbarHeight = 0;
	sbarHeight = 0;
	return self;
}

- (void) hook {
	// FIXME: setStyleMask requires Snow Leopard or later..
	[wnd setStyleMask: [wnd styleMask] | NSTexturedBackgroundWindowMask];
	[wnd setAutorecalculatesContentBorderThickness: NO forEdge: NSMaxYEdge];
	[wnd setContentBorderThickness: tbarHeight forEdge: NSMaxYEdge];
}

- (void) setToolbarHeight: (CGFloat)height {
	tbarHeight = height;
}

- (void) setStatusbarHeight: (CGFloat)height {
	sbarHeight = height;
}

// http://borkware.com/quickies/single?id=274
+ (float) titleBarHeight
{
    NSRect frame = NSMakeRect (0, 0, 100, 100);
	
    NSRect contentRect;
	contentRect = [NSWindow contentRectForFrameRect: frame
										  styleMask: NSTitledWindowMask];
	
    return (frame.size.height - contentRect.size.height);
	
} // titleBarHeight

static void PatternCallback (void* aInfo, CGContextRef aContext)
{
	WindowGradientHelper* wgh = (WindowGradientHelper*)aInfo;
	
	BOOL isMain = [[wgh window] isMainWindow];
	float frameHeight = [[wgh window] frame].size.height;
	float gradientHeight = [WindowGradientHelper titleBarHeight] + [wgh toolbarHeight];
	
	// draw toolbar gradient
	CGContextDrawLinearGradient (aContext, isMain? [WindowGradientHelper activeTitle] : [WindowGradientHelper inactiveTitle], CGPointMake (0.0f, frameHeight), CGPointMake (0.0f, frameHeight - gradientHeight), 0);
	
	// draw statusbar gradient (if there is one)
	// Not needed? Gtk will always overdraw this i think
	/*
	if ([wgh statusbarHeight] != 0) {
		NSLog (@"Pattern drawing status bar of height: %f", [wgh statusbarHeight]);
		CGContextDrawLinearGradient (aContext, isMain? [WindowGradientHelper activeStatus] : [WindowGradientHelper inactiveStatus], CGPointMake (0.0f, [wgh statusbarHeight] - 3), CGPointMake (0.0f, 0.0f), 0);
	}
	 */
}

- (void)setFill
{
	CGContextRef context = (CGContextRef)[[wnd graphicsContext] graphicsPort];
	
	CGColorSpaceRef patternSpace = CGColorSpaceCreatePattern(NULL);
	CGContextSetFillColorSpace(context, patternSpace);
	CGColorSpaceRelease(patternSpace);
	
	CGPatternRef pattern = [self createUnifiedPattern];
	CGFloat component = 1.0f;
	CGContextSetFillPattern(context, pattern, &component);
	CGPatternRelease(pattern);
}

- (void)set
{
	[self setFill];
}

- (NSString*)colorSpaceName { return NSDeviceRGBColorSpace; }
- (NSWindow*) window { return wnd; }
- (CGFloat) toolbarHeight { return tbarHeight; }
- (CGFloat) statusbarHeight { return sbarHeight; }

- (CGPatternRef) createUnifiedPattern
{
	CGPatternCallbacks callbacks = {0, &PatternCallback, NULL};
	CGPatternRef pattern = CGPatternCreate(self, CGRectMake(0.0f, 0.0f, 1.0f, [wnd frame].size.height), 
										   CGAffineTransformIdentity, 1.0f, [wnd frame].size.height,
										   kCGPatternTilingConstantSpacing, true, &callbacks);
	
	return pattern;
}

+ (void) createGradients
{
	CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB ();
	CGFloat locations[2] = { 0.0, 1.0 };
	
	float start = NativeGreyColorAsFloat (headerStartGrey, YES);
	float end   = NativeGreyColorAsFloat (headerEndGrey, YES);
	{
		CGFloat components[8] = { start, start, start, 1.0f,  end, end, end, 1.0f };
		aTitle = CGGradientCreateWithColorComponents (cs, components, locations, 2);
	}
	
	start = NativeGreyColorAsFloat (headerStartGrey, NO);
	end   =  NativeGreyColorAsFloat (headerEndGrey, NO);
	{
		CGFloat components[8] = { start, start, start, 1.0f,  end, end, end, 1.0f };
		iTitle = CGGradientCreateWithColorComponents (cs, components, locations, 2);
	}
	
	start = NativeGreyColorAsFloat (statusbarGradientStartGrey, YES);
	end   =  NativeGreyColorAsFloat (statusbarGradientEndGrey, YES);
	{
		CGFloat components[8] = { start, start, start, 1.0f,  end, end, end, 1.0f };
		aStatus = CGGradientCreateWithColorComponents (cs, components, locations, 2);
	}
	
	start = NativeGreyColorAsFloat (statusbarGradientStartGrey, NO);
	end   =  NativeGreyColorAsFloat (statusbarGradientEndGrey, NO);
	{
		CGFloat components[8] = { start, start, start, 1.0f,  end, end, end, 1.0f };
		iStatus = CGGradientCreateWithColorComponents (cs, components, locations, 2);
	}
	CGColorSpaceRelease (cs);	
}

+ (CGGradientRef) activeTitle { return aTitle; }
+ (CGGradientRef) inactiveTitle { return iTitle; }
+ (CGGradientRef) activeStatus { return aStatus; }
+ (CGGradientRef) inactiveStatus { return iStatus; }

@end
