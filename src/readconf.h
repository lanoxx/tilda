/*
 * Generic configuration file handling.
 * By Andy McFadden.  This software is in the public domain.
 * Code formatted with 4-space tabs (use ":set ts=4" in vi).
 *
 *
 * readconf.h revision history:
 *
 * v1.1    ATM  15-Feb-96
 *	Reformatted source, converting to ANSI C.  Added "dispose" functions.
 *  Made config table an argument (useful for large projects with multiple
 *  config files).
 * v1.0    ATM  22-Jul-94
 *	Seems to work.
 */
 
#ifndef _READCONF_H
#define _READCONF_H
#include <stdio.h>
#include <stdlib.h>

/*
 * Structure definition for configuration spec.
 */
typedef enum {
    CF_BOOLEAN, CF_INT, CF_DOUBLE, CF_STRING, CF_MULTI_STRING, CF_MULTI_LINE
} CF_LINE_KIND;

typedef struct {
	CF_LINE_KIND kind;		/* what kind of statement this is */
	char *tag;				/* tag name */
	void *reference;		/* variable to change or function to call */
	int size;				/* used by string items only */
	char *delim;			/* used by multi-items only */
	int flags;				/* reserved, set to zero */
	void (*dispose)(void);	/* optional dispose function */
} CONFIG;

/*
 * Prototoypes.
 */
int read_config(const char *argv0, const CONFIG *config_tab, int config_size, FILE *fp);
int read_config_file(const char *argv0, const CONFIG *config_tab, int config_count, const char *filename);
int dispose_config(const char *argv0, const CONFIG *config_tab, int config_count);

#endif /*_READCONF_H*/
