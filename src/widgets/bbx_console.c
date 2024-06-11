#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include "BabyX.h"

#define CONM_ADDTEXT 0x8001
#define CONM_SETCOL  0x8002

typedef struct
{
	int topline;     // top line selected
	int topcolumn;   // top column selected
	int botline;     // bottom line selected
	int botcolumn;   // bottom column selected
	int topfixed;    // is the top the fixed point of the selection?
	int active;      // is the selection active?
	int mousegrabbed;// is the mouse grabbed
} SELECTION;

typedef struct colourchange
{
	int xpos;
	BBX_RGBA col;
	struct colourchange *next;
} COLOURCHANGE;

typedef struct
{
	char *text;
	COLOURCHANGE *colourchange;
	BBX_RGBA startcol;
	int ret;
} LINE;

typedef struct
{
	BABYX *bbx;
	BBX_Panel *pan;
	BBX_Canvas *can;
	BBX_Scrollbar *sb;
	struct bitmap_font *font;
	LINE *lines;
	int capacity;
	int nlines;
	int pos;
	int cursor;
	BBX_RGBA colour;
	int width;
	int height;
	int fntwidth;
	int fntheight;
	SELECTION selection;
	int onmodal; 
	int cursorblink;
	char *input_buff;
} TEXT;

static BBX_DialogPanel *hconsole = 0;

//static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void layout(void *ptr, int width, int height);
static void keyin(void *ptr, int ch);
static void ticktick(void * ptr);


static TEXT *createtext(int capacity);
static void killtext(TEXT *text);
static void killcolchange(COLOURCHANGE *col);
static void redraw(TEXT *text);
static void paintselection(unsigned char *rgba, int width, int height, TEXT *text);
static int lineselected(TEXT *text, int i);
static void getlineselection(TEXT *text, int i, int *start, int *end);
static void addwordwrap(TEXT *text, char *str);
static void addtext(TEXT *text, char *str);
static void backspace(TEXT *text);

static void scrollaline(TEXT *text);
static void scrollme(void *ptr, int pos);
static void DoScrollBar(TEXT *text);
static void MouseMove(void *text, int action, int x, int y, int buttons);
static void StartSelection(TEXT *text, int x, int y);
static void SelectText(TEXT *text, int x, int y);
static void CopyToClipboard(TEXT *text);
static char *getselection(TEXT *text);
static void xytolinecol(TEXT *text, int x, int y, int *line, int *col);
static void changecolour(TEXT *text, BBX_RGBA col);
static BBX_RGBA currentcolour(TEXT *text);

static char **strwrap(char *str, int x, int width, int *N);
static void killlist(char **list, int N);
static void getfontmetrics(TEXT *text, struct bitmap_font *font);
static int firstwordlen(char *str);
static char *xgetline(char *str);
static char *cat(char *str, char *str2);
static int utf8_choplast(char *str);
static char *mystrdup(char *str);



void bbx_console_printf(BBX_Panel *obj, char *fmt, ...)
{
	TEXT *text;
	char buff[1024];
	va_list args;

	assert(!strcmp(bbx_panel_gettag(obj), "console"));
	text = bbx_panel_getptr(obj);
	va_start(args, fmt);
	vsprintf(buff, fmt, args);
	addtext(text, buff);
	DoScrollBar(text);
	redraw(text);
	va_end(args);
}

BBX_RGBA bbx_console_setcon(BBX_Panel *obj, BBX_RGBA col)
{
	assert(!strcmp(bbx_panel_gettag(obj), "console"));
}

BBX_Panel *bbx_console(BABYX *bbx, BBX_Panel *parent)
{
	TEXT *text;

	text = createtext(1024);
	text->bbx = bbx;
	text->pan = bbx_panel(bbx, parent, "console", layout, text);
	
	text->can = bbx_canvas(bbx, text->pan, 10, 10, bbx_color("black"));
	text->can = 0;
	text->sb = bbx_scrollbar(bbx, text->pan, BBX_SCROLLBAR_VERTICAL, scrollme, text);
	text->font = bbx->user_font2;

	text->onmodal = 0;
	text->cursorblink = 0;
	bbx_panel_setmousefunc(text->pan, MouseMove, text);
	bbx_panel_setkeyfunc(text->pan, keyin, text);
	
	return text->pan;
}

void bbx_console_kill(BBX_Panel *obj)
{
	TEXT *text;

	if (obj)
	{
		text = bbx_panel_getptr(obj);
		bbx_canvas_kill(text->can);
		bbx_scrollbar_kill(text->sb);
		bbx_panel_kill(text->pan);
		killtext(text);
	}
}

char *bbx_console_getline(BBX_Panel *obj)
{
	TEXT *text = bbx_panel_getptr(obj);
	void *ticker;

	ticker = bbx_addticker(text->bbx, 550, ticktick, text);
	text->input_buff = 0;
	
	text->onmodal = 1;
	text->cursorblink = 1;
	BBX_MakeModal(obj->bbx, obj->win);
	bbx_removeticker(text->bbx, ticker);

	text->onmodal = 0;
	return text->input_buff;
}

static void layout(void *ptr, int width, int height)
{
	TEXT *text = ptr;

	bbx_canvas_kill(text->can);
	text->can = bbx_canvas(text->bbx, text->pan, width - 15, height, bbx_rgba(0, 0, 0, 255));
	//bbx_canvas_setmousefunc(text->can, MouseMove, text);
	bbx_setpos(text->bbx, text->can, 0, 0, width - 15, height);
	bbx_setpos(text->bbx, text->sb, width - 15, 0, 15, height);
	DoScrollBar(text);
	getfontmetrics(text, text->font);
	redraw(text);
}

static void keyin(void *ptr, int ch)
{
	TEXT *text = ptr;
	char buff[32];
	int i;

	if (ch == 3)
	{
		char *selection = getselection(text);
		if (!selection)
			return;
		bbx_copytexttoclipboard(text->bbx, selection);
		free(selection);
		return;
	}

	if (!text->onmodal)
		return;

	if (ch == BBX_KEY_BACKSPACE)
	{
		if (text->input_buff && strlen(text->input_buff))
		{
			utf8_choplast(text->input_buff);
			ch = '\b';
			backspace(text);
			redraw(text);
			return;
		}
	}
	if (ch < 0)
		return;
	i = bbx_utf8_putch(buff, ch);
	buff[i] = 0;
	text->input_buff = cat(text->input_buff, buff);

	addtext(text, buff);
	DoScrollBar(text);
	redraw(text);
	if (ch == '\n')
		BBX_DropModal(text->bbx);
}

static void ticktick(void * ptr)
{
	TEXT *text = ptr;

	text->cursorblink ^= 1;
	redraw(text);
}
static TEXT *createtext(int capacity)
{
	TEXT *answer;
	int i;

	answer = bbx_malloc(sizeof(TEXT));
	answer->lines = bbx_malloc(capacity * sizeof(LINE));
	for (i = 0; i<capacity; i++)
	{
		answer->lines[i].text = 0;
		answer->lines[i].colourchange = 0;
		answer->lines[i].startcol = bbx_rgba(0,255,0,255);
		answer->lines[i].ret = 0;
	}
	answer->capacity = capacity;
	answer->nlines = 0;
	answer->pos = 0;
	answer->cursor = 0;
	answer->colour = bbx_rgba(0,255,0,255);

	answer->width = 0;
	answer->height = 0;

	answer->selection.active = 0;
	answer->selection.mousegrabbed = 0;

	return answer;
}

static void killtext(TEXT *text)
{
	int i;

	for (i = 0; i<text->nlines; i++)
	{
		if (text->lines[i].colourchange)
			killcolchange(text->lines[i].colourchange);
		if (text->lines[i].text)
			free(text->lines[i].text);
	}
	free(text->lines);
	free(text);
}

static void killcolchange(COLOURCHANGE *col)
{
	if (col->next)
		killcolchange(col->next);
	free(col);
}


static void redraw(TEXT *text)
{
	int i;
	COLOURCHANGE *cc;
	char *substr;
	int len;
	unsigned char *rgba;
	int width, height;

	rgba = bbx_canvas_rgba(text->can, &width, &height);
	
	bbx_rectangle(rgba, width, height, 0, 0, width, height, bbx_rgba(0,0,0,255));

	for (i = text->pos; i<text->pos + text->height; i++)
	{
		if (text->lines[i].text)
		{
			if (!text->lines[i].colourchange)
			{
				bbx_drawutf8(rgba, width, height, 0, (i - text->pos) * text->fntheight + text->font->ascent,
					text->lines[i].text, bbx_utf8_Nchars(text->lines[i].text),
					text->font, text->lines[i].startcol);
			}
			/*
			else
			{
			//	 line has colour changes 
				TextOut(hdc, 0, (i - text->pos) *text->fntheight, text->lines[i].text, text->lines[i].colourchange->xpos);
				cc = text->lines[i].colourchange;
				while (cc)
				{
					SetTextColor(hdc, cc->col);
					substr = text->lines[i].text + cc->xpos;
					if (cc->next)
						len = cc->next->xpos - cc->xpos;
					else
						len = strlen(text->lines[i].text) - cc->xpos;
					TextOut(hdc, text->fntwidth * cc->xpos, (i - text->pos) *text->fntheight, substr, len);
					cc = cc->next;
				}
			}
		*/
		}
	}
	if (text->cursorblink)
	{
		char *lastline = text->lines[text->nlines].text;
		int xpos = lastline ?  bbx_textwidth(text->font, lastline, bbx_utf8_Nchars(lastline)) : 0;
		int ypos = (text->nlines - text->pos) * text->fntheight;
		bbx_rectangle(rgba, width, height, xpos, ypos, xpos + text->fntwidth, ypos + text->fntheight, bbx_rgba(0, 255, 0, 255));
	}

	if (text->selection.active)
	{
		paintselection(rgba, width, height, text);
	}

	bbx_canvas_flush(text->can);
}


static void paintselection(unsigned char *rgba, int width, int height, TEXT *text)
{
	int i;
	int start;
	int end;
	char *str;
	char *ptr;
	int x;
	int y;
	int len;
	int Nch;
	BBX_RGBA blue = bbx_rgba(0, 0, 255, 255);
	BBX_RGBA white = bbx_rgba(255, 255, 255, 255);


	for (i = text->pos; i<text->pos + text->height; i++)
	{
		if (text->lines[i].text)
		{
			if (lineselected(text, i))
			{
				getlineselection(text, i, &start, &end);
				str = text->lines[i].text + start;
				Nch = 0;
				for (ptr = str; ptr < str + end; ptr += bbx_utf8_skip(ptr))
					Nch++;
				x = start * text->fntwidth;
				y = (i - text->pos) * text->fntheight;
				len = bbx_utf8width(text->font, str, Nch);
				bbx_rectangle(rgba, width, height, x, y, x + len, y + text->fntheight, blue);
				bbx_drawutf8(rgba, width, height, x, y + text->font->ascent, str, Nch, text->font, white);
				//TextOut(hdc, x, y, str, end - start);
			}
		}
	}
}


static int lineselected(TEXT *text, int i)
{
	if (i >= text->selection.topline && i <= text->selection.botline)
		return 1;
	else
		return 0;
}

static void getlineselection(TEXT *text, int i, int *start, int *end)
{
	if (text->selection.topline == i)
		*start = text->selection.topcolumn;
	else
		*start = 0;

	if (text->selection.botline == i)
		*end = text->selection.botcolumn + 1;
	else
		*end = strlen(text->lines[i].text);
}

static void addwordwrap(TEXT *text, char *str)
{
	char *block;
	char **lines;
	int N;
	int i;

	while (*str)
	{
		block = xgetline(str);
		lines = strwrap(block, text->cursor, text->width - 1, &N);
		for (i = 0; i<N; i++)
		{
			addtext(text, lines[i]);
		}
		str += strlen(block);
		if (*str == '\n')
		{
			addtext(text, "\n");
			text->lines[text->nlines - 1].ret = 1;
			str++;
		}
		killlist(lines, N);
		free(block);
	}

	DoScrollBar(text);
	
	redraw(text);
}

static void addtext(TEXT *text, char *str)
{
	/* see if a colourchange is in effect */
	if (*str && *str != '\n' && text->cursor)
	{
		if (currentcolour(text) != text->colour)
			changecolour(text, text->colour);
	}
	while (*str)
	{
		if (text->cursor == 0)
		{
			text->lines[text->nlines].text = malloc(text->width + 1);
			text->lines[text->nlines].startcol = text->colour;
			memset(text->lines[text->nlines].text, 0, text->width + 1);
		}
		if (*str == '\b')
		{
			backspace(text);
			str++;
			continue;
		}
		else if (*str != '\n')
		{
			text->lines[text->nlines].text[text->cursor++] = *str++;
			if (text->cursor == text->width)
				text->cursor = 0;
		}
		else
		{
			str++;
			text->cursor = 0;
			text->lines[text->nlines].ret = 1;
		}
		if (text->cursor == 0)
		{
			if (text->nlines == text->capacity - 1)
				scrollaline(text);
			else
				text->nlines++;
		}
	}
	text->pos = text->nlines - text->height +1;
	if (text->pos < 0)
		text->pos = 0;
}

static void backspace(TEXT *text)
{
	if (text->cursor > 0)
	{
		utf8_choplast(text->lines[text->nlines].text);
		text->cursor = strlen(text->lines[text->nlines].text);
	}
	else
	{
		if (text->nlines > 0)
		{
			text->nlines--;
			text->cursor = strlen(text->lines[text->nlines].text);
			if (text->cursor > 0)
			{
				utf8_choplast(text->lines[text->nlines].text);
				text->cursor = strlen(text->lines[text->nlines].text);
			}
			if (text->pos > 0)
				text->pos--;
		}
	}
}

static void scrollaline(TEXT *text)
{
	if (text->lines[0].text)
		free(text->lines[0].text);
	if (text->lines[0].colourchange)
		killcolchange(text->lines[0].colourchange);

	memmove(&text->lines[0], &text->lines[1], sizeof(LINE) *(text->capacity - 1));
	text->lines[text->capacity - 1].text = 0;
	text->lines[text->capacity - 1].colourchange = 0;
	text->lines[text->capacity - 1].startcol = text->colour;
	text->lines[text->capacity - 1].ret = 0;

	text->selection.topline -= 1;
	text->selection.botline -= 1;
	if (text->selection.topline < 0)
		text->selection.topline = 0;
	if (text->selection.topline > text->selection.botline || (text->selection.topline == text->selection.botline && text->selection.topcolumn > text->selection.botcolumn))
		text->selection.active = 0;
}

static void scrollme(void *ptr, int pos)
{
	TEXT *text = ptr;

	text->pos = pos;

	if (text->pos > text->nlines + 1 - text->height)
		text->pos = text->nlines + 1- text->height;
	if (text->pos < 0)
		text->pos = 0;

	redraw(text);
}

static void DoScrollBar(TEXT *text)
{
	bbx_scrollbar_set(text->sb, text->nlines +1, text->height, text->pos);
}

static void MouseMove(void *obj, int action, int x, int y, int buttons)
{
	TEXT *text = obj;
	int width, height;

	if (!(buttons & BBX_MOUSE_BUTTON1) )
		return;

	if (action == BBX_MOUSE_CLICK)
	{
		StartSelection(text, x, y);
		return;
	}

	bbx_getsize(text->bbx, text->can, &width, &height);

	if (y < 0)
	{
		if (text->pos)
			text->pos--;
		DoScrollBar(text);
	
	//	SetCursorPos(pt.x, pt.y);
	}
	else if (y > height)
	{
		if (text->pos < text->nlines - text->height)
			text->pos++;
		DoScrollBar(text);
	
	//	SetCursorPos(pt.x, pt.y);
	}

	SelectText(text, x, y);

	redraw(text);
}

static void StartSelection(TEXT *text, int x, int y)
{
	if (text->selection.active)
	{
		text->selection.active = 0;
		redraw(text);
	}
	text->selection.topfixed = 1;
	xytolinecol(text, x, y, &text->selection.topline, &text->selection.topcolumn);
	if (y == -1)
		return;
	text->selection.mousegrabbed = 1;
}

static void SelectText(TEXT *text, int x, int y)
{
	int line;
	int col;
	int temp;

	xytolinecol(text, x, y, &line, &col);
	if (y == -1)
		return;

	if (text->selection.topfixed)
	{
		if (line < text->selection.topline || (line == text->selection.topline && col < text->selection.topcolumn))
		{
			temp = text->selection.topline;
			text->selection.topline = line;
			text->selection.botline = temp;

			temp = text->selection.topcolumn;
			text->selection.topcolumn = col;
			text->selection.botcolumn = temp;

			text->selection.topfixed = 0;
		}
		else
		{
			text->selection.botline = line;
			text->selection.botcolumn = col;
		}
	}
	else
	{
		if (line > text->selection.botline || (line == text->selection.botline && col > text->selection.botcolumn))
		{
			temp = text->selection.botline;
			text->selection.botline = line;
			text->selection.topline = temp;

			temp = text->selection.botcolumn;
			text->selection.botcolumn = col;
			text->selection.topcolumn = temp;

			text->selection.topfixed = 1;
		}
		else
		{
			text->selection.topline = line;
			text->selection.topcolumn = col;
		}
	}

	if (text->selection.topline != text->selection.botline || text->selection.topcolumn != text->selection.botcolumn)
		text->selection.active = 1;
	else
		text->selection.active = 0;
}

static char *getselection(TEXT *text)
{
	char *ptr;
	int i;
	int start;
	int end;
	int buffsize;
	char *answer;

 	if (!text->selection.active)
		return 0;
	
	buffsize = (text->selection.botline - text->selection.topline + 1) * (text->width + 1) +1;
	answer = bbx_malloc(buffsize);
	ptr = answer;


	for (i = text->selection.topline; i <= text->selection.botline; i++)
	{
		if (!text->lines[i].text)
			continue;
		if (text->lines[i].text[0] == 0)
			continue;

		if (i == text->selection.topline)
			start = text->selection.topcolumn;
		else
			start = 0;
		if (i == text->selection.botline)
			end = text->selection.botcolumn + 1;
		else
			end = strlen(text->lines[i].text);
		strncpy(ptr, text->lines[i].text + start, end - start);
		ptr += end - start;
		*ptr = 0;
		if (!text->lines[i].ret)
			strcat(ptr, " ");
		else
			strcat(ptr, "\n");
		ptr++;
	}
	*ptr = 0;

	return answer;
}

static void xytolinecol(TEXT *text, int x, int y, int *line, int *col)
{
	if (text->nlines == 0 && text->cursor == 0)
	{
		*line = -1;
		*col = -1;
		return;
	}

	*line = y / text->fntheight + text->pos;
	*col = x / text->fntwidth;

	if (*line > text->nlines)
		*line = text->nlines;
	if (*line < 0)
		*line = 0;

	if (*col < 0)
		*col = 0;
	if (text->lines[*line].text)
	{
		if (*col >= (int)strlen(text->lines[*line].text))
			*col = (int)strlen(text->lines[*line].text) - 1;
	}
	else
		*col = 0;

	assert(*line <= text->nlines);
	assert(*col < text->width);
}


static void changecolour(TEXT *text, BBX_RGBA col)
{
	COLOURCHANGE *cc;
	COLOURCHANGE *newcol;

	newcol = malloc(sizeof(COLOURCHANGE));
	newcol->col = col;
	newcol->next = 0;
	newcol->xpos = text->cursor;
	cc = text->lines[text->nlines].colourchange;
	if (cc)
	{
		while (cc->next)
			cc = cc->next;
		cc->next = newcol;
	}
	else
		text->lines[text->nlines].colourchange = newcol;
}

/*
Gets the currently set colour at the end of the line
Params: text - text structure
Returns: the colour current.
*/
static BBX_RGBA currentcolour(TEXT *text)
{
	COLOURCHANGE *cc;

	if (text->lines[text->nlines].colourchange)
	{
		cc = text->lines[text->nlines].colourchange;
		while (cc->next)
			cc = cc->next;
		return cc->col;
	}

	else
		return text->lines[text->nlines].startcol;
}

static char **strwrap(char *str, int x, int width, int *N)
{
	char **answer = 0;
	int nlines = 0;
	char *last;
	int i;

	assert(str);

	answer = malloc(sizeof(char *));
	if (strlen(str) <= (unsigned)width - x)
	{
		answer[0] = mystrdup(str);
		*N = 1;
		return answer;
	}

	last = str + width - x;
	while (*last != ' ' && last > str)
		last--;
	if (last == str && firstwordlen(str) >= width)
		last = str + width - x;
	answer[0] = malloc(last - str + 2);
	memcpy(answer[0], str, last - str);
	answer[0][last - str] = 0;
	nlines = 1;
	str = last;
	if (isspace(*str))
		str++;

	while (strlen(str) > (unsigned)width)
	{
		nlines++;
		answer = realloc(answer, nlines * sizeof(char *));
		last = str + width;
		while (*last != ' ' && last > str)
			last--;
		if (last == str)
			last = str + width;
		answer[nlines - 1] = malloc(last - str + 2);
		memcpy(answer[nlines - 1], str, last - str);
		answer[nlines - 1][last - str] = 0;

		str = last;
		if (isspace(*str))
			str++;
	}
	if (*str)
	{
		nlines++;
		answer = realloc(answer, nlines * sizeof(char *));
		answer[nlines - 1] = mystrdup(str);
	}

	*N = nlines;

	for (i = 0; i<nlines - 1; i++)
		strcat(answer[i], "\n");

	return answer;
}

static void killlist(char **list, int N)
{
	int i;

	for (i = 0; i<N; i++)
		free(list[i]);

	free(list);
}

static void getfontmetrics(TEXT *text, struct bitmap_font *font)
{
	int width, height;

	bbx_getsize(text->bbx, text->can, &width, &height);

	text->width = (width) / font->width;
	text->height = (height) / font->height;
	text->fntwidth = font->width;
	text->fntheight = font->height;
}

static int firstwordlen(char *str)
{
	int answer = 0;

	while (*str && !isspace(*str))
	{
		str++;
		answer++;
	}

	return answer;
}

static char *xgetline(char *str)
{
	char *ptr;
	char *answer;

	ptr = str;

	while (*ptr && *ptr != '\n')
		ptr++;

	answer = malloc(ptr - str + 1);
	memcpy(answer, str, ptr - str);
	answer[ptr - str] = 0;

	return answer;
}


static char *cat(char *str, char *str2)
{
	if (!str)
		return mystrdup(str2);
	
	str = bbx_realloc(str, strlen(str) + strlen(str2) + 1);
	strcat(str, str2);
	return str;
}

static int utf8_choplast(char *str)
{
	int len = 0;
	int cut;

	while (str[len])
		len++;
	cut = 1;
	while (cut <= len && cut < 8)
	{
		if (bbx_isutf8z(str + len - cut))
		{
			str[len - cut] = 0;
			return 0;
		}
		else
			cut++;
	}
	return -1;
}

static char *mystrdup(char *str)
{
	char *answer;

	answer = malloc(strlen(str) + 1);
	strcpy(answer, str);
	return answer;
}
