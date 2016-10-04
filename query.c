#include "db.h"

#include <stdio.h>
#include <string.h>

#include <json-glib/json-glib.h>

static gboolean export_single(const Record *data, gpointer user_data)
{
  JsonBuilder *jsb = (JsonBuilder *) user_data;
  //see http://clang.llvm.org/docs/JSONCompilationDatabase.html

  json_builder_begin_object(jsb);
  json_builder_set_member_name(jsb, "directory");
  json_builder_add_string_value(jsb, (const gchar *) data->dir);

  json_builder_set_member_name(jsb, "command");
  json_builder_add_string_value(jsb, (const gchar *) data->args);

  json_builder_set_member_name(jsb, "file");
  json_builder_add_string_value(jsb, (const gchar *) data->filename);

  //printf("%s, %s, %s\n", data->dir, data->filename, data->args);

  json_builder_end_object(jsb);
  return TRUE;
}

static int export_json(sqlite3 *db, const char *path)
{
  g_autoptr(JsonBuilder) jsb = json_builder_new();

  json_builder_begin_array(jsb);
  db_query(db, path, export_single, jsb);
  json_builder_end_array(jsb);

  g_autoptr(JsonNode) root = json_builder_get_root(jsb);
  g_autoptr(JsonGenerator) gen = json_generator_new();

  json_generator_set_pretty(gen, TRUE);
  json_generator_set_root(gen, root);

  gsize len = 0;
  g_autofree gchar *data = json_generator_to_data(gen, &len);
  g_print("%s\n", data);

  return 0;
}

int main(int argc, char **argv)
{
  const char *path = "*";
  if (argc > 1) {
    path = argv[1];
  }

  const gchar *db_path = g_getenv("CDCC_DB");

  if (db_path == NULL) {
    fprintf(stderr, "CDCC_DB not specified");
    return 1;
  }

  sqlite3 *db;
  db = db_open(db_path);

  if (db == NULL) {
    return 1;
  }

  int res = export_json(db, path);

  db_close(db);
  return res;
}
