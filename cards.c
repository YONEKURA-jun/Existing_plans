// cards.c
#define _CRT_SECURE_NO_WARNINGS
#include "cards.h"
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_card(Card* c, int id, CardType type, const char* name, int score) {
    c->id = id;
    c->type = type;
    strncpy(c->name, name, sizeof(c->name) - 1);
    c->name[sizeof(c->name) - 1] = '\0';
    c->score_value = score;
}

void init_card_deck(Card deck[TOTAL_CARDS]) {
    int idx = 0;

    set_card(&deck[idx++], idx, CARD_ZODIAC_BODY, "封印されし猫ゾディア（本体）", 0);
    set_card(&deck[idx++], idx, CARD_ZODIAC_R_ARM, "封印されし猫ゾディア（右手）", 0);
    set_card(&deck[idx++], idx, CARD_ZODIAC_L_ARM, "封印されし猫ゾディア（左手）", 0);
    set_card(&deck[idx++], idx, CARD_ZODIAC_R_LEG, "封印されし猫ゾディア（右足）", 0);
    set_card(&deck[idx++], idx, CARD_ZODIAC_L_LEG, "封印されし猫ゾディア（左足）", 0);

    for (int i = 0; i < 10; i++) set_card(&deck[idx++], idx, CARD_GOLD, "金塊", 1);
    for (int i = 0; i < 5; i++) set_card(&deck[idx++], idx, CARD_CURSE_DOLL, "呪いの人形", -1);
    for (int i = 0; i < 5; i++) set_card(&deck[idx++], idx, CARD_SWAP, "すりかえ", 0);
    for (int i = 0; i < 5; i++) set_card(&deck[idx++], idx, CARD_JOB_CHANGE, "ジョブチェンジ", 0);
    for (int i = 0; i < 4; i++) set_card(&deck[idx++], idx, CARD_TELEPORT, "テレポート", 0);
    for (int i = 0; i < 4; i++) set_card(&deck[idx++], idx, CARD_ILLUSION, "イリュージョン", 0);
    for (int i = 0; i < 4; i++) set_card(&deck[idx++], idx, CARD_MAGIC_HAND, "マジックハンド", 0);
    for (int i = 0; i < 3; i++) set_card(&deck[idx++], idx, CARD_DANCE, "踊る", 0);
    for (int i = 0; i < 3; i++) set_card(&deck[idx++], idx, CARD_CHECK, "チェック", 0);
    for (int i = 0; i < 2; i++) set_card(&deck[idx++], idx, CARD_ENCORE, "アンコール", 0);
}

void shuffle_and_exclude_one(Card deck[TOTAL_CARDS], Card board_cards[BOARD_TILES], Card* excluded_card) {
    for (int i = TOTAL_CARDS - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Card tmp = deck[i];
        deck[i] = deck[j];
        deck[j] = tmp;
    }
    *excluded_card = deck[0];
    for (int i = 0; i < BOARD_TILES; i++) {
        board_cards[i] = deck[i + 1];
    }
}

const char* card_type_name(CardType type) {
    switch (type) {
    case CARD_ZODIAC_BODY:  return "猫ゾディア(本体)";
    case CARD_ZODIAC_R_ARM: return "猫ゾディア(右手)";
    case CARD_ZODIAC_L_ARM: return "猫ゾディア(左手)";
    case CARD_ZODIAC_R_LEG: return "猫ゾディア(右足)";
    case CARD_ZODIAC_L_LEG: return "猫ゾディア(左足)";
    case CARD_GOLD:         return "金塊(+1)";
    case CARD_CURSE_DOLL:   return "呪いの人形(-1)";
    case CARD_SWAP:         return "すりかえ";
    case CARD_JOB_CHANGE:   return "ジョブチェンジ";
    case CARD_TELEPORT:     return "テレポート";
    case CARD_ILLUSION:     return "イリュージョン";
    case CARD_MAGIC_HAND:   return "マジックハンド";
    case CARD_CHECK:        return "チェック";
    case CARD_DANCE:        return "踊る";
    case CARD_ENCORE:       return "アンコール";
    default:                return "なし";
    }
}

bool has_complete_zodiac(const Card hand[], int hand_count) {
    bool body = false, ra = false, la = false, rl = false, ll = false;
    for (int i = 0; i < hand_count; i++) {
        switch (hand[i].type) {
        case CARD_ZODIAC_BODY:  body = true; break;
        case CARD_ZODIAC_R_ARM: ra = true; break;
        case CARD_ZODIAC_L_ARM: la = true; break;
        case CARD_ZODIAC_R_LEG: rl = true; break;
        case CARD_ZODIAC_L_LEG: ll = true; break;
        default: break;
        }
    }
    return (body && ra && la && rl && ll);
}

int calculate_player_score(const Card hand[], int hand_count) {
    int score = 0;
    for (int i = 0; i < hand_count; i++) {
        score += hand[i].score_value;
    }
    return score;
}

// モック用の基本効果ディスパッチ（各効果の詳細は段階的に拡張）
bool use_card_effect(GameState* state, int player_idx, int hand_idx) {
    if (!state || hand_idx < 0 || hand_idx >= state->players[player_idx].hand_count) {
        return false;
    }

    Card used = state->players[player_idx].hand[hand_idx];
    remove_card_from_hand(&state->players[player_idx], hand_idx);
    add_to_discard(state, used);

    printf("  >> [%s] が「%s」を使用しました！\n", state->players[player_idx].name, used.name);

    switch (used.type) {
    case CARD_DANCE:
    case CARD_ENCORE:
        printf("     踊り/アンコール効果：パス扱いにならず手番を締めくくれます。\n");
        break;
    case CARD_TELEPORT: {
        int tx, ty;
        printf("     テレポート先 (x y, 0..6): ");
        if (scanf("%d %d", &tx, &ty) == 2 && can_move_to(state, player_idx, tx, ty, true)) {
            move_player_piece(state, player_idx, tx, ty);
        }
        else {
            while (getchar() != '\n');
            printf("     移動先が不正だったため移動は不発です。\n");
        }
        break;
    }
    default:
        printf("     （※このカードの詳細インタラクションはモック拡張予定）\n");
        break;
    }
    return true;
}