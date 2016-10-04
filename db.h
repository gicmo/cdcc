#ifndef CDCC_DB_H
#define CDCC_DB_H

#include <glib.h>
#include <sqlite3.h>

G_BEGIN_DECLS

typedef struct Record {
  const unsigned char *dir;
  const unsigned char *filename;
  const unsigned char *args;
} Record;

typedef gboolean (*db_query_result_fn) (const Record *data, gpointer user_data);

sqlite3 *   db_open      (const char *path);
void        db_close     (sqlite3 *db);
void        db_insert    (sqlite3 *db, GList *files, const gchar * const *argv);

gboolean    db_query     (sqlite3 *db, const char *path, db_query_result_fn fn,
                          gpointer user_data);

G_END_DECLS

#endif
