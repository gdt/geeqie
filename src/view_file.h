/*
 * Geeqie
 * Copyright (C) 2008 - 2012 The Geeqie Team
 *
 * Author: Laurent Monin
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */

#ifndef VIEW_FILE_H
#define VIEW_FILE_H

#define VIEW_FILE_TYPES_COUNT 2

#define VFLIST(_vf_) ((ViewFileInfoList *)(_vf_->info))
#define VFICON(_vf_) ((ViewFileInfoIcon *)(_vf_->info))

void vf_send_update(ViewFile *vf);

ViewFile *vf_new(FileViewType type, FileData *dir_fd);

void vf_set_status_func(ViewFile *vf, void (*func)(ViewFile *vf, gpointer data), gpointer data);
void vf_set_thumb_status_func(ViewFile *vf, void (*func)(ViewFile *vf, gdouble val, const gchar *text, gpointer data), gpointer data);

void vf_set_layout(ViewFile *vf, LayoutWindow *layout);

gboolean vf_set_fd(ViewFile *vf, FileData *fd);
gboolean vf_refresh(ViewFile *vf);
void vf_refresh_idle(ViewFile *vf);

void vf_thumb_set(ViewFile *vf, gboolean enable);
void vf_marks_set(ViewFile *vf, gboolean enable);
void vf_sort_set(ViewFile *vf, SortType type, gboolean ascend);

guint vf_marks_get_filter(ViewFile *vf);
void vf_mark_filter_toggle(ViewFile *vf, gint mark);

GList *vf_selection_get_one(ViewFile *vf, FileData *fd);
GList *vf_pop_menu_file_list(ViewFile *vf);
GtkWidget *vf_pop_menu(ViewFile *vf);

FileData *vf_index_get_data(ViewFile *vf, gint row);
gint vf_index_by_fd(ViewFile *vf, FileData *in_fd);
guint vf_count(ViewFile *vf, gint64 *bytes);
GList *vf_get_list(ViewFile *vf);

gint vf_index_is_selected(ViewFile *vf, gint row);
guint vf_selection_count(ViewFile *vf, gint64 *bytes);
GList *vf_selection_get_list(ViewFile *vf);
GList *vf_selection_get_list_by_index(ViewFile *vf);

void vf_select_all(ViewFile *vf);
void vf_select_none(ViewFile *vf);
void vf_select_invert(ViewFile *vf);
void vf_select_by_fd(ViewFile *vf, FileData *fd);

void vf_mark_to_selection(ViewFile *vf, gint mark, MarkToSelectionMode mode);
void vf_selection_to_mark(ViewFile *vf, gint mark, SelectionToMarkMode mode);

void vf_refresh_idle_cancel(ViewFile *vf);
void vf_notify_cb(FileData *fd, NotifyType type, gpointer data);

void vf_thumb_update(ViewFile *vf);
void vf_thumb_cleanup(ViewFile *vf);
void vf_thumb_stop(ViewFile *vf);

#endif /* VIEW_FILE_H */
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
