#include "../s21_viewer.h"

bool is_null_or_empty(const gchar* string) {
  bool return_value = true;

  if (string != NULL && strcmp(string, "") != 0) {
    size_t len = strlen(string);
    for (size_t i = 0; i < len && return_value == true; ++i) {
      if (isspace(string[i]) == false) {
        return_value = false;
      }
    }
  }

  return return_value;
}

bool convert_to_double(const gchar* string, double* val) {
  bool return_val = true;
  char* endptr = NULL;
  *val = strtod(string, &endptr);
  if (strcmp(endptr, "\0") != 0) {
    return_val = false;
  }
  return return_val;
}

bool convert_scale_to_double(const gchar* string, double* val) {
  bool return_val = true;
  if (is_null_or_empty(string)) {
    *val = 1;
  } else {
    return_val = convert_to_double(string, val);
  }
  return return_val;
}

void set_angle_struct(coords_t* angles, gchar* choice, double value) {
  if (strcmp(choice, "x") == 0) {
    angles->x = value;
  } else if (strcmp(choice, "y") == 0) {
    angles->y = value;
  } else if (strcmp(choice, "z") == 0) {
    angles->z = value;
  }
}

void load_file(GtkWidget* widget, gpointer ptr_array) {
  UNUSED(widget);

  GPtrArray* array = ptr_array;
  GtkBuilder* builder = array->pdata[0];
  obj_data* data = array->pdata[1];

  GtkFileChooser* file_chooser =
      GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "file_selector"));
  GObject* label = gtk_builder_get_object(builder, "viewer_label");

  gchar* filename = gtk_file_chooser_get_filename(file_chooser);
  if (is_null_or_empty(filename)) {
    gtk_label_set_label(GTK_LABEL(label), MISSING_FILE_MSG);
  } else {
    if (data->coords != NULL) {
      s21_free_obj_data(data);
    }
    result_code_t result_code = s21_parse_obj_to_struct(data, filename);
    if (result_code != SUCCESS) {
      gtk_label_set_label(GTK_LABEL(label), INVALID_FILE_MSG);
    }
  }
  g_free(filename);
}

void render_with_deltas(GtkWidget* widget, gpointer ptr_array) {
  UNUSED(widget);

  GPtrArray* array = ptr_array;
  GtkBuilder* builder = array->pdata[0];
  obj_data* data = array->pdata[1];

  GtkWidget* image =
      GTK_WIDGET(gtk_builder_get_object(builder, "visualization_image"));
  GtkLabel* label = GTK_LABEL(gtk_builder_get_object(builder, "viewer_label"));
  GPtrArray* delta_data = collect_delta_data(builder);

  if (delta_data->len != 0) {
    if (data->coords == NULL) {
      gtk_label_set_label(label, FILE_NOT_LOADED_MSG);
    } else {
      coords_t* scales_delta = delta_data->pdata[0];
      result_code_t scale_res = s21_scale(data, scales_delta);
      if (scale_res != SUCCESS) {
        gtk_label_set_label(label, ZERO_SCALE_MSG);
      }

      coords_t* coords_delta = delta_data->pdata[1];
      s21_move(data, coords_delta);

      coords_t* angles_delta = delta_data->pdata[2];
      s21_rotate(data, angles_delta);

      s21_write_coords_to_file(data, POINTS_FILE);
      call_gnuplot(image);
    }
  }
  for (guint i = 0; i < delta_data->len; ++i) {
    free(delta_data->pdata[i]);
  }
  g_ptr_array_free(delta_data, true);
}

GPtrArray* collect_delta_data(GtkBuilder* builder) {
  GPtrArray* delta_data = g_ptr_array_new();

  coords_t* coords = calloc(1, sizeof(coords_t));
  coords_t* angles = calloc(1, sizeof(coords_t));
  coords_t* scales = calloc(1, sizeof(coords_t));

  GObject* x_scale = gtk_builder_get_object(builder, "x_scale_delta_entry");
  GObject* y_scale = gtk_builder_get_object(builder, "y_scale_delta_entry");
  GObject* z_scale = gtk_builder_get_object(builder, "z_scale_delta_entry");
  GObject* x_coord = gtk_builder_get_object(builder, "x_coord_delta_entry");
  GObject* y_coord = gtk_builder_get_object(builder, "y_coord_delta_entry");
  GObject* z_coord = gtk_builder_get_object(builder, "z_coord_delta_entry");
  GObject* angle_obj = gtk_builder_get_object(builder, "angle_delta_entry");
  GObject* combo_object = gtk_builder_get_object(builder, "angle_combo_box");

  gchar* x_scale_text = (gchar*)gtk_entry_get_text(GTK_ENTRY(x_scale));
  gchar* y_scale_text = (gchar*)gtk_entry_get_text(GTK_ENTRY(y_scale));
  gchar* z_scale_text = (gchar*)gtk_entry_get_text(GTK_ENTRY(z_scale));
  gchar* x_coord_text = (gchar*)gtk_entry_get_text(GTK_ENTRY(x_coord));
  gchar* y_coord_text = (gchar*)gtk_entry_get_text(GTK_ENTRY(y_coord));
  gchar* z_coord_text = (gchar*)gtk_entry_get_text(GTK_ENTRY(z_coord));
  gchar* angle_text = (gchar*)gtk_entry_get_text(GTK_ENTRY(angle_obj));
  GtkComboBoxText* angle_combo = GTK_COMBO_BOX_TEXT(combo_object);
  gchar* combo_text = gtk_combo_box_text_get_active_text(angle_combo);

  replace_char(x_scale_text, '.', ',');
  replace_char(y_scale_text, '.', ',');
  replace_char(z_scale_text, '.', ',');
  replace_char(x_coord_text, '.', ',');
  replace_char(y_coord_text, '.', ',');
  replace_char(z_coord_text, '.', ',');
  replace_char(angle_text, '.', ',');

  double angle;
  if (convert_scale_to_double(x_scale_text, &scales->x) == false ||
      convert_scale_to_double(y_scale_text, &scales->y) == false ||
      convert_scale_to_double(z_scale_text, &scales->z) == false ||
      convert_to_double(x_coord_text, &coords->x) == false ||
      convert_to_double(y_coord_text, &coords->y) == false ||
      convert_to_double(z_coord_text, &coords->z) == false ||
      convert_to_double(angle_text, &angle) == false) {
    GObject* label = gtk_builder_get_object(builder, "viewer_label");
    gtk_label_set_label(GTK_LABEL(label), WRONG_INPUT_MSG);
  } else {
    set_angle_struct(angles, combo_text, angle);
    g_ptr_array_add(delta_data, scales);
    g_ptr_array_add(delta_data, coords);
    g_ptr_array_add(delta_data, angles);
  }
  g_free(combo_text);

  return delta_data;
}
