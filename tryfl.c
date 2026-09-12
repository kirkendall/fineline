#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include "fineline.h"

int main(int argc, char **argv)
{
	char *line;

	setlocale(LC_ALL, "");
	while ((line = fineline("Try>")) != NULL) {
		printf("\"%s\"\n", line);
		free(line);
	}
	return 0;
}
