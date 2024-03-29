
#include "db.h"

#include <glib.h>
#include <gio/gio.h>

#include <stdio.h>
#include <sqlite3.h>
#include <string.h>

#include <sys/types.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#endif

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

static GFile *current_dir()
{
  g_autofree gchar *cwd = g_get_current_dir();
  GFile *dir = g_file_new_for_path(cwd);

  return dir;
}

static gchar *
extract_toolname(const char *name) {
  gchar *pos = strrchr(name, '/');
  pos = strchr(pos ? pos : name, '-');

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
#ifdef _WIN32
  res = WaitForSingleObject(pid, INFINITE);
  if (res != WAIT_OBJECT_0)
  {
    int err = GetLastError();
    g_warning("WaitForSingleObject error %d", err);
    g_spawn_close_pid(pid);
    return 1;
  }
  GetExitCodeProcess(pid, (LPDWORD)&status);
  g_spawn_close_pid(pid);
#else
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

  status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
#endif

  return status;
}

static int save_flags(const gchar * const *args) {

  g_autofree gchar *dbpath = db_path();
  g_autoptr(GList) files = NULL;
  g_autofree gchar *cwd = g_get_current_dir();
  g_autoptr(GFile) dir = g_file_new_for_path(cwd);

  const gchar * const *iter;
  for (iter = args + 1; *iter; iter++) {
    const gchar *option = *iter;

    if (!g_str_has_prefix(option, "-")) {
      if (is_known_type(option)) {
        const gchar *fn = option;

        GFile *f = NULL;
        if (g_path_is_absolute(fn)) {
          f = g_file_new_for_path(fn);
        } else {
          f = g_file_resolve_relative_path(dir, fn);
        }

        files = g_list_append(files, f);
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

    db = db_open(dbpath);

    if (db == NULL) {
      goto compile;
    }

    g_autoptr (GFile) dir = current_dir();

    db_insert(db, dir, files, args);
    db_close(db);
  }

 compile:
  g_list_free_full(files, g_object_unref);
  return 0;
}

int main(int argc, char **argv) {

  g_autofree gchar *toolname = extract_toolname(argv[0]);

  if (toolname == NULL) {
    toolname = g_strdup("cc");
  }

  g_autofree gchar *toolpath = g_find_program_in_path(toolname);
  argv[0] = toolpath;

  int res = call_tool((const char **) argv);

  if (res != 0) {
    return res;
  }

  //it is not a fatal error if something goes wrong saving to the db
  save_flags((const char **) argv);

  return res;
}
