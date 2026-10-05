#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include "fineline.h"

/*#define PROMPT "Try>"*/
/*#define PROMPT "Try\u2502"*/
#define PROMPT "Try\u25b7"

int main(int argc, char **argv)
{
	char *line;

	setlocale(LC_ALL, "");
	while ((line = fineline(PROMPT)) != NULL) {
		printf("\"%s\"\n", line);
		free(line);
	}
	return 0;
}
