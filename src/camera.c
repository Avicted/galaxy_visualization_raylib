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
    f64 speed = CAMERA_MOVE_SPEED * dt;
    f64 vertical_speed = CAMERA_VERTICAL_SPEED * dt;

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

            cam->position = (Vector3){FREE_LOOK_POS_X, FREE_LOOK_POS_Y, FREE_LOOK_POS_Z};
            cam->target = (Vector3){FREE_LOOK_TARGET_X, FREE_LOOK_TARGET_Y, FREE_LOOK_TARGET_Z};
            *direction = (Vector3){FREE_LOOK_DIR_X, FREE_LOOK_DIR_Y, FREE_LOOK_DIR_Z};
            *yaw = FREE_LOOK_YAW;
            *pitch = FREE_LOOK_PITCH;
        }

        const Vector2 mouse_delta = GetMouseDelta();
        *yaw += mouse_delta.x * CAMERA_MOUSE_SENSITIVITY;
        *pitch -= mouse_delta.y * CAMERA_MOUSE_SENSITIVITY;

        const Vector2 mouse_pos = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
        SetMousePosition(mouse_pos.x, mouse_pos.y);

        if (*pitch > CAMERA_PITCH_LIMIT)
        {
            *pitch = CAMERA_PITCH_LIMIT;
        }
        if (*pitch < -CAMERA_PITCH_LIMIT)
        {
            *pitch = -CAMERA_PITCH_LIMIT;
        }

        direction->x = cosf(DEG2RAD * (*pitch)) * cosf(DEG2RAD * (*yaw));
        direction->y = sinf(DEG2RAD * (*pitch));
        direction->z = cosf(DEG2RAD * (*pitch)) * sinf(DEG2RAD * (*yaw));
        *direction = Vector3Normalize(*direction);

        const Vector3 right = Vector3Normalize(Vector3CrossProduct(*direction, cam->up));
        const Vector3 up = Vector3Normalize(Vector3CrossProduct(right, *direction));

        if (IsKeyDown(KEY_LEFT_CONTROL))
        {
            speed *= CAMERA_SLOW_FACTOR;
            vertical_speed *= CAMERA_SLOW_FACTOR;
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
        previous_time_since_start += dt * CAMERA_ORBIT_SPEED;

        cam->position.x = CAMERA_ORBIT_RADIUS * cosf(previous_time_since_start) * app_state->camera_zoom;
        cam->position.y = CAMERA_ORBIT_HEIGHT;
        cam->position.z = CAMERA_ORBIT_RADIUS * sinf(previous_time_since_start) * app_state->camera_zoom;

        cam->target = Vector3Zero();
    }

    prev_is_paused = app_state->is_paused;
}
