#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <syslog.h>
#include <time.h>

#define FAN_PATH "/proc/acpi/ibm/fan"
#define TEMP_PATH "/sys/class/thermal/thermal_zone0/temp"
#define CONFIG_PATH "/etc/fan_custom/config.conf"

typedef struct {
    int temp;
    char speed[16];  // "auto", "full-speed", "0"-"7"
} FanCurvePoint;

FanCurvePoint *fan_curve = NULL;
int curve_length = 0;
time_t last_config_mod = 0;

void log_message(const char *msg) {
    syslog(LOG_INFO, "%s", msg);
    FILE *kmsg = fopen("/dev/kmsg", "w");
    if (kmsg) {
        fprintf(kmsg, "thinkfan_custom: %s\n", msg);
        fclose(kmsg);
    }
}

int file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

time_t get_file_mtime(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return st.st_mtime;
}

int get_cpu_temp() {
    FILE *temp_file = fopen(TEMP_PATH, "r");
    if (!temp_file) return -1;
    int temp;
    fscanf(temp_file, "%d", &temp);
    fclose(temp_file);
    return temp / 1000;  // millidegrees → °C
}

void set_fan_level(const char *level) {
    FILE *fan = fopen(FAN_PATH, "w");
    if (!fan) {
        log_message("Failed to open fan control");
        return;
    }
    fprintf(fan, "level %s\n", level);
    fclose(fan);
}

int parse_config(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        log_message("Config file missing");
        return 0;
    }

    // Temp storage for new curve
    FanCurvePoint *new_curve = NULL;
    int new_length = 0;
    char buffer[256];

    // Count valid lines
    while (fgets(buffer, sizeof(buffer), file)) {
        int temp;
        char speed[16];
        if (sscanf(buffer, " (%d , %15[^)])", &temp, speed) == 2) {
            new_length++;
        }
    }
    rewind(file);

    if (new_length == 0) {
        log_message("No valid config entries");
        fclose(file);
        return 0;
    }

    new_curve = malloc(new_length * sizeof(FanCurvePoint));
    if (!new_curve) {
        log_message("Memory allocation failed");
        fclose(file);
        return 0;
    }

    // Parse entries
    int i = 0;
    while (fgets(buffer, sizeof(buffer), file) && i < new_length) {
        if (sscanf(buffer, " (%d , %15[^)])", &new_curve[i].temp, new_curve[i].speed) == 2) {
            i++;
        }
    }
    fclose(file);

    // Sort by temperature
    for (int j = 0; j < new_length; j++) {
        for (int k = j + 1; k < new_length; k++) {
            if (new_curve[j].temp > new_curve[k].temp) {
                FanCurvePoint tmp = new_curve[j];
                new_curve[j] = new_curve[k];
                new_curve[k] = tmp;
            }
        }
    }

    // Replace old curve
    free(fan_curve);
    fan_curve = new_curve;
    curve_length = new_length;
    last_config_mod = get_file_mtime(CONFIG_PATH);

    char logmsg[256];
    snprintf(logmsg, sizeof(logmsg), "Reloaded config (%d points)", curve_length);
    log_message(logmsg);

    return 1;
}

const char* get_fan_speed(int temp) {
    if (curve_length == 0) return "auto";  // Fallback

    for (int i = 0; i < curve_length; i++) {
        if (temp <= fan_curve[i].temp) {
            return fan_curve[i].speed;
        }
    }
    return fan_curve[curve_length - 1].speed;  // Max temp
}

void check_reload_config() {
    time_t current_mod = get_file_mtime(CONFIG_PATH);
    if (current_mod != last_config_mod) {
        if (!parse_config(CONFIG_PATH)) {
            log_message("Failed to reload config, using old curve");
        }
    }
}

int main() {
    openlog("thinkfan_custom", LOG_PID, LOG_DAEMON);
    if (geteuid() != 0) {
        log_message("Must be run as root");
        return 1;
    }
    log_message("Starting thinkfan_custom (live-reload enabled)");

    // Wait for initial config
    while (!file_exists(CONFIG_PATH)) {
        log_message("Waiting for config...");
        sleep(5);
    }
    parse_config(CONFIG_PATH);

    while (1) {
        // Check for config updates
        check_reload_config();

        if (!file_exists(FAN_PATH) || !file_exists(TEMP_PATH)) {
            log_message("Required files missing, retrying...");
            sleep(5);
            continue;
        }

        int temp = get_cpu_temp();
        if (temp == -1) {
            log_message("Temp read error");
            sleep(5);
            continue;
        }

        const char *speed = get_fan_speed(temp);
        set_fan_level(speed);

        sleep(5);  // Adjust polling interval
    }

    free(fan_curve);
    closelog();
    return 0;
}
