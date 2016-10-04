#include "db.h"

#include <stdio.h>
#include <string.h>


static gboolean export_single(const Record *data, gpointer user_data)
{
  printf("%s, %s, %s\n", data->dir, data->filename, data->args);
  return TRUE;
}

static int export_data(const char *path) {
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

  db_query(db, path, export_single, NULL);

 out:
  db_close(db);
  return 0;
}

int main(int argc, char **argv)
{
  const char *path = "*";
  if (argc > 1) {
    path = argv[1];
  }

  int res = export_data(path);
  return res;
}
