
#ifndef __TOMBOY_KEY_BINDER_H__
#define __TOMBOY_KEY_BINDER_H__

#include <glib/gtypes.h>

G_BEGIN_DECLS

typedef void (* TomboyBindkeyHandler) (char *keystring, gpointer user_data);

void tomboy_keybinder_init   (void);

gboolean tomboy_keybinder_bind (const char           *keystring,
				TomboyBindkeyHandler  handler,
				gpointer              user_data);

void tomboy_keybinder_unbind   (const char           *keystring,
				TomboyBindkeyHandler  handler);

gboolean tomboy_keybinder_is_modifier (guint keycode);

guint32 tomboy_keybinder_get_current_event_time (void);

G_END_DECLS

#endif /* __TOMBOY_KEY_BINDER_H__ */

