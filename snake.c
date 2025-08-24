#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <ncurses.h>

/* ============================================================
   === Global constants and macros ===
   ============================================================ */
#define TBool            int
#define True             1
#define False            0

#define SHAPE_FOOD       '@'
#define SHAPE_SNAKE      '#'

#define GAMEWIN_YLEN     15
#define GAMEWIN_XLEN     60
#define LOGWIN_YLEN      7
#define LOGWIN_XLEN      (GAMEWIN_XLEN)
#define LOGBUF_NUM       (LOGWIN_YLEN-2)
#define LOGBUF_LEN       (GAMEWIN_XLEN-2)
#define MAXLEVEL         12

#define GetSnakeTail(s)  ((s)->head->front)

WINDOW *logwin;          /* Log window */
int g_level;             /* Current level */

#define INITRUNLOG()     logwin = newlogw()
#define RUNLOG(str)      runlog(logwin, str)
#define DESTROYRUNLOG()  delwin(logwin)

/* ============================================================
   === Data structures ===
   ============================================================ */
enum TDirection {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
};

struct TFood {
    int y;
    int x;
};

struct TSnakeNode {
    int y;
    int x;
    struct TSnakeNode *front;
};

struct TSnake {
    int    length;
    struct TSnakeNode *head;
    enum   TDirection  dir;
};

/* ============================================================
   === Prototypes ===
   ============================================================ */
WINDOW* newgamew();
WINDOW* newlogw();
void runlog(WINDOW *win, char *str);
void cleanline(WINDOW *win, int y, int x);

struct TSnake* initsnake();
struct TSnakeNode *newsnakenode(struct TSnakeNode **ppsnode, int y, int x);
void destroysnake(struct TSnake *psnake);

void drawsnakew(WINDOW *win, struct TSnake *psnake);
void movesnake(struct TSnake *psnake);
void snakegrowup(struct TFood *pfood, struct TSnake *psnake);

void drawfoodw(WINDOW *win, struct TFood *pfood, struct TSnake *psnake);
TBool checkfood(struct TFood *pfood, struct TSnake *psnake);

int checksnake(struct TFood *pfood, struct TSnake *psnake);
int refreshgamew(WINDOW *win, struct TSnake *psnake);
void gameover(WINDOW *win, char *str);

/* ============================================================
   === Main function ===
   ============================================================ */
int main()
{
    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);   /* enable special keys like arrows */
    curs_set(0);
    refresh();

    g_level = 1;

    INITRUNLOG();
    RUNLOG("Press 'x' or 'X' to quit.");
    RUNLOG("Use arrow keys to move the snake.");
    RUNLOG("Info:");

    WINDOW *gwin = newgamew();
    struct TSnake *psnake = initsnake();
    drawsnakew(gwin, psnake);

    while (refreshgamew(gwin, psnake) >= 0);

    getch();
    destroysnake(psnake);
    delwin(gwin);
    DESTROYRUNLOG();
    endwin();

    return 0;
}

/* ============================================================
   === Window management ===
   ============================================================ */
WINDOW* newgamew()
{
    WINDOW *win = newwin(GAMEWIN_YLEN, GAMEWIN_XLEN, 1, 3);
    box(win, 0, 0);
    mvwprintw(win, 0, 2, " GAME ");
    wrefresh(win);
    return win;
}

WINDOW* newlogw()
{
    WINDOW *win = newwin(LOGWIN_YLEN, LOGWIN_XLEN, GAMEWIN_YLEN + 2, 3);
    box(win, 0, 0);
    mvwprintw(win, 0, 2, " LOG ");
    wrefresh(win);
    return win;
}

void runlog(WINDOW *win, char *str)
{
    static char logbuf[LOGBUF_NUM][LOGBUF_LEN] = {0};
    static int  index = 0;

    strcpy(logbuf[index], str);
    for (int i = 0; i < LOGBUF_NUM; ++i) {
        cleanline(win, i+1, 1);
        mvwprintw(win, i+1, 1, "%s", logbuf[(index+i) % LOGBUF_NUM]);
        wrefresh(win);
    }
    index = (index + LOGBUF_NUM - 1) % LOGBUF_NUM;
}

void cleanline(WINDOW *win, int y, int x)
{
    char EMPTYLINE[LOGBUF_LEN] = {0};
    memset(EMPTYLINE, ' ', LOGBUF_LEN-1);
    mvwprintw(win, y, x, "%s", EMPTYLINE);
    wrefresh(win);
}

/* ============================================================
   === Snake management ===
   ============================================================ */
struct TSnake* initsnake()
{
    struct TSnake *psnake = (struct TSnake *)malloc(sizeof(struct TSnake));
    psnake->dir    = DIR_LEFT;
    psnake->length = 4;

    newsnakenode (
        &newsnakenode (
            &newsnakenode (
                &newsnakenode( &psnake->head, 4, 50 )->front, 4, 53
            )->front, 4, 52
        )->front, 4, 51
    )->front = psnake->head;

    return psnake;
}

struct TSnakeNode *newsnakenode(struct TSnakeNode **ppsnode, int y, int x)
{
    *ppsnode = (struct TSnakeNode *)malloc(sizeof(struct TSnakeNode));
    (*ppsnode)->y = y;
    (*ppsnode)->x = x;
    (*ppsnode)->front = NULL;
    return *ppsnode;
}

void destroysnake(struct TSnake *psnake)
{
    struct TSnakeNode *psnode = GetSnakeTail(psnake);
    struct TSnakeNode *ptmp   = NULL;

    for (int i = 0; i < psnake->length; ++i) {
        ptmp   = psnode;
        psnode = psnode->front;
        free(ptmp);
    }
    free(psnake);
}

/* ============================================================
   === Snake drawing and movement ===
   ============================================================ */
void drawsnakew(WINDOW *win, struct TSnake *psnake)
{
    static int taily = 0;
    static int tailx = 0;
    if (taily != 0 && tailx != 0)
        mvwaddch(win, taily, tailx, ' ');

    struct TSnakeNode *psnode = GetSnakeTail(psnake);
    for (int i = 0; i < psnake->length; ++i) {
        mvwaddch(win, psnode->y, psnode->x, SHAPE_SNAKE);
        psnode = psnode->front;
    }

    taily = GetSnakeTail(psnake)->y;
    tailx = GetSnakeTail(psnake)->x;
    wrefresh(win);
}

void movesnake(struct TSnake *psnake)
{
    int hy = psnake->head->y;
    int hx = psnake->head->x;
    psnake->head = GetSnakeTail(psnake);

    switch (psnake->dir) {
    case DIR_UP:    psnake->head->y = hy - 1; psnake->head->x = hx; break;
    case DIR_DOWN:  psnake->head->y = hy + 1; psnake->head->x = hx; break;
    case DIR_LEFT:  psnake->head->y = hy;     psnake->head->x = hx - 1; break;
    case DIR_RIGHT: psnake->head->y = hy;     psnake->head->x = hx + 1; break;
    }
}

void snakegrowup(struct TFood *pfood, struct TSnake *psnake)
{
    struct TSnakeNode *pnode = (struct TSnakeNode *)malloc(sizeof(struct TSnakeNode));
    switch (psnake->dir) {
    case DIR_UP:    pnode->y = psnake->head->y - 1; pnode->x = psnake->head->x; break;
    case DIR_DOWN:  pnode->y = psnake->head->y + 1; pnode->x = psnake->head->x; break;
    case DIR_LEFT:  pnode->y = psnake->head->y;     pnode->x = psnake->head->x - 1; break;
    case DIR_RIGHT: pnode->y = psnake->head->y;     pnode->x = psnake->head->x + 1; break;
    }
    pnode->front = GetSnakeTail(psnake);
    psnake->head->front = pnode;
    psnake->head = pnode;
    ++psnake->length;
}

/* ============================================================
   === Food ===
   ============================================================ */
void drawfoodw(WINDOW *win, struct TFood *pfood, struct TSnake *psnake)
{
    do {
        pfood->y = random() % (GAMEWIN_YLEN - 2) + 1;
        pfood->x = random() % (GAMEWIN_XLEN - 2) + 1;
    } while (False == checkfood(pfood, psnake));
    mvwaddch(win, pfood->y, pfood->x, SHAPE_FOOD);
    wrefresh(win);
}

TBool checkfood(struct TFood *pfood, struct TSnake *psnake)
{
    struct TSnakeNode *pnode = GetSnakeTail(psnake);
    for (int i = 0; i < psnake->length; ++i, pnode = pnode->front)
        if (pfood->y == pnode->y && pfood->x == pnode->x)
            return False;
    return True;
}

/* ============================================================
   === Game logic ===
   ============================================================ */
int checksnake(struct TFood *pfood, struct TSnake *psnake)
{
    if ( psnake->head->y <= 0 || psnake->head->y >= GAMEWIN_YLEN
      || psnake->head->x <= 0 || psnake->head->x >= GAMEWIN_XLEN)
        return -1;

    struct TSnakeNode *pnode = GetSnakeTail(psnake);
    for (int i = 0; i < psnake->length - 1; ++i, pnode = pnode->front)
        if (psnake->head->y == pnode->y && psnake->head->x == pnode->x)
            return -1;

    if (psnake->head->y == pfood->y && psnake->head->x == pfood->x)
        return 1;

    return 0;
}

int refreshgamew(WINDOW *win, struct TSnake *psnake)
{
    static TBool ffood = False;
    struct TFood pfood;

    if (!ffood) {
        drawfoodw(win, &pfood, psnake);
        ffood = True;
    }

    int key = -1;
    fd_set set;
    FD_ZERO(&set);
    FD_SET(0, &set);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec= (6 - (int)(g_level/3)) * 100*1000;

    if (select(1, &set, NULL, NULL, &timeout) < 0)
        return -1;

    if (FD_ISSET(0, &set)) {
        while ((key = getch()) == -1);
        switch (key) {
        case KEY_UP:    (psnake->dir == DIR_DOWN)? : (psnake->dir = DIR_UP); break;
        case KEY_DOWN:  (psnake->dir == DIR_UP)?   : (psnake->dir = DIR_DOWN); break;
        case KEY_LEFT:  (psnake->dir == DIR_RIGHT)? : (psnake->dir = DIR_LEFT); break;
        case KEY_RIGHT: (psnake->dir == DIR_LEFT)?  : (psnake->dir = DIR_RIGHT); break;
        case 'x': case 'X': RUNLOG("Quit Game!"); gameover(win, "Quit Game!"); return -1;
        }
    }

    movesnake(psnake);
    drawsnakew(win, psnake);

    switch (checksnake(&pfood, psnake)) {
    case 0: break;
    case 1:
        ffood = False;
        if (++g_level > MAXLEVEL) {
            RUNLOG("You Win!!!");
            gameover(win, "You Win!!!");
            return -1;
        }
        mvwprintw(win, GAMEWIN_YLEN-1, 2, " Level: %d ", g_level);
        mvwprintw(win, GAMEWIN_YLEN-1, 30, " Speed: %d ", (int)(g_level/3));
        wrefresh(win);
        RUNLOG("Level UP!");
        snakegrowup(&pfood, psnake);
        break;
    default:
        RUNLOG("Game Over!");
        gameover(win, "Game Over!");
        return -1;
    }

    return 1;
}

void gameover(WINDOW *win, char *str)
{
    mvwprintw(win, GAMEWIN_YLEN/2, GAMEWIN_XLEN/2 - (int)strlen(str)/2, "%s", str);
    mvwprintw(win, GAMEWIN_YLEN/2 + 1, 20, "Press any key to quit...");
    wrefresh(win);
}
