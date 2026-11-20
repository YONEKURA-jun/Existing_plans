#pragma once
// game.h
#ifndef UGOKUTORU_GAME_H
#define UGOKUTORU_GAME_H

#include "cards.h"
#include <stdbool.h>

#define PLAYER_COUNT 4

typedef enum {
    ROLE_HERO = 0,   // 勇者（王将：全方向1マス）
    ROLE_THIEF,      // 盗賊（ナイト：L字ジャンプ・飛び越え可）
    ROLE_CLERIC,     // 僧侶（ビショップ：斜め無制限・飛び越え不可）
    ROLE_MAGE        // 魔法使い（ルーク：縦横無制限・飛び越え不可）
} RoleType;

typedef struct {
    int id;
    char name[32];
    RoleType role;
    int x;
    int y;
    Card hand[MAX_HAND_SIZE];
    int hand_count;
    int consecutive_passes;
    bool eliminated;
} Player;

struct GameState {
    Player players[PLAYER_COUNT];
    Card board_cards[BOARD_TILES];
    bool card_taken[BOARD_TILES];
    bool is_hole[BOARD_SIZE][BOARD_SIZE];
    Card excluded_card; // 最初にランダム除外した1枚
    Card discard[MAX_DISCARD_SIZE];
    int discard_count;
    int hero_start_index; // 開始時の勇者席順（手番順の基準）
    int current_turn_idx;
    bool end_countdown_active;
    int remaining_end_turns;
    bool game_over;
    int winner_idx; // -1 で引き分け
};

const char* role_name(RoleType role);
void init_game(GameState* state);
void setup_initial_positions(GameState* state);
bool can_move_to(const GameState* state, int player_idx, int tx, int ty, bool ignore_role_rule);
void move_player_piece(GameState* state, int player_idx, int tx, int ty);
bool take_adjacent_card(GameState* state, int player_idx, int tx, int ty);
void remove_card_from_hand(Player* player, int hand_idx);
void add_to_discard(GameState* state, Card card);
void run_game_loop(GameState* state);
// game.h（初期配置ヘルパー関数の追加）
void get_corner_for_player(int player_idx, int* corner_x, int* corner_y);
void get_initial_candidates(int player_idx, int cand_x[4], int cand_y[4]);
bool is_valid_initial_position(const GameState* state, int player_idx, int x, int y);

#endif