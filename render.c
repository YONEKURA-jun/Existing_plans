// render.c
#include "render.h"
#include "game.h"
#include <stdio.h>

static char role_symbol(RoleType role) {
    switch (role) {
    case ROLE_HERO:   return 'H'; // 勇者
    case ROLE_THIEF:  return 'T'; // 盗賊
    case ROLE_CLERIC: return 'C'; // 僧侶
    case ROLE_MAGE:   return 'M'; // 魔法使い
    default:          return '?';
    }
}

void render_board(const GameState* state) {
    printf("\n=== 7x7 盤面ビュー (H:勇者 T:盗賊 C:僧侶 M:魔法使い / # : 穴 / . : カード有) ===\n");
    printf("    0 1 2 3 4 5 6\n");
    for (int y = 0; y < BOARD_SIZE; y++) {
        printf("%d | ", y);
        for (int x = 0; x < BOARD_SIZE; x++) {
            int occ = -1;
            for (int p = 0; p < PLAYER_COUNT; p++) {
                if (!state->players[p].eliminated &&
                    state->players[p].x == x && state->players[p].y == y) {
                    occ = p;
                    break;
                }
            }
            if (occ != -1) {
                printf("%c ", role_symbol(state->players[occ].role));
            }
            else if (state->is_hole[y][x]) {
                printf("# ");
            }
            else {
                printf(". ");
            }
        }
        printf("\n");
    }
    printf("===========================================================================\n");
}

void render_players_summary(const GameState* state, int current_player) {
    printf("\n--- プレイヤー状況 ---\n");
    for (int i = 0; i < PLAYER_COUNT; i++) {
        const Player* p = &state->players[i];
        printf("%s P%d [%s] 役職:%s 位置:(%d,%d) 手札:%d枚 連続パス:%d %s\n",
            (i == current_player) ? "->" : "  ",
            i + 1,
            p->name,
            role_name(p->role),
            p->x, p->y,
            p->hand_count,
            p->consecutive_passes,
            p->eliminated ? "[除外済]" : "");
    }
}

void render_discard_pile(const GameState* state) {
    printf("\n--- 使用済みカード一覧 (%d枚) ---\n", state->discard_count);
    if (state->discard_count == 0) {
        printf("  (なし)\n");
        return;
    }
    for (int i = 0; i < state->discard_count; i++) {
        printf("  %d: %s\n", i + 1, state->discard[i].name);
    }
}

void render_board_with_candidates(const GameState* state, const int cand_x[], const int cand_y[], int cand_count) {
    printf("\n=== 7x7 盤面ビュー (* : 今回の初期配置候補 / H:勇者 T:盗賊 C:僧侶 M:魔法使い) ===\n");
    printf("    0 1 2 3 4 5 6\n");
    for (int y = 0; y < BOARD_SIZE; y++) {
        printf("%d | ", y);
        for (int x = 0; x < BOARD_SIZE; x++) {
            int occ = -1;
            for (int p = 0; p < PLAYER_COUNT; p++) {
                if (!state->players[p].eliminated &&
                    state->players[p].x == x && state->players[p].y == y) {
                    occ = p;
                    break;
                }
            }
            bool is_cand = false;
            for (int c = 0; c < cand_count; c++) {
                if (cand_x[c] == x && cand_y[c] == y) {
                    is_cand = true;
                    break;
                }
            }

            if (occ != -1) {
                printf("%c ", role_symbol(state->players[occ].role));
            }
            else if (state->is_hole[y][x]) {
                printf("# ");
            }
            else if (is_cand) {
                printf("* ");
            }
            else {
                printf(". ");
            }
        }
        printf("\n");
    }
    printf("==================================================================================\n");
}