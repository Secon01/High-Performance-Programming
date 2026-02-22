#include "gol.h"

// Fill grid with zeros using a single linear loop (avoids i*W+j multiplications)
void fill_zero(uint8_t *grid, int W, int H)
{
    int total = W * H;  // total number of cells (contiguous memory block)
    for (int i = 0; i < total; i++)
        grid[i] = 0;    // set each cell to dead (0)
}
// Initialize the values of the grid randomly 
// 1 stands for alive cell and 0 stands for dead cell
void initialize_grid(double p, uint8_t *grid, int W, int H)
{
    for (int i = 0; i < H; i++) 
    {
        for (int j = 0; j < W; j++) 
        {
            double r = (double)rand() / (double)RAND_MAX;   // random number
            if (r < p)
            {
                grid[i * W + j] = 1;
            } else 
            {
                grid[i * W + j] = 0;
            }
        }
    }
}
// Print the grid
void print_grid(uint8_t *grid, int W, int H)
{
    printf("\033[H\033[J");         // clear screen
    for (int i = 0; i < H; i++)
    {
        for (int j = 0; j < W; j++)
        {
            putchar(grid[i * W + j] ? '#' : ' ');
        }
        putchar('\n');
    }
    usleep(1000000);  // 100ms delay
}
#ifdef DEBUG
// Count the alive neighbors of a cell
int count_neighbors(uint8_t *grid, int i, int j, int W, int H)
{
    int N = 0;
    // Move −1, 0, or +1 in both directions to find the 8 neighbors of each cell
    for (int di = -1; di <= 1; di++)
    {
        for (int dj = -1; dj <= 1; dj++)
        {
            if (di == 0 && dj == 0) continue; // skip itself
            // Neighbor position
            int ni = i + di;   
            int nj = j + dj;
            // Out of bounds check
            if (ni < 0 || ni >= H || nj < 0 || nj >= W) continue;
            N += grid[ni * W + nj]; // count neighbors
        }        
    }
    return N;
}
#endif
// Set the value of the grid's borders to zero
void set_borders_dead(uint8_t *next, int W, int H)
{
    // If grid is too small, everything is border
    if (W <= 0 || H <= 0) return;

    for (int j = 0; j < W; j++) {
        next[0 * W + j]     = 0;
        next[(H-1) * W + j] = 0;
    }
    for (int i = 0; i < H; i++) {
        next[i * W + 0]     = 0;
        next[i * W + (W-1)] = 0;
    }
}

// Update grid for a specific range of rows
void step_range(uint8_t *grid, uint8_t *next, int W, int start_row, int end_row)
{
    // update only interior rows in [start_row, end_row)
    for (int i = start_row; i < end_row; i++)
    {
        int imW = (i-1)*W;
        int iW  = i*W;
        int ipW = (i+1)*W;

        for (int j = 1; j < W-1; j++)
        {
            int idx = iW + j;
            uint8_t cell = grid[idx];

            int n =
                grid[imW + (j-1)] + grid[imW + j] + grid[imW + (j+1)] +
                grid[iW  + (j-1)] +                 grid[iW  + (j+1)] +
                grid[ipW + (j-1)] + grid[ipW + j] + grid[ipW + (j+1)];

            if (cell == 1) next[idx] = (n == 2 || n == 3);
            else           next[idx] = (n == 3);
        }
    }
}

// Compute next generation
void step(uint8_t *grid, uint8_t *next, int W, int H)
{
    // If there is no interior, everything is border -> fixed-dead
    if (W <= 2 || H <= 2) 
    {
        fill_zero(next, W, H);
        return;
    }
    set_borders_dead(next, W, H);           // make borders dead     
    step_range(grid, next, W, 1, H-1);   // interior rows only
}