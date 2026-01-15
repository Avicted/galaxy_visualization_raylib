#include "camera.h"
#include "macros.h"

void camera_handle_resize(app_state_t *app_state)
{
    if (IsWindowResized() && !IsWindowFullscreen())
    {
        app_state->window_width = GetScreenWidth();
        app_state->window_height = GetScreenHeight();
    }

    if ((IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT))) ||
        (IsKeyPressed(KEY_F11)))
    {
        i32 current_display = GetCurrentMonitor();

        if (IsWindowFullscreen())
        {
            SetWindowSize(app_state->window_width, app_state->window_height);
        }
        else
        {
            SetWindowSize(GetMonitorWidth(current_display), GetMonitorHeight(current_display));
        }

        ToggleFullscreen();

        app_state->window_width = GetScreenWidth();
        app_state->window_height = GetScreenHeight();
    }
}

void camera_update(app_state_t *app_state, f64 dt)
{
    Camera3D *cam = &app_state->main_camera;
    f64 speed = 10.0f * dt;
    f64 vertical_speed = 5.0f * dt;

    f64 *yaw = &app_state->camera_yaw;
    f64 *pitch = &app_state->camera_pitch;
    Vector3 *direction = &app_state->camera_direction;

    local_persist bool prev_is_paused = false;
    bool entered_free_look = (app_state->is_paused && !prev_is_paused);

    if (app_state->is_paused)
    {
        if (entered_free_look)
        {
            HideCursor();
            app_state->cursor_enabled = false;

            cam->position = (Vector3){13.632f, 1.377f, 9.318f};
            cam->target = (Vector3){14.176f, 1.954f, 9.927f};
            *direction = (Vector3){0.545f, 0.577f, 0.609f};
            *yaw = 48.0f;
            *pitch = 35.220f;
        }

        const Vector2 mouse_delta = GetMouseDelta();
        *yaw += mouse_delta.x * 0.1f;
        *pitch -= mouse_delta.y * 0.1f;

        const Vector2 mouse_pos = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
        SetMousePosition(mouse_pos.x, mouse_pos.y);

        if (*pitch > 89.0f)
        {
            *pitch = 89.0f;
        }
        if (*pitch < -89.0f)
        {
            *pitch = -89.0f;
        }

        direction->x = cosf(DEG2RAD * (*pitch)) * cosf(DEG2RAD * (*yaw));
        direction->y = sinf(DEG2RAD * (*pitch));
        direction->z = cosf(DEG2RAD * (*pitch)) * sinf(DEG2RAD * (*yaw));
        *direction = Vector3Normalize(*direction);

        const Vector3 right = Vector3Normalize(Vector3CrossProduct(*direction, cam->up));
        const Vector3 up = Vector3Normalize(Vector3CrossProduct(right, *direction));

        if (IsKeyDown(KEY_LEFT_CONTROL))
        {
            speed *= 0.1f;
            vertical_speed *= 0.1f;
        }

        if (IsKeyDown(KEY_W))
        {
            cam->position = Vector3Add(cam->position, Vector3Scale(*direction, speed));
        }
        if (IsKeyDown(KEY_S))
        {
            cam->position = Vector3Subtract(cam->position, Vector3Scale(*direction, speed));
        }
        if (IsKeyDown(KEY_D))
        {
            cam->position = Vector3Add(cam->position, Vector3Scale(right, speed));
        }
        if (IsKeyDown(KEY_A))
        {
            cam->position = Vector3Subtract(cam->position, Vector3Scale(right, speed));
        }
        if (IsKeyDown(KEY_LEFT_SHIFT))
        {
            cam->position = Vector3Subtract(cam->position, Vector3Scale(up, vertical_speed));
        }
        if (IsKeyDown(KEY_SPACE))
        {
            cam->position = Vector3Add(cam->position, Vector3Scale(up, vertical_speed));
        }

        cam->target = Vector3Add(cam->position, *direction);
    }
    else
    {
        if (prev_is_paused)
        {
            ShowCursor();
            app_state->cursor_enabled = true;
        }

        local_persist f64 previous_time_since_start = 0.0f;
        previous_time_since_start += dt * 0.2f;

        cam->position.x = 25.0f * cosf(previous_time_since_start) * app_state->camera_zoom;
        cam->position.y = 50.0f;
        cam->position.z = 25.0f * sinf(previous_time_since_start) * app_state->camera_zoom;

        cam->target = Vector3Zero();
    }

    prev_is_paused = app_state->is_paused;
}
