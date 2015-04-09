#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <string.h>
#include "lib.h"
#include "core.h"
#ifdef POSIX
#include <unistd.h>
#endif

#define drawSecret()                                                           \
	drawCombination(session->secret->val, session->config->guesses, 0)
#define drawGuess(p) drawCombination(session->panel[p].combination, p, 1)
#define sdl_print_center(s, x, y, color) sdl_print(s, x, y, color, 0)
#define sdl_print_left(s, x, y, color) sdl_print(s, x, y, color, -1)

#define TAB_GAME (uint8_t)0
#define TAB_SETTINGS (uint8_t)1

int SCREEN_HEIGHT = 640;
int SCREEN_WIDTH = 480;
typedef struct {
	unsigned x;
	unsigned y;
	unsigned w;
	unsigned h;
	unsigned rows;
	unsigned cols;
} SDL_Table;

SDL_Window *win = NULL;
SDL_Renderer *rend = NULL;
TTF_Font *font = NULL, *icons = NULL;
;
mm_session *session = NULL;
uint8_t *curGuess = NULL; // combination of last guess combination
SDL_Table panel, state, control, play;
unsigned case_w, case_h, button_w;
SDL_Color *colors = NULL;  // colors used on drawing combinations
uint8_t curTab = TAB_GAME; // Current tab being drawed

void init_sdl()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
					 "SDL could not be initialize",
					 SDL_GetError(), NULL);
		exit(EXIT_FAILURE);
	}
	if (SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT,
					SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE,
					&win, &rend)) {
		SDL_ShowSimpleMessageBox(
		    SDL_MESSAGEBOX_ERROR,
		    "Error on creating window and gettings renderer",
		    SDL_GetError(), NULL);
		exit(EXIT_FAILURE);
	}
	if (TTF_Init() == -1) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
					 "SDL_ttf cannont intialize",
					 TTF_GetError(), NULL);
		exit(EXIT_FAILURE);
	}
	font = TTF_OpenFont(FONTSDIR "ProFont_r400-29.pcf", 28);
	icons = TTF_OpenFont(FONTSDIR "fontawesome-webfont.ttf", 31);
	if (font == NULL || icons == NULL) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
					 "Failed to load font", TTF_GetError(),
					 NULL);
		exit(EXIT_FAILURE);
	}
}
void clean()
{
	if (session) {
		mm_session_exit(session);
	}
	if (curGuess)
		free(curGuess);
	TTF_CloseFont(font);
	SDL_DestroyRenderer(rend);
	// SDL_FreeSurface(surf);
	SDL_DestroyWindow(win);
	TTF_Quit();
	SDL_Quit();
}
unsigned sdl_print(char *s, int x, int y, SDL_Color *color, int align)
{
	SDL_Texture *tex;
	SDL_Rect rect;
	SDL_Surface *surf = TTF_RenderUTF8_Solid(
	    font, s, (color == NULL) ? (SDL_Color)fg_color : *color);
	if (surf == NULL) {
		SDL_Log("Unable to render font! Error: %s\n", TTF_GetError());
		clean();
		exit(EXIT_FAILURE);
	}
	tex = SDL_CreateTextureFromSurface(rend, surf);
	if (tex == NULL) {
		SDL_Log("Unable to create texture! Error: %s\n",
			SDL_GetError());
		clean();
		exit(EXIT_FAILURE);
	}
	rect.w = surf->w;
	rect.h = surf->h;
	rect.y = y - surf->h / 2;
	switch (align) {
	case -1:
		rect.x = x;
		break;
	case 0:
		rect.x = x - surf->w / 2;
		break;
	case 1:
		rect.x = x - surf->w;
		break;
	}
	SDL_RenderCopyEx(rend, tex, NULL, &rect, 0, 0, 0);
	SDL_FreeSurface(surf);
	SDL_DestroyTexture(tex);
	return rect.w;
}
unsigned sdl_print_icon(uint16_t c, int x, int y, SDL_Color *color)
{
	SDL_Texture *tex;
	SDL_Rect rect;
	SDL_Surface *surf = TTF_RenderGlyph_Blended(
	    icons, c, (color == NULL) ? (SDL_Color)fg_color : *color);
	if (surf == NULL) {
		SDL_Log("Unable to render font! Error: %s\n", TTF_GetError());
		clean();
		exit(EXIT_FAILURE);
	}
	tex = SDL_CreateTextureFromSurface(rend, surf);
	if (tex == NULL) {
		SDL_Log("Unable to create texture! Error: %s\n",
			SDL_GetError());
		clean();
		exit(EXIT_FAILURE);
	}
	rect = (SDL_Rect){x - surf->w / 2, y - surf->h / 2, surf->w, surf->h};
	SDL_RenderCopyEx(rend, tex, NULL, &rect, 0, 0, 0);
	SDL_FreeSurface(surf);
	SDL_DestroyTexture(tex);
	return rect.w;
}
int setBg()
{
	SDL_SetRenderDrawColor(rend, (SDL_Color)bg_color.r,
			       (SDL_Color)bg_color.g, (SDL_Color)bg_color.b,
			       (SDL_Color)bg_color.a);
	SDL_RenderFillRect(rend, NULL);
	return 0;
}
void initTables()
{
	case_w = SCREEN_WIDTH / (session->config->holes + 4);
	case_h = SCREEN_HEIGHT / (session->config->guesses + 4);
	button_w = (SCREEN_WIDTH - case_w) / 9;
	panel = (SDL_Table){.x = case_w / 2,
			    .y = case_h / 2,
			    .w = case_w * session->config->holes,
			    .h = case_h * (session->config->guesses + 1),
			    .rows = session->config->guesses,
			    .cols = session->config->holes};
	state = (SDL_Table){.x = case_w / 2 + ((panel.cols + 1) * case_w),
			    .y = case_h / 2,
			    .w = case_w * 2,
			    .h = panel.h,
			    .rows = session->config->guesses,
			    .cols = 2};
	control = (SDL_Table){.x = case_w / 2,
			      .y = SCREEN_HEIGHT - (case_h * 1.5F),
			      .w = button_w * 3,
			      .h = case_h,
			      .rows = 1,
			      .cols = 3};
	play = (SDL_Table){.x = case_w / 2 + button_w * 5,
			   .y = control.y,
			   .w = button_w * 4,
			   .h = case_h,
			   .rows = 1,
			   .cols = 2};
}
void initColors()
{
	unsigned i;
	if (colors)
		free(colors);
	colors =
	    (SDL_Color *)malloc(sizeof(SDL_Color) * session->config->colors);
	SDL_Color cl[] = {fg_red,  fg_green,   fg_yellow,
			  fg_blue, fg_magenta, fg_cyan};
	for (i = 0; i < session->config->colors && i < LEN(cl); i++)
		colors[i] = (SDL_Color){cl[i].r, cl[i].g, cl[i].b, cl[i].a};
	for (; i < session->config->colors; i++)
		colors[i] = (SDL_Color){255 / (i + 1), (150 * 2) % 200,
					100 / (i % 3 + 1), 255};
}
int drawTableBottom(SDL_Table *T)
{
	unsigned i, w;
	w = T->w / T->cols;
	for (i = 0; i <= T->rows; i++)
		SDL_RenderDrawLine(rend, T->x, T->y + (case_h * i), T->x + T->w,
				   T->y + (case_h * i));
	for (i = 0; i <= T->cols; i++)
		SDL_RenderDrawLine(rend, T->x + (w * i), T->y, T->x + (w * i),
				   T->y + T->h);
	return 0;
}
void drawTableTop(SDL_Table *T)
{
	unsigned i;
	for (i = 0; i <= T->rows + 1; i++) {
		if (i == session->guessed + 1 && session->state != MM_SUCCESS &&
		    session->state != MM_FAIL)
			continue;
		SDL_RenderDrawLine(rend, T->x, T->y + (case_h * i), T->x + T->w,
				   T->y + (case_h * i));
	}
	for (i = 0; i <= T->cols + 1; i++)
		SDL_RenderDrawLine(rend, T->x + (case_w * i), T->y,
				   T->x + (case_w * i), T->y + T->h);
}
void drawCombination(uint8_t *G, unsigned p, unsigned drawState)
{
	unsigned i;
	SDL_Rect rect;
	rect.h = case_h / 3;
	rect.w = case_w / 3;
	rect.x = panel.x + rect.w;
	rect.y = panel.y + case_h * p + case_h / 3;
	SDL_Color green, yellow;
	green = (SDL_Color)fg_green;
	yellow = (SDL_Color)fg_yellow;
	// char c[2] = "a";
	for (i = 0; i < panel.cols; i++) {
		/*
		SDL_SetRenderDrawColor(rend, colors[G[i]].r, colors[G[i]].g,
				       colors[G[i]].b, colors[G[i]].a);
		SDL_RenderFillRect(rend, &rect);
		*/
		// c[0] = 'a' + G[i];
		sdl_print_icon(0xF111, rect.x + rect.w / 2, rect.y + rect.h / 3,
			       colors + G[i]);
		// sdl_print_center(c, rect.x + rect.w / 2, rect.y + rect.h / 3,
		//		 NULL);
		rect.x += case_w;
	}
	if (drawState) {
		char s[2];
		sprintf(s, "%d", session->panel[p].inplace);
		sdl_print_center(s, state.x + state.w / 4, rect.y + rect.h / 3,
				 &green);
		sprintf(s, "%d",
			session->panel[p].insecret - session->panel[p].inplace);
		sdl_print_center(s, state.x + (state.w / 4) * 3,
				 rect.y + rect.h / 3, &yellow);
	}
}
void drawSelector()
{
	unsigned y, x, i;
	x = case_w;
	y = case_h * (session->guessed + 1);
	char c[2] = "a";
	for (i = 0; i < session->config->holes; i++) {
		sdl_print_icon(0xF0DE, x, y, NULL);
		c[0] = curGuess[i] + 'a';
		sdl_print_icon(0xF111, x, y + case_h / 2, &colors[c[0] - 'a']);
		// sdl_print_center(c, x, y + case_h / 2, NULL);
		sdl_print_icon(0xF0DD, x, y + case_h, NULL);
		x += case_w;
	}
}
void redraw_settings()
{
	unsigned x, y;
	x = case_w;
	y = case_h;
	char str[3];
	mm_conf_t *conf;
	SDL_Table button;
	for (conf = mm_confs; conf < mm_confs + LEN(mm_confs); conf++) {
		sdl_print_left(conf->str.name, x, y, NULL);
		switch (conf->type) {
		case MM_CONF_BOOL:
			sdl_print_icon((conf->bool.val == 0 ? 0xF204 : 0xF205),
				       SCREEN_WIDTH - case_w * 2, y, NULL);
			break;
		case MM_CONF_INT:
			sprintf(str, "%d", conf->nbre.val);
			sdl_print_icon(0xF0D7, SCREEN_WIDTH - case_w * 2.5, y,
				       NULL);
			sdl_print_center(str, SCREEN_WIDTH - case_w * 2, y,
					 NULL);
			sdl_print_icon(0xF0D8, SCREEN_WIDTH - case_w * 1.5, y,
				       NULL);
			break;
		case MM_CONF_STR:
			sdl_print_center(conf->str.val,
					 SCREEN_WIDTH - case_w * 2, y, NULL);
			break;
		}
		y += case_h;
	}
	button = (SDL_Table){.x = case_w * 0.5,
			     .y = SCREEN_HEIGHT - case_h * 1.5,
			     .w = button_w * 3,
			     .h = case_h,
			     .cols = 1,
			     .rows = 1};
	SDL_SetRenderDrawColor(rend, (SDL_Color)br_color.r,
			       (SDL_Color)br_color.g, (SDL_Color)br_color.b,
			       (SDL_Color)br_color.a);
	drawTableBottom(&button);
	sdl_print_center("< back", case_w * 0.5 + button_w * 1.5,
			 SCREEN_HEIGHT - case_h * 1, NULL);
}
void redraw_game()
{
	unsigned i;
	SDL_SetRenderDrawColor(rend, (SDL_Color)br_color.r,
			       (SDL_Color)br_color.g, (SDL_Color)br_color.b,
			       (SDL_Color)br_color.a);
	drawTableTop(&panel);
	drawTableTop(&state);
	drawTableBottom(&control);
	drawTableBottom(&play);
	sdl_print_icon(0xF05A, control.x + button_w * 0.5,
		       control.y + case_h / 2, NULL);
	sdl_print_icon(0xF085, control.x + button_w * 1.5,
		       control.y + case_h / 2, NULL); // 0xF013
	sdl_print_icon(0xF097, control.x + button_w * 2.5,
		       control.y + case_h / 2, NULL);
	sdl_print_center("rest", play.x + button_w, play.y + case_h / 2, NULL);
	sdl_print_center("play", play.x + button_w * 3, play.y + case_h / 2,
			 NULL);
	for (i = 0; i < session->guessed; i++)
		drawGuess(i);
	if (session->state == MM_NEW || session->state == MM_PLAYING)
		drawSelector();
	else
		drawSecret();
}
void redraw()
{
	SDL_RenderClear(rend);
	setBg();
	switch (curTab) {
	case TAB_GAME:
		redraw_game();
		break;
	case TAB_SETTINGS:
		redraw_settings();
		break;
	}
	SDL_RenderPresent(rend);
}
unsigned onMouseUp(SDL_MouseButtonEvent e)
{
	unsigned i;
	if (curTab == TAB_SETTINGS) {
		if (e.x > case_w * 0.5 && e.x < button_w * 3 + case_w * 0.5 &&
		    e.y > SCREEN_HEIGHT - case_h * 1.5 &&
		    e.y < SCREEN_HEIGHT - case_h * 0.5) {
			curTab = TAB_GAME;
			redraw();
		} else if (e.x > SCREEN_WIDTH - case_w * 3 &&
			   e.x < SCREEN_WIDTH - case_w && e.y > case_h * 0.5 &&
			   e.y < case_h * LEN(mm_confs) + case_h * 0.5) {
			i = (unsigned)(e.y - case_h * 0.5) / case_h;
			if (e.y < case_h * (i + 0.75) &&
			    e.y > case_h * (i + 1.25))
				return 1;
			SDL_Log("I'm here! %d\n", i);
			switch (mm_confs[i].type) {
			case MM_CONF_BOOL:
				mm_confs[i].bool.val =
				    mm_confs[i].bool.val == 0 ? 1 : 0;
				break;
			case MM_CONF_INT:
				if (e.x < SCREEN_WIDTH - case_w * 2 &&
				    mm_confs[i].nbre.val > mm_confs[i].nbre.min)
					mm_confs[i].nbre.val--;
				if (e.x > SCREEN_WIDTH - case_w * 2 &&
				    mm_confs[i].nbre.val < mm_confs[i].nbre.max)
					mm_confs[i].nbre.val++;
			}
			mm_config_save();
			redraw();
		}
		return 1;
	}
	if (e.x > panel.x && e.x < panel.x + panel.w &&
	    e.y > panel.y + case_h * session->guessed &&
	    e.y < panel.y + case_h * (session->guessed + 2)) {
		i = (e.x - panel.x) / case_w;
		if (e.y < panel.y + case_h * (session->guessed + 1)) {
			if (curGuess[i] < session->config->colors - 1)
				curGuess[i]++;
		} else {
			if (curGuess[i] > 0)
				curGuess[i]--;
		}
		redraw();
	} else if (e.x > play.x && e.x < play.x + play.w && e.y > play.y &&
		   e.y < play.y + play.h) {
		return (e.x < play.x + play.w / 2) ? 0 : 2;
	} else if (e.x > control.x && e.x < control.x + control.w &&
		   e.y > control.y && e.y < control.y + control.h) {
		if (e.x < control.x + button_w) {
			SDL_ShowSimpleMessageBox(
			    SDL_MESSAGEBOX_INFORMATION, _("Help"),
			    _(PROGRAM_NAME ": Simple mastermind implemetation"),
			    win);
		} else if (e.x < control.x + button_w * 2) {
			curTab = TAB_SETTINGS;
			redraw();
		} else {
			const mm_scores_t *scores = mm_scores_get();
			char *s;
			int j = 0;
			if (scores->len == 0)
				s = _("No score yet!\n");
			else
				s = (char *)malloc(sizeof(char) * 42 *
						   scores->len);
			for (i = 0; i < scores->len; i++)
				j += snprintf(s + j, 42, "%-2d) %-15ld %s\n", i,
					      scores->T[i].score,
					      scores->T[i].account);
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,
						 _("Scores"), s, win);
			if (scores->len)
				free(s);
		}
	}
	return 1;
}
uint8_t *getGuess(unsigned *play)
{
	SDL_Event event;
	uint8_t *str;
	unsigned i = 0;
	while (i < session->config->holes && SDL_WaitEvent(&event) > -1 &&
	       *play) {
		// SDL_PollEvent returns either 0 or 1
		switch (event.type) {
		case SDL_QUIT:
			clean();
			exit(EXIT_SUCCESS);
			break;
		case SDL_KEYDOWN:
			if (curTab == TAB_GAME &&
			    event.key.keysym.sym >= SDLK_a &&
			    event.key.keysym.sym <
				(session->config->colors + SDLK_a)) {
				curGuess[i++] = event.key.keysym.sym - SDLK_a;
				redraw();
			} else if (curTab == TAB_SETTINGS &&
				   event.key.keysym.sym == SDLK_ESCAPE) {
				curTab = TAB_GAME;
				redraw();
			}
			SDL_Log("Key down event: %d (%c) name: %s\n",
				event.key.keysym.sym, event.key.keysym.sym,
				SDL_GetKeyName(event.key.keysym.sym));
			break;
		case SDL_WINDOWEVENT:
			SDL_Log("Window Event: id: %d, event: %d\n",
				event.window.windowID, event.window.event);
			if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
			    event.window.event == SDL_WINDOWEVENT_EXPOSED ||
			    event.window.event ==
				SDL_WINDOWEVENT_SIZE_CHANGED) {
				SDL_GetWindowSize(win, &SCREEN_WIDTH,
						  &SCREEN_HEIGHT);
				initTables();
				redraw();
			}
			break;
		case SDL_MOUSEBUTTONUP:
			SDL_Log("MouseButtonEvent: button: %d, x= %d, y= %d\n",
				event.button.button, event.button.x,
				event.button.y);
			*play = onMouseUp(event.button);
			if (*play == 2)
				goto done;
			break;
		}
	}
done:
	str = (uint8_t *)malloc(sizeof(uint8_t) * session->config->holes);
	for (i = 0; i < session->config->holes; i++)
		str[i] = curGuess[i];
	return str;
}
int main(int argc, char *argv[])
{
	uint8_t *g;
	unsigned i, play;
#ifdef DEBUG
#ifdef POSIX
	char pwd[2000];
	getcwd(pwd, 2000);
	SDL_Log("PWD: %s\n", pwd);
#endif // POSIX
	SDL_Log("FONTSDIR: " FONTSDIR "\n");
	SDL_Log("LOCALEDIR: " LOCALEDIR "\n");
#endif // DEBUG
	init_sdl();
#ifdef __ANDROID__
	// use android app internal path
	mm_init(SDL_AndroidGetInternalStoragePath());
#elif __IPHONEOS__
	mm_init("../Documents"); // FIXME: "../Library/Prefferences"
#endif // __IPHONEOS__
	session = mm_session_restore();
	if (session == NULL)
		session = mm_session_new();
#ifdef DEBUG
	extern char *mm_config_path, *mm_score_path;
	SDL_Log("mm_config_path: %s\n", mm_config_path);
	SDL_Log("mm_score_path: %s\n", mm_score_path);
#endif // DEBUG
	initTables();
	initColors();
	for (;;) {
		curGuess =
		    (uint8_t *)malloc(sizeof(uint8_t) * session->config->holes);
		for (i = 0; i < session->config->holes; i++)
			curGuess[i] = 0;
		redraw();
		play = 1;
		while ((session->state == MM_PLAYING ||
			session->state == MM_NEW) &&
		       play) {
			do {
				play = 1;
				g = getGuess(&play);
			} while (mm_play(session, g) && play);
			redraw();
		}
		while (play && getGuess(&play))
			;
		SDL_RenderClear(rend);
		mm_session_free(session);
		free(curGuess);
		curGuess = NULL;
		session = mm_session_new();
		initTables();
		initColors();
	}
	clean();
	exit(EXIT_SUCCESS);
}
