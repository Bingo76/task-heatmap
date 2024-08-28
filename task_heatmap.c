#define _XOPEN_SOURCE
#include <gtk/gtk.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h> // For g_get_home_dir
#include <sys/stat.h> // For mkdir

#define DAYS_IN_YEAR 365
#define FILENAME ".task-heatmap/tasks.dat"

// Ensure directory path length
#define DIR_PATH_LENGTH 1024
#define FILE_PATH_LENGTH 1024

typedef struct {
    char date[11];  // YYYY-MM-DD
    int hours;
} Task;

static Task tasks[DAYS_IN_YEAR] = {0};
static GtkWidget *stats_label;

// Function Declarations
void get_current_date(char *buffer);
void get_dotfile_path(char *path, size_t size);
void ensure_directory_exists(const char *path);
void save_tasks();
void load_tasks();
void reset_today_hours();
void calculate_stats(int *total_hours, double *avg_hours_per_day, double *avg_hours_per_week, double *avg_hours_per_month, double *percent_studied);
void update_stats_label();
gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
void on_log_hours(GtkButton *button, gpointer entry);
void on_reset_hours(GtkButton *button, gpointer data);

void get_current_date(char *buffer) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(buffer, "%04d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
}

void get_dotfile_path(char *path, size_t size) {
    const char *home = g_get_home_dir(); // Get home directory
    snprintf(path, size, "%s/%s", home, FILENAME); // Construct path
}

void ensure_directory_exists(const char *path) {
    char dir_path[DIR_PATH_LENGTH];
    snprintf(dir_path, sizeof(dir_path), "%s", path);

    // Find the last '/' in the path and null terminate to get the directory path
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash != NULL) {
        *last_slash = '\0'; // Null terminate at the last '/'
        // Create the directory if it does not exist
        struct stat st = {0};
        if (stat(dir_path, &st) == -1) {
            mkdir(dir_path, 0700); // Create the directory with permissions 0700
        }
    }
}

void save_tasks() {
    char path[FILE_PATH_LENGTH];
    get_dotfile_path(path, sizeof(path));

    ensure_directory_exists(path); // Ensure the directory exists

    FILE *file = fopen(path, "w");
    if (file == NULL) {
        perror("Could not open file");
        exit(1);
    }
    fwrite(tasks, sizeof(Task), DAYS_IN_YEAR, file);
    fclose(file);
}

void load_tasks() {
    char path[FILE_PATH_LENGTH];
    get_dotfile_path(path, sizeof(path));

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return;  // No file means no data yet
    }

    fread(tasks, sizeof(Task), DAYS_IN_YEAR, file);
    fclose(file);
}

void reset_today_hours() {
    char date[11];
    get_current_date(date);

    for (int i = 0; i < DAYS_IN_YEAR; i++) {
        if (strcmp(tasks[i].date, date) == 0) {
            tasks[i].hours = 0;  // Reset hours for today
            break;
        }
    }

    save_tasks();
    update_stats_label(); // Update stats after resetting hours
}

void calculate_stats(int *total_hours, double *avg_hours_per_day, double *avg_hours_per_week, double *avg_hours_per_month, double *percent_studied) {
    *total_hours = 0;
    int days_with_hours = 0;

    for (int i = 0; i < DAYS_IN_YEAR; i++) {
        if (tasks[i].hours > 0) {
            *total_hours += tasks[i].hours;
            days_with_hours++;
        }
    }

    *avg_hours_per_day = (double)(*total_hours) / DAYS_IN_YEAR;
    *avg_hours_per_week = *avg_hours_per_day * 7;
    *avg_hours_per_month = *avg_hours_per_day * 30;
    *percent_studied = (double)days_with_hours / DAYS_IN_YEAR * 100;
}

void update_stats_label() {
    int total_hours;
    double avg_hours_per_day, avg_hours_per_week, avg_hours_per_month, percent_studied;

    calculate_stats(&total_hours, &avg_hours_per_day, &avg_hours_per_week, &avg_hours_per_month, &percent_studied);

    char stats_text[256];
    snprintf(stats_text, sizeof(stats_text),
             "You spent %.2f%% of the year studying.\n"
             "Total hours this year: %d hrs\n"
             "Avg hours per day: %.2f hrs\n"
             "Avg hours per week: %.2f hrs\n"
             "Avg hours per month: %.2f hrs",
             percent_studied, total_hours, avg_hours_per_day, avg_hours_per_week, avg_hours_per_month);

    gtk_label_set_text(GTK_LABEL(stats_label), stats_text);
}

gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    int square_size = 20;
    int padding = 2;
    int first_entry_index = -1;

    // Find the first entry
    for (int i = 0; i < DAYS_IN_YEAR; i++) {
        if (tasks[i].hours > 0) {
            first_entry_index = i;
            break;
        }
    }

    for (int i = 0; i < DAYS_IN_YEAR; i++) {
        int display_index = (first_entry_index >= 0) ? (i - first_entry_index + DAYS_IN_YEAR - 1) % DAYS_IN_YEAR : i;
        int hours = tasks[display_index].hours;
        int x = (i % 30) * (square_size + padding);
        int y = (i / 30) * (square_size + padding);

        if (hours == 0) {
            cairo_set_source_rgb(cr, 0.8, 0.8, 0.8); // Light gray color for no hours
        } else {
            // Define a range of pastel green colors
            double intensity = (double)hours / 24.0;
            double red = 0.8 - 0.4 * intensity;   // Light to darker green (pastel)
            double green = 1.0 - 0.5 * intensity; // Vary green component more
            double blue = 0.8 - 0.4 * intensity;  // Light to darker green (pastel)
            cairo_set_source_rgb(cr, red, green, blue);
        }

        cairo_rectangle(cr, x, y, square_size, square_size);
        cairo_fill(cr);
    }

    return FALSE;
}

void on_log_hours(GtkButton *button, gpointer entry) {
    const char *hours_str = gtk_entry_get_text(GTK_ENTRY(entry));
    int hours = atoi(hours_str);

    char date[11];
    get_current_date(date);

    int found = 0;
    for (int i = 0; i < DAYS_IN_YEAR; i++) {
        if (strcmp(tasks[i].date, date) == 0) {
            tasks[i].hours = hours;  // Update the hours for the current day
            found = 1;
            break;
        }
        if (tasks[i].hours == 0) {
            strcpy(tasks[i].date, date);
            tasks[i].hours = hours;
            found = 1;
            break;
        }
    }

    if (!found) {
        // Handle the case where all days are occupied
        // For simplicity, replace the oldest entry (could be improved)
        int oldest_index = 0;
        strcpy(tasks[oldest_index].date, date);
        tasks[oldest_index].hours = hours;
    }

    save_tasks();
    update_stats_label(); // Update stats after logging hours
    gtk_widget_queue_draw(GTK_WIDGET(gtk_widget_get_toplevel(GTK_WIDGET(button))));
}

void on_reset_hours(GtkButton *button, gpointer data) {
    reset_today_hours();
    update_stats_label(); // Update stats after resetting hours
    gtk_widget_queue_draw(GTK_WIDGET(gtk_widget_get_toplevel(GTK_WIDGET(button))));
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    load_tasks();

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Task Heatmap");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);

    GtkWidget *button_log = gtk_button_new_with_label("Log Hours");
    gtk_box_pack_start(GTK_BOX(vbox), button_log, FALSE, FALSE, 0);
    g_signal_connect(button_log, "clicked", G_CALLBACK(on_log_hours), entry);

    GtkWidget *button_reset = gtk_button_new_with_label("Reset Today's Hours");
    gtk_box_pack_start(GTK_BOX(vbox), button_reset, FALSE, FALSE, 0);
    g_signal_connect(button_reset, "clicked", G_CALLBACK(on_reset_hours), NULL);

    GtkWidget *drawing_area = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(vbox), drawing_area, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(on_draw), NULL);

    stats_label = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(vbox), stats_label, FALSE, FALSE, 0);

    update_stats_label(); // Initialize the stats label

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}

