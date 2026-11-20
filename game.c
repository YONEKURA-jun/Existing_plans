// game.c（コア進行・移動・取得・パス除外・勝敗判定）
#define _CRT_SECURE_NO_WARNINGS
#include "game.h"
#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* role_name(RoleType role) {
    switch (role) {
    case ROLE_HERO:   return "勇者";
    case ROLE_THIEF:  return "盗賊";
    case ROLE_CLERIC: return "僧侶";
    case ROLE_MAGE:   return "魔法使い";
    default:          return "不明";
    }
}

static bool is_in_bounds(int x, int y) {
    return (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE);
}

static bool is_occupied_by_other(const GameState* state, int player_idx, int x, int y) {
    for (int i = 0; i < PLAYER_COUNT; i++) {
        if (i == player_idx || state->players[i].eliminated) continue;
        if (state->players[i].x == x && state->players[i].y == y) return true;
    }
    return false;
}

static bool is_path_clear(const GameState* state, int sx, int sy, int tx, int ty) {
    int dx = (tx > sx) ? 1 : ((tx < sx) ? -1 : 0);
    int dy = (ty > sy) ? 1 : ((ty < sy) ? -1 : 0);
    int cx = sx + dx;
    int cy = sy + dy;
    while (cx != tx || cy != ty) {
        if (state->is_hole[cy][cx]) return false;
        for (int i = 0; i < PLAYER_COUNT; i++) {
            if (!state->players[i].eliminated &&
                state->players[i].x == cx && state->players[i].y == cy) {
                return false;
            }
        }
        cx += dx;
        cy += dy;
    }
    return true;
}

bool can_move_to(const GameState* state, int player_idx, int tx, int ty, bool ignore_role_rule) {
    if (!is_in_bounds(tx, ty)) return false;
    if (state->is_hole[ty][tx]) return false;
    if (is_occupied_by_other(state, player_idx, tx, ty)) return false;
    if (ignore_role_rule) return true;

    const Player* p = &state->players[player_idx];
    int dx = abs(tx - p->x);
    int dy = abs(ty - p->y);
    if (dx == 0 && dy == 0) return false;

    switch (p->role) {
    case ROLE_HERO: // 王将：全方向1マス
        return (dx <= 1 && dy <= 1);
    case ROLE_THIEF: // ナイト：L字ジャンプ（穴・他駒を飛び越え可）
        return ((dx == 1 && dy == 2) || (dx == 2 && dy == 1));
    case ROLE_CLERIC: // ビショップ：斜め無制限（穴・他駒を飛び越え不可）
        if (dx != dy) return false;
        return is_path_clear(state, p->x, p->y, tx, ty);
    case ROLE_MAGE: // ルーク：縦横無制限（穴・他駒を飛び越え不可）
        if (dx != 0 && dy != 0) return false;
        return is_path_clear(state, p->x, p->y, tx, ty);
    default:
        return false;
    }
}

void move_player_piece(GameState* state, int player_idx, int tx, int ty) {
    state->players[player_idx].x = tx;
    state->players[player_idx].y = ty;
    printf("  -> [%s] が (%d, %d) へ移動しました。\n", state->players[player_idx].name, tx, ty);
}

bool take_adjacent_card(GameState* state, int player_idx, int tx, int ty) {
    if (!is_in_bounds(tx, ty)) return false;
    Player* p = &state->players[player_idx];
    int dx = abs(tx - p->x);
    int dy = abs(ty - p->y);
    if (dx > 1 || dy > 1 || (dx == 0 && dy == 0)) return false; // 8近傍チェック
    if (state->is_hole[ty][tx]) return false;
    if (p->x == tx && p->y == ty) return false;
    if (is_occupied_by_other(state, player_idx, tx, ty)) return false;

    int idx = ty * BOARD_SIZE + tx;
    if (state->card_taken[idx]) return false;

    Card got = state->board_cards[idx];
    state->card_taken[idx] = true;
    state->is_hole[ty][tx] = true;

    if (p->hand_count < MAX_HAND_SIZE) {
        p->hand[p->hand_count++] = got;
    }
    printf("  -> [%s] が (%d, %d) から「%s」を取得しました！（マスは穴になります）\n",
        p->name, tx, ty, got.name);
    return true;
}

void remove_card_from_hand(Player* player, int hand_idx) {
    for (int i = hand_idx; i < player->hand_count - 1; i++) {
        player->hand[i] = player->hand[i + 1];
    }
    player->hand_count--;
}

void add_to_discard(GameState* state, Card card) {
    if (state->discard_count < MAX_DISCARD_SIZE) {
        state->discard[state->discard_count++] = card;
    }
}

void init_game(GameState* state) {
    memset(state, 0, sizeof(GameState));
    state->winner_idx = -1;

    Card deck[TOTAL_CARDS];
    init_card_deck(deck);
    shuffle_and_exclude_one(deck, state->board_cards, &state->excluded_card);

    RoleType roles[PLAYER_COUNT] = { ROLE_HERO, ROLE_THIEF, ROLE_CLERIC, ROLE_MAGE };
    for (int i = PLAYER_COUNT - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        RoleType tmp = roles[i];
        roles[i] = roles[j];
        roles[j] = tmp;
    }

    for (int i = 0; i < PLAYER_COUNT; i++) {
        state->players[i].id = i;
        snprintf(state->players[i].name, sizeof(state->players[i].name), "プレイヤー%d", i + 1);
        state->players[i].role = roles[i];
        if (roles[i] == ROLE_HERO) {
            state->hero_start_index = i;
        }
    }
    state->current_turn_idx = state->hero_start_index;
}

// 各プレイヤー（席順 0..3）に対応する「自分に一番近い角」を返す
// 0: 左上(0,0), 1: 右上(6,0), 2: 右下(6,6), 3: 左下(0,6)
void get_corner_for_player(int player_idx, int* corner_x, int* corner_y) {
    switch (player_idx % PLAYER_COUNT) {
    case 0: *corner_x = 0; *corner_y = 0; break;
    case 1: *corner_x = BOARD_SIZE - 1; *corner_y = 0; break;
    case 2: *corner_x = BOARD_SIZE - 1; *corner_y = BOARD_SIZE - 1; break;
    case 3: *corner_x = 0; *corner_y = BOARD_SIZE - 1; break;
    default: *corner_x = 0; *corner_y = 0; break;
    }
}

static const char* corner_label(int player_idx) {
    switch (player_idx % PLAYER_COUNT) {
    case 0: return "左上角";
    case 1: return "右上角";
    case 2: return "右下角";
    case 3: return "左下角";
    default: return "角";
    }
}

// 基準となる角(1マス)＋その周囲(3マス)の計4候補を返す
void get_initial_candidates(int player_idx, int cand_x[4], int cand_y[4]) {
    int cx = 0, cy = 0;
    get_corner_for_player(player_idx, &cx, &cy);

    int dx = (cx == 0) ? 1 : -1;
    int dy = (cy == 0) ? 1 : -1;

    cand_x[0] = cx;      cand_y[0] = cy;      // 0: 角そのもの
    cand_x[1] = cx + dx; cand_y[1] = cy;      // 1: 横隣
    cand_x[2] = cx;      cand_y[2] = cy + dy; // 2: 縦隣
    cand_x[3] = cx + dx; cand_y[3] = cy + dy; // 3: 斜め隣
}

bool is_valid_initial_position(const GameState* state, int player_idx, int x, int y) {
    if (!is_in_bounds(x, y) || is_occupied_by_other(state, player_idx, x, y)) {
        return false;
    }
    int cx[4], cy[4];
    get_initial_candidates(player_idx, cx, cy);
    for (int i = 0; i < 4; i++) {
        if (cx[i] == x && cy[i] == y) {
            return true;
        }
    }
    return false;
}

void setup_initial_positions(GameState* state) {
    // 配置決定順：魔法使い -> 僧侶 -> 盗賊 -> 勇者
    RoleType order[PLAYER_COUNT] = { ROLE_MAGE, ROLE_CLERIC, ROLE_THIEF, ROLE_HERO };

    printf("\n=== 初期配置フェーズ ===\n");
    printf("各プレイヤーは「自分に一番近い角＋周囲3マス（計4マス）」から1マスを選択します。\n");
    printf("配置順: 魔法使い -> 僧侶 -> 盗賊 -> 勇者\n");

    for (int o = 0; o < PLAYER_COUNT; o++) {
        for (int p = 0; p < PLAYER_COUNT; p++) {
            if (state->players[p].role != order[o]) continue;

            int cx[4], cy[4];
            get_initial_candidates(p, cx, cy);

            render_board_with_candidates(state, cx, cy, 4);
            printf("\n[%s (P%d) / %s] 基準角: %s (%d, %d)\n",
                state->players[p].name,
                p + 1,
                role_name(state->players[p].role),
                corner_label(p),
                cx[0], cy[0]);
            printf("選択可能な4マス:\n");
            for (int i = 0; i < 4; i++) {
                bool taken = is_occupied_by_other(state, p, cx[i], cy[i]);
                const char* tag = (i == 0) ? "[角]" : "[周囲]";
                printf("  %d: (%d, %d) %s %s\n",
                    i + 1, cx[i], cy[i], tag, taken ? "(占有済)" : "(選択可)");
            }

            int pick = 0;
            while (true) {
                printf("番号を入力してください (1-4): ");
                if (scanf("%d", &pick) != 1) {
                    while (getchar() != '\n');
                    printf("  [再入力] 1?4の番号を入力してください。\n");
                    continue;
                }
                if (pick < 1 || pick > 4) {
                    printf("  [再入力] 1?4の範囲で選択してください。\n");
                    continue;
                }
                int tx = cx[pick - 1];
                int ty = cy[pick - 1];
                if (!is_valid_initial_position(state, p, tx, ty)) {
                    printf("  [再入力] (%d, %d) は選択できません。別の番号を選んでください。\n", tx, ty);
                    continue;
                }
                state->players[p].x = tx;
                state->players[p].y = ty;
                printf("  -> [%s] を %s 付近の (%d, %d) に配置しました。\n",
                    state->players[p].name, corner_label(p), tx, ty);
                break;
            }
        }
    }
}

static void judge_winner(GameState* state) {
    // 1. 猫ゾディア即勝利は手番中にも判定されるが念のため確認
    for (int i = 0; i < PLAYER_COUNT; i++) {
        if (!state->players[i].eliminated && has_complete_zodiac(state->players[i].hand, state->players[i].hand_count)) {
            state->winner_idx = i;
            printf("\n[WIN] 勝利者: [%s]（封印されし猫ゾディア完成）！\n", state->players[i].name);
            return;
        }
    }

    // 2. 得点計算（除外プレイヤーは対象外・最下位）
    int best_score = -999999;
    int top_indices[PLAYER_COUNT];
    int top_count = 0;

    for (int i = 0; i < PLAYER_COUNT; i++) {
        if (state->players[i].eliminated) continue;
        int sc = calculate_player_score(state->players[i].hand, state->players[i].hand_count);
        printf("  [%s] スコア: %d点\n", state->players[i].name, sc);
        if (sc > best_score) {
            best_score = sc;
            top_count = 0;
            top_indices[top_count++] = i;
        }
        else if (sc == best_score) {
            top_indices[top_count++] = i;
        }
    }

    if (top_count == 1) {
        state->winner_idx = top_indices[0];
        printf("\n[WIN] 勝利者: [%s]（最高得点 %d点）！\n", state->players[state->winner_idx].name, best_score);
        return;
    }

    if (top_count > 1) {
        // 同率1位タイブレーク：最初に除外した1枚を当てる
        printf("\n同率1位タイブレーク！最初に除外されたカード(ID:%d)を当ててください。\n", state->excluded_card.id);
        for (int t = 0; t < top_count; t++) {
            int idx = top_indices[t];
            int guess_id = -1;
            printf("  [%s] の予想カードIDを入力: ", state->players[idx].name);
            if (scanf("%d", &guess_id) == 1 && guess_id == state->excluded_card.id) {
                state->winner_idx = idx;
                printf("\n[WIN] 勝利者: [%s]（除外カード「%s」を的中）！\n", state->players[idx].name, state->excluded_card.name);
                return;
            }
            while (getchar() != '\n');
        }
    }

    state->winner_idx = -1;
    printf("\n[DRAW] 勝敗決定に至らず【引き分け】です。\n");
}

void run_game_loop(GameState* state) {
    setup_initial_positions(state);

    int consecutive_all_pass = 0;

    while (!state->game_over) {
        int p_idx = state->current_turn_idx;
        Player* p = &state->players[p_idx];

        if (!p->eliminated) {
            render_board(state);
            render_players_summary(state, p_idx);

            bool did_move = false;
            bool did_act2 = false;
            bool chose_pass = false;

            while (true) {
                printf("\n[%s / %s] 手番アクション選択:\n", p->name, role_name(p->role));
                printf("  1: 移動する%s\n", did_move ? " (済)" : "");
                printf("  2: 周囲1マスからカード取得 / 手札カード使用%s\n", did_act2 ? " (済)" : "");
                printf("  3: 捨て札/使用済みカード確認\n");
                printf("  4: 手番を終了する\n");
                printf("  5: パスする（移動・カード使用不可時）\n");
                printf("選択: ");

                int cmd = 0;
                if (scanf("%d", &cmd) != 1) {
                    while (getchar() != '\n');
                    continue;
                }

                if (cmd == 1 && !did_move) {
                    int tx, ty;
                    printf("  移動先 (x y): ");
                    if (scanf("%d %d", &tx, &ty) == 2 && can_move_to(state, p_idx, tx, ty, false)) {
                        move_player_piece(state, p_idx, tx, ty);
                        did_move = true;
                    }
                    else {
                        while (getchar() != '\n');
                        printf("  [不可] そのマスには移動できません。\n");
                    }
                }
                else if (cmd == 2 && !did_act2) {
                    int sub = 0;
                    printf("  2-1: 周囲1マス(8近傍)のカード取得 / 2-2: 手札カード使用 (1 or 2): ");
                    if (scanf("%d", &sub) == 1 && sub == 1) {
                        int tx, ty;
                        printf("  取得するカード座標 (x y): ");
                        if (scanf("%d %d", &tx, &ty) == 2 && take_adjacent_card(state, p_idx, tx, ty)) {
                            did_act2 = true;
                            if (has_complete_zodiac(p->hand, p->hand_count)) {
                                state->game_over = true;
                                state->winner_idx = p_idx;
                                break;
                            }
                        }
                        else {
                            while (getchar() != '\n');
                            printf("  [不可] 取得できません。\n");
                        }
                    }
                    else if (sub == 2 && p->hand_count > 0) {
                        for (int h = 0; h < p->hand_count; h++) {
                            printf("    [%d] %s\n", h, p->hand[h].name);
                        }
                        int h_idx = -1;
                        printf("  使用する手札番号: ");
                        if (scanf("%d", &h_idx) == 1 && use_card_effect(state, p_idx, h_idx)) {
                            did_act2 = true;
                            if (has_complete_zodiac(p->hand, p->hand_count)) {
                                state->game_over = true;
                                state->winner_idx = p_idx;
                                break;
                            }
                        }
                        else {
                            while (getchar() != '\n');
                            printf("  [不可] カードを使用できませんでした。\n");
                        }
                    }
                }
                else if (cmd == 3) {
                    render_discard_pile(state);
                }
                else if (cmd == 4) {
                    if (!did_move && !did_act2) {
                        printf("  何も行動していない場合は「5: パス」を選んでください。\n");
                    }
                    else {
                        p->consecutive_passes = 0;
                        consecutive_all_pass = 0;
                        break;
                    }
                }
                else if (cmd == 5) {
                    p->consecutive_passes++;
                    consecutive_all_pass++;
                    chose_pass = true;
                    printf("  -> [%s] がパスしました（連続 %d 回）。\n", p->name, p->consecutive_passes);
                    break;
                }
            }

            if (state->game_over) break;

            // 2ターン連続パスによる除外判定
            if (chose_pass && p->consecutive_passes >= 2) {
                p->eliminated = true;
                p->hand_count = 0; // 除外手札は別管理・非公開扱い
                printf("  !! [%s] は2連続パスのためゲームから除外されました！\n", p->name);

                if (!state->end_countdown_active) {
                    state->end_countdown_active = true;
                    int active_others = 0;
                    for (int i = 0; i < PLAYER_COUNT; i++) {
                        if (i != p_idx && !state->players[i].eliminated) active_others++;
                    }
                    state->remaining_end_turns = active_others;
                }
            }

            // 全員パス一巡で即終了
            int active_count = 0;
            for (int i = 0; i < PLAYER_COUNT; i++) {
                if (!state->players[i].eliminated) active_count++;
            }
            if (active_count > 0 && consecutive_all_pass >= active_count) {
                printf("\n参加プレイヤー全員がパスを一巡したためゲーム終了です。\n");
                break;
            }

            if (state->end_countdown_active) {
                // 除外発生後の残り1回ずつ手番カウント
                state->remaining_end_turns--;
                if (state->remaining_end_turns <= 0) break;
            }
        }

        state->current_turn_idx = (state->current_turn_idx + 1) % PLAYER_COUNT;
    }

    judge_winner(state);
}