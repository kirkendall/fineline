# fineline

The fineline library reads an input line from the user.
It fills the same role as the GNU ReadLine library,
but it has a more modern feel, and is easier to work with.
Its biggest features are:

* **Name completion**, similar to ReadLine but easier to implement.
* **Simple TTY or ncursesw-based full-screen** API.
  This is the main reason I wrote fineline.
  I needed this ability for my "edj" JSON project, and no other ReadLine-like
  library can do it.
* **Multi-line inputs**. I implemented this mostly so I could use
  multi-line examples in my JSON project.
* **Syntax coloring** as you type.
* **Hinting** - suggestions of what comes next, such as the parameters of a function.
* **Full UTF-8** input, including double-wide characters.
  (Assuming your terminal supports it, of course.)
* **Chat**, displays asynchronous text from another source such as AI.
  Code embedded in the AI's response can be inserted into the input.
* Uses **common editing commands**, such as \<Ctrl-V> to paste.  See below.

An extremely minimalist file editor is included.
Since *fineline* can handle multi-line entries, this was almost free.
As with command-line entry, this will only take up as many rows of
the screen as needed to display the text (the contents of the file).
"History" doesn't make sense for a file editor, though it does still
support undo/redo and all of the other editing commands.
You can also easily implement a similar editor in your own app.

The editing commands are:
| Keys | Action |
|:----:|:-------|
| \<Left> and \<Right> | Move the cursor by one character. |
| \<Ctrl-Left> and \<Ctrl-Right> | Move the cursor by one word. |
| \<Up> and \<Down> | Move within a multi-line input, or move through history. |
| \<Ctrl-Up> and \<Ctrl-Down> | Move through history. |
| \<Home> and \<End> | Move to start/end of line, or if already there then move to the start/end of the entire multi-line input. |
| \<Shift> with any of the above | Select text for cut/copy/paste. |
| \<Ctrl-Home> and \<Ctrl-End> | Select text like \<Shift-Home> and \<Shift-End> for *gnome-terminal* and *xfce4-terminal* |
| \<Backspace> | Delete the character before the cursor, and move left. |
| \<Delete> | Delete the character at the cursor. | 
| \<Enter> | Submit the line, or insert a newline. |
| \<Insert> | Toggle between insert and replace modes. |
| \<PgUp> and \<PgDn> | Scroll within a large mult-line inputs. |
| \<Shift-Tab> | Delete whitespace to previous tabstop. |
| \<Tab> | Perform name completion, or indent to next tabstop. |
| \<Esc> | Save the line in history but don't process it. |
| \<Ctrl-@> | (unassigned, often \<Ctrl-Shift-2>) |
| \<Ctrl-A> | Select all text |
| \<Ctrl-B> | Bounce between a history line and the new input line. |
| \<Ctrl-C> | Copy the selected text to the paste buffer but don't delete it. |
| \<Ctrl-D> | Exit (not for file editor) |
| \<Ctrl-E> | Invoke an external editor on the input. |
| \<Ctrl-F> | Find forward.  Prompts for text to search for within edit buffer. |
| \<Ctrl-G> | Goto a given line number or symbol definition. |
| \<Ctrl-I> | (same as \<Tab>) |
| \<Ctrl-J> | (same as \<Enter>) |
| \<Ctrl-K> | Configure fineline's options. |
| \<Ctrl-L> | Redraw the line from scratch. |
| \<Ctrl-M> | (same as \<Enter>) |
| \<Ctrl-N> | Find next. Repeats previous search in forward direction. |
| \<Ctrl-O> | (reserved for overlapping window commands) |
| \<Ctrl-P> | Find previous. Repeats previous search in backward direction. |
| \<Ctrl-Q> | Quit without saving (for file editor) |
| \<Ctrl-R> | Find backward.  Prompts to search for within edit buffer or history. |
| \<Ctrl-S> | Save the input, then quit (for file editor) |
| \<Ctrl-T> | (reserved for tiling window commands) |
| \<Ctrl-U> | Delete to the start of the line, or start of multi-line input. |
| \<Ctrl-V> | Paste the text.  If other text is selected, swap them. |
| \<Ctrl-W> | Delete the previous word. |
| \<Ctrl-X> | Copy the selected text to the paste buffer, and delete it. |
| \<Ctrl-Y> | Redo. |
| \<Ctrl-Z> | Undo. |
| \<Ctrl-[> | (same as \<Esc>) |
| \<Ctrl-\\> | Insert the next keystroke as text, even if it's a command key |
| \<Ctrl-]> | Maybe perform a tag search, as in vi? |
| \<Ctrl-^> | (unassigned, often \<Ctrl-Shift-6>) |
| \<Ctrl-_> | Expand the selection to whole lines (often \<Ctrl-Shift-Minus>) |

In practice, this feels normal:
You type in a command line, hit \<Enter>, and the command is processed/executed.
No surprises.
The fancier edit commands are useful if you want to pull parts of previous
lines from the history, and assemble them to form a new command line.

Here's a simple program using fineline:

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

To access the fancier features such as name completion, or to embed *fineline*
input into a full-screen program using *ncursesw*, there are some other
functions to call.  Generally, you'll want to allocate a `fineline_t` data
structure and initialize some of its members to point to callback functions
and the like.  The documentation shows the full details.
