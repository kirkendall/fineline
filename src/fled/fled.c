#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fineline.h>

int main(int argc, char **argv)
{
	char *textin, *textout;
	size_t textlen, textsize;
	ssize_t more;
	fineline_t *fine, *editor;
	struct stat st;
	int	fd;

	/* Common flags */
	if (argc > 2 || (argc > 1 && !strcmp(argv[1], "--help"))) {
		puts("Usage: fled [filename]");
		return 0;
	}
	if (!argc > 1 && !strcmp(argv[1], "--version")) {
		printf("fled %s copyright 2027 by Steve Kirkendall\n", FINELINE_VERSION);
		puts("Freely redistributable under the terms of the GNU Limited General Public License");
		return 0;
	}

	/* Expect one filename */
	if (argc < 2 || stat(argv[1], &st) < 0 || st.st_size == 0) {
		textsize = 1;
		textlen = 1;
		textin = strdup("");
	} else {
		textsize = st.st_size;
		textin = malloc(textsize);
		fd = open(argv[1], O_RDONLY);
		if (fd < 0) {
			perror(argv[1]);
			return 1;
		}
		for (textlen = 0; textlen < textsize && (more = read(fd, textin+textlen, textsize - textlen)) > 0; textlen += more) {
		}
		close(fd);

		/* trim the last newline */
		if (textlen > 0 && textin[textlen - 1] == '\n')
			textin[--textlen] = '\0';
	}

	/* Create an edit buffer */
	fine = fineline_tty_alloc();
	editor = fineline_mode_editor(fine, textin, textlen);

	/* Edit it */
	textout = fineline_tty(editor, "  1|");

	/* Maybe save it */
	if (textout && strcmp(textin, textout)) {
		fd = open(argv[1], O_WRONLY);
		if (fd < 0) {
			perror(argv[1]);
			return 1;
		}
		write(fd, textout, strlen(textout));
		write(fd, "\n", 1);
		close(fd);
	}

	/* Clean up */
	free(textin);
	free(textout);
	fineline_free(editor);
	fineline_free(fine);

	return 0;
}
