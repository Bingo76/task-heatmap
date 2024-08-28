#define _XOPEN_SOURCE
#include <gtk/gtk.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h> // For g_get_home_dir
#include <sys/stat.h> // For mkdir

#define DAYS_IN_YEAR 365
#define MAX_TASKS 10
#define FILENAME ".taskheatmap/tasks.dat"
#define ACTIVITIES_FILE ".taskheatmap/activities.dat"
#define FILE_PATH_LENGTH 256
#define DIR_PATH_LENGTH 256

// Define a structure to hold task data
typedef struct {
    char date[11];  // YYYY-MM-DD
    int hours;
} Task;

typedef struct {
    char name[50];
    Task tasks[DAYS_IN_YEAR];
} TaskData;

static TaskData task_data[MAX_TASKS] = {0};
static char activities[MAX_TASKS][50] = {0};
static GtkWidget *stats_label;
static GtkWidget *drawing_area;
static GtkWidget *task_combo_box;
static GtkWidget *entry;
static GtkWidget *settings_vbox;
static GtkWidget *entry_add_activity;
static GtkWidget *button_add_activity;
static GtkWidget *button_remove_activity;
static GtkWidget *advanced_checkbox;
static int current_task_index = 0;

// Function Declarations
void get_current_date(char *buffer);
void get_dotfile_path(char *path, size_t size);
void ensure_directory_exists(const char *path);
void save_tasks();
void load_tasks();
void save_activities();
void load_activities();
void add_activity(const char *activity);
void reset_today_hours();
void calculate_stats(int *total_hours, double *avg_hours_per_day, double *avg_hours_per_week, double *avg_hours_per_month, double *percent_studied);
void update_stats_label();
gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data);
void on_log_hours(GtkButton *button, gpointer entry);
void on_reset_hours(GtkButton *button, gpointer data);
void on_task_changed(GtkComboBox *combo_box, gpointer data);
void on_add_activity(GtkButton *button, gpointer entry);
void on_remove_activity(GtkButton *button, gpointer entry);
void on_advanced_checkbox_toggled(GtkToggleButton *toggle_button, gpointer data);

// Helper Functions
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
    fwrite(task_data, sizeof(TaskData), MAX_TASKS, file);
    fclose(file);
}

void load_tasks() {
    char path[FILE_PATH_LENGTH];
    get_dotfile_path(path, sizeof(path));

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return;  // No file means no data yet
    }

    fread(task_data, sizeof(TaskData), MAX_TASKS, file);
    fclose(file);
}

void save_activities() {
    char path[FILE_PATH_LENGTH];
    snprintf(path, sizeof(path), "%s/%s", g_get_home_dir(), ACTIVITIES_FILE);

    ensure_directory_exists(path); // Ensure the directory exists

    FILE *file = fopen(path, "w");
    if (file == NULL) {
        perror("Could not open file");
        exit(1);
    }
    for (int i = 0; i < MAX_TASKS; i++) {
        if (strlen(activities[i]) > 0) {
            fprintf(file, "%s\n", activities[i]);
        }
    }
    fclose(file);
}

void load_activities() {
    char path[FILE_PATH_LENGTH];
    snprintf(path, sizeof(path), "%s/%s", g_get_home_dir(), ACTIVITIES_FILE);

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return;  // No file means no data yet
    }

    int i = 0;
    while (i < MAX_TASKS && fgets(activities[i], sizeof(activities[i]), file)) {
        activities[i][strcspn(activities[i], "\n")] = '\0'; // Remove newline
        i++;
    }
    fclose(file);
}

void add_activity(const char *activity) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (strlen(activities[i]) == 0) {
            strncpy(activities[i], activity, sizeof(activities[i]));
            break;
        }
    }
    save_activities();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_combo_box), activity);
}

void reset_today_hours() {
    char date[11];
    get_current_date(date);

    TaskData *data = &task_data[current_task_index];

    for (int i = 0; i < DAYS_IN_YEAR; i++) {
        if (strcmp(data->tasks[i].date, date) == 0) {
            data->tasks[i].hours = 0;  // Reset hours for today
            save_tasks();
            break;
        }
    }

    update_stats_label(); // Update stats after resetting hours
}

void calculate_stats(int *total_hours, double *avg_hours_per_day, double *avg_hours_per_week, double *avg_hours_per_month, double *percent_studied) {
    TaskData *data = &task_data[current_task_index];

    *total_hours = 0;
    int days_with_hours = 0;

    for (int i = 0; i < DAYS_IN_YEAR; i++) {
        if (data->tasks[i].hours > 0) {
            *total_hours += data->tasks[i].hours;
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
             "You spent %.2f%% of the year on this task.\n"
             "Total hours this year: %d hrs\n"
             "Avg hours per day: %.2f hrs\n"
             "Avg hours per week: %.2f hrs\n"
             "Avg hours per month: %.2f hrs",
             percent_studied, total_hours, avg_hours_per_day, avg_hours_per_week, avg_hours_per_month);

    gtk_label_set_text(GTK_LABEL(stats_label), stats_text);
}

gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    int square_size = 20;
    int padding = 2;
    int first_entry_index = -1;

    TaskData *task_data_current = &task_data[current_task_index];

    // Find the first entry
    for (int i = 0; i < DAYS_IN_YEAR; i++) {
        if (task_data_current->tasks[i].hours > 0) {
            first_entry_index = i;
            break;
        }
    }

    for (int i = 0; i < DAYS_IN_YEAR; i++) {
        int display_index = (first_entry_index >= 0) ? (i - first_entry_index + DAYS_IN_YEAR - 1) % DAYS_IN_YEAR : i;
        int hours = task_data_current->tasks[display_index].hours;
        int x = (i % 30) * (square_size + padding); // 30 days per row
        int y = (i / 30) * (square_size + padding);

        if (hours <= 0) {
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // White for no hours
        } else {
            double intensity = (hours > 8) ? 1.0 : (double)hours / 8.0;
            cairo_set_source_rgb(cr, 0.0, intensity, 0.0); // Green based on hours
        }

        cairo_rectangle(cr, x, y, square_size, square_size);
        cairo_fill(cr);
    }

    return FALSE;
}

void on_log_hours(GtkButton *button, gpointer entry) {
    int hours = atoi(gtk_entry_get_text(GTK_ENTRY(entry)));

    if (hours > 0) {
        char date[11];
        get_current_date(date);

        TaskData *data = &task_data[current_task_index];
        for (int i = 0; i < DAYS_IN_YEAR; i++) {
            if (strcmp(data->tasks[i].date, date) == 0) {
                data->tasks[i].hours = hours;
                save_tasks();
                break;
            }
        }

        update_stats_label();
        gtk_widget_queue_draw(drawing_area); // Redraw the heatmap
    }
}

void on_reset_hours(GtkButton *button, gpointer data) {
    reset_today_hours();
    update_stats_label();
    gtk_widget_queue_draw(drawing_area); // Redraw the heatmap
}

void on_task_changed(GtkComboBox *combo_box, gpointer data) {
    current_task_index = gtk_combo_box_get_active(combo_box);

    if (current_task_index >= 0) {
        // Update the task data display
        gtk_widget_queue_draw(drawing_area);
        update_stats_label();
    }
}

void on_add_activity(GtkButton *button, gpointer entry) {
    const char *activity = gtk_entry_get_text(GTK_ENTRY(entry));

    if (strlen(activity) > 0) {
        add_activity(activity);
        gtk_entry_set_text(GTK_ENTRY(entry), ""); // Clear the entry field
    }
}

void on_remove_activity(GtkButton *button, gpointer entry) {
    GtkComboBoxText *combo_box = GTK_COMBO_BOX_TEXT(task_combo_box);
    gint active_index = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_box));

    if (active_index != -1) {
        // Remove only the current day's hours
        char date[11];
        get_current_date(date);

        TaskData *data = &task_data[current_task_index];
        if (strcmp(data->tasks[active_index].date, date) == 0) {
            data->tasks[active_index].hours = 0; // Reset today's hours
            save_tasks();
        }

        gtk_combo_box_text_remove(combo_box, active_index);
        // Remove the activity from the list
        for (int i = 0; i < MAX_TASKS; i++) {
            if (strcmp(activities[i], gtk_combo_box_text_get_active_text(combo_box)) == 0) {
                memset(activities[i], 0, sizeof(activities[i]));
                break;
            }
        }

        save_activities();
        update_stats_label();
        gtk_widget_queue_draw(drawing_area);
    }
}

void on_advanced_checkbox_toggled(GtkToggleButton *toggle_button, gpointer data) {
    gboolean active = gtk_toggle_button_get_active(toggle_button);
    gtk_widget_set_visible(settings_vbox, active);
    gtk_widget_set_visible(button_add_activity, active);
    gtk_widget_set_visible(entry_add_activity, active);
    gtk_widget_set_visible(button_remove_activity, active);
}

int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *scrolled_window;
    GtkWidget *button_log_hours;
    GtkWidget *button_reset_hours;
    GtkWidget *label_task;
    GtkWidget *box;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Task Heatmap");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    stats_label = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(vbox), stats_label, FALSE, FALSE, 0);

    drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(drawing_area, 800, 400);
    gtk_box_pack_start(GTK_BOX(vbox), drawing_area, TRUE, TRUE, 0);
    g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw), NULL);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label_task = gtk_label_new("Task:");
    gtk_box_pack_start(GTK_BOX(hbox), label_task, FALSE, FALSE, 0);

    task_combo_box = gtk_combo_box_text_new();
    gtk_box_pack_start(GTK_BOX(hbox), task_combo_box, TRUE, TRUE, 0);
    g_signal_connect(task_combo_box, "changed", G_CALLBACK(on_task_changed), NULL);

    button_log_hours = gtk_button_new_with_label("Log Hours");
    gtk_box_pack_start(GTK_BOX(hbox), button_log_hours, FALSE, FALSE, 0);

    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);

    g_signal_connect(button_log_hours, "clicked", G_CALLBACK(on_log_hours), entry);

    button_reset_hours = gtk_button_new_with_label("Reset Hours");
    gtk_box_pack_start(GTK_BOX(hbox), button_reset_hours, FALSE, FALSE, 0);
    g_signal_connect(button_reset_hours, "clicked", G_CALLBACK(on_reset_hours), NULL);

    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, FALSE, 0);

    advanced_checkbox = gtk_check_button_new_with_label("Show Advanced Settings");
    gtk_box_pack_start(GTK_BOX(box), advanced_checkbox, FALSE, FALSE, 0);
    g_signal_connect(advanced_checkbox, "toggled", G_CALLBACK(on_advanced_checkbox_toggled), NULL);

    settings_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), settings_vbox, FALSE, FALSE, 0);

    entry_add_activity = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(settings_vbox), entry_add_activity, FALSE, FALSE, 0);

    button_add_activity = gtk_button_new_with_label("Add Activity");
    gtk_box_pack_start(GTK_BOX(settings_vbox), button_add_activity, FALSE, FALSE, 0);
    g_signal_connect(button_add_activity, "clicked", G_CALLBACK(on_add_activity), entry_add_activity);

    button_remove_activity = gtk_button_new_with_label("Remove Activity");
    gtk_box_pack_start(GTK_BOX(settings_vbox), button_remove_activity, FALSE, FALSE, 0);
    g_signal_connect(button_remove_activity, "clicked", G_CALLBACK(on_remove_activity), NULL);

    gtk_widget_set_visible(settings_vbox, FALSE);
    gtk_widget_set_visible(button_add_activity, FALSE);
    gtk_widget_set_visible(entry_add_activity, FALSE);
    gtk_widget_set_visible(button_remove_activity, FALSE);

    load_tasks();
    load_activities();
    update_stats_label();

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}

