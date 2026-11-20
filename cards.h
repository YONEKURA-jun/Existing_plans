#pragma once
// cards.h
#ifndef UGOKUTORU_CARDS_H
#define UGOKUTORU_CARDS_H

#include <stdbool.h>

#define TOTAL_CARDS 50
#define BOARD_SIZE 7
#define BOARD_TILES (BOARD_SIZE * BOARD_SIZE) // 49
#define MAX_HAND_SIZE 32
#define MAX_DISCARD_SIZE 64
#define ZODIAC_PARTS 5

typedef enum {
    CARD_NONE = 0,
    CARD_ZODIAC_BODY,
    CARD_ZODIAC_R_ARM,
    CARD_ZODIAC_L_ARM,
    CARD_ZODIAC_R_LEG,
    CARD_ZODIAC_L_LEG,
    CARD_GOLD,          // +1点 (10枚)
    CARD_CURSE_DOLL,    // -1点 (5枚)
    CARD_SWAP,          // すりかえ (5枚)
    CARD_JOB_CHANGE,    // ジョブチェンジ (5枚)
    CARD_TELEPORT,      // テレポート (4枚)
    CARD_ILLUSION,      // イリュージョン (4枚)
    CARD_MAGIC_HAND,    // マジックハンド (4枚)
    CARD_CHECK,         // チェック (3枚)
    CARD_DANCE,         // 踊る (3枚)
    CARD_ENCORE         // アンコール (2枚)
} CardType;

typedef struct {
    int id;
    CardType type;
    char name[64];
    int score_value; // 金塊:+1, 呪いの人形:-1, その他:0
} Card;

// 前方宣言
typedef struct GameState GameState;

void init_card_deck(Card deck[TOTAL_CARDS]);
void shuffle_and_exclude_one(Card deck[TOTAL_CARDS], Card board_cards[BOARD_TILES], Card* excluded_card);
const char* card_type_name(CardType type);
bool has_complete_zodiac(const Card hand[], int hand_count);
int calculate_player_score(const Card hand[], int hand_count);
bool use_card_effect(GameState* state, int player_idx, int hand_idx);

#endif