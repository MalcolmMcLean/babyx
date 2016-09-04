/*
  Baby X font picker dialog

  This version Windows only (you just need to change the font path enumeration for Linux)

*/

#include <sys/types.h>
#include <sys/stat.h>
#include "BabyX.h"

#include "loadttffont.h"

struct bitmap_font *loadttffont(char *fname, int points, char *letters);
static long filelength(char *path);

typedef struct
{
	BABYX *bbx;
	BBX_Panel *pan;
	BBX_Button *ok_but;
	BBX_Button *cancel_but;
	BBX_Panel *fontbox;
	BBX_Spinner *points_spn;
	BBX_Label *points_lab;
	BBX_LineEdit *sample_edt;
	struct bitmap_font *chosen_font;
	char **names;
	struct bitmap_font **fonts;
	int selected;
	int positive;
} FONTPICKER;

static void fontpickerlayout(void *obj, int width, int height);
static void fontpickerpresscancel(void *obj);
static void fontpickerpressok(void *obj);
static void fontselected(void *ptr, int index);
static void spinme(void *ptr, double value);

BBX_Panel *bbx_fontbox(BABYX *bbx, BBX_Panel *parent, struct bitmap_font **fonts, char **names, int N,
	void (*selection)(void *ptr, int i), void *ptr);
void bbx_fontbox_kill(BBX_Panel *obj);


static char **commonfonts(void);
static char **enumfonts(void);
static struct bitmap_font *loadsamplefont(char *name, char **nameout);
static struct bitmap_font *loadfullfont(char *name, int points);

static void font_kill(struct bitmap_font *font);


struct bitmap_font *pickfont(BABYX *bbx)
{
	FONTPICKER op;
	struct bitmap_font *answer = 0;
	char **facenames;
	int N;
	int i;

	op.bbx = bbx;
	op.pan = bbx_dialogpanel(bbx, "choose font", 400, 500, fontpickerlayout, &op);
	bbx_dialogpanel_setclosefunc(op.pan, fontpickerpresscancel, &op);

	op.points_spn = bbx_spinner(bbx, op.pan, 12, 4, 100, 1.0, spinme, &op);
	op.points_lab = bbx_label(bbx, op.pan, "Points");
	op.ok_but = bbx_button(bbx, op.pan, "OK", fontpickerpressok, &op);
	op.cancel_but = bbx_button(bbx, op.pan, "Cancel", fontpickerpresscancel, &op);
	op.chosen_font = 0;
	op.selected = -1;
	op.names = enumfonts();
	for (i = 0; op.names[i]; i++)
		;
	N = i;
	facenames = malloc(N * sizeof(char *));
	op.fonts = bbx_malloc(N * sizeof(struct bitmap_font *));
	for (i = 0; i < N; i++)
		op.fonts[i] = loadsamplefont(op.names[i], &facenames[i]);


	op.fontbox = bbx_fontbox(bbx, op.pan, op.fonts, facenames, N, fontselected, &op);
	op.sample_edt = bbx_lineedit(bbx, op.pan, "Sample text", 0, 0);


	bbx_dialogpanel_makemodal(op.pan);

	if (op.positive)
		answer = op.chosen_font;
	else
	{
		font_kill(op.chosen_font);
		op.chosen_font = 0;
	}

	for (i = 0; i < N; i++)
	{
		free(facenames[i]);
		free(op.names[i]);
		font_kill(op.fonts[i]);
	}
	free(facenames);
	free(op.names);
	free(op.fonts);
	bbx_label_kill(op.points_lab);
	bbx_spinner_kill(op.points_spn);
	bbx_fontbox_kill(op.fontbox);
	bbx_lineedit_kill(op.sample_edt);
	bbx_button_kill(op.ok_but);
	bbx_button_kill(op.cancel_but);
	bbx_dialogpanel_kill(op.pan);

	return answer;
}

static void fontpickerlayout(void *obj, int width, int height)
{
	FONTPICKER *fp = obj;

	bbx_setpos(fp->bbx, fp->fontbox, 10, 10, width - 20, height - 150);
	bbx_setpos(fp->bbx, fp->sample_edt, 10, height - 130, width - 20, 50);
	bbx_setpos(fp->bbx, fp->points_spn, width - 70, height - 60, 50, 20);
	bbx_setpos(fp->bbx, fp->points_lab, width - 150, height - 60, 80, 20);
	bbx_setpos(fp->bbx, fp->ok_but, width - 300, height - 30, 50, 25);
	bbx_setpos(fp->bbx, fp->cancel_but, width - 150, height - 30, 75, 25);
}

static void fontpickerpresscancel(void *obj)
{
	FONTPICKER *fp = obj;
	fp->positive = 0;
	bbx_dialogpanel_dropmodal(fp->pan);
}

static void fontpickerpressok(void *obj)
{
	FONTPICKER *fp = obj;
	
	fp->positive = 1;

	bbx_dialogpanel_dropmodal(fp->pan);
}

static void fontselected(void *ptr, int index)
{
	FONTPICKER *fp = ptr;
	struct bitmap_font *font;
	int points;

	points = bbx_spinner_getvalue(fp->points_spn);
	font_kill(fp->chosen_font);
	font = loadfullfont(fp->names[index], points);
	fp->chosen_font = font;
	if (font)
	{
		fp->selected = index;
		bbx_lineedit_setfont(fp->sample_edt, font);
	}
	else
	{
		bbx_lineedit_settext(fp->sample_edt, "Error loading font");
		bbx_lineedit_setfont(fp->sample_edt, fp->bbx->user_font2);
	}
}

static void spinme(void *ptr, double value)
{
	FONTPICKER *fp = ptr;

	if (fp->selected != -1)
		fontselected(fp, fp->selected);
}

typedef struct
{
	BABYX *bbx;
	BBX_Panel *pan;
	BBX_Scrollbar *sb;
	BBX_Canvas *can;
	struct bitmap_font **fonts;
	char **names;
	int N;
	int selected;
	int ypos;
	void(*selfunc)(void *ptr, int i);
	void *ptr;
} FONTBOX;

void fontbox_layout(void *ptr, int width, int height);
void fontbox_redraw(FONTBOX *fb);
void fontbox_scroll(void *ptr, int pos);
void fontbox_mouse(void *ptr, int action, int x, int y, int buttons);

BBX_Panel *bbx_fontbox(BABYX *bbx, BBX_Panel *parent, struct bitmap_font **fonts, char **names, int N,
	void (*selected)(void *ptr, int i), void *ptr)
{
	FONTBOX *fb;
	
	fb = bbx_malloc(sizeof(FONTBOX));
	fb->bbx = bbx;
	fb->pan = bbx_panel(bbx, parent, "fontbox", fontbox_layout, fb);
	fb->sb = bbx_scrollbar(bbx, fb->pan, BBX_SCROLLBAR_VERTICAL, fontbox_scroll, fb);
	fb->can = 0;
	fb->fonts = fonts;
	fb->names = names;
	fb->N = N;
	fb->selected = -1;
	fb->ypos = 0;
	fb->selfunc = selected;
	fb->ptr = ptr;
	return fb->pan;
}

void bbx_fontbox_kill(BBX_Panel *obj)
{
	FONTBOX *fb;
	if (obj)
	{
		fb = bbx_panel_getptr(obj);
		if (fb)
		{
			bbx_scrollbar_kill(fb->sb);
			bbx_canvas_kill(fb->can);
			free(fb);
		}
		bbx_panel_kill(obj);
	}
}


void fontbox_layout(void *ptr, int width, int height)
{
	FONTBOX *fb = ptr;
	int fbheight = 0;
	int i;

	bbx_canvas_kill(fb->can);
	fb->can = bbx_canvas(fb->bbx, fb->pan, width-15, height, bbx_rgba(255, 255, 255, 255));
	bbx_setpos(fb->bbx, fb->can, 0, 0, width - 15, height);
	bbx_setpos(fb->bbx, fb->sb, width - 15, 0, 15, height);

	for (i = 0; i < fb->N; i++)
	{
		if (fb->fonts[i])
			fbheight += fb->fonts[i]->height + 2;
	}
	bbx_scrollbar_set(fb->sb, fbheight, height, 0);
	bbx_canvas_setmousefunc(fb->can, fontbox_mouse, fb);
	fontbox_redraw(fb);


}

void fontbox_redraw(FONTBOX *fb)
{
	unsigned char *rgba;
	int width;
	int height;
	int i;
	int y;
	BBX_RGBA col;

	rgba = bbx_canvas_rgba(fb->can, &width, &height);
	
	bbx_rectangle(rgba, width, height, 0, 0, width, height, bbx_rgba(255, 255, 255, 255));
	bbx_line(rgba, width, height, 0, 0, width - 1, 0, bbx_color("dark grey"));
	bbx_line(rgba, width, height, 0, 0, 0, height - 1, bbx_color("dark grey"));
	bbx_line(rgba, width, height, 0, height - 1, width - 1, height - 1, bbx_color("light grey"));
	bbx_line(rgba, width, height, width - 1, 0, width - 1, height - 1, bbx_color("light grey"));
	y = 2 - fb->ypos;
	for (i = 0; i < fb->N; i++)
	{
		if (fb->fonts[i])
		{
			char tbuff[64];
		
			if (y > 0)
			{
				if (i == fb->selected)
				{
					bbx_rectangle(rgba, width, height, 0, y, width, y + fb->fonts[i]->height, bbx_color("blue"));
					col = bbx_rgba(255, 255, 255, 255);
				}
				else
					col = bbx_rgba(0, 0, 0, 255);
				bbx_drawutf8(rgba, width, height, 5, y + fb->fonts[i]->ascent, fb->names[i], bbx_utf8_Nchars(fb->names[i]),
					fb->fonts[i], col);
			}
			y += fb->fonts[i]->height + 2;
		}
		if (y > height)
			break;
	}
	
	bbx_canvas_flush(fb->can);
}

void fontbox_scroll(void *ptr, int pos)
{
	FONTBOX *fb = ptr;
	fb->ypos = pos;

	fontbox_redraw(fb);

}

void fontbox_mouse(void *ptr, int action, int x, int y, int buttons)
{
	FONTBOX *fb = ptr;
	int y1, y2;
	int i;

	if (action == BBX_MOUSE_CLICK)
	{
		y1 = 2;
		for (i = 0; i < fb->N; i++)
		{
			y2 = y1 + fb->fonts[i]->height + 2;
			if (y + fb->ypos > y1 && y + fb->ypos < y2)
			{
				fb->selected = i;
				fontbox_redraw(fb);
				if (fb->selfunc)
					(*fb->selfunc)(fb->ptr, i);
				break;
			}
			y1 = y2;
		}
	}
}

typedef struct
{
	int folder;
	char *name;
} DIRENTRY;

static DIRENTRY *readdirectoryfilt(char *dir, int *N, int hidden, char *filt);
static DIRENTRY *readdirectory(char *dir, int *N, int hidden);
static int wildcard_match_icase(const char *data, const char *mask);

static char **commonfonts(void)
{
	static char* commonfonts[] =
	{
		"Arial.ttf",
		"Arialbd.ttf",	
		//"Book Antiqua.ttf"	
		"Comic.ttf", //"Comic Sans MS.ttf",
		"Cour.ttf", //Courier New.ttf",
		"Georgia.ttf",
		"Impact.ttf",
		"Lucon.ttf", //"Lucida Console.ttf",
		"Lucida Sans Unicode.ttf",
		//"MS Sans Serif.ttf",
		//"MS Serif.ttf",
		"Pala.ttf", //Palatino Linotype.ttf",
		"Tahoma.ttf",
		"Times.ttf",
		"Trebuchet MS.ttf",
		"Verdana.ttf",
		"Symbol.tff",
		"Webdings.ttf",
		"Wingdings.ttf",
		0
	};

	return commonfonts;
}
static char **enumfonts(void)
{
	int N;
	int i;
	int j = 0;
	DIRENTRY *dir;
	char **answer;

	dir = readdirectoryfilt("C:\\Windows\\Fonts", &N, 0, "*.ttf");
	answer = bbx_malloc((N+1) * sizeof(char *));
	for (i = 0; i < N; i++)
		if (!dir[i].folder)
			answer[j++] = dir[i].name;

	free(dir);
	answer[j] = 0;

	return answer;
}

static struct bitmap_font *loadsamplefont(char *name, char **nameout)
{
	char buff[1024];
	char namebuff[1024];
	struct bitmap_font *answer = 0;

	*nameout = 0;
	sprintf(buff, "C:\\Windows\\Fonts\\%s", name);
	answer = loadttffontsample(buff, 12, namebuff, 1024);
	if (answer)
		*nameout = bbx_strdup(namebuff);

	return answer;
}

static struct bitmap_font *loadfullfont(char *name, int points)
{
	char buff[1024];
	char namebuff[1024];
	struct bitmap_font *answer = 0;

	sprintf(buff, "C:\\Windows\\Fonts\\%s", name);
	answer = loadttffontfull(buff, points);

	return answer;
}

static long filelength(char *path)
{
	struct stat statbuf;

	if (stat(path, &statbuf) == -1)
		return -1;
	return statbuf.st_size;
}

static DIRENTRY *readdirectoryfilt(char *dir, int *N, int hidden, char *filt)
{
	DIRENTRY *answer;
	DIRENTRY *fulldir;
	int j = 0;
	int i;

	fulldir = readdirectory(dir, N, hidden);
	if (!fulldir)
	{
		*N = 0;
		return 0;
	}
	answer = bbx_malloc(*N * sizeof(DIRENTRY));
	for (i = 0; i<*N; i++)
	{
		if (fulldir[i].folder == 1 || wildcard_match_icase(fulldir[i].name, filt))
		{
			answer[j].folder = fulldir[i].folder;
			answer[j].name = bbx_strdup(fulldir[i].name);
			j++;
		}
	}
	killentries(fulldir, *N);
	*N = j;
	answer = bbx_realloc(answer, j * sizeof(DIRENTRY));
	return answer;
}

#include <assert.h>
static DIRENTRY *readdirectory(char *dir, int *N, int hidden)
{
	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;
	TCHAR szDir[MAX_PATH];
	size_t length_of_arg;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0;
	int Nch;
	int Nread = 0;
	int i, j;
	char *ptr;
	int len;
	int count;
	DIRENTRY *answer = 0;

	if (!strcmp(dir, "\\"))
	{
		GetLogicalDriveStrings(MAX_PATH, szDir);
		i = 0;
		while (szDir[i])
		{
			Nread++;
			while (szDir[i])
				i++;
			i++;
		}
		answer = bbx_malloc(Nread * sizeof(DIRENTRY));
		i = 0;
		count = 0;

		while (szDir[i])
		{
			Nch = 0;
			j = i;
			while (szDir[j])
			{
				Nch += bbx_utf8_charwidth(szDir[j]);
				j++;
			}
			answer[count].name = bbx_malloc(Nch + 1);
			Nch = 0;
			j = i;
			while (szDir[j])
			{
				Nch += bbx_utf8_putch(answer[count].name + Nch, szDir[j]);
				j++;
			}
			answer[count].name[Nch] = 0;
			answer[count].folder = 1;
			answer[count].name[strlen(answer[count].name) - 1] = 0;
			i = j + 1;
			count++;
		}
		*N = Nread;
		return answer;
	}



	Nch = bbx_utf8_Nchars(dir);
	ptr = dir;
	for (i = 0; i < Nch; i++)
	{
		szDir[i] = bbx_utf8_getch(ptr);
		ptr += bbx_utf8_skip(ptr);
	}
	szDir[i] = '\\';
	szDir[i + 1] = '*';
	szDir[i + 2] = 0;

	// Find the first file in the directory.
	hFind = FindFirstFile(szDir, &ffd);

	if (INVALID_HANDLE_VALUE == hFind)
	{
		answer = bbx_malloc(sizeof(DIRENTRY));
		answer[0].name = bbx_strdup("..");
		answer[0].folder = 1;
		*N = 1;
		return answer;
	}

	// List all the files in the directory with some info about them.

	do
	{
		if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) && !hidden)
			continue;
		answer = bbx_realloc(answer, (Nread + 1) * sizeof(DIRENTRY));
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			answer[Nread].folder = 1;
		else
			answer[Nread].folder = 0;
		len = 0;
		for (i = 0; ffd.cFileName[i]; i++)
		{
			len += bbx_utf8_charwidth(ffd.cFileName[i]);
		}
		answer[Nread].name = bbx_malloc(len + 1);
		ptr = answer[Nread].name;
		for (i = 0; ffd.cFileName[i]; i++)
			ptr += bbx_utf8_putch(ptr, ffd.cFileName[i]);
		*ptr = 0;
		Nread++;
	} while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
	{
		return 0;
	}

	FindClose(hFind);

	for (i = 0; i < Nread; i++)
		if (!strcmp(answer[i].name, ".."))
			break;
	if (i == Nread)
	{
		answer = bbx_realloc(answer, (Nread + 1)*sizeof(DIRENTRY));
		answer[Nread].name = bbx_strdup("..");
		answer[Nread].folder = 1;
		Nread++;
	}

	*N = Nread;
	return answer;
}

#include <ctype.h>

#define WILDS '*'  /* matches 0 or more characters (including spaces) */
#define WILDQ '?'  /* matches ecactly one character */

#define NOMATCH 0
#define MATCH (match+sofar)

static int wildcard_match_int(const char *data, const char *mask, int icase)
{
	const char *ma = mask, *na = data, *lsm = 0, *lsn = 0;
	int match = 1;
	int sofar = 0;

	/* null strings should never match */
	if ((ma == 0) || (na == 0) || (!*ma) || (!*na))
		return NOMATCH;
	/* find the end of each string */
	while (*(++mask));
	mask--;
	while (*(++data));
	data--;

	while (data >= na) {
		/* If the mask runs out of chars before the string, fall back on
		* a wildcard or fail. */
		if (mask < ma) {
			if (lsm) {
				data = --lsn;
				mask = lsm;
				if (data < na)
					lsm = 0;
				sofar = 0;
			}
			else
				return NOMATCH;
		}

		switch (*mask) {
		case WILDS:                /* Matches anything */
			do
				mask--;                    /* Zap redundant wilds */
			while ((mask >= ma) && (*mask == WILDS));
			lsm = mask;
			lsn = data;
			match += sofar;
			sofar = 0;                /* Update fallback pos */
			if (mask < ma)
				return MATCH;
			continue;                 /* Next char, please */
		case WILDQ:
			mask--;
			data--;
			continue;                 /* '?' always matches */
		}
		if (icase ? (toupper(*mask) == toupper(*data)) :
			(*mask == *data)) {     /* If matching char */
			mask--;
			data--;
			sofar++;                  /* Tally the match */
			continue;                 /* Next char, please */
		}
		if (lsm) {                  /* To to fallback on '*' */
			data = --lsn;
			mask = lsm;
			if (data < na)
				lsm = 0;                /* Rewind to saved pos */
			sofar = 0;
			continue;                 /* Next char, please */
		}
		return NOMATCH;             /* No fallback=No match */
	}
	while ((mask >= ma) && (*mask == WILDS))
		mask--;                        /* Zap leftover %s & *s */
	return (mask >= ma) ? NOMATCH : MATCH;   /* Start of both = match */
}

static int wildcard_match(const char *data, const char *mask)
{
	return wildcard_match_int(data, mask, 0) != 0;
}

static int wildcard_match_icase(const char *data, const char *mask)
{
	return wildcard_match_int(data, mask, 1) != 0;
}


static void font_kill(struct bitmap_font *font)
{
	if (font)
	{
		free(font->widths);
		free(font->index);
		free(font->bitmap);
		free(font);
	}
}