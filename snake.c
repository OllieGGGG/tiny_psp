#include "display.h"
#include "buttons.h"

#define CELL_SIZE     (15)
#define GRID_WIDTH    (DISP_WIDTH / CELL_SIZE)
#define GRID_HEIGHT   (DISP_HEIGHT / CELL_SIZE)
#define MAX_SNAKE_LEN (GRID_WIDTH * GRID_HEIGHT)

#define SNAKE_STEP_MS (200)

#define COLOR_BG   BRED
#define COLOR_HEAD RED
#define COLOR_BODY WHITE
#define COLOR_FOOD BLUE

enum __attribute__((packed)) Direction {
    DirUp = 2,
    DirDown,
    DirLeft,
    DirRight,
};

struct Cell {
    uint8_t x;
    uint8_t y;
};

struct Snake {
    struct Cell body[MAX_SNAKE_LEN];
    uint16_t len;
    enum Direction dir;
    enum Direction next_dir;
    bool alive;
};

struct GameState {
    struct Snake snake;
    struct Cell food;
    uint16_t score;
};

static uint32_t rand32(void) {
    static uint32_t g_rand_state = 0x12345678;
    g_rand_state = g_rand_state * 1664525u + 1013904223u;
    return g_rand_state;
}

static bool cell_equal(struct Cell a, struct Cell b) {
    return (a.x == b.x) && (a.y == b.y);
}

static bool is_opposite_dir(enum Direction a, enum Direction b) {
    return ((a == DirUp && b == DirDown) ||
            (a == DirDown && b == DirUp) ||
            (a == DirLeft && b == DirRight) ||
            (a == DirRight && b == DirLeft));
}

static bool snake_contains(const struct Snake *snake, struct Cell cell) {
    for (uint16_t i = 0; i < snake->len; i++) {
        if (cell_equal(snake->body[i], cell)) {
            return true;
        }
    }
    return false;
}

static void snake_reset(struct Snake *snake) {
    snake->len = 3;
    snake->dir = DirRight;
    snake->next_dir = DirRight;
    snake->alive = true;

    snake->body[0].x = GRID_WIDTH / 2;
    snake->body[0].y = GRID_HEIGHT / 2;

    snake->body[1].x = (GRID_WIDTH / 2) - 1;
    snake->body[1].y = GRID_HEIGHT / 2;

    snake->body[2].x = (GRID_WIDTH / 2) - 2;
    snake->body[2].y = GRID_HEIGHT / 2;
}

static void spawn_food(struct GameState *game) {
    struct Cell c;

    do {
        c.x = rand32() % GRID_WIDTH;
        c.y = rand32() % GRID_HEIGHT;
    } while (snake_contains(&game->snake, c));

    game->food = c;
}

static void game_reset(struct GameState *game) {
    snake_reset(&game->snake);
    game->score = 0;
    spawn_food(game);
}

static void handle_input(struct Snake *snake, enum ButtonEvent btn_event) {
    enum Direction requested = snake->next_dir;
    bool changed = false;

    switch (btn_event) {
        case ButtonEventLUpPress: requested = DirUp; changed = true; break;
        case ButtonEventLDownPress: requested = DirDown; changed = true; break;
        case ButtonEventLLeftPress: requested = DirLeft; changed = true; break;
        case ButtonEventLRightPress: requested = DirRight; changed = true; break;
        default: break;
    }

    if (changed && !is_opposite_dir(snake->dir, requested)) {
        snake->next_dir = requested;
    }
}

static struct Cell get_next_head(const struct Snake *snake) {
    struct Cell head = snake->body[0];

    switch (snake->dir) {
        case DirUp:
            if (head.y > 0) {
                head.y--;
            } else {
                head.y = 255;
            }
            break;
        case DirDown:
            head.y++;
            break;
        case DirLeft:
            if (head.x > 0) {
                head.x--;
            } else {
                head.x = 255;
            }
            break;
        case DirRight:
            head.x++;
            break;
    }

    return head;
}

static bool hits_wall(struct Cell c) {
    return (c.x >= GRID_WIDTH || c.y >= GRID_HEIGHT);
}

static bool hits_self(const struct Snake *snake, struct Cell head) {
    for (uint16_t i = 0; i < snake->len; i++) {
        if (cell_equal(snake->body[i], head)) {
            return true;
        }
    }
    return false;
}

static void snake_step(struct GameState *game, bool food) {
    struct Snake *snake = &game->snake;
    if (!snake->alive) {
        return;
    }

    snake->dir = snake->next_dir;
    struct Cell new_head = get_next_head(snake);

    if (hits_wall(new_head)) {
        snake->alive = false;
        return;
    }

    if (hits_self(snake, new_head)) {
        snake->alive = false;
        return;
    }

    bool grow = cell_equal(new_head, game->food);

    if (grow) {
        if (snake->len < MAX_SNAKE_LEN) {
            for (int i = snake->len; i > 0; i--) {
                snake->body[i] = snake->body[i - 1];
            }
            snake->body[0] = new_head;
            snake->len++;
        } else {
            for (int i = snake->len - 1; i > 0; i--) {
                snake->body[i] = snake->body[i - 1];
            }
            snake->body[0] = new_head;
        }

        game->score++;
        if (food) {
            spawn_food(game);
        }
    } else {
        for (int i = snake->len - 1; i > 0; i--) {
            snake->body[i] = snake->body[i - 1];
        }
        snake->body[0] = new_head;
    }
}

static void draw_cell(struct Cell c, uint16_t color) {
    uint16_t x0 = c.x * CELL_SIZE;
    uint16_t y0 = c.y * CELL_SIZE;
    uint16_t x1 = x0 + CELL_SIZE - 1;
    uint16_t y1 = y0 + CELL_SIZE - 1;

    disp_draw_rectangle(x0, y0, x1, y1, color);
}

static void draw_game(const struct GameState *game) {
    disp_clear(COLOR_BG);

    draw_cell(game->food, COLOR_FOOD);

    for (uint16_t i = 0; i < game->snake.len; i++) {
        if (i == 0) {
            draw_cell(game->snake.body[i], COLOR_HEAD);
        } else {
            draw_cell(game->snake.body[i], COLOR_BODY);
        }
    }

    disp_render();
}

