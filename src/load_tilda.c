/*
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <gtk/gtk.h>

#include "tilda.h"

gboolean load_tilda (gboolean from_main)
{
	int TRANS_LEVEL = 0;       /* how transparent the window is, percent from 0-100 */
    VteTerminalAntiAlias antialias = VTE_ANTI_ALIAS_USE_DEFAULT;
	gboolean scroll = FALSE, highlight_set = FALSE, cursor_set = FALSE, 
    		 use_antialias = FALSE, bool_use_image = FALSE;
    gboolean image_set_clo=FALSE, antialias_set_clo=FALSE,
    	     scroll_set_clo=FALSE; 
	GdkColor fore, back, tint, highlight, cursor;
     
    back.red = back.green = back.blue = 0xffff;
    fore.red = fore.green = fore.blue = 0x0000;
    highlight.red = highlight.green = highlight.blue = 0xc000;
    cursor.red = 0xffff;
    cursor.green = cursor.blue = 0x8000;
    tint.red = tint.green = tint.blue = 0;
    tint = back;
        
    TRANS_LEVEL = (transparency)/100; 

	if (strcmp (s_use_image, "TRUE") == 0 || image_set_clo == TRUE)
   		bool_use_image = TRUE;
    else
      	bool_use_image = FALSE;
    
    if (strcmp (s_antialias, "TRUE") == 0 || antialias_set_clo == TRUE)
   	    use_antialias = TRUE;
    else
       	use_antialias = FALSE;
    
	if (strcmp (s_scrollbar, "TRUE") == 0 || scroll_set_clo == TRUE)
    	scroll = TRUE;       
    else
       	scroll = FALSE;   
	
    if (bool_use_image) 
    {
        vte_terminal_set_background_image_file (VTE_TERMINAL(widget), s_image);
    }
    
    if (TRANS_LEVEL > 0) 
    {
        vte_terminal_set_background_saturation (VTE_TERMINAL (widget), TRANS_LEVEL);
        vte_terminal_set_background_transparent (VTE_TERMINAL(widget), TRUE);
    }
    
    if (strcasecmp (s_background, "black") == 0)
    {
        back.red = back.green = back.blue = 0x0000;
        fore.red = fore.green = fore.blue = 0xffff;
    }

    vte_terminal_set_background_tint_color (VTE_TERMINAL(widget), &tint);
    vte_terminal_set_colors (VTE_TERMINAL(widget), &fore, &back, NULL, 0);
    
    if (highlight_set) 
    {
        vte_terminal_set_color_highlight (VTE_TERMINAL(widget), &highlight);
    }
    
    if (cursor_set) 
    {
        vte_terminal_set_color_cursor (VTE_TERMINAL(widget), &cursor);
    }
    
    if (use_antialias)
        vte_terminal_set_font_from_string_full (VTE_TERMINAL(widget), s_font, antialias);
    else
        vte_terminal_set_font_from_string (VTE_TERMINAL(widget), s_font);
    
   	if (!from_main)
    {
    	gtk_widget_hide (window);       
            
       	if (scroll)
        	gtk_widget_show (scrollbar);
        else
        	gtk_widget_hide (scrollbar);   
           
        gtk_widget_show (window);     
            
    	if ((strcasecmp (s_pinned, "true")) == 0)
			gtk_window_stick (GTK_WINDOW (window));

    	if ((strcasecmp (s_above, "true")) == 0)
			gtk_window_set_keep_above (GTK_WINDOW (window), TRUE); 
    }
    else
    {
    	if (scroll)
        	gtk_widget_show (scrollbar);
        else
    		gtk_widget_hide (scrollbar);
	}
    
    if ((strcasecmp (s_notaskbar, "true")) == 0)
        gtk_window_set_skip_taskbar_hint (GTK_WINDOW(window), TRUE);
    else
    	gtk_window_set_skip_taskbar_hint (GTK_WINDOW(window), FALSE);
    
    gtk_widget_show (widget);
    gtk_widget_show (hbox);
    
    
	return TRUE;
}
