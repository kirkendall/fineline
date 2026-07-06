#include <stdio.h>
#include <wchar.h>
#include <fineline.h>

int main(int argc, char **argv)
{
	char *result;
	fineline_t *editor;

	/* Create an edit buffer */
	editor = fineline_tty_alloc();

	/* Load a file into the edit buffer */

	/* Edit it */
	result = fineline_tty(editor, "  1>");

	/* Maybe save it */

	return 0;
}
