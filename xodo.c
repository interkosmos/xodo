/*

  xodometer - Track the total distance your pointing device and cursor
              travel.  The distance can be displayed in various units.

  Inspired by the MacIntosh Mouse Odometer by Sean P. Nolan.

  Template code from xneko by Masayuki Koba.

  For X11 Release 6.

  Stephen O. Lidie, 93/01/20.  lusol@Lehigh.EDU
  Stephen O. Lidie, 96/01/04.  lusol@Lehigh.EDU


  Given the number of pixels and the screen dimensions in millimeters
  we use this distance formula:

  distance = sqrt( (dX * (Xmm/Xpixels))**2 + (dY * (Ymm/Ypixels))**2 )

  Where dX and dY are pixel differentials, and Xmm, Ymm and Xpixels,
  Ypixels are the screen dimensions in millimeters and pixels,
  respectively.

  My first X application and my first graphical application too.  Yes,
  I used Xlib for the experience... never again though!

*/


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <sys/time.h>
#include <sys/errno.h>

#include "evap/evap.h"
#include "evap/xodo_pdt.out"

#include "patchlevel.h"

#include "bitmaps/icon.xbm"
#include "bitmaps/cursor.xbm"
#include "bitmaps/cur_mask.xbm"
#include "bitmaps/pulldown.xbm"

#define DEBUG           0   /* 1 IFF debug */

#define CURSOR_S        "Cursor"
#define POINTER_S       "Pointer"

/*
  EVENT_MASK1 - Top level window.
  EVENT_MASK2 - Action windows like About, Quit, Units and trip odometer reset buttons.
  EVENT_MASK3 - Menu pane windows.
*/

#define EVENT_MASK1 ButtonPressMask | ButtonReleaseMask | ExposureMask | StructureNotifyMask
#define EVENT_MASK2 ButtonPressMask | ButtonReleaseMask | OwnerGrabButtonMask | EnterWindowMask | LeaveWindowMask
#define EVENT_MASK3     ButtonPressMask | ButtonReleaseMask | ExposureMask | EnterWindowMask | LeaveWindowMask

/* The ALL_EVENTS_MASK mask is a superset of all possible events that ANY window might need. */

#define ALL_EVENTS_MASK EVENT_MASK1 | EVENT_MASK2 | EVENT_MASK3

/* Action window ordinals. */

#define TRIP1           0
#define TRIP2           1
#define QUIT            2
#define UNITS           3
#define ABOUT           4

/* Odometer ordinals. */

#define CURSOR          0
#define POINTER         1
#define BOTH            2

/* Function prototypes, sorta. */

void                    compute_font_and_resize_data();
void                    draw_action_windows();
void                    draw_menu();
void                    draw_menu_panes();
void                    draw_misc_info();
void                    draw_odometers();
void                    draw_odometer_digit();
int                     evap();
void                    evap_type_conversion();
void                    finish_xodo();
void                    highlight_action_window();
void                    initialize_graphics_contexts();
void                    initialize_menu();
void                    initialize_xodo();
Bool                    process_event();
void                    process_pointer();
void                    save_xodo();

/* Global variables, more or less alphabetically. */

char                    *about[] = {
                          "         xodometer 1.2",
                          " ",
                          "The Mac Mouse Odometer, X Style ",
                          "   Stephen O. Lidie, 96/01/04",
                          "       lusol@Lehigh.EDU",
              " ",
              "   For help:  xodo -full_help",
            };
short                   about_active = False;
int                     about_count = sizeof( about ) / sizeof( *about );
int                     about_width, about_height;
int                     about_width_old, about_height_old, about_x_old, about_y_old;
int                     about_width_x;
int                     accel_numerator, accel_denominator;
double                  acceleration;
int                     ascent;
float                   aspect;
Atom                    atom_wm_save_yourself;
int                     autosave_count, autosave_ticks;
unsigned int        BorderWidth = 2;
short                   button_depressed = False;
char                    calibration[] = "2.54 cm/in";
int                     calibration_width;
int                     coordinates_x, coordinates_y;

#if DEBUG
FILE                    *d;
#endif

int                     descent;
int                     direction;
unsigned int            display_width, display_height;
unsigned int            display_widthmm, display_heightmm;
double                  distance;
struct distances {
  char *name;
  char *abbreviation;
  Window menu_pane;
  double value;
}                       distances[] = {
                          {"millimeters",        "mm ", None, 1.0},
                          {"centimeters",        "cm ", None, 0.1},
                          {"decimeters",         "dm ", None, 0.01},
                          {"meters",             "m  ", None, 0.001},
                          {"dekameters",         "dam", None, 0.0001},
                          {"hectometers",        "hm ", None, 0.00001},
                          {"kilometers",         "km ", None, 0.000001},
                          {"myriameters",        "mym", None, 0.0000001},
                          {"inches",             "in ", None, 0.1/2.54},
                          {"feet",               "ft ", None, 0.1/2.54/12.0},
                          {"yards",              "yd ", None, 0.1/2.54/12.0/3.0},
                          {"rods",               "rd ", None, 0.1/2.54/12.0/3.0/5.5},
                          {"miles",              "mi ", None, 0.1/2.54/12.0/3.0/1760.0},
                          {"furlongs",           "fl ", None, 0.1/2.54/12.0/3.0/220.0},
                          {"fathoms",            "fm ", None, 0.1/2.54/12.0/6.0},
                          {"light-nanoseconds",  "lns", None, 0.001/299792458.0*1.0E+9},
              {"marine_leagues",     "mlg", None, 0.001/1852.0/3.0},
              {"nautical_miles",     "nm ", None, 0.001/1852.0},
                        };
char                    distances_human[] = "U=lns";
int                     distances_ordinal = 0;
extern                  errno;
XFontStruct             *font_info;
XFontStruct             *font_info2;
char                    *fontname;
int                     font_width, font_height;
GC                      gc;
GC                      gc2;
GC                      gc3;
GC                      gc_reverse;
GC                      gc2_reverse;
int                     label_cursor_x, label_cursor_y;
int                     label_pointer_x, label_pointer_y;
short                   menu_active = False;
int                     menu_border_width = 4;
int                     menu_pane_count = sizeof( distances) / sizeof( struct distances );
int                     menu_pane_height;
int                     menu_width, menu_height;
int                     menu_width_old, menu_height_old, menu_x_old, menu_y_old;
unsigned long           motion_buffer_size;
int                     odometer_count = BOTH;
FILE                    *OF;
char                    old_line1[20] = "!!!!!!!!!!!!!!!!!!!";
char                    old_line2[20] = "!!!!!!!!!!!!!!!!!!!";
char                    old_line3[20] = "!!!!!!!!!!!!!!!!!!!";
char                    old_line4[20] = "!!!!!!!!!!!!!!!!!!!";
XCharStruct             overall;
int                     pixels_per_cm_x, pixels_per_cm_y;
int                     pixels_per_inch_x, pixels_per_inch_y;
int         PointerX;
int         PointerY;
int         PrevPointerX = 0;
int         PrevPointerY = 0;
char            *ProgramName;
Pixmap                  pulldown;
XRectangle              rectangles[10] = {   3, 25, 20, 20,
                                            23, 25, 20, 20,
                        43, 25, 20, 20,
                        63, 25, 20, 20,
                        83, 25, 20, 20,
                       103, 25, 20, 20,
                       123, 25, 20, 20,
                       143, 25, 20, 20,
                       163, 25, 20, 20,
                       183, 25, 20, 20,
                                         };
XRectangle              rectangles2[10];
XRectangle              rectangles3[10];
XRectangle              rectangles4[10];
Status                  status;
XColor          theBackgroundColor;
unsigned long       theBackgroundPixel;
XColor                  theBorderColor;
unsigned long           theBorderPixel;
Cursor                  theCursor;
unsigned int        theDepth;
Display         *theDisplay;
XColor          theExactColor;
XColor          theForegroundColor;
unsigned long       theForegroundPixel;
Pixmap          theIconPixmap;
Window                  theMenu;
Window          theRoot;
int         theScreen;
Window          theWindow;
int                     threshold;
double                  total_cursor_distance, trip_cursor_distance; /* distances in millimeters */
double                  total_pointer_distance, trip_pointer_distance; /* distances in millimeters */
unsigned long           valuemask;
XGCValues               values;
struct windata{
  Window window;
  int color;
  char *text;
  int x;
  int y;
  int width;
  int height;
  int border;
}                       windata[] = {
                          {None, 0, "",          0, 0,  8,  3, 2},
                          {None, 0, "",          0, 0,  8,  3, 2},
                          {None, 0, "Quit",     40, 2, 25, 10, 2},
                          {None, 0, "  Units", 205, 2, 45, 10, 2},
                          {None, 0, "About",     2, 2, 30, 10, 2},
                        };
int                     windata_count = sizeof( windata) / sizeof( struct windata );
int                     window_width = 1;
int                 window_height = 1;
unsigned int        WindowHeight = 1;
int             WindowPointX = 1;
int             WindowPointY = 1;
unsigned int        WindowWidth = 1;
double                  X_mm_per_pixel, Y_mm_per_pixel;
char                    status_line[100];




/* Global procedures; again, alphabetically. */




void
compute_font_and_resize_data( font_info ) /* size rectangles et. al. based on font */
     XFontStruct                *font_info;
{

  int                           i, width_incr;

  if ( font_info != NULL ) {

    font_width = font_info->max_bounds.rbearing - font_info->min_bounds.lbearing;
    font_height = font_info->max_bounds.ascent + font_info->max_bounds.descent;

    for ( i = 0; i < 10; i++ ) {
      rectangles[i].height = font_height + 5;
      rectangles[i].width = rectangles[i].height;
      if ( i > 0 )
    rectangles[i].x = rectangles[i-1].x + font_height + 5;
      if ( font_height + 5 > 25 )
    rectangles[i].y = font_height + 5;
    }

  }

  width_incr = (int)( aspect * (float)rectangles[0].width - (float)rectangles[0].width );

  for( i = 0; i < 10; i++ ){
    if ( i >= 5 )
      rectangles[i].height += 1;
    rectangles[i].width += width_incr;
    rectangles[i].x = rectangles[i].x + (i * width_incr);

    rectangles2[i] = rectangles[i];
    rectangles2[i].y = rectangles[i].y + (font_height * 2);

    rectangles3[i] = rectangles[i];
    rectangles3[i].y = rectangles[i].y + (font_height * 5);

    rectangles4[i] = rectangles[i];
    rectangles4[i].y = rectangles[i].y + (font_height * 7);
  }

  windata[TRIP1].x = rectangles2[0].x;
  windata[TRIP1].y = rectangles2[0].y - 7;
  windata[TRIP2].x = rectangles4[0].x;
  windata[TRIP2].y = rectangles4[0].y - 7;

  window_width = rectangles[9].x + rectangles[9].width + 3;

  i = font_info2->max_bounds.ascent + font_info2->max_bounds.descent;
  if ( font_height > i )
    window_height = rectangles4[0].y + 7 + (font_height * 2);
  else
    window_height = rectangles4[0].y + 7 + (i * 2);
  if ( odometer_count != BOTH )
    window_height = window_height + rectangles2[0].y - rectangles4[0].y;

  about_width_x = window_width;

  XTextExtents( font_info, CURSOR_S, strlen( CURSOR_S ), &direction, &ascent, &descent, &overall );
  label_cursor_x = ( window_width - overall.width ) / 2;
  label_cursor_y = rectangles[0].y - (font_height/2) + font_info->max_bounds.descent;
  XTextExtents( font_info, POINTER_S, strlen( POINTER_S ), &direction, &ascent, &descent, &overall );
  label_pointer_x = ( window_width - overall.width ) / 2;
  label_pointer_y = rectangles3[0].y - (font_height/2) + font_info->max_bounds.descent;

  if ( font_info != NULL ) {
    if ( (WindowWidth != window_width) || (WindowHeight != window_height) ) {
      WindowWidth = window_width;
      WindowHeight = window_height;
      XMoveWindow( theDisplay, windata[TRIP1].window, windata[TRIP1].x, windata[TRIP1].y );
      if ( odometer_count == BOTH )
    XMoveWindow( theDisplay, windata[TRIP2].window, windata[TRIP2].x, windata[TRIP2].y );
      XMoveWindow( theDisplay, windata[UNITS].window, window_width-50, windata[UNITS].y );
      XResizeWindow( theDisplay, theWindow, WindowWidth, WindowHeight );
    }
  } /* ifend font_info != NULL */

  coordinates_x = 1;        /* compute Units and absolute coordinates display postion */
  coordinates_y = rectangles4[0].y + font_height + (font_height / 2) + font_info2->max_bounds.ascent + 2;
  if ( odometer_count != BOTH ) {
    coordinates_y += (rectangles2[0].y - rectangles4[0].y);
  }

} /* end compute_font_and_resize_data */




void
do_distances( refresh_digits )  /* compute and display the distances */
     short                      refresh_digits;
{

    Window              QueryRoot, QueryChild;
    int                 AbsoluteX, AbsoluteY;
    int                 RelativeX, RelativeY;
    unsigned int            ModKeyMask;
    double              dXmm, dYmm;
    int                         dX, dY;
    char                        line1[20], line2[20], line3[20], line4[20];
    double                      units;
    int                         i, n, x, y;
    char                        coordinates[13]; /* allows for a 99,999 by 99,999 display */
    int                         coord_len;

    if ( refresh_digits ) {
      strcpy( old_line1, "!!!!!!!!!!!!!!!!!!!" );
      strcpy( old_line2, "!!!!!!!!!!!!!!!!!!!" );
      strcpy( old_line3, "!!!!!!!!!!!!!!!!!!!" );
      strcpy( old_line4, "!!!!!!!!!!!!!!!!!!!" );
    }

    XQueryPointer( theDisplay, theWindow, &QueryRoot, &QueryChild, &AbsoluteX, &AbsoluteY, &RelativeX, &RelativeY, &ModKeyMask );

    PrevPointerX = PointerX;
    PrevPointerY = PointerY;

    PointerX = AbsoluteX;
    PointerY = AbsoluteY;

    dX = PointerX - PrevPointerX;
    dY = PointerY - PrevPointerY;

    if ( dX < 0.0 )
      dX = -dX;
    if ( dY < 0.0 )
      dY = -dY;

    dXmm = (double)dX * X_mm_per_pixel;
    dYmm = (double)dY * Y_mm_per_pixel;

    distance = sqrt( (dXmm * dXmm) + (dYmm * dYmm) );

    if ( distance != 0 ) {

      trip_cursor_distance += distance;
      total_cursor_distance += distance;
      if ( dX > threshold || dY > threshold ) {
    distance /= acceleration; /* if cursor was accelerated, we suppose! */
      }
      distance /= pvt[P_pointer_scale_factor].value.real_value;
      trip_pointer_distance += distance;
      total_pointer_distance += distance;

      units = distances[distances_ordinal].value;

      if ( odometer_count == CURSOR || odometer_count == BOTH ) {
    sprintf( line1, "%011.5f", fmod( (total_cursor_distance * units), 100000.0 ) );
    sprintf( line2, "%011.5f", fmod( (trip_cursor_distance * units), 100000.0 ) );
      } else if ( odometer_count == POINTER ) {
    sprintf( line1, "%011.5f", fmod( (total_pointer_distance * units), 100000.0 ) );
    sprintf( line2, "%011.5f", fmod( (trip_pointer_distance * units), 100000.0 ) );
      }
      if ( odometer_count == BOTH ) {
    sprintf( line3, "%011.5f", fmod( (total_pointer_distance * units), 100000.0 ) );
    sprintf( line4, "%011.5f", fmod( (trip_pointer_distance * units), 100000.0 ) );
      }

      for(i = 0, n = 0; i < 11; i++) {
    if ( i == 5 ) {
      continue;     /* skip units point */
    } else if ( i < 5 ) {   /* draw digits to the left of the point */
      draw_odometer_digit( gc, rectangles[n], line1+i, old_line1+i );
      draw_odometer_digit( gc, rectangles2[n], line2+i, old_line2+i );
      if ( odometer_count == BOTH ) {
        draw_odometer_digit( gc, rectangles3[n], line3+i, old_line3+i );
        draw_odometer_digit( gc, rectangles4[n], line4+i, old_line4+i );
      }
    } else {            /* draw digits to the right of the point */
      draw_odometer_digit( gc_reverse, rectangles[n], line1+i, old_line1+i );
      draw_odometer_digit( gc_reverse, rectangles2[n], line2+i, old_line2+i );
      if ( odometer_count == BOTH ) {
        draw_odometer_digit( gc_reverse, rectangles3[n], line3+i, old_line3+i );
        draw_odometer_digit( gc_reverse, rectangles4[n], line4+i, old_line4+i );
      }
    }
    n++;            /* advance to next rectangle */
      }

      sprintf( coordinates, "(%d,%d)", PointerX, PointerY ); /* draw absolute pointer coordinates */
                                                             /* Einstein: 'You said "absolute"?' */
      coord_len = strlen( coordinates );
      strncpy( coordinates+coord_len, "             ", 13-coord_len );
      XDrawImageString( theDisplay, theWindow, gc2, coordinates_x+33, coordinates_y, coordinates, 13 );
    } /* ifend distance != 0 */

} /* end do_distances */




void
draw_action_windows( gc )
     GC             gc;
{

  int                           i;

  for ( i = 0; i < windata_count; i++ ) {
    if ( odometer_count != BOTH && i == TRIP2 )
      continue;         /* skip second trip odometer if only 1 odometer */
    XDrawImageString( theDisplay, windata[i].window, gc, font_info2->max_bounds.lbearing-3, font_info2->max_bounds.ascent+1,
          windata[i].text, strlen(windata[i].text) );
  } /* forend */
  XCopyPlane( theDisplay, pulldown, windata[UNITS].window, gc, 0, 0, pulldown_width, pulldown_height, 1, 1, 1 );

} /* end draw_action_windows */




void
draw_menu()
{

  int               new_width, new_height;

  menu_active = True;
  XMapWindow( theDisplay, theMenu );
  menu_width_old = WindowWidth;
  menu_height_old = WindowHeight;
  if ( WindowWidth < menu_width + font_info->max_bounds.width )
    new_width = menu_width + font_info->max_bounds.width;
  else
    new_width = WindowWidth;
  if ( WindowHeight < menu_height + menu_pane_height )
    new_height = menu_height + menu_pane_height;
  else
    new_height = WindowHeight;
  menu_x_old = WindowPointX;
  menu_y_old = WindowPointY;
  if ( WindowPointY + new_height > display_height ) {
    menu_y_old = display_height - new_height;
  }

  XMoveResizeWindow( theDisplay, theWindow, menu_x_old, menu_y_old, new_width, new_height );
  menu_y_old = WindowPointY;
  XGrabPointer( theDisplay, theMenu, True, EVENT_MASK2, GrabModeAsync, GrabModeAsync, None, theCursor, CurrentTime );

} /* end draw_menu */




void
draw_menu_panes( gc, window )
     GC             gc;
     Window                     window;
{

  int                           i, j;
  char                          units_name[100];

  for ( i = 0; i < menu_pane_count; i++ ) {
    if ( window  == distances[i].menu_pane ) {
      strcpy( units_name, distances[i].name );
      for ( j = 0; j < strlen( units_name ); j++ )
    if ( units_name[j] == '_' )
      units_name[j] = ' ';
      XDrawImageString( theDisplay, distances[i].menu_pane, gc, 1, font_info->max_bounds.ascent, units_name,
            strlen( units_name ) );
    } /* ifend */
  } /* forend */

} /* end draw_menu_panes */




void
draw_misc_info()
{

  int                           x, y;
  int                           i, n;

  /* Draw the Units. */

  XDrawImageString( theDisplay, theWindow, gc2, coordinates_x, coordinates_y, distances_human, strlen( distances_human ) );

  /* Draw the About information in a (normally) hidden part of the window. */

  for ( i = 0, n = font_height; i < about_count; i++, n += font_height ) {
    XDrawImageString( theDisplay, theWindow, gc, about_width_x, n, about[i], strlen( about[i] ) );
  }
  XCopyPlane( theDisplay, theIconPixmap, theWindow, gc, 0, 0, icon_width, icon_height, about_width_x,
         about_height-icon_height, 1 );
  XDrawImageString( theDisplay, theWindow, gc2, about_width_x+icon_width, about_height-(icon_height/2),
        status_line, strlen( status_line ) );

  /* Draw the calibration information in inches and centimeters. */

  XDrawLine( theDisplay, theWindow, gc3, about_width-3, about_height, about_width-3, about_height-pixels_per_inch_y );
  XDrawLine( theDisplay, theWindow, gc3, about_width-3, about_height-pixels_per_cm_y,
        about_width-3-8, about_height-pixels_per_cm_y );
  XDrawLine( theDisplay, theWindow, gc3, about_width-3, about_height-2*pixels_per_cm_y,
        about_width-3-8, about_height-2*pixels_per_cm_y );

  XDrawLine( theDisplay, theWindow, gc3, about_width-3, about_height, about_width-3-pixels_per_inch_x, about_height );
  XDrawLine( theDisplay, theWindow, gc3, about_width-3-pixels_per_cm_x, about_height,
        about_width-3-pixels_per_cm_x, about_height-8 );
  XDrawLine( theDisplay, theWindow, gc3, about_width-3-2*pixels_per_cm_x, about_height,
        about_width-3-2*pixels_per_cm_x, about_height-8 );
  XDrawImageString( theDisplay, theWindow, gc3, about_width-calibration_width-20, about_height-pixels_per_cm_y, calibration,
        strlen( calibration ) );

} /* end draw_misc_info */




void
draw_odometers()
{

  if ( odometer_count == CURSOR || odometer_count == BOTH ) {
    XDrawImageString( theDisplay, theWindow, gc, label_cursor_x, label_cursor_y, CURSOR_S, strlen( CURSOR_S ) );
  } else {
    XDrawImageString( theDisplay, theWindow, gc, label_cursor_x, label_cursor_y,  POINTER_S, strlen( POINTER_S ) );
  }
  XDrawRectangles( theDisplay, theWindow, gc, rectangles, 5 );
  XFillRectangles( theDisplay, theWindow, gc, rectangles+5, 5 );

  XDrawRectangles( theDisplay, theWindow, gc, rectangles2, 5 );
  XFillRectangles( theDisplay, theWindow, gc, rectangles2+5, 5 );

  if ( odometer_count == BOTH ) {
    XDrawImageString( theDisplay, theWindow, gc, label_pointer_x, label_pointer_y, POINTER_S, strlen( POINTER_S ) );

    XDrawRectangles( theDisplay, theWindow, gc, rectangles3, 5 );
    XFillRectangles( theDisplay, theWindow, gc, rectangles3+5, 5 );

    XDrawRectangles( theDisplay, theWindow, gc, rectangles4, 5 );
    XFillRectangles( theDisplay, theWindow, gc, rectangles4+5, 5 );
  }

  do_distances( True );     /* repaint odometer digits */

} /* end draw_odometers */




void
draw_odometer_digit( gc, rectangle, line, old_line )
     GC                         gc;
     XRectangle         rectangle;
     char                       line[];
     char                       old_line[];
{

  if ( line[0] == old_line[0] )
    return;

  XDrawImageString( theDisplay, theWindow, gc, rectangle.x+(font_info->max_bounds.lbearing),
        rectangle.y+(font_info->max_bounds.ascent)+1, line+0, 1 );

  old_line[0] = line[0];

} /* end draw_odometer_digit */




void
finish_xodo()           /* finish xodo */
{

  save_xodo();          /* update state information */

  XUnloadFont( theDisplay, font_info->fid );
  XUnloadFont( theDisplay, font_info2->fid );
  XFreeGC( theDisplay, gc);
  XFreeGC( theDisplay, gc2);
  XFreeGC( theDisplay, gc3);
  XFreeGC( theDisplay, gc_reverse);
  XFreeGC( theDisplay, gc2_reverse);
  XCloseDisplay( theDisplay );

#if DEBUG
  fclose( d );
#endif

  exit( 0 );

} /* end finish_xodo */




void
highlight_action_window( gc, window, border )   /* either normal or highlighted */
     GC             gc;
     Window                     window;
     unsigned long              border;
{

  int                           i;

  if ( menu_active )
    return;

  for ( i = 0; i < windata_count; i++ ) {
    if ( window == windata[i].window ) {
      XSetWindowBorder( theDisplay, windata[i].window, border );
      XDrawImageString( theDisplay, windata[i].window, gc, font_info2->max_bounds.lbearing-3,
          font_info2->max_bounds.ascent+1, windata[i].text, strlen(windata[i].text) );
      break;
    } /* ifend */
  } /* forend */

  if ( i == UNITS )
    XCopyPlane( theDisplay, pulldown, windata[UNITS].window, gc, 0, 0, pulldown_width, pulldown_height, 1, 1, 1 );

} /* end highlight_action_window */




void
initialize_graphics_contexts()  /* initialize graphics context */
{

  unsigned int                  line_width = 1;
  int                           line_style = LineSolid;
  int                           cap_style = CapRound;
  int                           join_style = JoinMiter;

  fontname = pvt[P_fontname].value.string_value;

  if ( (font_info = XLoadQueryFont( theDisplay, fontname )) == NULL ) {
    fprintf( stderr, "%s:  cannot open %s font.\n", ProgramName, fontname );
    exit( 1 );
  }

  valuemask = GCFunction | GCForeground | GCBackground;
  values.function = GXcopy;
  values.foreground = theForegroundPixel;
  values.background = theBackgroundPixel;
  gc = XCreateGC( theDisplay, theWindow, valuemask, &values );
  XSetFont( theDisplay, gc, font_info->fid );
  XSetLineAttributes( theDisplay, gc, line_width, line_style, cap_style, join_style );

  valuemask = GCFunction | GCForeground | GCBackground;
  values.function = GXcopy;
  values.foreground = theBackgroundPixel;
  values.background = theForegroundPixel;
  gc_reverse = XCreateGC( theDisplay, theWindow, valuemask, &values );
  XSetFont( theDisplay, gc_reverse, font_info->fid );
  XSetLineAttributes( theDisplay, gc_reverse, line_width, line_style, cap_style, join_style );

  fontname = pvt[P_fontname2].value.string_value;
  if ( (font_info2 = XLoadQueryFont( theDisplay, fontname )) == NULL ) {
    fprintf( stderr, "%s:  cannot open %s font.\n", ProgramName, fontname );
    exit( 1 );
  }

  valuemask = GCFunction | GCForeground | GCBackground;
  values.function = GXcopy;
  values.foreground = theForegroundPixel;
  values.background = theBackgroundPixel;
  gc2 = XCreateGC( theDisplay, theWindow, valuemask, &values );
  XSetFont( theDisplay, gc2, font_info2->fid );
  XSetLineAttributes( theDisplay, gc2, line_width, line_style, cap_style, join_style );

  valuemask = GCFunction | GCForeground | GCBackground;
  values.function = GXcopy;
  values.foreground = theBackgroundPixel;
  values.background = theForegroundPixel;
  gc2_reverse = XCreateGC( theDisplay, theWindow, valuemask, &values );
  XSetFont( theDisplay, gc2_reverse, font_info2->fid );
  XSetLineAttributes( theDisplay, gc2_reverse, line_width, line_style, cap_style, join_style );

  valuemask = GCFunction | GCForeground | GCBackground;
  values.function = GXcopy;
  values.foreground = theBorderPixel;
  values.background = theBackgroundPixel;
  gc3 = XCreateGC( theDisplay, theWindow, valuemask, &values );
  XSetFont( theDisplay, gc3, font_info2->fid );
  XSetLineAttributes( theDisplay, gc3, line_width, line_style, cap_style, join_style );

} /* end initialize_graphics_contexts */




void
initialize_menu()       /* initialize the Units menu */
{

  char                          *string;
  int                           i, x, y;
  Pixmap                        theCursorSource, theCursorMask;

  string = distances[0].name;   /* find longest string */
  for ( i = 1; i < menu_pane_count; i++ ) {
    if( strlen( distances[i].name ) > strlen( string ) )
      string = distances[i].name;
  }
  XTextExtents( font_info, string, strlen( string ), &direction, &ascent, &descent, &overall );
  menu_width = overall.width + 4;
  menu_pane_height = overall.ascent + overall.descent + 4;
  menu_height = menu_pane_height * menu_pane_count;
  x = window_width - menu_width - ( 2 * menu_border_width);
  y = 0;

  theMenu = XCreateSimpleWindow( theDisplay, theWindow, x, y, menu_width, menu_height, menu_border_width, theBorderPixel,
        theBackgroundPixel );

  for( i = 0; i < menu_pane_count; i++ ) {
    distances[i].menu_pane = XCreateSimpleWindow( theDisplay, theMenu, 0, menu_height/menu_pane_count*i, menu_width,
          menu_pane_height, menu_border_width = 1, theForegroundPixel, theBackgroundPixel );
    XSelectInput( theDisplay, distances[i].menu_pane, EVENT_MASK3 );
  }

  theCursorSource = XCreateBitmapFromData( theDisplay, theWindow, cursor_bits, cursor_width, cursor_height );
  theCursorMask = XCreateBitmapFromData( theDisplay, theWindow, cursor_mask_bits, cursor_mask_width, cursor_mask_height );
  theCursor = XCreatePixmapCursor( theDisplay, theCursorSource, theCursorMask, &theForegroundColor, &theBackgroundColor,
        cursor_x_hot, cursor_y_hot );
  XDefineCursor( theDisplay, theMenu, theCursor );

  XMapSubwindows( theDisplay, theMenu );

} /* end initialize_menu */




void
initialize_xodo()       /* initialize xodometer */
{
    int             GeometryStatus;
    XSetWindowAttributes    theWindowAttributes;
    XSizeHints          theSizeHints;
    unsigned long       theWindowMask;
    XWMHints            theWMHints;
    Colormap            theColormap;
    int                         i;
    Window                  QueryRoot, QueryChild;
    int                         AbsoluteX, AbsoluteY;
    int                         RelativeX, RelativeY;
    unsigned int                ModKeyMask;
    XVisualInfo                 visual_info;
    unsigned long               background;
    char                        units[80];
    char                        *X_default;
    static char                 *argv[1] = {"xodo"};
    int                         argc = 1;
    long                        max_request_size;
    struct sigaction            action;

#if DEBUG
    d = fopen( "debug.log", "w" );
#endif

    sigemptyset( &action.sa_mask ); /* disable all signals */

    action.sa_flags = 0;
    action.sa_handler = finish_xodo;
    if ( sigaction( SIGINT, &action, NULL ) != 0 ) {
      fprintf( stderr, "Cannot set signal SIGINT!\n" );
      exit( 1 );
    }
    action.sa_flags = 0;
    action.sa_handler = save_xodo;
    if ( sigaction( SIGHUP, &action, NULL ) != 0 ) {
      fprintf( stderr, "Cannot set signal SIGHUP!\n" );
      exit( 1 );
    }

    ProgramName = pvt[P_HELP].unconverted_value;

    if ( strlen( pvt[P_display].value.string_value ) < 1 ) {
    pvt[P_display].value.string_value = NULL;
    }

    if ( ( theDisplay = XOpenDisplay( pvt[P_display].value.string_value ) ) == NULL ) {
    fprintf( stderr, "%s: Can't open display", ProgramName );
    if ( pvt[P_display].value.string_value != NULL ) {
        fprintf( stderr, " %s.\n", pvt[P_display].value.string_value );
    } else {
        fprintf( stderr, ".\n" );
    }
    exit( 1 );
    }

    if ( strlen( pvt[P_geometry].value.string_value ) < 1 ) {
    pvt[P_geometry].value.string_value = NULL;
    }

    /* For all unspecified evaluate_parameters command line parameters see if there is an X default value. */

    for ( i = 0 ; i < P_NUMBER_OF_PARAMETERS; i++ ) { /* for all evaluate_parameters parameters */
      if ( ! pvt[i].specified ) {
    X_default = XGetDefault( theDisplay, ProgramName, pvt[i].parameter );
    if ( X_default != NULL ) {
      pvt[i].unconverted_value = X_default;
      evap_type_conversion( &pvt[i] ); /* convert string to proper type */
    } /* ifend non-null X default for this parameter */
      } /* ifend unspecified parameter */
    } /* forend all evaluate_parameters parameters */

    max_request_size = XMaxRequestSize( theDisplay );
    if ( ((max_request_size - 3) / 3) < 10 ) {
      fprintf( stderr, "XMaxRequestSize is too small for xodo the run!\n");
      exit( 1 );
    }

    motion_buffer_size = XDisplayMotionBufferSize( theDisplay );

    if ( strcmp( pvt[P_odometer].value.key_value, "cursor" ) == 0 )
      odometer_count = CURSOR;
    else if ( strcmp( pvt[P_odometer].value.key_value, "pointer" ) == 0 )
      odometer_count = POINTER;

    autosave_ticks = pvt[P_odometer_autosave_time].value.integer_value * 60 * 1000000 /
          pvt[P_microsecond_interval_time].value.integer_value;
    autosave_count = autosave_ticks;

    display_widthmm = pvt[P_display_width_millimeters].value.integer_value;
    display_heightmm = pvt[P_display_height_millimeters].value.integer_value;
    display_width = pvt[P_display_width_pixels].value.integer_value;
    display_height = pvt[P_display_height_pixels].value.integer_value;
    X_mm_per_pixel = (double)display_widthmm / display_width;
    Y_mm_per_pixel = (double)display_heightmm / display_height;
    pixels_per_inch_x = (int)( 25.4 / X_mm_per_pixel );
    pixels_per_inch_y = (int)( 25.4 / Y_mm_per_pixel );
    pixels_per_cm_x = (int)( 10.0 / X_mm_per_pixel );
    pixels_per_cm_y = (int)( 10.0 / Y_mm_per_pixel );
    aspect = (float)(X_mm_per_pixel / Y_mm_per_pixel);

    total_cursor_distance = 0.0;
    trip_cursor_distance = 0.0;
    total_pointer_distance = 0.0;
    trip_pointer_distance = 0.0;

    for( i=0; i < menu_pane_count; i++ ) { /* default units = kilometers */
      if( strcmp( distances[i].name, "kilometers" ) == 0 )
    distances_ordinal = i;
    }

    OF = fopen( pvt[P_odometer_file].value.file_value, "r" );
    if ( OF == NULL && errno != ENOENT ) {
      perror("Cannot open odometer_file");
      exit( 1 );
    } else if ( OF != NULL ) {
      fscanf( OF, "%lf %lf %s", &total_cursor_distance, &total_pointer_distance, units );
      for( i=0; i < menu_pane_count; i++ ) {
    if( strcmp( distances[i].name, units ) == 0 )
      distances_ordinal = i;
      }
      fclose( OF );
    }

    GeometryStatus = XParseGeometry( pvt[P_geometry].value.string_value, &WindowPointX, &WindowPointY, &WindowWidth,
          &WindowHeight );

    if ( !( GeometryStatus & XValue ) ) {
      WindowPointX = 1;
    } else if ( GeometryStatus & XNegative ) {
      WindowPointX = display_width + WindowPointX;
    }
    if ( !( GeometryStatus & YValue ) ) {
      WindowPointY = 1;
    } else if ( GeometryStatus & YNegative ) {
      WindowPointY = display_height + WindowPointY;
    }
    if ( !( GeometryStatus & WidthValue ) ) {
      WindowWidth = window_width;
    }
    if ( !( GeometryStatus & HeightValue ) ) {
      WindowHeight = window_height;
    }

    theScreen = DefaultScreen( theDisplay );
    theDepth = DefaultDepth( theDisplay, theScreen );
    theColormap = DefaultColormap( theDisplay, theScreen );
    theForegroundPixel = BlackPixel( theDisplay, theScreen );
    theBackgroundPixel = WhitePixel( theDisplay, theScreen );
    theBorderPixel = theForegroundPixel;

    if ( theDepth > 1 ) {   /* if possible color monitor */

      i = DirectColor;      /* StaticGray, GrayScale, StaticColor, PseudoColor, TrueColor, DirectColor */
      while ( ! XMatchVisualInfo( theDisplay, theScreen, theDepth, i--, &visual_info ) )
    ; /* whilend */

      if ( i >= StaticColor ) {
    if ( ! XAllocNamedColor( theDisplay, theColormap, pvt[P_background].value.name_value, &theBackgroundColor,
              &theExactColor ) ) {
      fprintf( stderr, "%s: Can't XAllocNamedColor( \"%s\" ).\n", ProgramName, pvt[P_background].value.name_value );
      exit( 1 );
    }
    if ( ! XAllocNamedColor( theDisplay, theColormap, pvt[P_foreground].value.name_value, &theForegroundColor,
              &theExactColor ) ) {
      fprintf( stderr, "%s: Can't XAllocNamedColor( \"%s\" ).\n", ProgramName, pvt[P_foreground].value.name_value );
      exit( 1 );
    }
    if ( ! XAllocNamedColor( theDisplay, theColormap, pvt[P_border].value.name_value, &theBorderColor, &theExactColor ) ) {
      fprintf( stderr, "%s: Can't XAllocNamedColor( \"%s\" ).\n", ProgramName, pvt[P_border].value.name_value );
      exit( 1 );
    }

    theForegroundPixel = theForegroundColor.pixel;
    theBackgroundPixel = theBackgroundColor.pixel;
    theBorderPixel = theBorderColor.pixel;
      } /* ifend color visual */

    } /* ifend depth > 1 */

    theWindowAttributes.border_pixel = theBorderPixel;
    theWindowAttributes.background_pixel = theBackgroundPixel;
    theWindowAttributes.override_redirect = False;
    theWindowAttributes.event_mask = EVENT_MASK1;

    theWindowMask = CWBackPixel | CWBorderPixel | CWEventMask | CWOverrideRedirect;

    theWindow = XCreateWindow( theDisplay, RootWindow( theDisplay, theScreen ), WindowPointX, WindowPointY, WindowWidth,
          WindowHeight, BorderWidth, theDepth, InputOutput, CopyFromParent, theWindowMask, &theWindowAttributes );

    initialize_graphics_contexts(); /* initialize graphics contexts */

    theIconPixmap = XCreateBitmapFromData( theDisplay, theWindow, icon_bits, icon_width, icon_height );
    pulldown = XCreateBitmapFromData( theDisplay, theWindow, pulldown_bits, pulldown_width, pulldown_height );

    theWMHints.icon_pixmap = theIconPixmap;
    if ( pvt[P_iconic].specified ) {
    theWMHints.initial_state = IconicState;
    } else {
    theWMHints.initial_state = NormalState;
    }
    theWMHints.flags = IconPixmapHint | StateHint;

    XSetWMHints( theDisplay, theWindow, &theWMHints );

    theSizeHints.flags = PPosition | PSize; /* PMinSize perhaps */
    theSizeHints.x = WindowPointX;
    theSizeHints.y = WindowPointY;
    theSizeHints.width = WindowWidth;
    theSizeHints.height = WindowHeight;
    theSizeHints.min_width = window_width;
    theSizeHints.min_height = window_height;

    XSetNormalHints( theDisplay, theWindow, &theSizeHints );

    if ( strlen( pvt[P_title].value.string_value ) >= 1 ) {
    XStoreName( theDisplay, theWindow, pvt[P_title].value.string_value );
    XSetIconName( theDisplay, theWindow, pvt[P_title].value.string_value );
    } else {
    XStoreName( theDisplay, theWindow, ProgramName );
    XSetIconName( theDisplay, theWindow, ProgramName );
    }

    XSetCommand( theDisplay, theWindow, argv, argc );
    atom_wm_save_yourself = XInternAtom( theDisplay, "WM_SAVE_YOURSELF", False );
    status = XSetWMProtocols( theDisplay, theWindow, &atom_wm_save_yourself, 1 );

    for( i = 0; i < windata_count; i++ ) { /* make Action buttons */
      if ( odometer_count != BOTH && i == TRIP2 )
    continue;       /* skip second trip odometer if only 1 odometer */
      background = theBackgroundPixel;
      if ( i == TRIP1 || i == TRIP2 )
    background = theBorderPixel;
      windata[i].window = XCreateSimpleWindow( theDisplay, theWindow, windata[i].x, windata[i].y, windata[i].width,
            windata[i].height, windata[i].border, theBorderPixel, background );
      XSelectInput( theDisplay, windata[i].window, EVENT_MASK2 );
    } /* forend */

    XQueryPointer( theDisplay, theWindow, &QueryRoot, &QueryChild, &AbsoluteX, &AbsoluteY, &RelativeX, &RelativeY, &ModKeyMask );
    PointerX = AbsoluteX;
    PointerY = AbsoluteY;

    XGetPointerControl( theDisplay, &accel_numerator, &accel_denominator, &threshold);
    acceleration = (double)accel_numerator / (double)accel_denominator;

    sprintf( status_line, "S=%.1f T=%d A=%.1f", pvt[P_pointer_scale_factor].value.real_value, threshold, acceleration );
    strncpy( distances_human+2, distances[distances_ordinal].abbreviation, 3 );

    XMapSubwindows( theDisplay, theWindow );

    compute_font_and_resize_data( font_info );

    XGetGeometry( theDisplay, theWindow, &theRoot, &WindowPointX, &WindowPointY, &WindowWidth, &WindowHeight, &BorderWidth,
          &theDepth );

    XTextExtents( font_info, about[2], strlen( about[2] ), &direction, &ascent, &descent, &overall );
    about_width = overall.width + 4 + about_width_x;
    about_height = ( overall.ascent + overall.descent + 4 ) * about_count;
    about_height += icon_height + font_height + 1;

    XTextExtents( font_info2, calibration, strlen( calibration ), &direction, &ascent, &descent, &overall );
    calibration_width = overall.width + 4;

    initialize_menu();      /* initialize the Units menu windows */

    XMapWindow( theDisplay, theWindow );
    XFlush( theDisplay );

} /* end initialize_xodo */




Bool
process_event()         /* handle all X events */
{
  XEvent                        theEvent;
  int                           new_width, new_height;
  int                           i;

  /* Check for ClientMessage type WM_SAVE_YOURSELF to save state information. */

  if ( XCheckTypedEvent( theDisplay, ClientMessage, &theEvent ) == True ) {
    if ( theEvent.xclient.data.l[0] == atom_wm_save_yourself )
      finish_xodo();        /* save state information and exit */
  } /* ifend ClientMessage received */

  while ( XCheckMaskEvent( theDisplay, ALL_EVENTS_MASK, &theEvent ) ) {

    switch ( theEvent.type ) {

    case MapNotify:      case MappingNotify:   case GraphicsExpose:   case NoExpose:
    case SelectionClear: case SelectionNotify: case SelectionRequest:

      break;            /* misc */

    case ClientMessage:

      finish_xodo();        /* will never get here via XCheckMaskEvent! */

      break;            /* ClientMessage */

    case ConfigureNotify:
      if ( theEvent.xconfigure.window == theWindow ) {
    WindowWidth = theEvent.xconfigure.width;
    WindowHeight = theEvent.xconfigure.height;
    if ( theEvent.xconfigure.x != 0 )
      WindowPointX = theEvent.xconfigure.x;
    if ( theEvent.xconfigure.y != 0 )
      WindowPointY = theEvent.xconfigure.y;
    BorderWidth = theEvent.xconfigure.border_width;
      }

      break;            /* ConfigureNotify */

    case Expose:

      if ( theEvent.xexpose.count == 0 ) {

    if ( theEvent.xexpose.window == theWindow ) {
      draw_odometers();
      draw_misc_info();
      draw_action_windows( gc2 );
        }

    draw_menu_panes( gc, theEvent.xexpose.window );

      } /* ifend event count = 0 */

      break;            /* Expose */

    case EnterNotify:

      if ( button_depressed ) {
    if (theEvent.xcrossing.window == windata[UNITS].window)
      draw_menu();
    else
      highlight_action_window( gc2_reverse, theEvent.xcrossing.window, theForegroundPixel );
      }

      draw_menu_panes( gc_reverse, theEvent.xcrossing.window );

      break;            /* EnterNotify */

    case LeaveNotify:

      highlight_action_window( gc2, theEvent.xcrossing.window, theBorderPixel );

      draw_menu_panes( gc, theEvent.xcrossing.window );

      break;            /* LeaveNotify */

    case ButtonPress:

      button_depressed = True;

      if ( theEvent.xbutton.window == windata[UNITS].window ) {
    draw_menu();
    break;
      }

      highlight_action_window( gc2_reverse, theEvent.xbutton.window, theForegroundPixel );

      break;            /* ButtonPress */

    case ButtonRelease:

      button_depressed = False;

      highlight_action_window( gc2, theEvent.xbutton.window, theBorderPixel );

      if ( menu_active == True ) {
    XUngrabPointer( theDisplay, CurrentTime );
    XUnmapWindow( theDisplay, theMenu );
    XMoveResizeWindow( theDisplay, theWindow, menu_x_old, menu_y_old, menu_width_old, menu_height_old );
    menu_active = False;
    for ( i = 0; i < menu_pane_count; i++ ) {
      if ( theEvent.xbutton.window == distances[i].menu_pane ) {
        distances_ordinal = i;
        strncpy( distances_human+2, distances[distances_ordinal].abbreviation, 3 );
        draw_misc_info();
      } /* ifend button release occurred in a menu pane */
    } /* forend */
    break; /* case ButtonRelease */
      } /* ifend menu_active */

      if ( theEvent.xbutton.window == windata[TRIP1].window ) {
    if ( odometer_count == CURSOR || odometer_count == BOTH ) {
      trip_cursor_distance = 0.0;
    } else {
      trip_pointer_distance = 0.0;
    }
      } else if ( theEvent.xbutton.window == windata[TRIP2].window ) {
    trip_pointer_distance = 0.0;
      } else if ( theEvent.xbutton.window == windata[QUIT].window ) {
    finish_xodo();      /* update distance totals */
      } else if ( theEvent.xbutton.window == windata[UNITS].window ) {
    XUnmapWindow( theDisplay, theMenu );
      } else if ( theEvent.xbutton.window == windata[ABOUT].window ) {
    if ( about_active ) {
      about_active = False;
      windata[ABOUT].text = "About";
      XMoveResizeWindow( theDisplay, theWindow, about_x_old, about_y_old, about_width_old, about_height_old );
    } else {
      about_active = True;
      about_width_old = WindowWidth;
      about_height_old = WindowHeight;
      windata[ABOUT].text = " OK! ";
      if ( WindowWidth < about_width )
        new_width = about_width;
      else
        new_width = WindowWidth;
      if ( WindowHeight < about_height )
        new_height = about_height;
      else
        new_height = WindowHeight;
      about_x_old = WindowPointX;
      about_y_old = WindowPointY;
      if ( WindowPointX + new_width > display_width )
        about_x_old = display_width - new_width - 10;
      if ( WindowPointY + new_height > display_height )
        about_y_old = display_height - new_height - 10;
      XMoveResizeWindow( theDisplay, theWindow, about_x_old, about_y_old, new_width, new_height+5 );
      about_x_old = WindowPointX;
      about_y_old = WindowPointY;
    }
      } else if ( theEvent.xbutton.button == Button2 ) {
    trip_cursor_distance = 0.0;
    trip_pointer_distance = 0.0;
      } /* ifend */

      break;            /* ButtonRelease */

    default:

      break;            /* unknown event */

    } /* casend event type */

  } /* whilend */

  return( True );

} /* end process_event */




void
process_pointer()       /* do stuff every time period */
{
  struct timeval                timer;

  timer.tv_sec = 0;
  timer.tv_usec = pvt[P_microsecond_interval_time].value.integer_value;

  do {

    do_distances( False );

    if ( --autosave_count <= 0 ) {
      autosave_count = autosave_ticks;
      save_xodo();      /* update state information */
    }

    if ( pvt[P_microsecond_interval_time].value.integer_value <= 999999 )
      select( 0, NULL, NULL, NULL, &timer );
    else
      sleep ( (unsigned)pvt[P_microsecond_interval_time].value.integer_value / 1000000 );

  } while ( process_event() );

} /* end process_pointer */




void
save_xodo()         /* update xodo distances */
{

  OF = fopen( pvt[P_odometer_file].value.file_value, "w" );
  if ( OF == NULL ) {
    perror("Cannot open odometer_file for write!");
  } else {
    fprintf( OF, "%f %f %s\n", total_cursor_distance, total_pointer_distance, distances[distances_ordinal].name );
    fclose( OF );
  }

} /* end save_xodo */



int
main( argc, argv )
    int     argc;
    char    *argv[];
{

  evap( &argc, &argv, pdt, NULL, pvt ); /* evaluate parameters */

  initialize_xodo();        /* all other xodometer initialization */

  process_pointer();        /* watch the pointing device */

  finish_xodo();        /* update distances for next time */

  exit( 0 );            /* success, just in case */

} /* end xodo main */
