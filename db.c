#include "db.h"

#include <string.h>
#include <stdio.h>

sqlite3 *db_open(const char *path)
{
  sqlite3 *db = NULL;

  int res = sqlite3_open(path, &db);
  if (res != SQLITE_OK) {
    g_warning("Cannot open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return NULL;
  }

  static const char *sql =
    "CREATE TABLE IF NOT EXISTS "
    "cflags(dir TEXT, file TEXT, flags TEXT, "
    "PRIMARY KEY(dir, file) ON CONFLICT REPLACE);";

  char *emsg = NULL;
  res = sqlite3_exec(db, sql, 0, 0, &emsg);

  if (res != SQLITE_OK) {
    g_warning("SQL error: %s\n", emsg);
    sqlite3_free(emsg);
    sqlite3_close(db);
    return NULL;
  }

  res = sqlite3_busy_timeout(db, 1000);

  if (res != SQLITE_OK) {
    g_warning("SQL: Could not set busy timeout");
  }

  return db;
}

void db_close(sqlite3 *db) {
  if (db != NULL) {
    sqlite3_close(db);
  }
}

void db_insert(sqlite3 *db, GList *files, const gchar * const *argv) {
  static const char *sql =
    "INSERT INTO cflags(dir, file, flags) VALUES(?, ?, ?);";

  sqlite3_stmt *stmt;

  int res = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
  if (res != SQLITE_OK) {
    g_warning("SQL: Could not prepare statement: %s", sqlite3_errmsg(db));
    return;
  }

  g_autofree gchar *cwd = g_get_current_dir();
  g_autofree gchar *flags = g_strjoinv(" ", (gchar **) argv);

  GList *iter;
  for (iter = files; iter; iter = iter ->next) {
    char *fn = iter->data;
    fprintf(stderr, "file: %s [%s]\n", fn, flags);

    res = sqlite3_bind_text(stmt, 1, cwd, strlen(cwd), 0);
    if (res != SQLITE_OK) {
      g_warning("SQL: could not bind for %s\n", fn);
      continue;
    }

    res = sqlite3_bind_text(stmt, 2, fn, strlen(fn), 0);
    if (res != SQLITE_OK) {
      g_warning("SQL: could not bind for %s\n", fn);
      continue;
    }

    res = sqlite3_bind_text(stmt, 3, flags, strlen(flags), 0);
    if (res != SQLITE_OK) {
      g_warning("SQL: could not bind for %s\n", fn);
      continue;
    }

    //insert data now
    res = sqlite3_step(stmt);
    if (res != SQLITE_DONE) {
      g_warning("SQL: could not insert for %s\n", fn);
      break;
    } else {
      //should not fail after a successful call to _step()
      sqlite3_reset(stmt);
    }
  }

}

