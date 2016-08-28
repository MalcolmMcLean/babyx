#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "BabyX.h"

void rgbapaste(unsigned char *rgba, int width, int height, unsigned char *sub, int swidth, int sheight, int x, int y);

#include "simplexnoise.h"
#include "wordmatcher.h"

#define uniform() (rand() / (RAND_MAX + 1.0))

typedef struct
{
	int x;
	int y;
	char ch;
} LETTER;

typedef struct
{
	float x;
	float y;
	float vx;
	float vy;
} SNOWFLAKE;


typedef struct
{
	char rack[32];
	LETTER *letters;
	int Nletters;
	SNOWFLAKE *snowflakes;
	int Nsnowflakes;
	int playerx;
	int playery;
	int playerwidth;
	int points;
	int inc;
} GAME;

typedef struct
{
	BABYX *bbx;
	BBX_Panel *root;
	BBX_Canvas *can;

	void *timer;
	GAME *game;
	unsigned char *skybuff;
	unsigned char *snowbuff;
	int snowdepth;
} APP;

static void TickFrame(void *ptr);

static void create(void *obj, BABYX *bbx, BBX_Panel *root);
static void layout(void *obj, int width, int height);

static void TickFrame(void *ptr);
static void sky(unsigned char *rgba, int width, int height, int inc);
static void scrollsky(unsigned char *rgba, int width, int height, int inc);
static void snow(unsigned char *rgba, int width, int height, int inc);

GAME *newgame(void);
void game_tick(GAME *game, int deltams);
void game_catchletter(GAME *game, char ch);
static void game_refill(GAME *game);
static int game_collision(GAME *game, LETTER *let);
char game_chooseletter(GAME *game);

extern unsigned char basket_rgba[];
extern int basket_width;
extern int basket_height;

extern unsigned char snowflake_rgba[];
extern int snowflake_width;
extern int snowflake_height;

void *playmp3(unsigned char *mp3_stream, int stream_size);

unsigned char *slurpb(char *fname, int *len)
{
	FILE *fp;
	unsigned char *answer = 0;
	unsigned char *temp;
	int capacity = 1024;
	int N = 0;
	int ch;

	fp = fopen(fname, "rb");
	if (!fp)
		return 0;
	answer = malloc(capacity);
	if (!answer)
		goto out_of_memory;
	while ( (ch = fgetc(fp)) != EOF)
	{
		answer[N++] = ch;
		if (N >= capacity)
		{
			temp = realloc(answer, capacity + capacity / 2);
			if (!temp)
				goto out_of_memory;
			answer = temp;
			capacity = capacity + capacity / 2;
		}
	}
	*len = N;
	fclose(fp);
	return answer;
out_of_memory:
	fclose(fp);
	*len = -1;
	free(answer);
	return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	APP app;
	unsigned char *tune;
	int N;

	srand(time(0));
	tune = slurpb("C:\\Users\\Malcolm\\Music\\away-in-a-manger.mp3", &N);
	playmp3(tune, N);
	startbabyx(hInstance, "Wordcatch", 320, 480, create, layout, &app);
}

static void create(void *obj, BABYX *bbx, BBX_Panel *root)
{
	APP *app = (APP *)obj;

	app->bbx = bbx;
	app->root = root;


	app->can = bbx_canvas(bbx, root, 320, 480, bbx_color("white"));
	app->timer = bbx_addticker(bbx, 40, TickFrame, app);

	app->game = newgame();
	app->snowbuff = 0;
	app->snowdepth = 0;
	app->skybuff = malloc(320 * 480 * 4);
	sky(app->skybuff, 320, 480, 0);
	
}

static void layout(void *ptr, int width, int height)
{
	APP *app = ptr;

	bbx_setpos(app->bbx, app->can, 0, 0, 320, 480);
}

static void redraw(APP *app)
{
	char buff[256];
	unsigned char *rgba;
	int width, height;
	GAME *game;
	int i;
	char score[32];

	game = app->game;

	rgba = bbx_canvas_rgba(app->can, &width, &height);
	
	bbx_rectangle(rgba, width, height, 0, 0, width, height, bbx_rgba(0, 255, 255, 255));
	//sky(rgba, width, height, game->inc);
	rgbapaste(rgba, width, height, app->skybuff, width, height, 0, 0);
	rgbapaste(rgba, width, height, basket_rgba, basket_width, basket_height, game->playerx, game->playery - 2);
	//bbx_drawstring(rgba, width, height, game->playerx, game->playery + 15, "XXX", 3, app->bbx->gui_font, bbx_rgba(0, 0, 0, 255));
	for (i = 0; i < game->Nletters; i++)
	{
		bbx_drawstring(rgba, width, height, game->letters[i].x, game->letters[i].y - app->bbx->gui_font->descent, 
			&game->letters[i].ch, 1, app->bbx->gui_font, bbx_rgba(255, 0, 0, 255));
		
	}
	for (i = 0; i < game->Nsnowflakes; i++)
	{
		rgbapaste(rgba, width, height, snowflake_rgba, snowflake_width, snowflake_height, game->snowflakes[i].x,
			game->snowflakes[i].y);
	}
	bbx_rectangle(rgba, width, height, 0, 5, width, 30, bbx_rgba(0, 0, 255, 128));
	
	bbx_drawstring(rgba, width, height, 10, 25, game->rack, strlen(game->rack), app->bbx->gui_font, bbx_rgba(255, 0, 0, 255));
	sprintf(score, "% 5d", game->points);
	bbx_drawstring(rgba, width, height, 250, 25, score, strlen(score), app->bbx->gui_font, bbx_rgba(255, 255, 0, 255));
	//bbx_rectangle(rgba, width, height, 0, game->playery +15, width, 480, bbx_rgba(255, 255, 255, 255));
	rgbapaste(rgba, width, height, app->snowbuff, width, app->snowdepth, 0, game->playery + 15);

	bbx_canvas_flush(app->can);
}

static void TickFrame(void *ptr)
{
	APP *app = ptr;
	GAME *game = app->game;
	SHORT pressed;
	int ly;

	pressed = GetAsyncKeyState(VK_LEFT);
	if (pressed & 0x8000)
		game->playerx -= 5;
	pressed = GetAsyncKeyState(VK_RIGHT);
	if (pressed & 0x8000)
		game->playerx += 5;

	if (game->playerx < 0)
		game->playerx = 0;
	if (game->playerx > 320 - game->playerwidth)
		game->playerx = 320 - game->playerwidth;

	pressed = GetAsyncKeyState(VK_SPACE);
	if (pressed & 0x8000)
		game_refill(game);
	ly = game->playery;
	game_tick(game, 40);
	if (game->playery != ly)
	{
		app->snowdepth = 480 - game->playery - 15;
		app->snowbuff = realloc(app->snowbuff, 320 * app->snowdepth * 4);
		snow(app->snowbuff, 320, app->snowdepth, 0);
	}
	app->game->inc++;
	scrollsky(app->skybuff, 320, 480, app->game->inc);
	redraw(app);

}

GAME *newgame(void)
{
	GAME *game;
	int i;

	game = malloc(sizeof(GAME));
	game->letters = malloc(20 * sizeof(LETTER));
	game->Nletters = 0;
	game->snowflakes = malloc(40 * sizeof(SNOWFLAKE));
	game->Nsnowflakes = 0;
	game->playerx = 150;
	game->playerwidth = 30;
	game->rack[0] = 0;
	game->points = 0;
	game->playery = 465;
	game->inc = 0;
	for (i = 0; i < 20; i++)
	{
		do
		{
			game->letters[i].ch = game_chooseletter(game);
			game->letters[i].x = rand() % 300;
			game->letters[i].y = -(rand() % 960);
		} while(game_collision(game, &game->letters[i]));
		game->Nletters++;
	}
	for (i = 0; i < 40; i++)
	{
		game->snowflakes[i].x = rand() % 310;
		game->snowflakes[i].y = (rand() % 480);
		game->Nsnowflakes++;
	}
	return game;
}

void game_tick(GAME *game, int deltams)
{
	int i;

	for (i = 0; i < game->Nsnowflakes; i++)
	{
		game->snowflakes[i].y += 3;
		if (game->snowflakes[i].y > 490)
		{
			game->snowflakes[i].y = -8;
			game->snowflakes[i].x = rand() % 310;
		}
	}

	for (i = 0; i < game->Nletters; i++)
	{
		game->letters[i].y+=3;
	}
	for (i = 0; i < game->Nletters; i++)
	{
		if (game->letters[i].y >= game->playery &&
			game->letters[i].y <= game->playery + 5 &&
			game->letters[i].x + 10 > game->playerx &&
			game->letters[i].x < game->playerx + game->playerwidth)
		{
			game_catchletter(game, game->letters[i].ch);
			memmove(&game->letters[i], &game->letters[i + 1], (game->Nletters - i - 1) * sizeof(LETTER));
			game->Nletters--;
		}
	}
	
	for (i = game->Nletters - 1; i >= 0; i--)
	{
		if (game->letters[i].y > game->playery + 20)
		{
			game->letters[i].y = game->playery - 480 * 2;
			game->playery-= 2;
		}
	}
}

/* letter frequencies */
static double frequencies[26] =
{
	/*a */	8.167,

	/* b */	1.492,

	/* c */	2.782,

	/* d */	4.253,

	/* e */	12.702,

	/* f */	2.228,

	/* g */	2.015,

	/* h */ 6.094,

	/* i*/	6.966,

	/* j */	0.153,

	/* k */ 0.772,

	/* l */	4.025,

	/* m */	2.406,

	/* n */	6.749,

	/* o */	7.507,

	/* p */	1.929,

	/* q */	0.095,

	/* r */	5.987,

	/* s */	6.327,

	/* t */	9.056,

	/* u */	2.758,

	/* v */	0.978,

	/* w */	2.360,

	/* x */	0.150,

	/* y */	1.974,

	/* z */	0.074,
};

char game_chooseletter(GAME *game)
{
	double p;
	double tot = 0;
	int i;

	if (uniform() < 0.1)
		return '*';
		
    p = uniform() * 100;
	for (i = 0; i < 25; i++)
	{
		tot += frequencies[i];
		if (tot > p)
			break;
	}
		
	return "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i];

}

void game_catchletter(GAME *game, char ch)
{
	int score;
	if (ch == '*')
	{
		game->rack[0] = 0;
		game_refill(game);
		return;
	}
	if (strlen(game->rack) == 31)
		return;
	game->rack[strlen(game->rack) + 1] = 0;
	game->rack[strlen(game->rack)] = ch;
	if (strlen(game->rack) >= 3 && wordindictionary(game->rack))
	{
		score = scoreword(game->rack);
		game->playery += score;
		game->rack[0] = 0;
		game->points += score;
		if (game->playery > 465)
			game->playery = 465;
		//game_refill(game);
	}
	else
	{
	}
}

static void game_refill(GAME *game)
{
	int i;

	for (i = game->Nletters; i < 20; i++)
	{
		do
		{
			game->letters[i].ch = game_chooseletter(game);
			game->letters[i].y = -(rand() % 480);
			game->letters[i].x = rand() % 300;
		} while (game_collision(game, &game->letters[game->Nletters]));
		game->Nletters++;

	}
}

static int game_collision(GAME *game, LETTER *let)
{
	int i;

	for (i = 0; i < game->Nletters; i++)
	{
		if (abs(game->letters[i].x - let->x) < 20 &&
			abs(game->letters[i].y - let->y) < 20)
			return 1;
	}
	return 0;
}

static int scoreword(char *str)
{
	int tot = 0;
	int i;

	for (i = 0; str[i]; i++)
	{
		switch (str[i])
		{
		case 'A':
		case 'E':
		case 'I':
		case 'O':
		case 'U':
		case 'L':
		case 'N':
		case 'S':
		case 'T':
		case 'R':
			tot += 1;
			break;
		case 'D':
		case 'G':
			tot += 2;
			break;
		case 'B':
		case 'C':
		case 'M':
		case 'P':
			tot += 3;
			break;
		case 'F':
		case 'H':
		case 'V':
		case 'W':
		case 'Y':
			tot += 4;
			break;
		case 'K':
			tot += 5;
			break;
		case 'J':
		case 'X':
			tot += 8;
			break;
		case 'Q':
		case 'Z':
			tot += 10;
			break;
		}
	}
		
	if (i > 3)
		tot += (tot * i) / 5;

	return tot;
}

static void sky(unsigned char *rgba, int width, int height, int inc)
{
	int x, y;
	float t;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			t = scaled_octave_noise_2d(2, 1.0, 0.003, 0.5, 1.0, x + inc, y);
			rgba[(y*width + x) * 4] = (unsigned char)(t * 255);
			rgba[(y*width + x) * 4+1] = (unsigned char)(t * 255);
			rgba[(y*width + x) * 4+2] = (unsigned char)(255);
			rgba[(y*width + x) * 4 + 3] = (unsigned char)(255);

		}
	}
}

static void scrollsky(unsigned char *rgba, int width, int height, int inc)
{
	int x, y;
	float t;

	for (y = 0; y < height; y++)
		memmove(rgba + y * width * 4, rgba + y *width * 4 + 4, width * 4 - 4);


	for (y = 0; y < height; y++)
	{
		for (x = width-1; x < width; x++)
		{
			t = scaled_octave_noise_2d(2, 1.0, 0.003, 0.5, 1.0, x + inc, y);
			rgba[(y*width + x) * 4] = (unsigned char)(t * 255);
			rgba[(y*width + x) * 4 + 1] = (unsigned char)(t * 255);
			rgba[(y*width + x) * 4 + 2] = (unsigned char)(255);
			rgba[(y*width + x) * 4 + 3] = (unsigned char)(255);

		}
	}
}
static void snow(unsigned char *rgba, int width, int height, int inc)
{
	int x, y;
	float t;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			t = scaled_octave_noise_2d(3, 5.0, 0.1, 0.5, 1.0, x + inc, y);
			rgba[(y*width + x) * 4] = (unsigned char)(t * 255);
			rgba[(y*width + x) * 4 + 1] = (unsigned char)(t * 255);
			rgba[(y*width + x) * 4 + 2] = (unsigned char)(t * 255);
			rgba[(y*width + x) * 4 + 3] = (unsigned char)(255);

		}
	}

	for (x = 0; x < width; x++)
	{
		t = scaled_octave_noise_2d(1, 5.0, 0.1, 0.0, 5.0, x + 100, y);
		for (y = 0; y < (int)t && y < height;y++)
			rgba[(y*width + x) * 4 + 3] = 0;
	}
}