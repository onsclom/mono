#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

const int MAX_LEVEL_SIZE = 100; // 10x10 level

typedef struct {
  int width;
  int height;
  char data[MAX_LEVEL_SIZE];
} level;

typedef enum {
  WALL,
  PLAYER,
  CONNECTOR,
  END,
} entity_type;

typedef struct {
  int x;
  int y;
  entity_type type;
} entity;

typedef struct {
  level l;
  entity entities[MAX_LEVEL_SIZE];
  int entity_count;
  int won;
} game;

game game_create(char *level_chars) {
  game g = {0};
  {
    int x = 0;
    int y = 0;
    for (int i = 0; level_chars[i] != '\0'; i++) {
      if (level_chars[i] == '\n') {
        g.l.width = x;
        x = 0;
        y++;
      }
      else {
        g.l.data[x + y * g.l.width] = level_chars[i];
        x++;
      }
    }
    g.l.height = y+1;
    g.l.width = x;
  }

  int entity_count = 0;
  for (int i = 0; i < MAX_LEVEL_SIZE; i++) {
    if (g.l.data[i] == '#') {
      g.entities[entity_count] = (entity){i % g.l.width, i / g.l.width, WALL};
      entity_count++;
    }
    else if (g.l.data[i] == 'P') {
      g.entities[entity_count] = (entity){i % g.l.width, i / g.l.width, PLAYER};
      entity_count++;
    }
    else if (g.l.data[i] == 'C') {
      g.entities[entity_count] = (entity){i % g.l.width, i / g.l.width, CONNECTOR};
      entity_count++;
    }
    else if (g.l.data[i] == 'E') {
      g.entities[entity_count] = (entity){i % g.l.width, i / g.l.width, END};
      entity_count++;
    }
  }
  g.entity_count = entity_count;

  return g;
}

int lines_to_clear = 0;
void game_print(game *g) {
  if (lines_to_clear) { printf("\033[%dA", lines_to_clear); }
  lines_to_clear = 0;

  // init grid to .
  char grid[MAX_LEVEL_SIZE][MAX_LEVEL_SIZE];
  for (int i = 0; i < g->l.height; i++) {
    for (int j = 0; j < g->l.width; j++) {
      grid[i][j] = '.';
    }
  }

  // add entities
  for (int i = 0; i < g->entity_count; i++) {
    entity e = g->entities[i];
    if (e.type == CONNECTOR) { grid[e.y][e.x] = 'C'; }
    else if (e.type == PLAYER) { grid[e.y][e.x] = 'P'; }
    else if (e.type == WALL) { grid[e.y][e.x] = '#'; }
    else if (e.type == END) { grid[e.y][e.x] = 'E'; }
  }

  // print grid
  for (int i = 0; i < g->l.height; i++) {
    for (int j = 0; j < g->l.width; j++) {
      printf("%c", grid[i][j]);
    }
    printf("\n");
  }
  lines_to_clear += g->l.height;
  printf("move: hjkl\n"
         "undo: u\n");
  lines_to_clear += 2;
}

void game_handle_input(game *g, char c) {
  int x = 0;
  int y = 0;
  if (c == 'h') { x = -1; }
  else if (c == 'j') { y = 1; }
  else if (c == 'k') { y = -1; }
  else if (c == 'l') { x = 1; }

  // copy entities
  entity old_entities[MAX_LEVEL_SIZE];
  memcpy(old_entities, g->entities, sizeof(g->entities));
  for (int i = 0; i < MAX_LEVEL_SIZE; i++) {
    if (g->entities[i].type == PLAYER) {
      const int new_x = g->entities[i].x + x;
      const int new_y = g->entities[i].y + y;
      const int in_bounds = new_x >= 0 && new_x < g->l.width && new_y >= 0 && new_y < g->l.height;
      const int is_wall = g->l.data[new_y * g->l.width + new_x] == '#';
      if (!in_bounds || is_wall) {
        memcpy(g->entities, old_entities, sizeof(g->entities));
        break;
      }

      g->entities[i].x = new_x;
      g->entities[i].y = new_y;
    }
  }

  // for every player
  for (int i = 0; i < g->entity_count; i++) {
    if (g->entities[i].type == PLAYER) {
      entity p = g->entities[i];
      // loop through all entities to find connectors to activate
      for (int j = 0; j < g->entity_count; j++) {
        entity e = g->entities[j];
        const int manhattan_distance = abs(e.x - p.x) + abs(e.y - p.y);
        if (e.type == CONNECTOR && manhattan_distance == 1) {
          g->entities[j].type = PLAYER;
        }
      }
    }
  }

  // check if player won
  // for every end, check if an entity is covering it
  for (int i = 0; i < g->entity_count; i++) {
    if (g->entities[i].type == END) {
      entity e = g->entities[i];
      int covered = 0;
      for (int j = 0; j < g->entity_count; j++) {
        if (i == j) { continue; }
        if (g->entities[j].x == e.x && g->entities[j].y == e.y) {
          covered = 1;
          break;
        }
      }
      if (!covered) { return; }
    }
  }
  g->won = 1;
}

void signal_handler(int sig) {
  (void)sig;
  printf("\033[?25h"); // enable cursor
  struct termios original;
  tcgetattr(STDIN_FILENO, &original);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
  exit(0);
}

int main(void) {
  struct termios original, raw;
  tcgetattr(STDIN_FILENO, &original);
  raw = original;
  raw.c_lflag &= ~(ECHO | ICANON);
  printf("\033[?25l"); // disable cursor
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

  int signals[] = { SIGINT, SIGTSTP, SIGTERM, SIGHUP, SIGQUIT, SIGABRT, SIGSEGV, SIGCONT };
  int num_signals = sizeof(signals) / sizeof(signals[0]);
  for (int i = 0; i < num_signals; i++) {
      signal(signals[i], signal_handler);
  }

  game game_state = game_create(
    "###.EE\n"
    "##..E.\n"
    "....E.\n"
    ".C....\n"
    ".#..C.\n"
    ".C....\n"
    ".....P"
  );
  game_print(&game_state);

  char c;
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
    game_handle_input(&game_state, c);
    game_print(&game_state);
    if (game_state.won) { break; }
  }
  printf("\033[?25h"); // show cursor
  printf("YOU WIN!\n");
  return 0;
}
