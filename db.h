#ifndef CDCC_DB_H
#define CDCC_DB_H

#include <glib.h>
#include <sqlite3.h>

G_BEGIN_DECLS


sqlite3 *   db_open      (const char *path);
void        db_close     (sqlite3 *db);
void        db_insert    (sqlite3 *db, GList *files, const gchar * const *argv);

G_END_DECLS

#endif
