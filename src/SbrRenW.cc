/*=========================================================================

  Program:   Visualization Library
  Module:    SbrRenW.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include <stdlib.h>
#include <iostream.h>
#include "SbrRenW.hh"
#include "SbrRen.hh"
#include "SbrProp.hh"
#include "SbrText.hh"
#include "SbrCam.hh"
#include "SbrLgt.hh"

#define MAX_LIGHTS 16

static char *lights[MAX_LIGHTS] =
{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

vlSbrRenderWindow::vlSbrRenderWindow()
{
  this->Fd = -1;
  this->StereoType = VL_STEREO_CRYSTAL_EYES;
  strcpy(this->Name,"Visualization Library - Starbase");
}

// Description:
// Create a starbase-specific actor.
vlActor *vlSbrRenderWindow::MakeActor()
{
  vlActor *actor;
  vlSbrProperty *prop;

  actor = new vlActor;
  prop  = new vlSbrProperty;

  actor->SetProperty((vlProperty *)prop);
  return (vlActor *)actor;
}

// Description:
// Create a starbase specific light.
vlLight *vlSbrRenderWindow::MakeLight()
{
  vlSbrLight *light;

  light = new vlSbrLight;
  return (vlLight *)light;
}

// Description:
// Create a starbase specific renderer.
vlRenderer *vlSbrRenderWindow::MakeRenderer()
{
  vlSbrRenderer *ren;

  ren = new vlSbrRenderer;
  this->AddRenderers(ren);

  // by default we are its parent
  ren->SetRenderWindow((vlRenderWindow*)this);
  
  return (vlRenderer *)ren;
}

// Description:
// Create a starbase specific camera.
vlCamera *vlSbrRenderWindow::MakeCamera()
{
  vlSbrCamera *camera;

  camera = new vlSbrCamera;
  return (vlCamera *)camera;
}

// Description:
// Create a starbase specific property.
vlProperty *vlSbrRenderWindow::MakeProperty()
{
  vlSbrProperty *property;

  property = new vlSbrProperty;
  return (vlProperty *)property;
}

// Description:
// Create a starbase specific texture.
vlTexture *vlSbrRenderWindow::MakeTexture()
{
  vlSbrTexture *texture;

  texture = new vlSbrTexture;
  return (vlTexture *)texture;
}

// Description:
// Begin the rendering process.
void vlSbrRenderWindow::Start(void)
{
  // if the renderer has not been initialized, do so now
  if (this->Fd == -1)
    this->Initialize();

  flush_matrices(this->Fd);
}

// Description:
// Update system if needed due to stereo rendering.
void vlSbrRenderWindow::StereoUpdate(void)
{
  // if stereo is on and it wasn't before
  if (this->StereoRender && (!this->StereoStatus))
    {
    switch (this->StereoType) 
      {
      case VL_STEREO_CRYSTAL_EYES:
	{
	gescape_arg arg1,arg2;
	arg1.i[0] = 1;
	gescape(this->Fd,STEREO,&arg1,&arg2);
	// make sure we are in full screen
        this->StereoStatus = 1;
	this->FullScreenOn();
	}
	break;
      case VL_STEREO_RED_BLUE:
	{
        this->StereoStatus = 1;
	}
      }
    }
  else if ((!this->StereoRender) && this->StereoStatus)
    {
    switch (this->StereoType) 
      {
      case VL_STEREO_CRYSTAL_EYES:
	{
	gescape_arg arg1,arg2;
	arg1.i[0] = 0;
	gescape(this->Fd,STEREO,&arg1,&arg2);
	// make sure we are in full screen
        this->StereoStatus = 0;
	this->FullScreenOff();
	}
	break;
      case VL_STEREO_RED_BLUE:
	{
        this->StereoStatus = 0;
	}
      }
    }
}

// Description:
// End the rendering process and display the image.
void vlSbrRenderWindow::Frame(void)
{
  // flush and display the buffer
  if (this->DoubleBuffer) 
    {
    dbuffer_switch(this->Fd, this->Buffer = !(this->Buffer));
    }
}

/*
 * get the visual type which matches the depth argument
 */
static Visual * xlib_getvisual(Display *display,int screen,int depth)
{
    XVisualInfo templ;
    XVisualInfo *visuals, *v;
    Visual	*vis = NULL;
    int		nvisuals;
    int		i;

	templ.screen = screen;
	templ.depth = depth;

	vis = DefaultVisual(display, screen);

	visuals = XGetVisualInfo(display, VisualScreenMask | VisualDepthMask,
			&templ, &nvisuals);

	for (v = visuals, i = 0; i < nvisuals; v++, i++)
		if (v->c_class == vis->c_class) {
			vis = v->visual;
			break;						
		}

	return(vis);
}

/*
 * get a PseudoColor visual
 */
static Visual * xlib_getpseudocolorvisual(Display *display,int screen,
					  int depth)
{
    XVisualInfo templ;
    XVisualInfo *visuals, *v;
    Visual	*vis = NULL;
    int		nvisuals;
    int		i;

	  templ.screen = screen;
	  templ.depth = depth;

	  visuals = XGetVisualInfo(display, VisualScreenMask | VisualDepthMask,
			&templ, &nvisuals);

	  for (v = visuals, i = 0; i < nvisuals; v++, i++)
		if (v->c_class == PseudoColor) {
			vis = v->visual;
			break;						
		}

	return(vis);
}

/*
 * get a TrueColor visual
 */
static Visual * xlib_gettruecolorvisual(Display *display,int screen,int depth)
{
    XVisualInfo templ;
    XVisualInfo *visuals, *v;
    Visual	*vis = NULL;
    int		nvisuals;
    int		i;

	  templ.screen = screen;
	  templ.depth = depth;

	  visuals = XGetVisualInfo(display, VisualScreenMask | VisualDepthMask,
			&templ, &nvisuals);

	  for (v = visuals, i = 0; i < nvisuals; v++, i++)
		if (v->c_class == TrueColor) {
			vis = v->visual;
			break;						
		}

	return(vis);
}

/*
 * get a DirectColor visual
 */
static Visual * xlib_getdirectcolorvisual(Display *display,
					  int screen,int depth)
{
    XVisualInfo templ;
    XVisualInfo *visuals, *v;
    Visual	*vis = NULL;
    int		nvisuals;
    int		i;

	  templ.screen = screen;
	  templ.depth = depth;

	  visuals = XGetVisualInfo(display, VisualScreenMask | VisualDepthMask,
			&templ, &nvisuals);

	  for (v = visuals, i = 0; i < nvisuals; v++, i++)
		if (v->c_class == DirectColor) {
			vis = v->visual;
			break;						
		}

	return(vis);
}



/*
 * get the best visual for XGL accelerated colors
 * This should be called instead of xglut_argprocess if you
 * don't want your user to be able to select the color type
 * and visual of xglut.
 */
static int xlib_get_best_depth(Display *display)
{
  int depth;
  Visual *vis;

  vis = xlib_gettruecolorvisual(display, DefaultScreen(display), 24);
  if (vis == NULL) 
    {
    vis = xlib_getdirectcolorvisual(display, DefaultScreen(display), 24);
    if (vis == NULL) 
      {
      vis = xlib_getpseudocolorvisual(display, DefaultScreen(display), 8); 
      if (vis == NULL) 
	{
	fprintf(stderr,"can't get visual info\n");
	exit(1);
	}
      else 
	{
	depth = 8;
	}
      }
    else 
      {
      depth = 24;
      }
    }
  else 
    {
    depth = 24;
    }

  return(depth);
}

/*
 * get the best visual for XGL accelerated colors
 * This should be called instead of xglut_argprocess if you
 * don't want your user to be able to select the color type
 * and visual of xglut.
 */
static Visual *xlib_get_best_visual(Display *display)
{
  int depth;
  Visual *vis;

    vis = xlib_getdirectcolorvisual(display, DefaultScreen(display), 24);
    if (vis == NULL) 
      {
      vis = xlib_getpseudocolorvisual(display, DefaultScreen(display), 8); 
      if (vis == NULL) 
	{
	fprintf(stderr,"can't get visual info\n");
	exit(1);
	}
      else 
	{
	depth = 8;
	}
      }
    else 
      {
      depth = 24;
      }

  return (vis);
}


int vlSbrRenderWindow::GetDesiredDepth ()
{
  int depth;

  /* get the default display connection */
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay((char *)NULL); 
    if (this->DisplayId == NULL) 
      {
      vlErrorMacro(<< "bad X server connection.\n");
      }
    }

  /* get the default depth to use */
  depth = xlib_get_best_depth(this->DisplayId);

  return depth;  
}

// Description:
// Obtain a colormap from windowing system.
Colormap vlSbrRenderWindow::GetDesiredColormap ()
{
  XVisualInfo *pVisInfo, visInfo;
  Colormap cmapID;
  int depth;
  unsigned int mask;
  int retVal;
  Display *dpy;

  /* get the default depth to use */
  depth = xlib_get_best_depth(this->DisplayId);
  dpy = this->DisplayId;

  visInfo.screen = 0;
  visInfo.depth = depth;

  if(depth == 4)
    visInfo.c_class = PseudoColor;
  if(depth == 8)
    visInfo.c_class = PseudoColor;
  if(depth == 12)
    visInfo.c_class = PseudoColor;
  if(depth == 16)
    visInfo.c_class = PseudoColor;
  
  /* DirectColor visual is used needed for CMAP_FULL */
  if(depth == 24)
    visInfo.c_class = DirectColor;
  
  vlDebugMacro(<< "Starbase: The depth is " << depth << "\n");
  /*
   * First, ask for the desired visual
   */
  mask = VisualScreenMask | VisualDepthMask | VisualClassMask;
  
  pVisInfo = XGetVisualInfo(dpy, mask, &visInfo, &retVal);
  
  if (!retVal) 
    {
    if (depth == 24) 
      {
      /*
       * try again with 16 bits
       */
      visInfo.depth = 16;
      visInfo.c_class = PseudoColor;
      pVisInfo = XGetVisualInfo(dpy, mask, &visInfo, &retVal);
      if (!retVal) 
	{
	fprintf(stderr, "Could not get visual info\n");
	return 0;
	}
      } 
    else 
      {
      fprintf(stderr,"Could not get visual info\n");
      return 0;
      }
    }
  
  if (retVal != 1) 
    {
    fprintf(stderr,"Too many visuals match display+depth+class\n");
    return 0;
    }
  

  /*
   * a ColorMap MUST be created
   */
  if (!this->ColorMap)
    {
    cmapID = 
      XCreateColormap(this->DisplayId,
		      RootWindowOfScreen(ScreenOfDisplay(dpy,0)),
		      pVisInfo->visual, AllocNone);
    if (!cmapID) 
      {
      fprintf(stderr,"Could not create color map\n");
      return 0;
      }
    this->ColorMap = cmapID;
    }

  return this->ColorMap;  
}


// Description:
// Get a visual from the windowing system.
Visual *vlSbrRenderWindow::GetDesiredVisual ()
{
  Visual *vis;

  /* get the default display connection */
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay((char *)NULL); 
    if (this->DisplayId == NULL) 
      {
      vlErrorMacro(<< "bad X server connection.\n");
      }
    }

  /* get the default visual to use */
  vis = xlib_get_best_visual(this->DisplayId);

  return vis;  
}


// Description:
// Create a window for starbase output.
int vlSbrRenderWindow::CreateXWindow(Display *dpy,int xpos,int ypos, 
				     int width,int height,int depth, 
				     char name[80])
{
  Window win;
  XEvent event;
  XVisualInfo *pVisInfo,visInfo;
  Colormap cmapID;
  XColor c0, c1; 
  XSetWindowAttributes winattr;
  char *window_name;
  Pixmap icon_pixmap;
  int screen;
  unsigned int mask;
  int retVal;
  XSizeHints xsh;
  
  visInfo.screen = 0;
  visInfo.depth = depth;
  icon_pixmap = 0;
  
  /*
   * PseudoColor visual is used needed for CMAP_NORMAL
   */
  if(depth == 4)
    visInfo.c_class = PseudoColor;
  
  if(depth == 8)
    visInfo.c_class = PseudoColor;
  
  if(depth == 12)
    visInfo.c_class = PseudoColor;
  
  if(depth == 16)
    visInfo.c_class = PseudoColor;
  
  /*
   * DirectColor visual is used needed for CMAP_FULL
   */
  if(depth == 24)
    visInfo.c_class = DirectColor;
  
  vlDebugMacro(<< "Starbase: The depth is " << depth << "\n");
  /*
   * First, ask for the desired visual
   */
  mask = VisualScreenMask | VisualDepthMask | VisualClassMask;
  
  pVisInfo = XGetVisualInfo(dpy, mask, &visInfo, &retVal);
  
  if (!retVal) {
    if (depth == 24) {
      /*
       * try again with 16 bits
       */
      visInfo.depth = 16;
      visInfo.c_class = PseudoColor;
      pVisInfo = XGetVisualInfo(dpy, mask, &visInfo, &retVal);
      if (!retVal) {
	fprintf(stderr, "Could not get visual info\n");
	return 0;
      }
    } else {
      fprintf(stderr,"Could not get visual info\n");
        return 0;
    }
  }
  
  if (retVal != 1) {
    fprintf(stderr,"Too many visuals match display+depth+class\n");
    return 0;
  }
  
  // a ColorMap MUST be created
  if (!this->ColorMap)
    {
    cmapID = XCreateColormap(dpy,
			     RootWindowOfScreen(ScreenOfDisplay(dpy,0)),
			     pVisInfo->visual, AllocNone);
    if (!cmapID) 
      {
      fprintf(stderr,"Could not create color map\n");
      return 0;
      }
    this->ColorMap = cmapID;
    }

  
  // Border and background info MUST be passed in also
  winattr.event_mask = 0;
  winattr.border_pixel = 1;
  winattr.background_pixel = 0;
  winattr.colormap = this->ColorMap;
  if ((xpos >= 0)&&(ypos >= 0))
    {
    xsh.flags = USPosition | USSize;
    }
  else
    {
    xsh.flags = PPosition | PSize;
    }
  xsh.x = ((xpos >= 0) ? xpos : 5);
  xsh.y = ((ypos >= 0) ? ypos : 5);
  xsh.width = WidthOfScreen(ScreenOfDisplay(dpy,0));
  xsh.height = HeightOfScreen(ScreenOfDisplay(dpy,0));
  
  // if both the position and size have been set, override the window
  // manager
  winattr.override_redirect = False;
  if ((width > 0) && (xpos >= 0) && (!this->Borders))
    winattr.override_redirect = True;

  XFlush(dpy);
  
  /*
   * create the parent X11 Window
   */
  
  win = XCreateWindow(dpy, RootWindowOfScreen(ScreenOfDisplay(dpy,0)),
                      xsh.x, xsh.y, xsh.width, xsh.height, 0, depth,
                      InputOutput, pVisInfo->visual,
                      CWColormap | CWBorderPixel | CWBackPixel |
			CWEventMask | CWOverrideRedirect , &winattr);
  if(! win) 
    {
    fprintf(stderr,"Could not create window\n");
    return 0;
    }

  /*
   * Give the window a name
   */
  XSetStandardProperties(dpy, win, name, name, icon_pixmap, NULL, 0, &xsh);
  XSelectInput(dpy, win, KeyPressMask|ExposureMask|StructureNotifyMask);

  /*
   * set the default window
   */
  this->WindowId = win;  
  this->DisplayId = dpy;
  XSync(dpy, False );

  return 1;
}
 
// Description:
// Initialize the rendering window.
void vlSbrRenderWindow::WindowInitialize (void)
{
  char *device, *driver, *str;
  int planes, depth, mode;
  int create_xwindow();
  int cmap_size;
  XSizeHints *size_hints;
  XClassHint *class_hint;
  XWMHints *wm_hints;
  XTextProperty window_name, icon_name;
  char *list[1];
  XEvent event;
  XWindowAttributes winattr;

  // get the default depth to use
  depth = this->GetDesiredDepth();

  mode = OUTDEV;
  if (this->WindowId == 0) 
    {
    if (! 
	this->CreateXWindow(this->DisplayId, this->Position[0],
			    this->Position[1],
			    this->Size[0],
			    this->Size[1],
			    depth, this->Name)) 
      {
      vlErrorMacro(<< "Couldn't create window\n");
      return;
      }
    this->OwnWindow = 1;
    }
  else
    {
    XSetWindowAttributes xswattr;

    this->OwnWindow = 0;
    /* make sure the window is unmapped */
    XUnmapWindow(this->DisplayId, this->WindowId);
    XSync(this->DisplayId,False);
    vlDebugMacro(<< "Unmapping the xwindow\n");
    XGetWindowAttributes(this->DisplayId,
			 this->WindowId,&winattr);
    while (winattr.map_state != IsUnmapped)
      {
      XNextEvent(this->DisplayId, &event);
      XGetWindowAttributes(this->DisplayId,
			   this->WindowId,&winattr);
      }; 
    
    /* make sure the window is full screen */
    vlDebugMacro( << "Resizing the xwindow\n");
    XSelectInput(this->DisplayId, this->WindowId, 
		 KeyPressMask|ExposureMask);

    xswattr.override_redirect = False;
    if ((!this->Borders))
      xswattr.override_redirect = True;

    XChangeWindowAttributes(this->DisplayId,this->WindowId,
			    CWOverrideRedirect, &xswattr);

    XResizeWindow(this->DisplayId,this->WindowId,
		  WidthOfScreen(ScreenOfDisplay(this->DisplayId,0)),
		  HeightOfScreen(ScreenOfDisplay(this->DisplayId,0)));
    XSync(this->DisplayId,False);
    XGetWindowAttributes(this->DisplayId,
			 this->WindowId,&winattr);
    while (winattr.width != 
	   WidthOfScreen(ScreenOfDisplay(this->DisplayId,0)))
      {
/*      XNextEvent(this->DisplayId, &event); */
      XGetWindowAttributes(this->DisplayId,
			   this->WindowId,&winattr);
      } 
    }

  // convert window id to something Starbase can open
  device = make_X11_gopen_string(this->DisplayId, (Window)this->WindowId);
  if (!device) 
    {
    vlErrorMacro(<< "Could not create device file for window.\n");
    device = "/dev/crt";
    }
  
  driver = getenv("SB_OUTDRIVER");
  if ((this->Fd = 
       gopen(device, mode, driver, 
	     RESET_DEVICE | INIT | THREE_D | MODEL_XFORM)) == -1) 
    {
    vlErrorMacro(<< "cannot open starbase driver error number= " 
    << errno << "\n");
    return;
    }

  // RESIZE THE WINDOW TO THE DESIRED SIZE
  vlDebugMacro(<< "Resizing the xwindow\n");
  XResizeWindow(this->DisplayId,this->WindowId,
		((this->Size[0] > 0) ? 
		 (int)(this->Size[0]) : 256),
		((this->Size[1] > 0) ? 
		 (int)(this->Size[1]) : 256));
  XSync(this->DisplayId,False);

  list[0] = this->Name;
  XStringListToTextProperty( list, 1, &window_name );
  list[0] = this->Name;
  XStringListToTextProperty( list, 1, &icon_name );
    
  size_hints = XAllocSizeHints();
  size_hints->flags = USSize;
  if ((this->Position[0] >= 0)&&(this->Position[1] >= 0))
    {
    size_hints->flags |= USPosition;
    size_hints->x =  (int)(this->Position[0]);
    size_hints->y =  (int)(this->Position[1]);
    }
  
  size_hints->width  = 
    ((this->Size[0] > 0) ? (int)(this->Size[0]) : 256);
  size_hints->height = 
    ((this->Size[1] > 0) ?  (int)(this->Size[1]) : 256);
  
  wm_hints = XAllocWMHints();

  class_hint = XAllocClassHint();
  class_hint->res_name = this->Name;
  class_hint->res_class = this->Name;
  
  XSetWMProperties(this->DisplayId, 
		   this->WindowId, &window_name, &icon_name,
		   NULL, 0, size_hints, wm_hints, class_hint );

  /* Finally -- we can map the window!  We won't actually render anything
     to the window until the expose event happens later. */
  vlDebugMacro(<< "Mapping the xwindow\n");
  XMapWindow(this->DisplayId, this->WindowId);
  XSync(this->DisplayId,False);
  XGetWindowAttributes(this->DisplayId,
		       this->WindowId,&winattr);
  while (winattr.map_state == IsUnmapped)
    {
    XGetWindowAttributes(this->DisplayId,
			 this->WindowId,&winattr);
    };
  
  // free up the memory allocated above 
  free(device);

  set_p1_p2(this->Fd, FRACTIONAL, 0.0, 
	    0.0,
	    0.0, 1.0, 1.0, 1.0);

  mapping_mode(this->Fd, DISTORT);
  vlDebugMacro(<< "SB_mapping_mode: DISTORT\n");
  
  // set clipping
  clip_rectangle(this->Fd, 0.0, 1.0, 0.0, 1.0);
  clip_depth(this->Fd, 0.0, 1.0);
  clip_indicator(this->Fd, CLIP_TO_VIEWPORT);
  depth_indicator(this->Fd, TRUE, TRUE);
  
  // use the full color map, initialize it and turn shading on 
  shade_mode(this->Fd, CMAP_FULL | INIT, TRUE); 
  
  // set Fd update state - reset viewport and buffer commands
  this->NumPlanes = depth;
  if (this->DoubleBuffer > 0.0) 
    {
    if ((planes = double_buffer(this->Fd, 
				TRUE | INIT | SUPPRESS_CLEAR,
				depth)) != depth)
      {
      vlDebugMacro(<< "Only " << planes <<
      " planes available for double buffering\n");
      this->NumPlanes = planes;
      }
    dbuffer_switch(this->Fd,this->Buffer); 
    buffer_mode(this->Fd, TRUE);
    }


  // turn on z buffering and disable backface culling 
  hidden_surface(this->Fd, TRUE, FALSE);
  clear_control(this->Fd, CLEAR_DISPLAY_SURFACE | CLEAR_ZBUFFER);
  
  // set back faces of polygons to be rendered same as front 
  bf_control(this->Fd, FALSE, FALSE);
  // make default polymarker a dot (pixel) 
  marker_type(this->Fd, 0);

  // clear the display 
  clear_view_surface(this->Fd);

  clear_control(this->Fd, CLEAR_VIEWPORT | CLEAR_ZBUFFER);

  // ignore errors 
  gerr_print_control(NO_ERROR_PRINTING);
  this->Mapped = 1;
}

// Description:
// Initialize the rendering window.
void vlSbrRenderWindow::Initialize (void)
{
  // make sure we haven't already been initialized 
  if (this->Fd != -1) return;

  // now initialize the window 
  this->WindowInitialize();
}


// Description:
// Change the window to fill the entire screen.
void vlSbrRenderWindow::SetFullScreen(int arg)
{
  int *temp;

  if (this->FullScreen == arg) return;
  
  if (!this->Mapped)
    {
    this->PrefFullScreen();
    return;
    }

  // set the mode 
  this->FullScreen = arg;
  if (this->FullScreen <= 0)
    {
    this->Position[0] = this->OldScreen[0];
    this->Position[1] = this->OldScreen[1];
    this->Size[0] = this->OldScreen[2]; 
    this->Size[1] = this->OldScreen[3];
    this->Borders = this->OldScreen[4];
    }
  else
    {
    // if window already up get its values 
    if (this->WindowId)
      {
      XWindowAttributes attribs;
      
      //  Find the current window size 
      XGetWindowAttributes(this->DisplayId, 
			   this->WindowId, &attribs);
      
      this->OldScreen[2] = attribs.width;
      this->OldScreen[3] = attribs.height;;

      temp = this->GetPosition();      
      this->OldScreen[0] = temp[0];
      this->OldScreen[1] = temp[1];

      this->OldScreen[4] = this->Borders;
      this->PrefFullScreen();
      }
    }
  
  // remap the window 
  this->WindowRemap();

  // if full screen then grab the keyboard 
  if (this->FullScreen)
    {
    XGrabKeyboard(this->DisplayId,this->WindowId,
		  False,GrabModeAsync,GrabModeAsync,CurrentTime);
    }
  this->Modified();
}

// Description:
// Set the preferred window size to full screen.
void vlSbrRenderWindow::PrefFullScreen()
{
  int *size;

  size = this->GetScreenSize();

  // use full screen 
  this->Position[0] = 0;
  this->Position[1] = 0;
  this->Size[0] = size[0];
  this->Size[1] = size[1];

  // don't show borders 
  this->Borders = 0;
}


// Description:
// Resize the window.
void vlSbrRenderWindow::WindowRemap()
{
  // close the starbase window 
  if (this->Fd)
    {
    gclose(this->Fd);
    }
  this->Fd = -1;
  
  /* free the Xwindow we created no need to free the colormap */
  if (this->OwnWindow)
    {
    XDestroyWindow(this->DisplayId,this->WindowId);
    }
  XSync(this->DisplayId,0);
  this->WindowId = this->NextWindowId;
  this->NextWindowId = 0;

  // configure the window 
  this->WindowInitialize();
}


// Description:
// Specify the size of the rendering window.
void vlSbrRenderWindow::SetSize(int x,int y)
{
  // if we arent mappen then just set the ivars 
  if (!this->Mapped)
    {
    if ((this->Size[0] != x)||(this->Size[1] != y))
      {
      this->Modified();
      }
    this->Size[0] = x;
    this->Size[1] = y;
    return;
    }
  
  if ((this->Size[0] != x)||(this->Size[1] != y))
    {
    this->Modified();
    }
  this->Size[0] = x;
  this->Size[1] = y;
  
  XResizeWindow(this->DisplayId,this->WindowId,x,y);
  XSync(this->DisplayId,False);
}

void vlSbrRenderWindow::PrintSelf(ostream& os, vlIndent indent)
{
  this->vlXRenderWindow::PrintSelf(os,indent);

  os << indent << "Fd: " << this->Fd << "\n";
}

#define RGB_TO_332( r, g, b ) ( (  r       & 0xe0 ) + \
				( (g >> 3) & 0x1c ) + \
				( (b >> 6) & 0x03 ) )
#define RGB_TO_666_FACTOR 0.0196078
#define RGB_TO_666( r, g, b ) ( 40 + \
(unsigned char)(r * RGB_TO_666_FACTOR) * 36 + \
(unsigned char)(g * RGB_TO_666_FACTOR) * 6 + \
(unsigned char)(b * RGB_TO_666_FACTOR) )
#define RED_FROM_332(p) ((p & 0xe0))
#define GREEN_FROM_332(p) ((p & 0x1c) << 3)
#define BLUE_FROM_332(p) ((p & 0x03) << 6)
#define RED_FROM_666(p) ((p/36)*51)
#define GREEN_FROM_666(p) ((p%36)/6*51)
#define BLUE_FROM_666(p) ((p%6)*51)

unsigned char *vlSbrRenderWindow::GetPixelData(int x1, int y1, int x2, int y2)
{
  long     xloop,yloop;
  int     y_low, y_hi;
  int     x_low, x_hi;
  unsigned char   *buff1;
  unsigned char   *buff2;
  unsigned char   *buff3;
  unsigned char   *data = NULL;
  unsigned char   *p_data = NULL;

  buff1 = new unsigned char[abs(x2 - x1)+1];
  buff2 = new unsigned char[abs(x2 - x1)+1];
  buff3 = new unsigned char[abs(x2 - x1)+1];
  data = new unsigned char[(abs(x2 - x1) + 1)*(abs(y2 - y1) + 1)*3];

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }

  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }

  /* We'll turn off clipping so that we can do the block write anywhere */
  clip_indicator(this->Fd, CLIP_OFF );

  /* now read the binary info one row at a time */
  p_data = data;
  for (yloop = y_low; yloop <= y_hi; yloop++)
    {
    if (this->NumPlanes == 24)
      {
      /* No conversion is needed if we're working with a 24-bit frame
	 buffer.  However, the bank_switch() calls are needed to
	 let Starbase know which of the 3 frame buffer banks (red, green,
	 or blue) we wish to write to.  Bank 2 is red, bank 1 is green,
	 and bank 0 is blue. */
      bank_switch( this->Fd, 2, 0 );
      dcblock_read( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff1,FALSE );
      bank_switch( this->Fd, 1, 0 );
      dcblock_read( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff2,FALSE );
      bank_switch( this->Fd, 0, 0 );
      dcblock_read( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff3,FALSE );
      for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
	{
	*p_data = buff1[xloop]; p_data++; 
	*p_data = buff2[xloop]; p_data++; 
	*p_data = buff3[xloop]; p_data++; 
	}
      }
    if (this->NumPlanes == 12)
      {
      /*
	If the frame buffer depth is 12, we still have the red, green, and blue
	banks.  Again, each bank can be considered an array of 8-bit data.  In
	this case, however, there are only 4 bits of meaningful information per
	pixel. Whether this information is contained in the most significant or
	least significant 4 bits of the 8-bit data depends upon a variety of
	factors.  So, when preparing an 8-bit value to be written it is best to
	ensure that the most significant 4 bits are duplicated in the least
	significant 4 bits. When the read occurs Starbase will ensure that 
	the appropriate half of the actual frame buffer data will be modified.
	*/
      
      /* In this case, we will duplicate the most significant nibble
	 (4 bits) of each red, green, and blue value into the least
	 significant nibble. */
      bank_switch( this->Fd, 2, 0 );
      dcblock_read( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff1,FALSE );
      bank_switch( this->Fd, 1, 0 );
      dcblock_read( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff2,FALSE );
      bank_switch( this->Fd, 0, 0 );
      dcblock_read( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff3,FALSE );

      if (this->Buffer)
	{
	for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
	  {
	  *p_data = (buff1[xloop] & 0xf0) | ((buff1[xloop] & 0xf0) >> 4); 
	  p_data++; 
	  *p_data = (buff2[xloop] & 0xf0) | ((buff2[xloop] & 0xf0) >> 4); 
	  p_data++; 
	  *p_data = (buff3[xloop] & 0xf0) | ((buff3[xloop] & 0xf0) >> 4); 
	  p_data++; 
	  }
	}
      else
	{
	for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
	  {
	  *p_data = ((buff1[xloop] & 0x0f) << 4) | (buff1[xloop] & 0x0f); 
	  p_data++; 
	  *p_data = ((buff2[xloop] & 0x0f) << 4) | (buff2[xloop] & 0x0f); 
	  p_data++; 
	  *p_data = ((buff3[xloop] & 0x0f) << 4) | (buff3[xloop] & 0x0f); 
	  p_data++; 
	  }
	}
      }
    if (this->NumPlanes == 8)
      {
      /*
	If the frame buffer depth is 8, we have a single bank of 8-bit data.
	This case is a bit more complicated because, for each pixel, we need to
	"pack" the 24 bits of red, green, and blue data into a single 8-bit
	value.  If the SB_X_SHARED_CMAP environment variable is set, we pack
	the data using a 6|6|6 scheme. Otherwise, we must pack the data using a
	3:3:2 scheme.  See the "CRX Family of Device Drivers" chapter of the
	Starbase Device Drivers manual for detailed information about the 6|6|6
	and 3:3:2 schemes. In our example, we will define a couple of macros to
	simplify conversion to 6|6|6 and 3:3:2.
	*/
      /* There are two possible 8-bit formats, commonly known as 3:3:2 and
	 6|6|6.  If the SB_X_SHARED_CMAP environment variable is set, we
	 will use the 6|6|6 format.  Otherwise, we use the 3:3:2 format.
	 We'll define some macros to make this easier. */

      dcblock_read( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff1,FALSE );
      if (getenv("SB_X_SHARED_CMAP"))
	{
	for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
	  {
	  *p_data = RED_FROM_666(buff1[xloop]); p_data++;
	  *p_data = GREEN_FROM_666(buff1[xloop]); p_data++;
	  *p_data = BLUE_FROM_666(buff1[xloop]); p_data++;
	  }
	}
      else
	{
	for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
	  {
	  *p_data = RED_FROM_332(buff1[xloop]); p_data++;
	  *p_data = GREEN_FROM_332(buff1[xloop]); p_data++;
	  *p_data = BLUE_FROM_332(buff1[xloop]); p_data++;
	  }
	}
      }
    }
  
  /* Restore the clip_indicator() back to its default value */
  clip_indicator( this->Fd, CLIP_TO_VIEWPORT);

  delete buff1;
  delete buff2;
  delete buff3;

  return data;
}

void vlSbrRenderWindow::SetPixelData(int x1, int y1, int x2, int y2,
				     unsigned char *data)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     xloop,yloop;
  unsigned char   *buff1;
  unsigned char   *buff2;
  unsigned char   *buff3;
  unsigned char   *p_data = NULL;

  /* We'll turn off clipping so that we can do the block write anywhere */
  clip_indicator(this->Fd, CLIP_OFF );

 
  if (this->DoubleBuffer)
    {
/*    double_buffer(this->Fd, TRUE | DFRONT | INIT, this->NumPlanes);*/
    }

  buff1 = new unsigned char[abs(x2 - x1)+1];
  buff2 = new unsigned char[abs(x2 - x1)+1];
  buff3 = new unsigned char[abs(x2 - x1)+1];

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }
  
  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }
  
  /* now write the binary info one row at a time */
  p_data = data;
  for (yloop = y_low; yloop <= y_hi; yloop++)
    {
    if (this->NumPlanes == 24)
      {
      for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
	{
	buff1[xloop] = *p_data; p_data++; 
	buff2[xloop] = *p_data; p_data++;
	buff3[xloop] = *p_data; p_data++;
	}
      /* No conversion is needed if we're working with a 24-bit frame
	 buffer.  However, the bank_switch() calls are needed to
	 let Starbase know which of the 3 frame buffer banks (red, green,
	 or blue) we wish to write to.  Bank 2 is red, bank 1 is green,
	 and bank 0 is blue. */
      bank_switch( this->Fd, 2, 0 );
      dcblock_write( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff1,FALSE );
      bank_switch( this->Fd, 1, 0 );
      dcblock_write( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff2,FALSE );
      bank_switch( this->Fd, 0, 0 );
      dcblock_write( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff3,FALSE );
      }
    if (this->NumPlanes == 12)
      {
      /*
	If the frame buffer depth is 12, we still have the red, green, and blue
	banks.  Again, each bank can be considered an array of 8-bit data.  In
	this case, however, there are only 4 bits of meaningful information per
	pixel. Whether this information is contained in the most significant or
	least significant 4 bits of the 8-bit data depends upon a variety of
	factors.  So, when preparing an 8-bit value to be written it is best to
	ensure that the most significant 4 bits are duplicated in the least
	significant 4 bits. When the write occurs Starbase will ensure that 
	the appropriate half of the actual frame buffer data will be modified.
	*/
      
      /* In this case, we will duplicate the most significant nibble
	 (4 bits) of each red, green, and blue value into the least
	 significant nibble. */
      for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
	{
	buff1[xloop] = (*p_data & 0xf0) | (*p_data >> 4); p_data++; 
	buff2[xloop] = (*p_data & 0xf0) | (*p_data >> 4); p_data++; 
	buff3[xloop] = (*p_data & 0xf0) | (*p_data >> 4); p_data++; 
	}
      bank_switch( this->Fd, 2, 0 );
      dcblock_write( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff1,FALSE );
      bank_switch( this->Fd, 1, 0 );
      dcblock_write( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff2,FALSE );
      bank_switch( this->Fd, 0, 0 );
      dcblock_write( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff3,FALSE );
      }
    if (this->NumPlanes == 8)
      {
      /*
	If the frame buffer depth is 8, we have a single bank of 8-bit data.
	This case is a bit more complicated because, for each pixel, we need to
	"pack" the 24 bits of red, green, and blue data into a single 8-bit
	value.  If the SB_X_SHARED_CMAP environment variable is set, we pack
	the data using a 6|6|6 scheme. Otherwise, we must pack the data using a
	3:3:2 scheme.  See the "CRX Family of Device Drivers" chapter of the
	Starbase Device Drivers manual for detailed information about the 6|6|6
	and 3:3:2 schemes. In our example, we will define a couple of macros to
	simplify conversion to 6|6|6 and 3:3:2.
	*/
      /* There are two possible 8-bit formats, commonly known as 3:3:2 and
	 6|6|6.  If the SB_X_SHARED_CMAP environment variable is set, we
	 will use the 6|6|6 format.  Otherwise, we use the 3:3:2 format.
	 We'll define some macros to make this easier. */

      if (getenv("SB_X_SHARED_CMAP"))
	{
	for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
	  {
	  buff1[xloop] = RGB_TO_666(p_data[0], p_data[1], p_data[2]);
          p_data += 3; 
	  }
	}
      else
	{
	for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
	  {
	  buff1[xloop] = RGB_TO_332(p_data[0], p_data[1], p_data[2]);
          p_data += 3; 
	  }
	}
      /* Now that the data has been converted, we will write the 8-bit
	 values into the window.  There is no need for a bank_switch()
	 since we know that the appropriate 8-bit bank is already
	 enabled for writing. */
      dcblock_write( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff1,FALSE );
      }
    }
  
  delete buff1;
  delete buff2;
  delete buff3;

  if (this->DoubleBuffer)
    {
/*    double_buffer(this->Fd, TRUE | INIT | SUPPRESS_CLEAR, this->NumPlanes);*/
    }

  /* Restore the clip_indicator() back to its default value */
  clip_indicator( this->Fd, CLIP_TO_VIEWPORT);
}
 


// Description:
// Handles work required at end of render cycle
void vlSbrRenderWindow::CopyResultFrame(void)
{
  if (this->ResultFrame)
    {
    int *size;

    // get the size
    size = this->GetSize();

    this->SetPixelData(0,0,size[0]-1,size[1]-1,this->ResultFrame);
    delete this->ResultFrame;
    this->ResultFrame = NULL;
    }

  this->Frame();
}
