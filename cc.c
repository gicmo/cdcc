#include <glib.h>
#include <stdio.h>
#include <sqlite3.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>

static const char *known_types[] = {
  ".h",
  ".hpp",
  ".c",
  ".cc",
  ".cxx",
  ".cpp",
  NULL
};

static gboolean is_known_type(const char *name)
{
  const char **iter;

  for (iter = known_types; *iter; iter++) {
    if (g_str_has_suffix(name, *iter)) {
      return TRUE;
    }
  }

  return FALSE;
}

static sqlite3 *db_open(const char *path)
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

static void db_close(sqlite3 *db) {
  if (db != NULL) {
    sqlite3_close(db);
  }
}

static void db_insert(sqlite3 *db, GList *files, const gchar * const *argv) {
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


    res = sqlite3_step(stmt);
    if (res != SQLITE_DONE) {
      g_warning("SQL: could not insert for %s\n", fn);
    }
  }

}

static gchar *
extract_toolname(const char *name) {
  gchar *pos = g_strstr_len(name, -1, "-");

  if (pos == NULL || *++pos == '\0') {
    return NULL;
  }

  return g_strdup(pos);
}

static int call_tool(const gchar * const *args) {

  g_auto(GStrv) argv = g_strdupv((gchar **) args);

  //
  GPid pid = 0;
  GError *err = NULL;
  gboolean ok = g_spawn_async_with_pipes(NULL, //wdir, i.e. inherit
                                         argv, //argv
                                         NULL, //evnp, i.e. inherit
                                         G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                                         NULL, NULL, //child setup func
                                         &pid,
                                         NULL, NULL, NULL, //std{i, o, e}, i.e inherit
                                         &err);

  if (!ok) {
    g_warning("Could not spawn %s: %s", args[0], err->message);
    return 1;
  }


  int res, status;
 again:
  res = waitpid(pid, &status, 0);

  if (res < 0) {
    if (errno == EINTR) {
      goto again;
    }
    int errsave = errno;

    g_warning("waitpid error %s", g_strerror(errsave));
    return 1;
  }


  return status;
}

static int save_flags(const gchar * const *args) {

  const gchar *db_path = g_getenv("CDCC_DB");
  if (db_path == NULL) {
    return 0;
  }

  GList* files = NULL;

  const gchar * const *iter;
  for (iter = args + 1; *iter; iter++) {
    const gchar *option = *iter;
    if (!g_str_has_prefix(option, "-")) {
      if (is_known_type(option)) {
        files = g_list_append(files, (gpointer) option);
      }

      continue;
    }

    if (g_str_equal(option, "-o")) {
      iter++;
      continue;
    }
  }


  //success for the tool/compiler, let's record the files in the db
  if (g_list_length(files) > 0) {
    sqlite3 *db;

    db = db_open(db_path);

    if (db == NULL) {
      goto compile;
    }

    db_insert(db, files, args);
    db_close(db);
  }

 compile:
  return 0;
}

static GStrv
convert_argv(int argc, char **argv, const gchar *toolpath) {
  int i;

  GStrv args = g_new0(gchar *, argc+1);
  args[0] = g_strdup(toolpath);
  for (i = 1; i < argc; i++) {
    args[i] = g_strdup(argv[i]);
  }

  return args;
}

int main(int argc, char **argv) {

  g_autofree gchar *toolname = extract_toolname(argv[0]);

  if (toolname == NULL) {
    toolname = g_strdup("cc");
  }

  g_autofree gchar *toolpath = g_find_program_in_path(toolname);
  g_auto(GStrv) args = convert_argv(argc, argv, toolpath);

  int res = call_tool((const char **) args);

  if (res != 0) {
    return res;
  }

  //it is not a fatal error if something goes wrong saving to the db
  save_flags((const char**) args);

  return res;
}
