#include "db.h"

#include <gio/gio.h>

#include <string.h>
#include <stdio.h>

gchar *db_path(gboolean warn)
{
  const gchar *db_path = g_getenv("CDCC_DB");
  if (db_path == NULL && warn) {
    fprintf(stderr, "CDCC_DB not specified\n");
  }
  return g_strdup(db_path);
}

sqlite3 *db_open(const char *path)
{
  sqlite3 *db = NULL;

  int res = sqlite3_open(path, &db);
  if (res != SQLITE_OK) {
    g_warning("Cannot open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return NULL;
  }

  res = sqlite3_busy_timeout(db, 1000);

  if (res != SQLITE_OK) {
    g_warning("SQL: Could not set busy timeout");
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

  return db;
}

void db_close(sqlite3 *db) {
  if (db != NULL) {
    sqlite3_close(db);
  }
}

void db_insert(sqlite3 *db, GFile *dir, GList *files, const gchar * const *argv) {
  static const char *sql =
    "INSERT INTO cflags(dir, file, flags) VALUES(?, ?, ?);";

  sqlite3_stmt *stmt;

  int res = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
  if (res != SQLITE_OK) {
    g_warning("SQL: Could not prepare statement: %s", sqlite3_errmsg(db));
    return;
  }

  g_autofree gchar *cwd = g_file_get_path(dir);
  g_autofree gchar *flags = g_strjoinv(" ", (gchar **) argv);

  GList *iter;
  for (iter = files; iter; iter = iter ->next) {
    GFile *f = (GFile *) iter->data;
    g_autofree char *abspath = g_file_get_path(f);

    res = sqlite3_bind_text(stmt, 1, cwd, strlen(cwd), 0);
    if (res != SQLITE_OK) {
      g_warning("SQL: could not bind for %s\n", abspath);
      continue;
    }

    res = sqlite3_bind_text(stmt, 2, abspath, strlen(abspath), 0);

    if (res != SQLITE_OK) {
      g_warning("SQL: could not bind for %s\n", abspath);
      continue;
    }

    res = sqlite3_bind_text(stmt, 3, flags, strlen(flags), 0);
    if (res != SQLITE_OK) {
      g_warning("SQL: could not bind for %s\n", abspath);
      continue;
    }

    //insert data now
    res = sqlite3_step(stmt);

    if (res != SQLITE_DONE) {
      g_warning("SQL: could not insert for %s\n", abspath);
      break;
    } else {
      //should not fail after a successful call to _step()
      sqlite3_reset(stmt);
    }
  }

}

gboolean
db_query (sqlite3 *db, const char *path, db_query_result_fn fn, gpointer user_data)
{
   const char *sql = "SELECT * from cflags WHERE file GLOB ?";

  sqlite3_stmt *stmt;

  int res = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
  if (res != SQLITE_OK) {
    g_warning("SQL: Could not prepare statement: %s", sqlite3_errmsg(db));
    return FALSE;
  }

  res = sqlite3_bind_text(stmt, 1, path, strlen(path), 0);
  if (res != SQLITE_OK) {
    g_warning("SQL: could not bind for %s\n", path);
    return FALSE;
  }

  for (gboolean keep_going = TRUE; keep_going; ) {
    res = sqlite3_step(stmt);

    if (res == SQLITE_ROW) {
      Record rec;
      rec.dir = sqlite3_column_text(stmt, 0);
      rec.filename = sqlite3_column_text(stmt, 1);
      rec.args = sqlite3_column_text(stmt, 2);

      if (rec.dir == NULL || rec.filename == NULL || rec.args == NULL) {
        fprintf(stderr, "SQL: NULL values in row. skipping");
        continue;
      }

      keep_going = (*fn)(&rec, user_data);

    } else if (res == SQLITE_DONE) {
      break;
    } else {
      fprintf(stderr, "SQL: Could not get data: %s\n", sqlite3_errmsg(db));
      return FALSE;
    }

  }

  sqlite3_finalize(stmt);
  return TRUE;
}
