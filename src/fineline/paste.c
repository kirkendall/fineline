#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <sys/stat.h>
#include <fineline.h>

/* paste.c -- This manages copy/paste in fineline. 
 *
 * There is a single cut/paste buffer shared among all fineline_t instances.
 * This allows you to copy text from one window to another.  It can also
 * interface with X11 or Wayland via the xclip and wl-copy/wl-paste programs,
 * respectively.
 */

/* These store the cut buffer contents */
static char *buffer;	/* Memory to hold the text */
static size_t bufsize;	/* Size of the buffer */
static size_t bufused;	/* Number of bytes used in the buffer */

/* These configure the library to access the GUI's cut buffer */
static int didcopypaste = 1;	/* so first copy/paste can auto-detect */
static const char *aftercopycmd, *beforepastecmd;
static void (*aftercopy)(void);
static void (*beforepaste)(void);


/* Run a program to send copied text to the GUI via xclip */
static void runcopy(void)
{
	FILE *fp = popen(aftercopycmd, "w");
	if (fp) {
		fwrite(buffer, sizeof(char), bufused, fp);
		pclose(fp);
	}
}

/* Run a program to fetch text to paste from the GUI via xclip */
static void runpaste(void)
{
	size_t	sofar;
	ssize_t	more;
	size_t	guisize = 4096;
	char	*guibuf = malloc(guisize);
	FILE *fp = popen(beforepastecmd, "r");

	if (fp) {
		/* Fetch data from the program */
		more = sofar = 0;
		do {
			sofar += more;

			/* If less than 1K of buffer space, expand it */
			if (guisize - sofar < 1024) {
				guisize += 4096;
				guibuf = realloc(guibuf, guisize);
			}

			/* Read more into buffer */
			more = fread(guibuf + sofar, sizeof(char), guisize - sofar, fp);
		} while (more > 0);

		/* If the program succeeded and we got data, use it */
		if (pclose(fp) == 0 && sofar > 0) {
			if (sofar > bufsize) {
				bufsize = sofar;
				buffer = realloc(buffer, bufsize);
			}
			memcpy(buffer, guibuf, sofar);
			bufused = sofar;
		}
	}

	/* Clean up */
	free(guibuf);
}

/* Configure function to call after copying and before pasting */
void fineline_paste_hook(void (*copyfn)(void), void(*pastefn)(void))
{
	aftercopy = copyfn;
	beforepaste = pastefn;
	didcopypaste = 1;
}

/* Configure shell commands to run after copying and before pasting */
void fineline_paste_cmds(const char *copycmd, const char *pastecmd)
{
	aftercopycmd = copycmd;
	beforepastecmd = pastecmd;
	fineline_paste_hook(runcopy, runpaste);
}


/* Try to guess whether to use X11 or Wayland copy/paste programs */
static void guessgui(void)
{
	struct stat st;

	/* If we did this already, skip it now */
	if (didcopypaste)
		return;

	/* Maybe use X11? */
	if (getenv("DISPLAY") && stat("/usr/bin/xclip", &st) >= 0) {
		fineline_paste_cmds("xclip -i 2>/dev/null", "xclip -o 2>/dev/null");
		return;
	}

	/* Maybe use Wayland? */
	if (getenv("WAYLAND_DISPLAY") && stat("/usr/bin/wl-copy", &st) >= 0) {
		fineline_paste_cmds("wl-copy 2>/dev/null", "wl-paste 2>/dev/null");
		return;
	}

	/* Nothing.  But at least stop trying */
	didcopypaste = 1;
}



/* Copy text into the text buffer */
void fineline_copy(const char *text, size_t len)
{
	/* Defend against empty copies */
	if (len == 0)
		return;

	/* Expand the buffer if necessary */
	if (len > bufsize) {
		bufsize = len;
		buffer = realloc(buffer, bufsize);
	}

	/* Copy the text */
	memcpy(buffer, text, len);
	bufused = len;

	/* Call the aftercopy function, if any */
	guessgui();
	if (aftercopy)
		aftercopy();
}

/* Return the size of the cut/paste text */
size_t fineline_paste_size(void)
{
	/* Allow the GUI to adjust the buffer */
	guessgui();
	if (beforepaste)
		beforepaste();
	return bufused;
}

/* Return a pointer to the cut/paste text */
const char *fineline_paste(void)
{
	return buffer;
}
