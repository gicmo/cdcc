#include "db.h"

#include <string.h>
#include <glib/gprintf.h>

static gboolean list_match(const Record *data, gpointer user_data)
{
  g_printf(" %s\n", data->filename);
  return TRUE;
}

int main(int argc, char **argv) {

  if (argc < 2) {
    return 0;
  }

  g_autofree gchar *dbpath = db_path();

  if (dbpath == NULL) {
    return 1;
  }

  sqlite3 *db;
  db = db_open(dbpath);

  const gchar *path = argv[1];

  g_autofree char *query = g_strdup_printf("*%s*", path);
  db_query(db, query, list_match, NULL);

  return 0;
}
