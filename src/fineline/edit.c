#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <fineline.h>

/* edit.c -- The basic line editor. */

/* Implement copy/paste functionality */
static void copypaste(fineline_t *fine, int copy, int cut, int paste)
{
	size_t	len;
	char	*start;

	/* If there's a selection and we're supposed to copy or cut, do that */
	if (fine->selection >= 0 && (copy || cut)) {
		/* find the start of the selection */
		if (fine->selection < fine->cursor) {
			start = &fine->line[fine->selection];
			len = fine->cursor - fine->selection + 1;
		} else {
			start = &fine->line[fine->cursor];
			len = fine->selection - fine->cursor + 1;
		}

		/* Maybe copy the text */
		if (copy)
			fineline_copy(start, len);

		/* Maybe delete the text */
		if (cut) {
			if (start[len + 1])
				memmove(start, start + len, strlen(start + len) + 1);
			fine->cursor = start - fine->line;
		}
	}

	/* Either way, no selection after this */
	fine->selection = -1;

	/* If supposed to paste, do that */
	if (paste) {
		/* IMPORTANT: We need to call fineline_paste_size() before
		 * fineline_paste(), to fetch text from the GUI.
		 */
		len = fineline_paste_size();
		fineline_edit_text(fine, fineline_paste(), len);
	}
}

/* Perform an edit operation at the cursor.  Return 0 if successful, 1 if
 * a complete line is ready to process, or -1 if error.  An error typically
 * means you've bumped into the edge of the line being edited.
 */
int fineline_edit(fineline_t *fine, fineline_edit_t edit)
{
	size_t	len;
	int	col, lnum, start, tmp;
	const char	*moved;

	/* Maybe start or cancel a selection */
	switch (edit) {
	case FINELINE_S_HOME:
	case FINELINE_S_END:
	case FINELINE_S_LEFT_WORD:
	case FINELINE_S_RIGHT_WORD:
	case FINELINE_S_LEFT:
	case FINELINE_S_RIGHT:
	case FINELINE_S_UP:
	case FINELINE_S_DOWN:
	case FINELINE_S_ALL:
	case FINELINE_S_LINE:
		/* If no selection is pending, then start one at the cursor's
		 * current position.
		 */
		if (fine->selection < 0)
			fine->selection = fine->cursor;
		break;

	case FINELINE_COPY:
	case FINELINE_CUT:
	case FINELINE_PASTE:
		/* Don't start a selection or end one */
		break;

	default:
		/* Any other command aborts a selection */
		fine->selection = -1;
	}

	/* If prompting for a search, maybe terminate it or do it */
	if (fine->searchprompt) {
		switch (edit) {
		case FINELINE_SEARCH_F:
		case FINELINE_SEARCH_R:
		case FINELINE_SEARCH_G:
			/* Just terminate the prompt */
			fine->searchprompt = 0;
			return 0;

		case FINELINE_BACK_SPACE:
			/* If searchbuf is empty, just end the prompt */
			if (!*fine->searchbuf) {
				fine->searchprompt = '\0';
				return 0;
			}

			/* Delete the last character from the searchbuf */
			if (*fine->searchbuf) {
				tmp = fineline_char_delta(fine->searchbuf, strlen(fine->searchbuf), -1);
				fine->searchbuf[tmp] = '\0';
			}
			return 0;

		case FINELINE_ENTER:
			/* Process the entered value */
			switch (fine->searchprompt) {
			case '>':	edit = FINELINE_NEXT_F;	break;
			case '<':	edit = FINELINE_NEXT_R;	break;
			case '@':
				/* If number then jump to numbered line */
				if (fine->searchbuf[0] >= '1' && fine->searchbuf[0] <= '9') {
					tmp = atoi(fine->searchbuf);
					start = fineline_char_line_offset(fine->line, tmp - 1);
					if (start == 0 && tmp != 1)
						return 1; /* error */
					fine->cursor = start;
					fine->searchprompt = 0;
					return 0;
				}

				/* If no "goto" hook, then find first instance
				 * of the entered text.  Hopefully this is a
				 * function definition.
				 */
				if (!fine->goto_hook) {
					len = strlen(fine->searchbuf);
					for (moved = fine->line;
					     (moved = strstr(moved, fine->searchbuf)) != NULL;
					     moved++) {
						if ((moved == fine->line || (moved[-1] < 'A' && moved[-1] > 'z'))
						 && (!moved[len] || (moved[len] < 'A' && moved[len] > 'z'))) {
							fine->cursor = moved - fine->line;
							fine->searchprompt = 0;
							return 0;
						}
					}
					return 1; /* error - not found */
				}
			default:
				/* If we get here, either we're doing a weird
				 * application-specific prompt, or language-
				 * specific definition lookup.
				 */
				/* If there's a hook, try it */
				if (fine->goto_hook) {
					start = fine->goto_hook(fine);
					if (start >= 0) {
						fine->cursor = start;
						fine->searchprompt = 0;
						return 0;
					}
				}
				fine->searchprompt = 0;
				return 1; /* error */
			}

			/* fall through... */
		default:
			/* Just end the prompt, and process edit key normally */
			fine->searchprompt = 0;
		}

	}

	/* Do the command */
	switch (edit) {
	case FINELINE_INSERT:
		/* Toggle insert/overwrite mode */
		fine->replace = !fine->replace;
		break;

	case FINELINE_DELETE:
		/* Delete character at cursor, unless at end of input */
		if (fine->line[fine->cursor]) {
			/* If on a history line, copy it to current line */
			fineline_history_edit(fine);

			/* Find the byte-length of the character */
			len = fineline_char_size(fine->line + fine->cursor, 1);

			/* Move the characters after the cursor, and the '\0'
			 * at the end of the input line.
			 */
			memmove(&fine->line[fine->cursor], &fine->line[fine->cursor + len], strlen(&fine->line[fine->cursor + len]) + 1);
		}
		break;

	case FINELINE_BACK_SPACE:
		/* Delete character before cursor */
		fineline_edit(fine, FINELINE_LEFT);
		fineline_edit(fine, FINELINE_DELETE);
		break;

	case FINELINE_BACK_WORD:
		/* Delete to start of word */
		moved = &fine->line[fine->cursor];
		fineline_edit(fine, FINELINE_LEFT_WORD);
		if (moved != &fine->line[fine->cursor]) {
			fineline_history_edit(fine);
			memmove(fine->line + fine->cursor, moved, strlen(moved) + 1);
		}
		break;

	case FINELINE_BACK_LINE:
		/* Delete to start of line */
		moved = &fine->line[fine->cursor];
		fineline_edit(fine, FINELINE_HOME);
		if (moved != &fine->line[fine->cursor]) {
			fineline_history_edit(fine);
			memmove(fine->line + fine->cursor, moved, strlen(moved) + 1);
		}
		break;

	case FINELINE_BACK_TAB:
		/* Delete spaces to previous tabstop */

		/* Find the desired column */
		col = fineline_char_column_number(fine->line, fine->cursor);
		if (col > 0 && col % fine->tabstop == 0)
			col -= fine->tabstop;
		else
			col -= col % fine->tabstop;

		/* Find the character at that column */
		moved = fineline_char_at_column(fine->line, col, NULL);

		/* We want to delete whitespace characters between the moved
		 * position and the cursor, but if there are non-whitespace
		 * characters then we want to keep them.
		 */
		for (start = fine->cursor - 1;
		     &fine->line[start] > moved
		        && (fine->line[start] == ' ' || fine->line[start] == '\t');
		     start--) {
		}
		if (start != fine->cursor) {
			/* If on a history line, copy it to current line */
			fineline_history_edit(fine);

			/* Move the characters after the cursor, and the '\0'
			 * at the end of the input line.
			 */
			memmove(&fine->line[start], &fine->line[fine->cursor], strlen(&fine->line[fine->cursor]) + 1);

			/* Adjust the cursor position */
			fine->cursor = start;
		}
		break;

	case FINELINE_TAB:	
		/* Insert spaces to next tabstop */
		col = fineline_char_column_number(fine->line, fine->cursor);
		do {
			fineline_edit_char(fine, L' ');
			col++;
		} while (col % fine->tabstop != 0);
		break;

	case FINELINE_HOME:	
	case FINELINE_S_HOME:
		/* Move cursor to start of line.  If it's already at the start
		 * of the line, then move to the start of the edit buffer.
		 */
		len = fineline_char_line_offset(fine->line, fineline_char_line_number(fine->line, fine->cursor));
		if (len == fine->cursor)
			len = 0;
		fine->cursor = len;
		break;

	case FINELINE_END:
	case FINELINE_S_END:
		/* Move cursor to end of line.  If already at the end of the
		 * line, then move to the end of the edit buffer.
		 */
		if (fine->line[fine->cursor] == '\n')
			fine->cursor = strlen(fine->line);
		else {
			while (fine->line[fine->cursor] && fine->line[fine->cursor] != '\n')
				fine->cursor++;
		}
		break;

	case FINELINE_LEFT_WORD:
	case FINELINE_S_LEFT_WORD:
		/* First move past whitespace before the cursor */
		for (start = fine->cursor; start > 0;) {
			start = fineline_char_delta(fine->line, start, -1);
			if (fine->line[start] != ' '
			 && fine->line[start] != '\n'
			 && fine->line[start] != '\t')
				break;
		}

		/* Then go past non-whitespace ALMOST to the next whitespace */
		for (; start > 0; start = tmp) {
			tmp = fineline_char_delta(fine->line, start, -1);
			if (fine->line[tmp] == ' '
			 || fine->line[tmp] == '\n'
			 || fine->line[tmp] == '\t')
				break;
		}
		fine->cursor = start;
		break;

	case FINELINE_RIGHT_WORD:
	case FINELINE_S_RIGHT_WORD:
		/* First move past non-whitespace characters */
		for (start = fine->cursor; fine->line[start]; ) {
			if (fine->line[start] == ' '
			 || fine->line[start] == '\n'
			 || fine->line[start] == '\t')
				break;
			start = fineline_char_delta(fine->line, start, 1);
		}

		/* Then go past whitespace */
		while (fine->line[start]) {
			if (fine->line[start] != ' '
			 && fine->line[start] != '\n'
			 && fine->line[start] != '\t')
				break;
			start = fineline_char_delta(fine->line, start, 1);
		}
		fine->cursor = start;
		break;

	case FINELINE_LEFT:
	case FINELINE_S_LEFT:
		/* Move cursor left */
		fine->cursor = fineline_char_delta(fine->line, fine->cursor, -1);
		break;

	case FINELINE_RIGHT:
	case FINELINE_S_RIGHT:
		/* Move cursor right */
		fine->cursor = fineline_char_delta(fine->line, fine->cursor, 1);
		break;

	case FINELINE_UP:
	case FINELINE_S_UP:
		/* Move back in history, or up within current input */

		/* First try moving up in the edit buffer */
		lnum = fineline_char_line_number(fine->line, fine->cursor);
		len = fineline_char_line_offset(fine->line, lnum - 1);
		if (len || lnum == 1) {
			/* Yes, just move the cursor */
			col = fineline_char_column_number(fine->line, fine->cursor);
			fine->cursor = fineline_char_at_column(fine->line + len, col, NULL) - fine->line;
		} else {
			/* Otherwise move back in history */
			fineline_history_show(fine, 1);
			fine->cursor = strlen(fine->line);
		}
		break;

	case FINELINE_DOWN:	
	case FINELINE_S_DOWN:
		/* Move forward in history, or down within current input */

		/* First try moving down in the edit buffer */
		lnum = fineline_char_line_number(fine->line, fine->cursor);
		len = fineline_char_line_offset(fine->line, lnum + 1);
		if (len) {
			/* Yes, just move the cursor */
			col = fineline_char_column_number(fine->line, fine->cursor);
			fine->cursor = fineline_char_at_column(fine->line + len, col, NULL) - fine->line;
		} else {
			/* Otherwise move forward in history */
			fineline_history_show(fine, -1);
			fine->cursor = strlen(fine->line);
		}
		break;

	case FINELINE_S_ALL:
		/* Select all text */
		fine->selection = 0;
		fine->cursor = strlen(fine->line);
		break;

	case FINELINE_S_LINE:
		/* Move the cursor and selection endpoint to the start and end
		 * of lines.
		 */
		if (fine->cursor < fine->selection) {
			/* Cursor to the start of its line, selection endpoint
			 * to the end of its line.
			 */
			fine->cursor = fineline_char_line_offset(fine->line, fineline_char_line_number(fine->line, fine->cursor));
			while (fine->line[fine->selection] && fine->line[fine->selection] != '\n')
				fine->selection++;
		} else {
			/* Selection endpoint to the start of its line, cursor
			 * to the end of its line.
			 */
			fine->selection = fineline_char_line_offset(fine->line, fineline_char_line_number(fine->line, fine->selection));
			while (fine->line[fine->cursor] && fine->line[fine->cursor] != '\n')
				fine->cursor++;
		}
		break;

	case FINELINE_ENTER:
		/* If the line is known to be incomplete, then just add a
		 * newline to the input.
		 */
		/*!!!*/

		/* else fall through to save */
	case FINELINE_SAVE:
		/* If we were looking at a history line (without editing it
		 * yet) then make it the current line before we do anything
		 * else.
		 */
		fineline_history_edit(fine);

		/* Save the current line to history */
		fineline_history_add(fine, fine->line);

		/* return the current line */
		if (!fine->runner || (*fine->runner)(fine->line) != 0)
			return 1;
		*fine->line = '\0';
		fine->cursor = 0;
		break;

	case FINELINE_EXIT:
		/* Fail if line is not empty */
		if (*fine->line)
			return -1;

		/* else fall though to quit... */
	case FINELINE_QUIT:
		/* return 2, indicating quit-no-processing */
		return 2;

	case FINELINE_CUT:
		/* Copy text to the cut buffer, and then delete it. */
		copypaste(fine, 1, 1, 0);
		break;

	case FINELINE_COPY:
		/* Copy text to the cut buffer */
		copypaste(fine, 1, 0, 0);
		break;

	case FINELINE_PASTE:
		/* Paste text.  If there's a pending selection, delete it first */
		copypaste(fine, 0, 1, 1);
		break;

	case FINELINE_UNDO:
	case FINELINE_REDO:
		return 1; /* not implemented yet. */

	case FINELINE_SEARCH_G:
		*fine->searchbuf = 0;
		fine->searchprompt = '@';
		break;

	case FINELINE_SEARCH_F:
		*fine->searchbuf = 0;
		fine->searchprompt = '>';
		break;

	case FINELINE_SEARCH_R:
		*fine->searchbuf = 0;
		fine->searchprompt = '<';
		break;

	case FINELINE_NEXT_F:
		/* Search forward */
		if (!*fine->searchbuf)
			return 1; /* error -- can't repeat an empty search */
		if (!fine->line[fine->cursor])
			return 1; /* error -- can't search forward at end */
		moved = strstr(&fine->line[fine->cursor + 1], fine->searchbuf);
		if (!moved)
			return 1; /* error -- not found */
		fine->cursor = (moved - fine->line);
		return 0;

	case FINELINE_NEXT_R:
		/* Reverse search -- this is much trickier than forward search
		 * since we also want to check in history.
		 */
		return 1; /* !!! not implemented yet */

	case FINELINE_BOUNCE:
		/* If in a history line, then remember it and jump back to the
		 * current line.  If in the current line, then jump back to the
		 * remembered history line.
		 */
		fineline_history_bounce(fine);
		return 0;

	/* These cases indicate special conditions */
	case FINELINE_QUOTE:	/* treat next character literally (like text) */
	case FINELINE_PAGE_DOWN:/* scroll forward */
	case FINELINE_PAGE_UP:	/* scroll back */
	case FINELINE_REDRAW:	/* redraw the input from scratch */
	case FINELINE_RESIZE:	/* the window was resized */
		return 0;

	/* These aren't meant to be used */
	case FINELINE_MIN:
	case FINELINE_MAX:
		abort();
	}

	return 0;
}


/* Insert text at the cursor.  "len" is the bytecount of UTF-8 data, not
 * the number of characters.
 */
void fineline_edit_text(fineline_t *fine, const char *text, size_t len)
{
	size_t	buflen;

	/* If doing a search prompt, then append it to the search string */
	if (fine->searchprompt) {
		buflen = strlen(fine->searchbuf);
		if (buflen + len + 1 < sizeof fine->searchbuf) {
			strncpy(fine->searchbuf + buflen, text, len);
			fine->searchbuf[buflen + len] = '\0';
			return;
		}
	}

	/* Start editing the shown line */
	fineline_history_edit(fine);

	/* If a selection is pending, delete it */
	if (fine->selection >= 0) {
		if (fine->selection > fine->cursor)
			memmove(fine->line + fine->cursor, fine->line + fine->selection + 1, strlen(fine->line + fine->selection));
		else {
			memmove(fine->line + fine->selection, fine->line + fine->cursor + 1, strlen(fine->line + fine->cursor));
			fine->cursor = fine->selection;
		}
		fine->selection = -1;
	}

	/* If necessary, enlarge the edit buffer */
	buflen = strlen(fine->line);
	if (buflen + len + 1 > fine->linesize) {
		fine->linesize = ((buflen + len) | 0xff) + 1;
		fine->line = fine->history[0] = realloc(fine->line, fine->linesize);
		fine->style = realloc(fine->style, fine->linesize * sizeof(char *));
	}

	/* Shift the end of the line to make room.  Even if the cursor is at
	 * the end of the line, we still want to shift the '\0' there.
	 */
	memmove(&fine->line[fine->cursor + len], &fine->line[fine->cursor], strlen(&fine->line[fine->cursor]) + 1);

	/* Copy the new text */
	strncpy(&fine->line[fine->cursor], text, len);

	/* Move the cursor to the end of the new text */
	fine->cursor += len;
}

/* Insert a single character at the cursor */
void fineline_edit_char(fineline_t *fine, wchar_t ch)
{
	char buf[MB_CUR_MAX + 1];
	size_t len;

	/* Convert it to a string */
	len = wctomb(buf, ch);

	/* insert it */
	fineline_edit_text(fine, buf, len);
}

/* Convert a control character into an edit command.  If the character isn't
 * a control character, or isn't a known command, then return FINELINE_MIN.
 */
fineline_edit_t fineline_edit_ctrl(wchar_t ch)
{
	static fineline_edit_t cmds[] = {
	    FINELINE_MIN,	/* ^@ */
	    FINELINE_S_ALL,	/* ^A - select all text */
	    FINELINE_BOUNCE,	/* ^B - bounce between history/current line */
	    FINELINE_COPY,	/* ^C - copy selected text */
	    FINELINE_QUIT,	/* ^D - no more lines to enter */
	    FINELINE_MIN,	/* ^E */
	    FINELINE_SEARCH_F,	/* ^F - prompt for forward search */
	    FINELINE_SEARCH_G,	/* ^G - go to line or function */
	    FINELINE_BACK_SPACE,/* ^H - delete character before cursor */
	    FINELINE_TAB,	/* ^I - insert spaces to next tabstop */
	    FINELINE_ENTER,	/* ^J - process line or insert newline */
	    FINELINE_MIN,	/* ^K */
	    FINELINE_REDRAW,	/* ^L - redraw input from scratch */
	    FINELINE_ENTER,	/* ^M - process line or insert newline */
	    FINELINE_NEXT_F,	/* ^N - repeat previous search forward */
	    FINELINE_MIN,	/* ^O */
	    FINELINE_NEXT_R,	/* ^P - repeat previous search backward */
	    FINELINE_EXIT,	/* ^Q - exit file editor without saving */
	    FINELINE_SEARCH_R,	/* ^R - prompt for reverse search */
	    FINELINE_SAVE,	/* ^S - save file and exit file editor */
	    FINELINE_MIN,	/* ^T */
	    FINELINE_BACK_LINE,	/* ^U - erase to start of line/input */
	    FINELINE_PASTE,	/* ^V - paste text from cut buffer */
	    FINELINE_BACK_WORD,	/* ^W - erase to start of word */
	    FINELINE_CUT,	/* ^X - copy selected text, and delete it */
	    FINELINE_REDO,	/* ^Y - redo */
	    FINELINE_UNDO,	/* ^Z - undo */
	    FINELINE_MIN,	/* ^[ <Esc> */
	    FINELINE_QUOTE,	/* ^\ - quote the next character */
	    FINELINE_MIN,	/* ^] */
	    FINELINE_MIN,	/* ^^ <Ctrl-Shift-6> */
	    FINELINE_S_LINE 	/* ^_ <Ctrl-Shift-Minus> - select whole lines */
	};

	/* Range check */
	if ((ch & 0xfffff) >= ' ')
		return FINELINE_MIN;

	/* Look it up */
	return cmds[ch & 0xfffff];
}
