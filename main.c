#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

// Массив из 4 меток для вывода температуры каждого ядра
GtkWidget *labels[4];

// Функция чтения температуры конкретного ядра (от 0 до 3)
double get_core_temperature(int core_id) {
    FILE *fp;
    long temp_raw = 0;
    char path[128];

    // В Linux датчики ядер Intel обычно нумеруются со смещением (temp2, temp3, temp4, temp5)
    // Мы ищем файлы в стандартном пути hwmon драйвера coretemp
    snprintf(path, sizeof(path), "/sys/devices/platform/coretemp.0/hwmon/hwmon2/temp%d_input", core_id + 2);
    fp = fopen(path, "r");

    // Если папка hwmon изменила номер (например, hwmon3 или hwmon4), проверяем альтернативный путь
    if (fp == NULL) {
        snprintf(path, sizeof(path), "/sys/devices/platform/coretemp.0/hwmon/hwmon3/temp%d_input", core_id + 2);
        fp = fopen(path, "r");
    }
    if (fp == NULL) {
        snprintf(path, sizeof(path), "/sys/devices/platform/coretemp.0/hwmon/hwmon4/temp%d_input", core_id + 2);
        fp = fopen(path, "r");
    }

    if (fp != NULL) {
        if (fscanf(fp, "%ld", &temp_raw) != 1) temp_raw = 0;
        fclose(fp);
    }

    return temp_raw / 1000.0;
}

// Функция обновления данных по таймеру
static gboolean update_all_cores(gpointer data) {
    char buffer[64];
    for (int i = 0; i < 4; i++) {
        double temp = get_core_temperature(i);
        if (temp > 0) {
            snprintf(buffer, sizeof(buffer), "Ядро %d:  %.1f °C", i, temp);
        } else {
            snprintf(buffer, sizeof(buffer), "Ядро %d:  Ошибка датчика", i);
        }
        gtk_label_set_text(GTK_LABEL(labels[i]), buffer);
    }
    return TRUE;
}

int main(int argc, char *argv[]) {

    return 0;
}
