
/*
  Baby X date picker, by Malcolm McLean
*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "BabyX.h"

typedef struct
{
	BABYX *bbx;
	BBX_DialogPanel *pan;
	BBX_Canvas *dates_can;
	BBX_Canvas *days_can;
	BBX_Label *month_lab;
	BBX_Button *back_but;
	BBX_Button *forwards_but;
	BBX_Button *ok_but;
	BBX_Button *cancel_but;
	int day;
	int month;
	int year;
	int selday;
	int selmonth;
	int selyear;
	int ok;

} DATEPICKER;

static void layout(void *obj, int width, int height);
static void redraw(DATEPICKER *cp);
static void drawdays(DATEPICKER *dp);
static void drawdates(DATEPICKER *dp);
static void calendarmouse(void *obj, int action, int x, int y, int buttons);
static int hitday(DATEPICKER *dp, int x, int y);
static void pressforwards(void *obj);
static void pressback(void *obj);
static void pressok(void *obj);
static void presscancel(void *obj);

static int daysinmonth(int y, int m);
static int isleapyear(int year);
static int dow(int y, int m, int d);

int pickdate(BABYX *bbx, int year, int month, int day, int *yearout, int *monthout, int *dayout)
{
	DATEPICKER dp;

	assert(month >= 1 && month <= 12);
	assert(year > 1582);
	assert(day == 0 || (day > 0 && day <= daysinmonth(year, month)));

	dp.bbx = bbx;
	dp.pan = bbx_dialogpanel(bbx, "Date Picker", 400, 300, layout, &dp);
	dp.dates_can = bbx_canvas(bbx, dp.pan, 7 * 40, 6 * 32, bbx_color("white"));
	dp.days_can = bbx_canvas(bbx, dp.pan, 7 * 40, 32, bbx_color("white"));
	dp.month_lab = bbx_label(bbx, dp.pan, "Month yyyy");
	dp.back_but = bbx_button(bbx, dp.pan, "<<", pressback, &dp);
	dp.forwards_but = bbx_button(bbx, dp.pan, ">>", pressforwards, &dp);
	dp.ok_but = bbx_button(bbx, dp.pan, "Ok", pressok, &dp);
	dp.cancel_but = bbx_button(bbx, dp.pan, "Cancel", presscancel, &dp);

	bbx_canvas_setmousefunc(dp.dates_can, calendarmouse, &dp);


	bbx_panel_setbackground(dp.pan, bbx_color("gray"));
	bbx_dialogpanel_setclosefunc(dp.pan, presscancel, &dp);

	dp.day = day;
	dp.month = month;
	dp.year = year;

	if (day != 0)
	{
		dp.selday = day;
		dp.selmonth = month;
		dp.selyear = year;
	}

	dp.ok = 0;

	redraw(&dp);
	bbx_dialogpanel_makemodal(dp.pan);

	bbx_canvas_kill(dp.dates_can);
	bbx_canvas_kill(dp.days_can);
	bbx_button_kill(dp.back_but);
	bbx_button_kill(dp.forwards_but);
	bbx_label_kill(dp.month_lab);
	bbx_button_kill(dp.ok_but);
	bbx_button_kill(dp.cancel_but);
	bbx_dialogpanel_kill(dp.pan);

	if (dp.ok)
	{
		if (yearout)
			*yearout = dp.selyear;
		if (monthout)
			*monthout = dp.selmonth;
		if (dayout)
			*dayout = dp.selday;
		return 0;
	}
	else
	{
		if (yearout)
			*yearout = 0;
		if (monthout)
			*monthout = 0;
		if (dayout)
			*dayout = 0;
		return -1;
	}
}

static void layout(void *obj, int width, int height)
{
	DATEPICKER *dp = obj;

	bbx_setpos(dp->bbx, dp->back_but, 10, 5, 35, 25);
	bbx_setpos(dp->bbx, dp->forwards_but, 40 * 7 - 35 + 10, 5, 35, 25);
	bbx_setpos(dp->bbx, dp->month_lab, 55, 5, 200, 25);
	bbx_setpos(dp->bbx, dp->days_can, 10, 30, 40 * 7, 32);
	bbx_setpos(dp->bbx, dp->dates_can, 10, 64, 40 *7, 6 * 32);

	bbx_setpos(dp->bbx, dp->ok_but, 10, height - 30, 60, 25);
	bbx_setpos(dp->bbx, dp->cancel_but, 100, height - 30, 60, 25);
}

static void redraw(DATEPICKER *dp)
{
	char *months[12] = { "January", "February", "March", "April", "May", "June", 
		"July", "August", "September", "October", "November", "December", };
	char buff[256];

	sprintf(buff, "%s %d\n", months[dp->month - 1], dp->year);
	bbx_label_settext(dp->month_lab, buff);
	drawdays(dp);
	drawdates(dp);

	if (dp->year == dp->selyear && dp->month == dp->selmonth)
	{
		bbx_button_enable(dp->ok_but);
	}
	else
	{
		bbx_button_disable(dp->ok_but);
	}
	bbx_button_enable(dp->back_but);
	if (dp->year < 1582 || dp->year == 1582 && dp->month == 1)
	{
		bbx_button_disable(dp->ok_but);
		bbx_button_disable(dp->back_but);
	}

}

static void drawdays(DATEPICKER *dp)
{
	char *days[7] = { "Sun", "Mon", "Tue", "Wed", "Thur", "Fri", "Sat" };
	int i;
	unsigned char *rgba;
	int width, height;
	int len;

	rgba = bbx_canvas_rgba(dp->days_can, &width, &height);
	bbx_rectangle(rgba, width, height, 0, 0, width, height, bbx_color("grey"));
	for (i = 0; i < 7; i++)
	{
		len = bbx_utf8width(dp->bbx->gui_font, days[i], bbx_utf8_Nchars(days[i]));
		bbx_drawutf8(rgba, width, height, i * 40 + (40 -len)/2, 25, days[i], bbx_utf8_Nchars(days[i]), dp->bbx->gui_font, bbx_color("black"));
	}
	bbx_canvas_flush(dp->days_can);
}

static void drawdates(DATEPICKER *dp)
{
	int x, y;
	unsigned char *rgba;
	int width, height;
	char buff[32];
	int start;
	int i;
	BBX_RGBA datecol;
	int len;

	rgba = bbx_canvas_rgba(dp->dates_can, &width, &height);
	bbx_rectangle(rgba, width, height, 0, 0, width, height, bbx_color("white"));
	for (y = 0; y < 5; y++)
	{
		bbx_line(rgba, width, height, 0, y * 32 + 31, width, y * 32 + 31, bbx_color("grey"));

	}
	for (x = 0; x < 6; x++)
	{
		bbx_line(rgba, width, height, x * 40 + 39, 0, x * 40 + 39, height-1, bbx_color("grey"));
	}
	start = dow(dp->year, dp->month, 1);
	for (i = 0; i < daysinmonth(dp->year, dp->month); i++)
	{
		x = start % 7;
		y = start / 7;
		sprintf(buff, "%d", i + 1);
		if (i + 1 == dp->selday && dp->month == dp->selmonth && dp->year == dp->selyear)
		{
			bbx_rectangle(rgba, width, height, x * 40, y * 32, x * 40 + 39, y * 32 + 32, bbx_color("blue"));
			datecol = bbx_color("white");
		}
		else
			datecol = bbx_color("black");
		len = bbx_utf8width(dp->bbx->gui_font, buff, bbx_utf8_Nchars(buff));
		bbx_drawutf8(rgba, width, height, x * 40 + (40-len)/2, y * 32 + 25, buff, bbx_utf8_Nchars(buff), dp->bbx->gui_font, datecol);
		start++;
	}
	if (dp->year < 1582 || dp->year == 1582 && dp->month == 1)
	{
		bbx_rectangle(rgba, width, height, 0, 0, width, height, bbx_color("white"));
		bbx_drawutf8(rgba, width, height, 10, height / 2, "By infallible decree of his Holiness", 36, dp->bbx->gui_font, bbx_color("red"));
		bbx_drawutf8(rgba, width, height, 10, height / 2 + 20, "Gregorius XIII", 14, dp->bbx->gui_font, bbx_color("red"));
		bbx_drawutf8(rgba, width, height, 10, height / 2 + 40, "The calendar starts here ...", 28, dp->bbx->gui_font, bbx_color("red"));
	}
	bbx_canvas_flush(dp->dates_can);
}

static void pressforwards(void *obj)
{
	DATEPICKER *dp = obj;
	dp->month++;
	if (dp->month > 12)
	{
		dp->month = 1;
		dp->year++;
	}
	redraw(dp);
}

static void pressback(void *obj)
{
	DATEPICKER *dp = obj;
	dp->month--;
	if (dp->month < 1)
	{
		dp->month = 12;
		dp->year--;
	}
	redraw(dp);
}

static void calendarmouse(void *obj, int action, int x, int y, int buttons)
{
	DATEPICKER *dp = obj;
	int day;

	if (action == BBX_MOUSE_CLICK)
	{
		if (dp->year < 1582 || dp->year == 1582 && dp->month == 1)
			return;
		day = hitday(dp, x, y);
		if (day)
		{
			dp->selday = day;
			dp->selmonth = dp->month;
			dp->selyear = dp->year;
			redraw(dp);
		}
	}
}

static int hitday(DATEPICKER *dp, int x, int y)
{
	int start;
	int i;
	int cx, cy;

	start = dow(dp->year, dp->month, 1);
	for (i = 0; i < daysinmonth(dp->year, dp->month); i++)
	{
		cx = start % 7;
		cy = start / 7;
		if (cx * 40 <= x && cx * 40 + 39 >= x && cy * 32 <= y && cy * 32 + 31 >= y)
			return i + 1;
		start++;
	}
	return 0;
}

static void pressok(void *obj)
{
	DATEPICKER *dp = obj;
	dp->ok = 1;
	bbx_dialogpanel_dropmodal(dp->pan);
}

static void presscancel(void *obj)
{
	DATEPICKER *dp = obj;
	bbx_dialogpanel_dropmodal(dp->pan);
}

static int daysinmonth(int y, int m)
{
	static int t[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	if (m != 2)
		return t[m - 1];
	if (isleapyear(y))
		return 29;
	return 28;
}

static int isleapyear(int year)
{
  return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

static int dow(int y, int m, int d)
{
	static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
	y -= m < 3;
	return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}
