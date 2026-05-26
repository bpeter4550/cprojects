#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GtkWidget *label_usd;
GtkWidget *label_eur;
GtkWidget *label_cny;

// Функция для безопасного поиска и извлечения курса валюты из XML-ответа ЦБ РФ
void get_rate(const char *xml_data, const char *char_code, char *output, size_t max_len) {
    // Ищем позицию кода валюты (например, "USD")
    char *pos = strstr(xml_data, char_code);
    if (!pos) {
        snprintf(output, max_len, "нет данных");
        return;
    }

    // В структуре XML ЦБ РФ значение курса идет после тега <Value>
    char *value_pos = strstr(pos, "<Value>");
    if (!value_pos) {
        snprintf(output, max_len, "ошибка тега");
        return;
    }

    value_pos += 7; // Сдвигаемся за предел "<Value>"
    char *end_pos = strstr(value_pos, "</Value>");
    if (!end_pos) {
        snprintf(output, max_len, "ошибка структуры");
        return;
    }

    size_t len = end_pos - value_pos;
    if (len >= max_len) len = max_len - 1;

    strncpy(output, value_pos, len);
    output[len] = '\0';
}
static gboolean update_currency_rates(gpointer data) {
    // Выделяем достаточно памяти (32 КБ), чтобы XML-файл поместился целиком
    static char xml_buffer[32768];
    char line[512];

    // Очищаем буфер перед каждым новым запросом
    memset(xml_buffer, 0, sizeof(xml_buffer));

    // Запрашиваем данные. Добавим ключ --no-check-certificate на случай проблем со старыми SSL-сертификатами
    FILE *fp = popen("wget -qO- --no-check-certificate https://www.cbr.ru/scripts/XML_daily.asp", "r");
    if (fp == NULL) {
        gtk_label_set_text(GTK_LABEL(label_usd), "Ошибка вызова wget");
        return TRUE;
    }

    // Построчно считываем весь XML в один большой буфер
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strlen(xml_buffer) + strlen(line) < sizeof(xml_buffer) - 1) {
            strcat(xml_buffer, line);
        }
    }
    pclose(fp);

    // Проверяем, не пустой ли ответ пришел от сервера
    if (strlen(xml_buffer) < 100) {
        gtk_label_set_text(GTK_LABEL(label_usd), "Ошибка: пустой ответ API");
        gtk_label_set_text(GTK_LABEL(label_eur), "Ошибка: пустой ответ API");
        gtk_label_set_text(GTK_LABEL(label_cny), "Ошибка: пустой ответ API");
        return TRUE;
    }

    char usd_val[16] = {0};
    char eur_val[16] = {0};
    char cny_val[16] = {0};
    char display_text[64];

    // Вытаскиваем значения валют из полного буфера
    get_rate(xml_buffer, "USD", usd_val, sizeof(usd_val));
    get_rate(xml_buffer, "EUR", eur_val, sizeof(eur_val));
    get_rate(xml_buffer, "CNY", cny_val, sizeof(cny_val));

    // Выводим результаты на экран
    snprintf(display_text, sizeof(display_text), "Доллар (USD):  %s ₽", usd_val);
    gtk_label_set_text(GTK_LABEL(label_usd), display_text);

    snprintf(display_text, sizeof(display_text), "Евро (EUR):     %s ₽", eur_val);
    gtk_label_set_text(GTK_LABEL(label_eur), display_text);

    snprintf(display_text, sizeof(display_text), "Юань (CNY):     %s ₽", cny_val);
    gtk_label_set_text(GTK_LABEL(label_cny), display_text);

    return TRUE;
}


int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *vbox;

    gtk_init(&argc, &argv);

    // Создание главного графического окна
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Курсы валют ЦБ РФ");
    gtk_window_set_default_size(GTK_WINDOW(window), 320, 220);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Вертикальный контейнер для строк
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);

    // Создание текстовых полей
    label_usd = gtk_label_new("Загрузка USD...");
    label_eur = gtk_label_new("Загрузка EUR...");
    label_cny = gtk_label_new("Загрузка CNY...");

    // Применяем красивый моноширинный жирный шрифт
    PangoFontDescription *font_desc = pango_font_description_from_string("Monospace Bold 16");
    gtk_widget_override_font(label_usd, font_desc);
    gtk_widget_override_font(label_eur, font_desc);
    gtk_widget_override_font(label_cny, font_desc);
    pango_font_description_free(font_desc);

    // Добавляем элементы в окно
    gtk_box_pack_start(GTK_BOX(vbox), label_usd, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), label_eur, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), label_cny, TRUE, TRUE, 0);

    // Первичный сбор данных при запуске
    update_currency_rates(NULL);

    // Настройка автоматического обновления (раз в 10 минут = 600 000 миллисекунд)
    g_timeout_add(600000, update_currency_rates, NULL);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
