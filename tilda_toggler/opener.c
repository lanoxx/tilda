#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "toggle.h"

int main() {

TilTilToggle *proxy;
proxy = til_til_toggle_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, "io.koosha", "/io/koosha/Toggle", NULL, NULL);
til_til_toggle_call_hello_sync(proxy, NULL, NULL);

g_object_unref(proxy);
return 0;
}

