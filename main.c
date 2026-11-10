/*
 * Cobra Arena v2 - jogo da cobra para PS3 (PSL1GHT), offline, sem áudio.
 *
 * Controles:
 *   D-Pad      : muda a direção (não é possível dar meia-volta)
 *   START      : recomeça depois de morrer
 *   Botão de sair do XMB: fecha o jogo
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/process.h>
#include <sysutil/sysutil.h>
#include <sysutil/video.h>
#include <rsx/rsx.h>
#include <io/pad.h>

#define HOST_SIZE   (32 * 1024 * 1024)
#define CB_SIZE     0x100000

#define GRID_W      48
#define GRID_H      30
#define CELL        20
#define MAX_SNAKE   (GRID_W * GRID_H)
#define MOVE_EVERY  6      /* frames entre cada passo (60 fps -> 10 passos/s) */

#define COL_BG      0x00101010u
#define COL_WALL    0x00404040u
#define COL_SNAKE   0x0040E040u
#define COL_HEAD    0x00A0FFA0u
#define COL_FOOD    0x00FF4040u
#define COL_DEAD    0x00802020u

typedef struct { int x, y; } Point;

static gcmContextData *context;
static u32 *fb[2];
static u32 fb_offset[2];
static u32 screenW, screenH, pitchPx;
static int curBuf = 0;
static volatile int exitRequested = 0;

static Point snake[MAX_SNAKE];
static int snakeLen;
static int dirX, dirY;      /* direção atual */
static int nextDirX, nextDirY;
static Point food;
static int alive;
static int score;
static int frameCount;
static u32 rngState = 12345;

static void sysCallback(u64 status, u64 param, void *userdata)
{
    (void)param;
    (void)userdata;
    if (status == SYSUTIL_EXIT_GAME)
        exitRequested = 1;
}

static void initVideo(void)
{
    videoState state;
    videoResolution res;
    videoConfiguration vconfig;
    void *hostAddr;
    int i;

    hostAddr = memalign(1024 * 1024, HOST_SIZE);
    rsxInit(&context, CB_SIZE, HOST_SIZE, hostAddr);

    videoGetState(0, 0, &state);
    videoGetResolution(state.displayMode.resolutionId, &res);

    memset(&vconfig, 0, sizeof(vconfig));
    vconfig.resolution = state.displayMode.resolutionId;
    vconfig.format = VIDEO_BUFFER_FORMAT_XRGB;
    vconfig.pitch = res.width * sizeof(u32);
    videoConfigure(0, &vconfig, NULL, 0);
    videoGetState(0, 0, &state);

    gcmSetFlipMode(GCM_FLIP_VSYNC);

    screenW = res.width;
    screenH = res.height;
    pitchPx = res.width;

    for (i = 0; i < 2; i++) {
        fb[i] = rsxMemalign(64, pitchPx * sizeof(u32) * screenH);
        rsxAddressToOffset(fb[i], &fb_offset[i]);
        gcmSetDisplayBuffer(i, fb_offset[i], pitchPx * sizeof(u32), screenW, screenH);
    }
}

static void flip(void)
{
    gcmSetWaitFlip(context);
    gcmSetFlip(context, curBuf);
    rsxFlushBuffer(context);
    curBuf ^= 1;
}

static void fillRect(int x, int y, int w, int h, u32 color)
{
    int px, py;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int)screenW) w = screenW - x;
    if (y + h > (int)screenH) h = screenH - y;
    if (w <= 0 || h <= 0) return;

    for (py = y; py < y + h; py++) {
        u32 *row = fb[curBuf] + py * pitchPx + x;
        for (px = 0; px < w; px++)
            row[px] = color;
    }
}

/* Área do tabuleiro centralizada na tela */
static int boardX(void) { return ((int)screenW - GRID_W * CELL) / 2; }
static int boardY(void) { return ((int)screenH - GRID_H * CELL) / 2; }

static void drawCell(int gx, int gy, u32 color)
{
    fillRect(boardX() + gx * CELL + 1, boardY() + gy * CELL + 1, CELL - 2, CELL - 2, color);
}

static u32 nextRand(void)
{
    rngState = rngState * 1103515245u + 12345u;
    return (rngState >> 16) & 0x7FFF;
}

static int occupied(int x, int y)
{
    int i;
    for (i = 0; i < snakeLen; i++)
        if (snake[i].x == x && snake[i].y == y)
            return 1;
    return 0;
}

static void placeFood(void)
{
    int tries = 0;
    do {
        food.x = nextRand() % GRID_W;
        food.y = nextRand() % GRID_H;
        tries++;
    } while (occupied(food.x, food.y) && tries < 1000);
}

static void resetGame(void)
{
    int i;
    int cx = GRID_W / 2, cy = GRID_H / 2;

    snakeLen = 4;
    for (i = 0; i < snakeLen; i++) {
        snake[i].x = cx - i;
        snake[i].y = cy;
    }
    dirX = 1; dirY = 0;
    nextDirX = 1; nextDirY = 0;
    score = 0;
    alive = 1;
    frameCount = 0;
    placeFood();
}

static void readInput(void)
{
    padData data;
    u16 btn;

    if (ioPadGetData(0, &data) != 0)
        return;
    btn = data.button[2];

    if ((btn & CELL_PAD_CTRL_UP) && dirY != 1) {
        nextDirX = 0; nextDirY = -1;
    } else if ((btn & CELL_PAD_CTRL_DOWN) && dirY != -1) {
        nextDirX = 0; nextDirY = 1;
    } else if ((btn & CELL_PAD_CTRL_LEFT) && dirX != 1) {
        nextDirX = -1; nextDirY = 0;
    } else if ((btn & CELL_PAD_CTRL_RIGHT) && dirX != -1) {
        nextDirX = 1; nextDirY = 0;
    }

    if ((btn & CELL_PAD_CTRL_START) && !alive)
        resetGame();
}

static void stepGame(void)
{
    Point head, tail;
    int i;

    dirX = nextDirX;
    dirY = nextDirY;

    head.x = snake[0].x + dirX;
    head.y = snake[0].y + dirY;

    /* parede */
    if (head.x < 0 || head.x >= GRID_W || head.y < 0 || head.y >= GRID_H) {
        alive = 0;
        return;
    }

    tail = snake[snakeLen - 1];
    /* a cauda sai do lugar, então só conta colisão com o corpo sem a cauda */
    for (i = 0; i < snakeLen - 1; i++) {
        if (snake[i].x == head.x && snake[i].y == head.y) {
            alive = 0;
            return;
        }
    }

    if (head.x == food.x && head.y == food.y) {
        if (snakeLen < MAX_SNAKE)
            snakeLen++;
        score += 10;
        placeFood();
        /* a cauda não é removida: o corpo cresce */
        for (i = snakeLen - 1; i > 0; i--)
            snake[i] = snake[i - 1];
        snake[0] = head;
        return;
    }

    for (i = snakeLen - 1; i > 0; i--)
        snake[i] = snake[i - 1];
    snake[0] = head;
    (void)tail;
}

static void drawFrame(void)
{
    int i;
    u32 bodyColor = alive ? COL_SNAKE : COL_DEAD;

    fillRect(0, 0, screenW, screenH, COL_BG);

    /* borda do tabuleiro */
    fillRect(boardX() - 4, boardY() - 4, GRID_W * CELL + 8, 4, COL_WALL);
    fillRect(boardX() - 4, boardY() + GRID_H * CELL, GRID_W * CELL + 8, 4, COL_WALL);
    fillRect(boardX() - 4, boardY() - 4, 4, GRID_H * CELL + 8, COL_WALL);
    fillRect(boardX() + GRID_W * CELL, boardY() - 4, 4, GRID_H * CELL + 8, COL_WALL);

    drawCell(food.x, food.y, COL_FOOD);

    for (i = snakeLen - 1; i >= 1; i--)
        drawCell(snake[i].x, snake[i].y, bodyColor);
    drawCell(snake[0].x, snake[0].y, alive ? COL_HEAD : COL_DEAD);

    /* barra de pontuação: um bloco a cada 10 pontos */
    for (i = 0; i < score / 10 && i < 60; i++)
        fillRect(boardX() + i * 12, boardY() - 24, 10, 10, COL_FOOD);
}

static void waitFlipReady(void)
{
    /* sem espera extra: gcmSetWaitFlip já sincroniza com o vsync */
}

int main(int argc, const char *argv[])
{
    (void)argc;
    (void)argv;

    sysRegisterCallback(SYSUTIL_EVENT_SLOT0, sysCallback, NULL);
    ioPadInit(7);
    initVideo();
    resetGame();

    while (!exitRequested) {
        sysUtilCheckCallback();
        readInput();

        if (alive) {
            frameCount++;
            if (frameCount >= MOVE_EVERY) {
                frameCount = 0;
                stepGame();
            }
        }

        drawFrame();
        waitFlipReady();
        flip();
    }

    ioPadEnd();
    return 0;
}
