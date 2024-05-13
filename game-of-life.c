#include <stdio.h>
#include <unistd.h>
#include <termios.h>

const int GRID_SIZE = 10;

void game_of_life(int grid[GRID_SIZE][GRID_SIZE]) {
  int new_grid[GRID_SIZE][GRID_SIZE];
  for (int i = 0; i < GRID_SIZE; i++) {
    for (int j = 0; j < GRID_SIZE; j++) {
      int neighbors = 0;
      for (int y = i - 1; y <= i + 1; y++) {
        for (int x = j - 1; x <= j + 1; x++) {
          if (y >= 0 && y < GRID_SIZE && x >= 0 && x < GRID_SIZE && !(y == i && x == j)) {
            neighbors += grid[y][x];
          }
        }
      }
      new_grid[i][j] = grid[i][j] ? (neighbors == 2 || neighbors == 3) : (neighbors == 3);
    }
  }
  for (int i = 0; i < GRID_SIZE; i++) {
    for (int j = 0; j < GRID_SIZE; j++) {
      grid[i][j] = new_grid[i][j];
    }
  }
}

int printed = 0;
void print_grid(int x, int y, int grid[GRID_SIZE][GRID_SIZE]) {
  if (printed) { printf("\033[%dA", GRID_SIZE+1); }
  printed = 1;
  for (int i = 0; i < GRID_SIZE; i++) {
    for (int j = 0; j < GRID_SIZE; j++) {
      if (y == i && j == x) {
        printf("\033[7m");
        printf(grid[i][j] ? "X" : ".");
        printf("\033[m");
      }
      else {
        printf(grid[i][j] ? "X" : ".");
      }
    }
    printf("\n");
  }
  printf("j/k/h/l: move, space: toggle, n: next, q: quit\n");
}

int main() {
  int grid[GRID_SIZE][GRID_SIZE];
  for (int i = 0; i < GRID_SIZE; i++) {
    for (int j = 0; j < GRID_SIZE; j++) {
      grid[i][j] = 0;
    }
  }

  struct termios original, raw;
  tcgetattr(STDIN_FILENO, &original);
  raw = original;
  raw.c_lflag &= ~(ECHO | ICANON);
  printf("\033[?25l"); // disable cursor
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

  int x = 0;
  int y = 0;
  print_grid(x, y, grid);

  char c;
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
      if (c == 'j') { y++; }
      else if (c == 'k') { y--; }
      else if (c == 'h') { x--; }
      else if (c == 'l') { x++; }
      else if (c == ' ') { grid[y][x] = !grid[y][x]; }
      else if (c == 'n') { game_of_life(grid); }

      if (x < 0) { x = 0; }
      else if (x >= GRID_SIZE) { x = GRID_SIZE - 1; }
      if (y < 0) { y = 0; }
      else if (y >= GRID_SIZE) { y = GRID_SIZE - 1; }
      print_grid(x, y, grid);
  }
  printf("\033[?25h"); // show cursor
  return 0;
}
