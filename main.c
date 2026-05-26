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
    GtkWidget *window;
    GtkWidget *vbox;

    gtk_init(&argc, &argv);

    // Настройка главного окна
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Монитор 4 ядер i7");
    gtk_window_set_default_size(GTK_WINDOW(window), 280, 200);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Вертикальный контейнер для размещения строк
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);

    // Создание 4 красивых текстовых меток
    PangoFontDescription *font_desc = pango_font_description_from_string("Monospace Bold 16");
    for (int i = 0; i < 4; i++) {
        labels[i] = gtk_label_new("Загрузка...");
        gtk_widget_override_font(labels[i], font_desc);
        gtk_box_pack_start(GTK_BOX(vbox), labels[i], TRUE, TRUE, 0);
    }
    pango_font_description_free(font_desc);

    // Снимаем первые показания и запускаем ежесекундный таймер
    update_all_cores(NULL);
    g_timeout_add(1000, update_all_cores, NULL);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
