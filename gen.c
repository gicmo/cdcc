#include "db.h"

#include <stdio.h>
#include <string.h>

#include <json-glib/json-glib.h>

typedef struct ExportData {
  JsonBuilder *builder;
  int rows;
} ExportData;

static gboolean export_single(const Record *data, gpointer user_data)
{
  ExportData *udata = (ExportData *) user_data;
  JsonBuilder *jsb = udata->builder;

  udata->rows += 1;
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
  ExportData udata = {.builder = jsb, .rows = 0 };

  json_builder_begin_array(jsb);
  g_autofree char *query = g_build_filename(path, "*", NULL);
  db_query(db, query, export_single, &udata);
  json_builder_end_array(jsb);

  g_autoptr(JsonNode) root = json_builder_get_root(jsb);
  g_autoptr(JsonGenerator) gen = json_generator_new();

  json_generator_set_pretty(gen, TRUE);
  json_generator_set_root(gen, root);

  if (udata.rows > 0) {
    g_autofree gchar *cdbpath = g_build_filename(path, "compile_commands.json", NULL);
    gboolean res = json_generator_to_file(gen, cdbpath, NULL);
    return res ? udata.rows : -1;
  }

  return 0;
}

int main(int argc, char **argv)
{
  if (argc < 2) {
    fprintf(stderr, "Nothing to do.\n");
    return 0;
  }

  g_autofree gchar *dbpath = db_path();

  if (dbpath == NULL) {
    return 1;
  }

  sqlite3 *db;
  db = db_open(dbpath);

  if (db == NULL) {
    return 1;
  }

  for (int i = 1; i < argc; i++) {
    g_autoptr(GFile) f = g_file_new_for_commandline_arg(argv[i]);
    GFileType ft = g_file_query_file_type(f, G_FILE_QUERY_INFO_NONE, NULL);

    g_autofree char *path = g_file_get_path(f);
    fprintf(stderr, " %s:", path);

    if (ft != G_FILE_TYPE_DIRECTORY) {
      fprintf(stderr, " Skipped\n");
      continue;
    }

    int res = export_json(db, path);
    if (res < 0) {
      fprintf(stderr, "\033[1;31m FAIL\033[0m\n");
    } else if (res == 0) {
      fprintf(stderr, "\033[1;33m No data\033[0m\n");
    } else {
      fprintf(stderr, "\033[1;32m OK\033[0m [%d]\n", res);
    }
  }

  db_close(db);
  return 0;
}
