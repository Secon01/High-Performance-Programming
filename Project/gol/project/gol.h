#ifndef GOL_H
#define GOL_H

#include <stdint.h>

/* Basic grid utilities. */
void fill_zero(uint8_t *grid, int width, int height);
void initialize_grid(double alive_probability, uint8_t *grid, int width, int height);
void print_grid(const uint8_t *grid, int width, int height);

/* Game of Life update helpers. */
int count_neighbors(const uint8_t *grid, int row, int col, int width, int height);
void set_borders_dead(uint8_t *grid, int width, int height);
void step_range(const uint8_t *grid, uint8_t *next, int width, int start_row, int end_row);
void step(const uint8_t *grid, uint8_t *next, int width, int height);

#endif
