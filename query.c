#include "db.h"

#include <stdio.h>
#include <string.h>


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


  const char *sql = "SELECT * from cflags WHERE dir GLOB ?";

  sqlite3_stmt *stmt;

  int res = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
  if (res != SQLITE_OK) {
    g_warning("SQL: Could not prepare statement: %s", sqlite3_errmsg(db));
    goto out;
  }

  res = sqlite3_bind_text(stmt, 1, path, strlen(path), 0);
  if (res != SQLITE_OK) {
    g_warning("SQL: could not bind for %s\n", path);
    goto out;
  }

  for ( ; ; ) {
    res = sqlite3_step(stmt);

    if (res == SQLITE_ROW) {

      const unsigned char *dir = sqlite3_column_text(stmt, 0);
      const unsigned char *filename = sqlite3_column_text(stmt, 1);
      const unsigned char *cflags = sqlite3_column_text(stmt, 2);

      if (dir == NULL || filename == NULL || cflags == NULL) {
        fprintf(stderr, "SQL: NULL values in row. skipping");
        continue;
      }

      printf("path: %s, file: %s, flags: %s", dir, filename, cflags);
    } else if (res == SQLITE_DONE) {
      break;
    } else {
      fprintf(stderr, "SQL: Could not get data: %s\n", sqlite3_errmsg(db));
      goto out;
    }

  }

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
