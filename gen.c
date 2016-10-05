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
  g_autofree char *query = g_build_filename(path, "*", NULL);
  db_query(db, query, export_single, jsb);
  json_builder_end_array(jsb);

  g_autoptr(JsonNode) root = json_builder_get_root(jsb);
  g_autoptr(JsonGenerator) gen = json_generator_new();

  json_generator_set_pretty(gen, TRUE);
  json_generator_set_root(gen, root);

  g_autofree gchar *cdbpath = g_build_filename(path, "compile_commands.json", NULL);
  gboolean res = json_generator_to_file(gen, cdbpath, NULL);
  return res ? 0 : 1;
}

int main(int argc, char **argv)
{
  if (argc < 2) {
    fprintf(stderr, "Nothing to do.\n");
    return 0;
  }

  const gchar *db_path = g_getenv("CDCC_DB");

  if (db_path == NULL) {
    fprintf(stderr, "CDCC_DB not specified\n");
    return 1;
  }

  sqlite3 *db;
  db = db_open(db_path);

  if (db == NULL) {
    return 1;
  }

  for (int i = 1; i < argc; i++) {
    const char *path = argv[i];
    fprintf(stderr, " %s: ", path);
    int res = export_json(db, argv[i]);
    if (res != 0) {
      fprintf(stderr, "FAIL (%d)\n", res);
    } else {
      fprintf(stderr, "OK\n");
    }
  }

  db_close(db);
  return 0;
}
