#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "core/display.h"
#include "core/game.h"
#include "core/input_service.h"
#include "core/renderer.h"
#include "core/ui.h"

static volatile sig_atomic_t stop_requested = 0;

static void handle_signal(int signal_number)
{
    (void)signal_number;
    stop_requested = 1;
}

static uint64_t monotonic_milliseconds(void)
{
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return 0U;
    }
    return (uint64_t)now.tv_sec * UINT64_C(1000) +
           (uint64_t)now.tv_nsec / UINT64_C(1000000);
}

static void sleep_until(uint64_t deadline_ms)
{
    struct timespec deadline;
    int result;

    deadline.tv_sec = (time_t)(deadline_ms / UINT64_C(1000));
    deadline.tv_nsec =
        (long)((deadline_ms % UINT64_C(1000)) * UINT64_C(1000000));
    do {
        result = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                                 &deadline, NULL);
    } while (result == EINTR && stop_requested == 0);
}

static int install_signal_handlers(void)
{
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_signal;
    if (sigemptyset(&action.sa_mask) != 0 ||
        sigaction(SIGINT, &action, NULL) != 0 ||
        sigaction(SIGTERM, &action, NULL) != 0) {
        return -1;
    }
    return 0;
}

static void process_input(game_t *game, input_service_t *input)
{
    touch_event_t event;

    while (input_service_try_pop(input, &event)) {
        game_command_t command;
        if (ui_command_from_touch(event, &command)) {
            (void)game_apply_command(game, &command);
        }
    }
}

int main(void)
{
    game_config_t game_config = game_default_config();
    game_render_view_t render_view;
    game_t *game = NULL;
    renderer_t *renderer = NULL;
    display_t *display = NULL;
    input_service_t *input = NULL;
    uint64_t previous_time;
    uint64_t next_frame_time;
    uint64_t accumulator = PVZ_GAME_TICK_MS;
    uint64_t last_overflow_count = 0U;
    int exit_status = 1;

    if (install_signal_handlers() != 0) {
        perror("install signal handlers");
        goto cleanup;
    }

    game_config.random_seed = (uint32_t)monotonic_milliseconds();
    game = game_create(&game_config);
    if (game == NULL) {
        (void)fprintf(stderr, "failed to create game\n");
        goto cleanup;
    }

    renderer = renderer_create(PVZ_SCREEN_WIDTH, PVZ_SCREEN_HEIGHT);
    if (renderer == NULL) {
        (void)fprintf(stderr, "failed to create renderer\n");
        goto cleanup;
    }

    display = display_open(PVZ_LCD_DEVICE);
    if (display == NULL) {
        perror("open display");
        goto cleanup;
    }

    input = input_service_start(PVZ_TOUCH_DEVICE);
    if (input == NULL) {
        perror("start input service");
        goto cleanup;
    }

    previous_time = monotonic_milliseconds();
    next_frame_time = previous_time;
    (void)printf("game running\n");

    while (stop_requested == 0 && game_get_status(game) == GAME_RUNNING) {
        uint64_t now = monotonic_milliseconds();
        uint64_t elapsed;
        unsigned int update_count = 0U;

        if (now < next_frame_time) {
            sleep_until(next_frame_time);
            now = monotonic_milliseconds();
        }

        elapsed = now >= previous_time ? now - previous_time : 0U;
        previous_time = now;
        if (elapsed > PVZ_GAME_TICK_MS * PVZ_MAX_CATCH_UP_STEPS) {
            elapsed = PVZ_GAME_TICK_MS * PVZ_MAX_CATCH_UP_STEPS;
        }
        accumulator += elapsed;

        process_input(game, input);
        if (input_service_status(input) == INPUT_SERVICE_FAILED) {
            int error_number = input_service_last_error(input);
            (void)fprintf(stderr, "input service failed: %s\n",
                          strerror(error_number));
            goto cleanup;
        }

        while (accumulator >= PVZ_GAME_TICK_MS &&
               update_count < PVZ_MAX_CATCH_UP_STEPS &&
               game_get_status(game) == GAME_RUNNING) {
            game_update(game, PVZ_GAME_TICK_MS);
            accumulator -= PVZ_GAME_TICK_MS;
            ++update_count;
        }
        if (accumulator >= PVZ_GAME_TICK_MS) {
            (void)fprintf(stderr, "warning: simulation fell behind; "
                          "discarding accumulated time\n");
            accumulator = 0U;
        }

        game_build_render_view(game, &render_view);
        renderer_render(renderer, &render_view);
        if (display_present(display, renderer_get_framebuffer(renderer)) != 0) {
            perror("present frame");
            goto cleanup;
        }

        {
            uint64_t overflow_count = input_service_overflow_count(input);
            if (overflow_count != last_overflow_count) {
                (void)fprintf(stderr, "warning: dropped touch events: %" PRIu64
                              "\n", overflow_count);
                last_overflow_count = overflow_count;
            }
        }

        next_frame_time += PVZ_GAME_TICK_MS;
        if (next_frame_time + PVZ_GAME_TICK_MS * PVZ_MAX_CATCH_UP_STEPS < now) {
            next_frame_time = now + PVZ_GAME_TICK_MS;
        }
    }

    if (game_get_status(game) == GAME_LOST) {
        (void)printf("game lost\n");
    }
    exit_status = 0;

cleanup:
    input_service_destroy(input);
    display_close(display);
    renderer_destroy(renderer);
    game_destroy(game);
    return exit_status;
}
