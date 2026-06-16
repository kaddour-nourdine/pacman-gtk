#include <gtk/gtk.h>
#include "gamemodel.h"
#include <cmath>
#include <chrono>

#define TILE_SIZE 30

static GameModel model;
static std::chrono::steady_clock::time_point last_time;

enum class AppMode { MENU, PLAYING, DEMO };
static AppMode appMode = AppMode::MENU;
static int menuSelection = 0;
static GtkWidget *mainWindow = NULL;
static double demoGameOverTimer = 0.0;
static bool prevDemoGameOver = false;

static void draw_menu(cairo_t *cr) {
    int centerX = GameModel::MAP_WIDTH * TILE_SIZE / 2;

    cairo_set_source_rgb(cr, 1, 1, 0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 52);
    cairo_text_extents_t ext;
    cairo_text_extents(cr, "PACMAN", &ext);
    cairo_move_to(cr, centerX - ext.width / 2.0, 170);
    cairo_show_text(cr, "PACMAN");

    cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
    cairo_set_font_size(cr, 14);
    cairo_text_extents(cr, "by Nourdine Kaddour", &ext);
    cairo_move_to(cr, centerX - ext.width / 2.0, 200);
    cairo_show_text(cr, "by Nourdine Kaddour");

    int optionY = 320;
    const char* labels[] = { "  Play Game", "Watch Demo" };
    for (int i = 0; i < 2; ++i) {
        if (menuSelection == i) {
            cairo_set_source_rgb(cr, 1, 1, 0);
            cairo_set_font_size(cr, 30);
        } else {
            cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
            cairo_set_font_size(cr, 22);
        }

        char buf[64];
        if (menuSelection == i)
            snprintf(buf, sizeof(buf), "> %s <", labels[i]);
        else
            snprintf(buf, sizeof(buf), "  %s  ", labels[i]);

        cairo_text_extents(cr, buf, &ext);
        cairo_move_to(cr, centerX - ext.width / 2.0, optionY + i * 60);
        cairo_show_text(cr, buf);
    }

    if (!model.hasRecording()) {
        cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
        cairo_set_font_size(cr, 12);
        const char* info = "(no recording - play a game first)";
        cairo_text_extents(cr, info, &ext);
        cairo_move_to(cr, centerX - ext.width / 2.0, optionY + 1 * 60 + 20);
        cairo_show_text(cr, info);
    }

    cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
    cairo_set_font_size(cr, 14);
    const char* instr = "UP / DOWN  select     ENTER  confirm";
    cairo_text_extents(cr, instr, &ext);
    cairo_move_to(cr, centerX - ext.width / 2.0, 520);
    cairo_show_text(cr, instr);
}

static void draw_maze(cairo_t *cr) {
    cairo_set_source_rgb(cr, 0, 0, 1);
    cairo_set_line_width(cr, 2);
    for (int y = 0; y < GameModel::MAP_HEIGHT; ++y) {
        for (int x = 0; x < GameModel::MAP_WIDTH; ++x) {
            if (model.getTile(x, y) == TileType::WALL) {
                cairo_rectangle(cr, x * TILE_SIZE + 2, y * TILE_SIZE + 2, TILE_SIZE - 4, TILE_SIZE - 4);
                cairo_stroke(cr);
            }
        }
    }
}

static void draw_pellets(cairo_t *cr) {
    cairo_set_source_rgb(cr, 1, 1, 1);
    for (int y = 0; y < GameModel::MAP_HEIGHT; ++y) {
        for (int x = 0; x < GameModel::MAP_WIDTH; ++x) {
            TileType tile = model.getTile(x, y);
            if (tile == TileType::PELLET) {
                cairo_arc(cr, x * TILE_SIZE + TILE_SIZE / 2.0, y * TILE_SIZE + TILE_SIZE / 2.0, 2, 0, 2 * M_PI);
                cairo_fill(cr);
            } else if (tile == TileType::POWER_PELLET) {
                cairo_arc(cr, x * TILE_SIZE + TILE_SIZE / 2.0, y * TILE_SIZE + TILE_SIZE / 2.0, 6, 0, 2 * M_PI);
                cairo_fill(cr);
            }
        }
    }
}

static void draw_pacman(cairo_t *cr) {
    const Entity& pacman = model.getPacman();
    if (model.isDying()) {
        cairo_set_source_rgb(cr, 1, 0, 0);
    } else {
        cairo_set_source_rgb(cr, 1, 1, 0);
    }

    double start_angle = 30 * M_PI / 180.0;
    double end_angle = 330 * M_PI / 180.0;

    switch (pacman.dir) {
        case Direction::UP: start_angle = 120 * M_PI / 180.0; end_angle = 420 * M_PI / 180.0; break;
        case Direction::DOWN: start_angle = 300 * M_PI / 180.0; end_angle = 600 * M_PI / 180.0; break;
        case Direction::LEFT: start_angle = 210 * M_PI / 180.0; end_angle = 510 * M_PI / 180.0; break;
        case Direction::RIGHT: start_angle = 30 * M_PI / 180.0; end_angle = 330 * M_PI / 180.0; break;
        case Direction::NONE: start_angle = 30 * M_PI / 180.0; end_angle = 330 * M_PI / 180.0; break;
    }

    auto now = std::chrono::steady_clock::now();
    double ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    double mouth = std::abs(std::sin(ms * 0.01)) * 30 * M_PI / 180.0;

    cairo_move_to(cr, pacman.x * TILE_SIZE + TILE_SIZE / 2.0, pacman.y * TILE_SIZE + TILE_SIZE / 2.0);
    cairo_arc(cr, pacman.x * TILE_SIZE + TILE_SIZE / 2.0, pacman.y * TILE_SIZE + TILE_SIZE / 2.0, TILE_SIZE / 2.0 - 2, start_angle + mouth, end_angle - mouth);
    cairo_close_path(cr);
    cairo_fill(cr);
}

static void draw_ghosts(cairo_t *cr) {
    const auto& ghosts = model.getGhosts();
    struct { double r, g, b; } colors[] = { {1,0,0}, {1,0.7,0.7}, {0,1,1}, {1,0.6,0} };

    for (size_t i = 0; i < ghosts.size(); ++i) {
        const Entity& ghost = ghosts[i];
        if (ghost.frightened) cairo_set_source_rgb(cr, 0, 0, 1);
        else cairo_set_source_rgb(cr, colors[i%4].r, colors[i%4].g, colors[i%4].b);

        cairo_arc(cr, ghost.x * TILE_SIZE + TILE_SIZE / 2.0, ghost.y * TILE_SIZE + TILE_SIZE / 2.0, TILE_SIZE / 2.0 - 2, M_PI, 2 * M_PI);
        cairo_rectangle(cr, ghost.x * TILE_SIZE + 2, ghost.y * TILE_SIZE + TILE_SIZE / 2.0, TILE_SIZE - 4, TILE_SIZE / 2.0 - 2);
        cairo_fill(cr);

        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_arc(cr, ghost.x * TILE_SIZE + 10, ghost.y * TILE_SIZE + 12, 4, 0, 2 * M_PI);
        cairo_arc(cr, ghost.x * TILE_SIZE + 20, ghost.y * TILE_SIZE + 12, 4, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_arc(cr, ghost.x * TILE_SIZE + 10, ghost.y * TILE_SIZE + 12, 2, 0, 2 * M_PI);
        cairo_arc(cr, ghost.x * TILE_SIZE + 20, ghost.y * TILE_SIZE + 12, 2, 0, 2 * M_PI);
        cairo_fill(cr);
    }
}

static void draw_ui(cairo_t *cr) {
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 18);
    char buf[64];

    if (appMode == AppMode::DEMO) {
        snprintf(buf, sizeof(buf), "REPLAY    Score: %d", model.getScore());
    } else {
        snprintf(buf, sizeof(buf), "Score: %d  Lives: %d", model.getScore(), model.getLives());
    }
    cairo_move_to(cr, 10, GameModel::MAP_HEIGHT * TILE_SIZE + 30);
    cairo_show_text(cr, buf);

    cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
    cairo_set_font_size(cr, 12);
    cairo_move_to(cr, 10, GameModel::MAP_HEIGHT * TILE_SIZE + 60);
    cairo_show_text(cr, "Author: Nourdine Kaddour");

    if (model.isGameOver()) {
        const char* text;
        if (appMode == AppMode::DEMO) {
            text = "REPLAY OVER";
        } else {
            text = "GAME OVER (Press R to restart)";
        }
        cairo_text_extents_t extents;
        cairo_set_font_size(cr, 32);
        cairo_text_extents(cr, text, &extents);
        cairo_move_to(cr, (GameModel::MAP_WIDTH * TILE_SIZE - extents.width) / 2.0, 350);
        cairo_set_source_rgb(cr, 1, 0, 0);
        cairo_show_text(cr, text);
    }
}

static void on_draw(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data) {
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_paint(cr);

    if (appMode == AppMode::MENU) {
        draw_menu(cr);
    } else {
        draw_maze(cr);
        draw_pellets(cr);
        draw_pacman(cr);
        draw_ghosts(cr);
        draw_ui(cr);
    }
}

static gboolean on_tick(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer data) {
    auto now = std::chrono::steady_clock::now();
    double deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(now - last_time).count() / 1000000.0;
    last_time = now;

    if (appMode == AppMode::DEMO && model.isGameOver()) {
        if (!prevDemoGameOver) {
            demoGameOverTimer = 4.0;
            prevDemoGameOver = true;
        }
        demoGameOverTimer -= deltaTime;
        if (demoGameOverTimer <= 0) {
            model.reset();
            prevDemoGameOver = false;
            appMode = AppMode::MENU;
        }
    } else if (appMode != AppMode::MENU) {
        model.update(deltaTime);
        prevDemoGameOver = false;
    }
    gtk_widget_queue_draw(widget);
    return G_SOURCE_CONTINUE;
}

static gboolean on_key_pressed(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data) {
    if (appMode == AppMode::MENU) {
        switch (keyval) {
            case GDK_KEY_Up: case GDK_KEY_w: case GDK_KEY_W:
                menuSelection = (menuSelection == 0) ? 1 : 0;
                return TRUE;
            case GDK_KEY_Down: case GDK_KEY_s: case GDK_KEY_S:
                menuSelection = (menuSelection == 1) ? 0 : 1;
                return TRUE;
            case GDK_KEY_Return: case GDK_KEY_KP_Enter: case GDK_KEY_space:
                if (menuSelection == 0) {
                    appMode = AppMode::PLAYING;
                    model.reset();
                    model.startRecording();
                } else if (model.hasRecording()) {
                    appMode = AppMode::DEMO;
                    model.startPlayback();
                }
                last_time = std::chrono::steady_clock::now();
                return TRUE;
            case GDK_KEY_Escape:
                if (mainWindow) gtk_window_destroy(GTK_WINDOW(mainWindow));
                return TRUE;
        }
        return TRUE;
    }

    switch (keyval) {
        case GDK_KEY_Up: case GDK_KEY_w: case GDK_KEY_W:
            if (appMode == AppMode::PLAYING) model.setPacmanNextDirection(Direction::UP);
            return TRUE;
        case GDK_KEY_Down: case GDK_KEY_s: case GDK_KEY_S:
            if (appMode == AppMode::PLAYING) model.setPacmanNextDirection(Direction::DOWN);
            return TRUE;
        case GDK_KEY_Left: case GDK_KEY_a: case GDK_KEY_A:
            if (appMode == AppMode::PLAYING) model.setPacmanNextDirection(Direction::LEFT);
            return TRUE;
        case GDK_KEY_Right: case GDK_KEY_d: case GDK_KEY_D:
            if (appMode == AppMode::PLAYING) model.setPacmanNextDirection(Direction::RIGHT);
            return TRUE;
        case GDK_KEY_r: case GDK_KEY_R:
            if (model.isGameOver()) {
                if (appMode == AppMode::PLAYING) {
                    model.reset();
                    model.startRecording();
                } else if (appMode == AppMode::DEMO) {
                    model.startPlayback();
                }
                last_time = std::chrono::steady_clock::now();
            }
            return TRUE;
        case GDK_KEY_Escape:
            model.stopRecording();
            appMode = AppMode::MENU;
            return TRUE;
    }
    return TRUE;
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    mainWindow = window;
    gtk_window_set_title(GTK_WINDOW(window), "Pacman by Nourdine Kaddour");
    gtk_window_set_default_size(GTK_WINDOW(window), GameModel::MAP_WIDTH * TILE_SIZE, GameModel::MAP_HEIGHT * TILE_SIZE + 100);

    GtkWidget *area = gtk_drawing_area_new();
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), on_draw, NULL, NULL);
    gtk_window_set_child(GTK_WINDOW(window), area);

    GtkEventController *controller = gtk_event_controller_key_new();
    gtk_widget_add_controller(window, controller);
    g_signal_connect(controller, "key-pressed", G_CALLBACK(on_key_pressed), NULL);

    last_time = std::chrono::steady_clock::now();
    gtk_widget_add_tick_callback(area, on_tick, NULL, NULL);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("org.pacman.gtk", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
