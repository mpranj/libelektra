#ifndef ELEKTRA_XFCONF_H
#define ELEKTRA_XFCONF_H

#include <xfconf/xfconf.h>
#include <gelektra-kdb.h>

#define I_(string) (g_intern_static_string((string)))

extern GList * channel_list;
extern GElektraKdb * gElektraKdb;

#endif
