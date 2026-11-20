// main.c
#define _CRT_SECURE_NO_WARNINGS
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
    srand((unsigned int)time(NULL));

    GameState state;
    init_game(&state);

    printf("========================================\n");
    printf("   ウゴクトル（UGOKUTORU）コンソールモック\n");
    printf("========================================\n");

    run_game_loop(&state);

    return 0;
}