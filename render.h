#pragma once
// render.h
#ifndef UGOKUTORU_RENDER_H
#define UGOKUTORU_RENDER_H

typedef struct GameState GameState;

void render_board(const GameState* state);
void render_players_summary(const GameState* state, int current_player);
void render_discard_pile(const GameState* state);
void render_board_with_candidates(const GameState* state, const int cand_x[], const int cand_y[], int cand_count);

#endif