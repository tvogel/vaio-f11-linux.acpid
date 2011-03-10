#include "acpi_funcs.h"

void acpi_event_loop(int fd, int bl_ctrl) {
    char* buf = NULL;

    char* evt_toks[4];
    unsigned int i = 0;

    struct ConstValues const vals = init_const_values(bl_ctrl);

    while ((buf = read_line(fd)) != NULL) {
        for (i = 0; i != 4; ++i) { /* Assuming it's always 4 items */
            if (!i)
                evt_toks[i] = strtok(buf, " ");
            else evt_toks[i] = strtok(NULL, " ");
        }

        handle_acpi_events(&vals, evt_toks[0], evt_toks[1],
                           evt_toks[2], evt_toks[3]);
    }
}

void handle_acpi_events(struct ConstValues const* vals,
                        char const* evt_cls, char const* evt_type,
                        char const* evt_major, char const* evt_minor) {
    /* Assuming all params are valid */
    int als_brgt = -1;
    int current_brgt = -1;
    int new_brgt = -1;
    int kbd_bl = 0;

    if (!strcmp(evt_cls, SONY_EVENT_CLASS) &&
        !strcmp(evt_type, SONY_EVENT_TYPE) &&
        !strcmp(evt_major, SONY_EVENT_MAJOR)) {
        /* Ambient lighting changed */
        if (!strcmp(evt_minor, SONY_EVENT_ALS)) {
            als_brgt = read_int_from_file(SONY_ALS_BL);
            current_brgt = read_int_from_file(NVIDIA_BL_BRGT);
            new_brgt = als_brgt/100.0f*(vals->max_brgt-vals->min_brgt)+
                       vals->min_brgt;

            if (vals->bl_ctrl == BC_ACPI)
                update_brightness(ACPI_BL_BRGT, current_brgt, new_brgt);
            else /* BC_NVIDIA */
                update_brightness(NVIDIA_BL_BRGT, current_brgt, new_brgt);

            /* Turn on keyboard backlight in dim lighting */
            kbd_bl = read_int_from_file(SONY_KBD_BL);
            if ((als_brgt < AMBIENT_TOO_DIM)^kbd_bl)
                write_int_to_file(SONY_KBD_BL, !kbd_bl);
        }
    }
}

struct ConstValues init_const_values(int bl_ctrl) {
    struct ConstValues vals;

    vals.bl_ctrl = bl_ctrl;

    if (bl_ctrl == BC_ACPI) {
        vals.max_brgt = read_int_from_file(ACPI_BL_BRGT_MAX);
        vals.min_brgt = 0;
    }
    else { /* BC_NVIDIA */
        vals.max_brgt = read_int_from_file(NVIDIA_BL_BRGT_MAX);
        vals.min_brgt = 1500;
    }

    return vals;
}

void update_brightness(char const* path, int current, int target) {
    struct timespec const ts = {0, 50*1000*1000};
    float const step = (target-current)/10.0;
    unsigned int i = 1;

    while (i != 11) {
        write_int_to_file(path, current+step*i);
        nanosleep(&ts, NULL); /* There is a failure possibility */
        ++i;
    }
}