#include <stdint.h>
void fill_zero(uint8_t *grid, int W, int H);
void initialize_grid(double p, uint8_t *grid, int W, int H);
void print_grid(uint8_t *grid, int W, int H);
int count_neighbors(uint8_t *grid, int i, int j, int W, int H);
void step(uint8_t *grid, uint8_t *next, int W, int H);
void set_borders_dead(uint8_t *next, int W, int H);
void step_range(uint8_t *grid, uint8_t *next, int W, int start_row, int end_row);