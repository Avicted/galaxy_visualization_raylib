#include "ui.h"
#include "utils.h"

static const Color PANEL_BG = {20, 20, 30, 200};
static const Color PANEL_BORDER = {60, 60, 80, 255};
static const Color TEXT_DIM = {180, 180, 180, 255};
static const Color ACCENT_BLUE = {64, 64, 255, 255};
static const Color ACCENT_RED = {255, 64, 64, 255};
static const Color ACCENT_GREEN = {100, 255, 64, 255};
static const Color ACCENT_PURPLE = {255, 64, 255, 255};

static void
ui_draw_info_panel(app_state_t *app_state)
{
    const i32 panel_w = 340;
    const i32 panel_h = 100;
    DrawRectangle(8, 8, panel_w, panel_h, PANEL_BG);
    DrawRectangleLines(8, 8, panel_w, panel_h, PANEL_BORDER);

    const char *dataset_names[] = {"Real Data", "Uniform", "Both", "Seyfert 3D"};
    Color dataset_colors[] = {ACCENT_BLUE, ACCENT_RED, WHITE, ACCENT_PURPLE};
    DrawTextEx(app_state->main_font, dataset_names[app_state->data_to_draw],
               (Vector2){16, 12}, 32, 1, dataset_colors[app_state->data_to_draw]);

    DrawTextEx(app_state->main_font, TextFormat("FPS: %.0f", app_state->fps_display),
               (Vector2){16, 52}, 24, 1, TEXT_DIM);

    ul galaxy_count = 0UL;
    if (app_state->data_to_draw == DRAW_DATA_REDSHIFT)
    {
        galaxy_count = app_state->redshift_galaxy_count;
    }
    else if (app_state->data_to_draw == DRAW_DATA_A || app_state->data_to_draw == DRAW_DATA_B || app_state->data_to_draw == DRAW_DATA_ALL)
    {
        galaxy_count = app_state->data_point_count * ((app_state->data_to_draw == DRAW_DATA_ALL) ? 2 : 1);
    }

    DrawTextEx(app_state->main_font, TextFormat("Galaxies: %s", format_u64_thousands_dots((u64)galaxy_count)),
               (Vector2){16, 74}, 24, 1, TEXT_DIM);
}

static void
ui_draw_mode_indicator(app_state_t *app_state)
{
    const i32 panel_width = 260;
    const i32 panel_x = app_state->window_width - panel_width - 8;

    const char *mode_text = app_state->is_paused ? "Free Look" : "Auto";
    Color mode_color = app_state->is_paused ? ACCENT_PURPLE : ACCENT_GREEN;
    i32 mode_panel_height = 32;

    DrawRectangle(panel_x, 8, panel_width, mode_panel_height, PANEL_BG);
    DrawRectangleLines(panel_x, 8, panel_width, mode_panel_height, mode_color);
    DrawTextEx(app_state->main_font, mode_text,
               (Vector2){(f32)(panel_x + 12), 12}, 24, 1, mode_color);

    if (app_state->data_to_draw == DRAW_DATA_REDSHIFT)
    {
        const i32 legend_y = 52;
        const i32 legend_height = 110;

        DrawRectangle(panel_x, legend_y, panel_width, legend_height, PANEL_BG);
        DrawRectangleLines(panel_x, legend_y, panel_width, legend_height, PANEL_BORDER);

        DrawTextEx(app_state->main_font, "Velocity (km/s)",
                   (Vector2){(f32)(panel_x + 10), (f32)(legend_y + 8)}, 24, 1, TEXT_DIM);

        const i32 bar_x = panel_x + 10;
        const i32 bar_y = legend_y + 40;
        const i32 bar_width = panel_width - 20;
        const i32 bar_height = 16;
        const i32 num_segments = 40;
        const f32 seg_width = (f32)bar_width / (f32)num_segments;

        for (i32 seg = 0; seg < num_segments; seg++)
        {
            f64 t = (f64)seg / (f64)(num_segments - 1);
            Color seg_color;

            if (t < 0.33)
            {
                f64 s = t / 0.33;
                seg_color.r = (u8)(100 + s * 155);
                seg_color.g = (u8)(255 - s * 25);
                seg_color.b = (u8)(255 - s * 200);
                seg_color.a = 255;
            }
            else if (t < 0.66)
            {
                f64 s = (t - 0.33) / 0.33;
                seg_color.r = 255;
                seg_color.g = (u8)(230 - s * 100);
                seg_color.b = (u8)(55 - s * 35);
                seg_color.a = 255;
            }
            else
            {
                f64 s = (t - 0.66) / 0.34;
                seg_color.r = (u8)(255 - s * 55);
                seg_color.g = (u8)(130 - s * 90);
                seg_color.b = (u8)(20 - s * 10);
                seg_color.a = 255;
            }

            DrawRectangle((i32)(bar_x + seg * seg_width), bar_y, (i32)(seg_width + 1), bar_height, seg_color);
        }
        DrawRectangleLines(bar_x, bar_y, bar_width, bar_height, PANEL_BORDER);

        DrawTextEx(app_state->main_font, "1k",
                   (Vector2){(f32)bar_x, (f32)(bar_y + bar_height + 8)}, 24, 1, TEXT_DIM);
        DrawTextEx(app_state->main_font, "86k",
                   (Vector2){(f32)(bar_x + bar_width - 40), (f32)(bar_y + bar_height + 8)}, 24, 1, TEXT_DIM);
    }
}

static void
ui_draw_bottom_hint(app_state_t *app_state)
{
    const char *hint_text = app_state->is_paused ? "R - Return to Auto Orbit" : "R - Enter Free Look";
    Color hint_color = app_state->is_paused ? ACCENT_PURPLE : ACCENT_GREEN;
    Vector2 text_size = MeasureTextEx(app_state->main_font, hint_text, 32, 2);
    f32 hint_x = (app_state->window_width - text_size.x) / 2.0f;
    f32 hint_y = app_state->window_height - 50.0f;

    DrawRectangle((i32)(hint_x - 12), (i32)(hint_y - 8), (i32)(text_size.x + 24), 50, PANEL_BG);
    DrawRectangleLines((i32)(hint_x - 12), (i32)(hint_y - 8), (i32)(text_size.x + 24), 50, hint_color);
    DrawTextEx(app_state->main_font, hint_text, (Vector2){hint_x, hint_y}, 32, 2, hint_color);
}

static void
ui_draw_help_panel(app_state_t *app_state)
{
    const i32 help_x = 8;
    const i32 help_y = 120;

    if (app_state->show_help)
    {
        const i32 help_width = 340;
        i32 help_height = app_state->is_paused ? 200 : 170;

        DrawRectangle(help_x, help_y, help_width, help_height, PANEL_BG);
        DrawRectangleLines(help_x, help_y, help_width, help_height, PANEL_BORDER);

        i32 line_y = help_y + 10;
        const i32 line_spacing = 24;

        DrawTextEx(app_state->main_font, "Controls", (Vector2){(f32)(help_x + 12), (f32)line_y}, 24, 2, WHITE);
        line_y += line_spacing + 4;

        DrawTextEx(app_state->main_font, "1-4  Dataset", (Vector2){(f32)(help_x + 12), (f32)line_y}, 24, 1, TEXT_DIM);
        line_y += line_spacing;
        DrawTextEx(app_state->main_font, "R    Camera mode", (Vector2){(f32)(help_x + 12), (f32)line_y}, 24, 1, TEXT_DIM);
        line_y += line_spacing;
        DrawTextEx(app_state->main_font, "H    Toggle help", (Vector2){(f32)(help_x + 12), (f32)line_y}, 24, 1, TEXT_DIM);
        line_y += line_spacing;
        DrawTextEx(app_state->main_font, "F11  Fullscreen", (Vector2){(f32)(help_x + 12), (f32)line_y}, 24, 1, TEXT_DIM);
        line_y += line_spacing;
        DrawTextEx(app_state->main_font, "Scroll  Zoom", (Vector2){(f32)(help_x + 12), (f32)line_y}, 24, 1, TEXT_DIM);

        if (app_state->is_paused)
        {
            line_y += line_spacing + 6;
            DrawTextEx(app_state->main_font, "WASD+Mouse Shift/Space", (Vector2){(f32)(help_x + 12), (f32)line_y}, 24, 1, ACCENT_PURPLE);
        }
    }
    else
    {
        DrawTextEx(app_state->main_font, "H - Help", (Vector2){16, help_y}, 24, 1, TEXT_DIM);
    }
}

static void
ui_draw_dataset_legend(app_state_t *app_state)
{
    const i32 legend_x = 8;
    const i32 legend_width = 240;
    const i32 legend_height = 130;
    const i32 legend_y = app_state->window_height - legend_height - 8;

    DrawRectangle(legend_x, legend_y, legend_width, legend_height, PANEL_BG);
    DrawRectangleLines(legend_x, legend_y, legend_width, legend_height, PANEL_BORDER);

    const i32 legend_text_x = legend_x + 8;
    const i32 legend_text_y = legend_y + 10;
    const i32 legend_line_spacing = 28;

    DrawTextEx(app_state->main_font, "1 Real (blue)",
               (Vector2){(f32)legend_text_x, (f32)(legend_text_y + 0 * legend_line_spacing)}, 24, 1, ACCENT_BLUE);
    DrawTextEx(app_state->main_font, "2 Uniform",
               (Vector2){(f32)legend_text_x, (f32)(legend_text_y + 1 * legend_line_spacing)}, 24, 1, ACCENT_RED);
    DrawTextEx(app_state->main_font, "3 Both",
               (Vector2){(f32)legend_text_x, (f32)(legend_text_y + 2 * legend_line_spacing)}, 24, 1, TEXT_DIM);
    DrawTextEx(app_state->main_font, "4 Seyfert",
               (Vector2){(f32)legend_text_x, (f32)(legend_text_y + 3 * legend_line_spacing)}, 24, 1, ACCENT_PURPLE);
}

void ui_draw(app_state_t *app_state)
{
    ui_draw_info_panel(app_state);
    ui_draw_mode_indicator(app_state);
    ui_draw_bottom_hint(app_state);
    ui_draw_help_panel(app_state);
    ui_draw_dataset_legend(app_state);
}
