#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include "fineline.h"

/* This stores the default user configuration.  It is used to initialize each
 * newly allocated fineline_t.
 */
static fineline_config_t defaults;

/* This macro returns the offset to a given field.  It is expressed as using
 * the "defaults" variable, but the offsets work for any fineline_config_t.
 */
#define CONFIG_OFFSET(field)	((char*)&defaults.field - (char*)&defaults)

/* This macro is used in conjunction with the values in lookup[] to generate
 * a pointer to a given config setting's value.
 */
#define CONFIG_PTR(config, offset, type) (type *)((char *)&(config) + (offset))

/* This is a table of names, types, and offsets of all config fields */
static struct {
	char	*name;
	char	typec;
	size_t	offset;
} lookup[] = {
	{ "autoindent",     'b', CONFIG_OFFSET(autoindent) },
	{ "color", 	    'b', CONFIG_OFFSET(color) },
	{ "externaleditor", 's', CONFIG_OFFSET(externaleditor) },
	{ "externalplus",   'b', CONFIG_OFFSET(externalplus) },
	{ "externalsuffix", 's', CONFIG_OFFSET(externalsuffix) },
	{ "hint", 	    'b', CONFIG_OFFSET(hint) },
	{ "historycurb",    'i', CONFIG_OFFSET(historycurb) },
	{ "historysize",    'i', CONFIG_OFFSET(historysize) },
	{ "matchparen",     'b', CONFIG_OFFSET(matchparen) },
	{ "tabspaces", 	    'b', CONFIG_OFFSET(tabspaces) },
	{ "tabstop", 	    'i', CONFIG_OFFSET(tabstop) },
	{ "undosize", 	    'i', CONFIG_OFFSET(undosize) },
	{ "window", 	    'i', CONFIG_OFFSET(window) },
	{ NULL }
};

/* If this is the first time, then initialize the default configuration */
static void init_config(void)
{
	static	int firsttime = 1;

	/* If already done, then skip this */
	if (!firsttime)
		return;
	firsttime = 0;

	/* Start with hardcoded defaults */
	defaults.window = 8;	 /* Limit the window height */
	defaults.matchparen = 0; /* Not always useful */
	defaults.autoindent = 0; /* Not always useful or expected */
	defaults.tabstop = 4;	 /* Traditionally 8, but 4 is becoming common */
	defaults.tabspaces = 0;	 /* Insert tabs as tabs, usually */
	defaults.color = 1;	 /* If syntax coloring is available, use it */
	defaults.hint = 1;	 /* If hinting is available, use it */
	defaults.undosize = 20;	 /* Levels of undo */
	defaults.historysize = 100; /* Levels of history */
	defaults.historycurb = 500; /* 0.5 seconds to use <Up> for history */
	defaults.externalsuffix = ""; /* No suffix by default */
	defaults.externalplus = 1; /* Most text editors support "+line" */

	/* Check environment variables for possible external editors */
	defaults.externaleditor = getenv("VISUAL");
	if (!defaults.externaleditor)
		defaults.externaleditor = getenv("EDITOR");
	if (!defaults.externaleditor)
		defaults.externaleditor = "";
}

/* Copy a config into a fineline_t.  If config is NULL, use defaults */
void fineline_config_copy(fineline_t *fine, fineline_config_t *config)
{
	/* Defend against self-copy */
	if (&fine->config == config)
		return;

	/* If config is NULL, then use defaults */
	if (!config) {
		init_config();
		config = &defaults;
	}

	/* Copy it.  Each fineline_t gets its own copy of strings */
	if (fine->config.externaleditor)
		free(fine->config.externaleditor);
	free(fine->config.externalsuffix);
	if (config)
		fine->config = *config;

	if (fine->config.externaleditor)
		fine->config.externaleditor = strdup(fine->config.externaleditor);
	fine->config.externalsuffix = strdup(fine->config.externalsuffix);
}

/* Free memory allocated for a fineline_t's config */
void fineline_config_free(fineline_t *fine)
{
	if (fine->config.externaleditor)
		free(fine->config.externaleditor);
	free(fine->config.externalsuffix);
}

/* Given a "name=value" string, store the value as appropriate.  If config is
 * NULL then use the defaults.  If len is 0, then use strlen() as the length.
 * Returns NULL normally or a static string if error.
*/
const char *fineline_config_set(fineline_config_t *config, const char *str, size_t len)
{
	char	*equals, **valptr;
	int	i, n;
	size_t	namelen;

	/* If config is NULL, use defaults */
	if (!config) {
		init_config();
		config = &defaults;
	}

	/* If length is 0 then use strlen() */
	if (len == 0)
		len = strlen(str);

	/* Look for "=" */
	equals = strchr(str, '=');
	if (!equals || equals >= &str[len])
		return "No \"=\"";

	/* Look up the name */
	namelen = equals - str;
	for (i = 0; lookup[i].name; i++) {
		if (!strncmp(lookup[i].name, str, namelen) && !lookup[i].name[namelen])
			break;
	}
	if (!lookup[i].name)
		return "Unknown config";

	/* The way the value is handled depends on its type */
	switch (lookup[i].typec) {
	case 'b': /* Boolean */
		*CONFIG_PTR(config, lookup[i].offset, int) = equals[1] == 't';
		break;

	case 'i': /* Integer */
		/* Convert to decimal, being mindful of len */
		for (n = 0, equals++; equals < str + len; equals++) {
			if (*equals < '0' || *equals > '9') {
				return "Number expected";
			}
			n = n * 10 + *equals - '0';
		}
		*CONFIG_PTR(config, lookup[i].offset, int) = n;
		break;

	case 's': /* String */
		valptr = CONFIG_PTR(config, lookup[i].offset, char *);
		if (*valptr)
			free(*valptr);
		*valptr = malloc(len - namelen);
		strncpy(*valptr, equals + 1, len - namelen - 1);
		(*valptr)[len - namelen] = '\0';
		break;
	}
	return NULL;
}


/* This is used to step through the possible names for variables.  It returns
 * the name in a static string, or NULL if there are no more values.  The
 * prevname argument should be a value returned by a previous call to this
 * function, or NULL.  The delta should should generally 1 or -1, though as
 * a special case if prevname is NULL then you can pass 0 to let it "guess"
 * which option the user is likely to want to change.
 */
const char *fineline_config_name(const char *prevname, int delta)
{
	int	i;

	/* If given a prevname, look it up.  If invalid, ignore it */
	if (prevname) {
		for (i = 0; lookup[i].name && lookup[i].name != prevname; i++) {
		}
		if (!lookup[i].name)
			prevname = NULL;
	}

	/* If no prevname, or an invalid one, then choose a starting point */
	if (!prevname) {
		if (delta == 1)
			i = -1;
		else if (delta == -1) {
			for (i = 0; lookup[i].name; i++){
			}
		} else {
			i = 3; /* tabstop */
			delta = 0;
		}
	}

	/* Apply the delta.  If out of range, return NULL else return name */
	i += delta;
	if (i < 0 || !lookup[i].name)
		return NULL;
	return lookup[i].name;
}
