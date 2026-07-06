# fineline

The fineline library reads an input line from the user.
It fills the same role as the GNU ReadLine library,
but it has a more modern feel, and is easier to work with.
Its biggest features are:

* /Name completion/, similar to ReadLine but easier to implement.
* /Simple TTY or ncursesw-based full-screen/ API.
  This is the main reason I wrote fineline.
  I needed this ability for my JSON project, and no other ReadLine-like
  library can do it.
* /Multi-line inputs/. I implemented this mostly so I could use
  multi-line examples in my JSON project.
* /Syntax coloring/ as you type.
* /Hinting/ - suggestions of what comes next, such as the parameters of a function.
* /Full UTF-8/ input, including double-wide characters.
  (Assuming your terminal supports it, of course.)
* /Chat/, displays asynchronous text from another source such as AI.
  Code embedded in the AI's response can be inserted into the input.
* Uses common editing commands, such as \<Ctrl-V> to paste.  See below.

The fact that it supports multi-line input makes it easy to implement a
minimalist file editor.
An example of a stand-alone program, "fled", is included with the library.
As with command-line entry, this will only take up as many rows of
the screen as needed to display the text (the contents of the file).
"History" doesn't make sense for a file editor, though it does still
support undo/redo and all of the other editing commands.
You can also easily implement a similar editor in your own ncursesw-based
full-screen app.

The editing commands are:
| Keys | Action |
|:----:|:-------|
| \<Left> and \<Right> | Move the cursor by one character. |
| \<Ctrl-Left> and \<Ctrl-Right> | Move the cursor by one word. |
| \<Up> and \<Down> | Move within a multi-line input, or move through history. |
| \<Ctrl-Up> and \<Ctrl-Down> | Move through history. |
| \<Home> and \<End> | Move to start/end of line, or if already there then move to the start/end of the entire multi-line input. |
| \<Shift> with any of the above | Select text for cut/copy/paste. |
| \<Ctrl-A> | Select all text |
| \<Ctrl-X> | Copy the selected text to the paste buffer, and delete it. |
| \<Ctrl-C> | Copy the selected text to the paste buffer but don't delete it. |
| \<Ctrl-V> | Paste the text.  If other text is selected, swap them. |
| \<Ctrl-Z> | Undo. |
| \<Ctrl-Y> | Redo. |
| \<Ctrl-F> | Find forward.  Prompts for text to search for within edit buffer. |
| \<Ctrl-F> | Find backward.  Prompts for text to search for within edit buffer. |
| \<Ctrl-N> | Find next. Repeats previous search in forward direction. |
| \<Ctrl-P> | Find previous. Repeats previous search in backward direction. |
| \<Ctrl-B> | Bounce between a history line and the new input line. |
| \<Ctrl-L> | Redraw the line from scratch. |
| \<Tab> | Perform name completion, or indent to next tabstop. |
| \<Shift-Tab> | Delete whitespace to previous tabstop. |
| \<Insert> | Toggle between insert and replace modes. |
| \<Delete> | Delete the character at the cursor. | 
| \<Backspace> | Delete the character before the cursor, and move left. |
| \<Ctrl-W> | Delete the previous word. |
| \<Ctrl-U> | Delete to the start of the line, or start of multi-line input. |
| \<Ctrl-\> | Insert the next keystroke as text, even if it's a command key |
| \<Ctrl-E> | Force AI chat to end its current response. |
| \<PgUp> and \<PgDn> | Scroll within a large mult-line inputs. |
| \<Enter> | Submit the line, or insert a newline. |
| \<Ctrl-D> | Exit (not for file editor) |
| \<Ctrl-S> | Save the input, then quit (for file editor) |
| \<Ctrl-Q> | Quit without saving (for file editor) |

In practice, this feels normal:
You type in a command line, hit \<Enter>, and the command is processed/executed.
No surprises.
The fancier edit commands are useful if you want to pull parts of previous
lines from the history, and assemble them to form a new command line.

To integrate this into your program, you have some options:
1) For a non-full-screen program such as a shell, you can call
   `fineline_tty(NULL, "prompt:")`.
   This is similar to GNU ReadLine() - it returns a dynamically allocated
   string,  or NULL if the user entered \<Ctrl-D>.
   You can also use `fineline_tty_alloc()`, set some of the fields in the
   returned `fineline_t`, and pass that as the first argument to
   `fineline_tty(fine, "Prompt:")`.
2) For a full-screen program using -lncursesw, you can use
   `#define FINELINE_CURSES` before the `#include <fineline.h>`
   in one of your source files.
   This will define the `fineline_curses(NULL, "prompt:")` and
   `fineline_curses_alloc()` functions, which resemble the "tty" functions.
   There's also a `wfineline_curses(fine, "prompt:") function, for doing
   the I/O in an ncursesw window instead of `stdscr`.
   (These functions are implemented in the `fineline.h` header instead of
   the shared library to avoid library dependencies.)
3) For other types of apps, such as a GUI, you need to implement more code.
   `fineline_alloc()` will create a generic instance of fineline().
   You must then add pointers to your own output functions.
   You also need to implement an event handling loop that calls
   `fineline_edit()` or `fineline_edit_text()` as appropriate.
   Lastly, you can define a line-processing callback function, instead of
   detecting when fineline_edit() returns a positive number to indicate that
   the user has hit \<Enter>.

You are also responsible for implementing functions to support syntax coloring,
hinting, and A.I. chat if you want those features.
The syntax coloring function is also responsible for determining whether an
\<Enter> keystroke marks the end of input, or is merely a line break in a
multi-line entry.
(In my JSON project, syntax coloring and hinting are implemented in the "line"
plugin, and A.I. chat is implemented in the "ai" plugin.)
