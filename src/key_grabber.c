/*
*
* Some stolen from yeahconsole -- loving that open source :)
*
*/

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tilda.h"

Display *dpy;
Window root;
Window win;
Window termwin;
Window last_focused;
int screen;
KeySym key;

int pos_x = 0;              /* x position of tilda on screen */
int pos_y = 0;              /* y position of tilda on screen */

void pull ()
{
    gint w, h;
    //gint min_h, max_h;

	gtk_window_get_size ((GtkWindow *) window, &w, &h);

    if (h == min_height)
    {  	
    	gdk_threads_enter();
        
        if (gtk_window_is_active ((GtkWindow *) window) == FALSE)
			gtk_window_present ((GtkWindow *) window);
   		else 
      		gtk_widget_show ((GtkWidget *) window);    
        
    	if ((strcasecmp (s_pinned, "true")) == 0)
			gtk_window_stick (GTK_WINDOW (window));

    	if ((strcasecmp (s_above, "true")) == 0)
			gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);

    	gtk_window_move ((GtkWindow *) window, pos_x, pos_y);      
    	
        /*for (max_h=h;max_h<=max_height;max_h++)
        {
    		gtk_window_resize ((GtkWindow *) window, max_width, max_h);
        	sleep (.01);
        }*/
        gtk_window_resize ((GtkWindow *) window, max_width, max_height);
        gdk_flush ();
		gdk_threads_leave();
    }
	else if (h == max_height)
    {   
    	gdk_threads_enter();
        
        /*for (min_h=h;min_h>1;min_h--)
        {
        	gtk_window_resize ((GtkWindow *) window, min_width, min_h);
        	sleep (.001);
        }*/
    	
        gtk_window_resize ((GtkWindow *) window, min_width, min_height);
        gtk_widget_hide ((GtkWidget *) window);
        
        gdk_flush ();
	 	gdk_threads_leave(); 
    }
}

void key_grab ()
{
    XModifierKeymap *modmap;
    unsigned int numlockmask = 0;
    unsigned int modmask = 0;
    int i, j;

    /* Key grabbing stuff taken from yeahconsole who took it from evilwm */
    modmap = XGetModifierMapping(dpy);
    for (i = 0; i < 8; i++) {
        for (j = 0; j < modmap->max_keypermod; j++) {
            if (modmap->modifiermap[i * modmap->max_keypermod + j] == XKeysymToKeycode(dpy, XK_Num_Lock)) {
                numlockmask = (1 << i);
            }
        }
    }
    XFreeModifiermap(modmap);

    if (strstr(s_key, "Control"))
        modmask = modmask | ControlMask;
        
    if (strstr(s_key, "Alt"))
        modmask = modmask | Mod1Mask;
        
    if (strstr(s_key, "Win"))
        modmask = modmask | Mod4Mask;
        
    if (strstr(s_key, "None"))
        modmask = 0;
        
    if (strtok(s_key, "+"))
        key = XStringToKeysym(strtok(NULL, "+"));
        
    XGrabKey(dpy, XKeysymToKeycode(dpy, key), modmask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, key), LockMask | modmask, root, True, GrabModeAsync, GrabModeAsync);
    
    if (numlockmask) 
    {
        XGrabKey(dpy, XKeysymToKeycode(dpy, key), numlockmask | modmask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dpy, XKeysymToKeycode(dpy, key), numlockmask | LockMask | modmask, root, True, GrabModeAsync, GrabModeAsync);
    }
}

void *wait_for_signal ()
{
    KeySym key;
    XEvent event;

    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "Shit -- can't open Display %s", XDisplayName(NULL));
    }
    
    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);

    key_grab ();
    
   	if (strcmp (s_down, "TRUE") == 0)
    	pull ();
    else
    	gtk_widget_hide (window);
    
    while (1) {
    	XNextEvent(dpy, &event);
        
        switch (event.type) {
        	case KeyPress:
            	key = XKeycodeToKeysym(dpy, event.xkey.keycode, 0);
            	if (key == key) {
            		pull (window);
                	break;
            	}
          	default:
            	break;
        }
    }
    
    return 0;
}
