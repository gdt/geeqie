/*
 * Geeqie
 * (C) 2004 John Ellis
 * Copyright (C) 2008 The Geeqie Team
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */


#include "main.h"
#include "collect.h"
#include "image.h"
#include "slideshow.h"
#include "filedata.h"

#include "layout.h"
#include "layout_image.h"
#include "ui_fileops.h"


static void slideshow_timer_stop(SlideShowData *ss);


void slideshow_free(SlideShowData *ss)
{
	if (!ss) return;

	slideshow_timer_stop(ss);

	if (ss->stop_func) ss->stop_func(ss, ss->stop_data);

	if (ss->filelist) filelist_free(ss->filelist);
	if (ss->cd) collection_unref(ss->cd);
	file_data_unref(ss->dir_fd);

	g_list_free(ss->list);
	g_list_free(ss->list_done);

	file_data_unref(ss->slide_fd);

	g_free(ss);
}

static GList *generate_list(SlideShowData *ss)
{
	GList *list = NULL;

	if (ss->from_selection)
		{
		list = layout_selection_list_by_index(ss->layout);
		}
	else
		{
		guint i;
		for (i = 0; i < ss->slide_count; i++)
			{
			list = g_list_prepend(list, GINT_TO_POINTER(i));
			}
		list = g_list_reverse(list);
		}

	return list;
}

static void ptr_array_add(gpointer data, GPtrArray *array)
{
	g_ptr_array_add(array, data);
}

static void list_prepend(gpointer data, GList **list)
{
	*list = g_list_prepend(*list, data);
}

static GPtrArray *generate_ptr_array_from_list(GList *src_list)
{
	GPtrArray *arr = g_ptr_array_sized_new(g_list_length(src_list));

	g_list_foreach(src_list, (GFunc) ptr_array_add, arr);

	return arr;
}

static void swap(GPtrArray *array, guint index1, guint index2)
{
	gpointer temp = g_ptr_array_index(array, index1);
	
	g_ptr_array_index(array, index1) = g_ptr_array_index(array, index2);
	g_ptr_array_index(array, index2) = temp;
}

static void ptr_array_random_shuffle(GPtrArray *array)
{
	guint i;
	for (i = 0; i < array->len; ++i)
		{
		guint p = (double)rand() / ((double)RAND_MAX + 1.0) * array->len;
		swap(array, i, p);
		}
}

static GList *generate_random_list(SlideShowData *ss)
{
	GList *src_list;
	GPtrArray *src_array;
	GList *list = NULL;

	src_list = generate_list(ss);
	src_array = generate_ptr_array_from_list(src_list);
	g_list_free(src_list);

	ptr_array_random_shuffle(src_array);
	g_ptr_array_foreach(src_array, (GFunc) list_prepend, &list);
	g_ptr_array_free(src_array, TRUE);
	
	return list;
}

static void slideshow_list_init(SlideShowData *ss, gint start_index)
{
	if (ss->list_done)
		{
		g_list_free(ss->list_done);
		ss->list_done = NULL;
		}

	if (ss->list) g_list_free(ss->list);

	if (options->slideshow.random)
		{
		ss->list = generate_random_list(ss);
		}
	else
		{
		ss->list = generate_list(ss);
		if (start_index >= 0)
			{
			/* start with specified image by skipping to it */
			gint i = 0;

			while (ss->list && i < start_index)
				{
				ss->list_done = g_list_prepend(ss->list_done, ss->list->data);
				ss->list = g_list_remove(ss->list, ss->list->data);
				i++;
				}
			}
		}
}

gint slideshow_should_continue(SlideShowData *ss)
{
	FileData *imd_fd;
	FileData *dir_fd;

	if (!ss) return FALSE;

	imd_fd = image_get_fd(ss->imd);

	if ( ((imd_fd == NULL) != (ss->slide_fd == NULL)) ||
	    (imd_fd && ss->slide_fd && imd_fd != ss->slide_fd) ) return FALSE;

	if (ss->filelist) return TRUE;

	if (ss->cd)
		{
		if (g_list_length(ss->cd->list) == ss->slide_count)
			return TRUE;
		else
			return FALSE;
		}

	if (!ss->layout) return FALSE;
	dir_fd = ss->layout->dir_fd;

	if (dir_fd && ss->dir_fd && dir_fd == ss->dir_fd)
		{
		if (ss->from_selection && ss->slide_count == layout_selection_count(ss->layout, NULL)) return TRUE;
		if (!ss->from_selection && ss->slide_count == layout_list_count(ss->layout, NULL)) return TRUE;
		}

	return FALSE;
}

static gint slideshow_step(SlideShowData *ss, gboolean forward)
{
	gint row;

	if (!slideshow_should_continue(ss))
		{
		return FALSE;
		}

	if (forward)
		{
		if (!ss->list) return TRUE;

		row = GPOINTER_TO_INT(ss->list->data);
		ss->list_done = g_list_prepend(ss->list_done, ss->list->data);
		ss->list = g_list_remove(ss->list, ss->list->data);
		}
	else
		{
		if (!ss->list_done || !ss->list_done->next) return TRUE;

		ss->list = g_list_prepend(ss->list, ss->list_done->data);
		ss->list_done = g_list_remove(ss->list_done, ss->list_done->data);
		row = GPOINTER_TO_INT(ss->list_done->data);
		}

	file_data_unref(ss->slide_fd);
	ss->slide_fd = NULL;

	if (ss->filelist)
		{
		ss->slide_fd = file_data_ref((FileData *)g_list_nth_data(ss->filelist, row));
		image_change_fd(ss->imd, ss->slide_fd, image_zoom_get_default(ss->imd));
		}
	else if (ss->cd)
		{
		CollectInfo *info;

		info = g_list_nth_data(ss->cd->list, row);
		ss->slide_fd = file_data_ref(info->fd);

		image_change_from_collection(ss->imd, ss->cd, info, image_zoom_get_default(ss->imd));
		}
	else
		{
		ss->slide_fd = file_data_ref(layout_list_get_fd(ss->layout, row));

		if (ss->from_selection)
			{
			image_change_fd(ss->imd, ss->slide_fd, image_zoom_get_default(ss->imd));
			layout_status_update_info(ss->layout, NULL);
			}
		else
			{
			layout_image_set_index(ss->layout, row);
			}
		}

	if (!ss->list && options->slideshow.repeat)
		{
		slideshow_list_init(ss, -1);
		}

	if (!ss->list)
		{
		return FALSE;
		}

	/* read ahead */

	if (options->image.enable_read_ahead)
		{
		gint r;
		if (forward)
			{
			if (!ss->list) return TRUE;
			r = GPOINTER_TO_INT(ss->list->data);
			}
		else
			{
			if (!ss->list_done || !ss->list_done->next) return TRUE;
			r = GPOINTER_TO_INT(ss->list_done->next->data);
			}

		if (ss->filelist)
			{
			image_prebuffer_set(ss->imd, g_list_nth_data(ss->filelist, r));
			}
		else if (ss->cd)
			{
			CollectInfo *info;
			info = g_list_nth_data(ss->cd->list, r);
			if (info) image_prebuffer_set(ss->imd, info->fd);
			}
		else if (ss->from_selection)
			{
			image_prebuffer_set(ss->imd, layout_list_get_fd(ss->layout, r));
			}
		}

	return TRUE;
}

static gint slideshow_loop_cb(gpointer data)
{
	SlideShowData *ss = data;

	if (ss->paused) return TRUE;

	if (!slideshow_step(ss, TRUE))
		{
		ss->timeout_id = -1;
		slideshow_free(ss);
		return FALSE;
		}

	return TRUE;
}

static void slideshow_timer_stop(SlideShowData *ss)
{
	if (ss->timeout_id == -1) return;

	g_source_remove(ss->timeout_id);
	ss->timeout_id = -1;
}

static void slideshow_timer_reset(SlideShowData *ss)
{
	if (options->slideshow.delay < 1) options->slideshow.delay = 1;

	if (ss->timeout_id != -1) g_source_remove(ss->timeout_id);
	ss->timeout_id = g_timeout_add(options->slideshow.delay * 1000 / SLIDESHOW_SUBSECOND_PRECISION,
				       slideshow_loop_cb, ss);
}

static void slideshow_move(SlideShowData *ss, gboolean forward)
{
	if (!ss) return;

	if (!slideshow_step(ss, forward))
		{
		slideshow_free(ss);
		return;
		}

	slideshow_timer_reset(ss);
}

void slideshow_next(SlideShowData *ss)
{
	slideshow_move(ss, TRUE);
}

void slideshow_prev(SlideShowData *ss)
{
	slideshow_move(ss, FALSE);
}

static SlideShowData *real_slideshow_start(ImageWindow *imd, LayoutWindow *lw,
					   GList *filelist, gint start_point,
					   CollectionData *cd, CollectInfo *start_info,
					   void (*stop_func)(SlideShowData *, gpointer), gpointer stop_data)
{
	SlideShowData *ss;
	gint start_index = -1;

	if (!filelist && !cd && layout_list_count(lw, NULL) < 1) return NULL;

	ss = g_new0(SlideShowData, 1);

	ss->imd = imd;

	ss->filelist = filelist;
	ss->cd = cd;
	ss->layout = lw;
	ss->dir_fd = NULL;

	ss->list = NULL;
	ss->list_done = NULL;

	ss->from_selection = FALSE;

	ss->stop_func = NULL;

	ss->timeout_id = -1;
	ss->paused = FALSE;

	if (ss->filelist)
		{
		ss->slide_count = g_list_length(ss->filelist);
		}
	else if (ss->cd)
		{
		collection_ref(ss->cd);
		ss->slide_count = g_list_length(ss->cd->list);
		if (!options->slideshow.random && start_info)
			{
			start_index = g_list_index(ss->cd->list, start_info);
			}
		}
	else
		{
		/* layout method */

		ss->slide_count = layout_selection_count(ss->layout, NULL);
		ss->dir_fd = file_data_ref(ss->layout->dir_fd);
		if (ss->slide_count < 2)
			{
			ss->slide_count = layout_list_count(ss->layout, NULL);
			if (!options->slideshow.random && start_point >= 0 && (guint) start_point < ss->slide_count)
				{
				start_index = start_point;
				}
			}
		else
			{
			ss->from_selection = TRUE;
			}
		}

	slideshow_list_init(ss, start_index);

	ss->slide_fd = file_data_ref(image_get_fd(ss->imd));
	if (slideshow_step(ss, TRUE))
		{
		slideshow_timer_reset(ss);

		ss->stop_func = stop_func;
		ss->stop_data = stop_data;
		}
	else
		{
		slideshow_free(ss);
		ss = NULL;
		}

	return ss;
}

SlideShowData *slideshow_start_from_filelist(ImageWindow *imd, GList *list,
					      void (*stop_func)(SlideShowData *, gpointer), gpointer stop_data)
{
	return real_slideshow_start(imd, NULL, list, -1, NULL, NULL, stop_func, stop_data);
}

SlideShowData *slideshow_start_from_collection(ImageWindow *imd, CollectionData *cd,
					       void (*stop_func)(SlideShowData *, gpointer), gpointer stop_data,
					       CollectInfo *start_info)
{
	return real_slideshow_start(imd, NULL, NULL, -1, cd, start_info, stop_func, stop_data);
}

SlideShowData *slideshow_start(ImageWindow *imd, LayoutWindow *lw, gint start_point,
			       void (*stop_func)(SlideShowData *, gpointer), gpointer stop_data)
{
	return real_slideshow_start(imd, lw, NULL, start_point, NULL, NULL, stop_func, stop_data);
}

gint slideshow_paused(SlideShowData *ss)
{
	if (!ss) return FALSE;

	return ss->paused;
}

void slideshow_pause_set(SlideShowData *ss, gint paused)
{
	if (!ss) return;

	ss->paused = paused;
}

gint slideshow_pause_toggle(SlideShowData *ss)
{
	slideshow_pause_set(ss, !slideshow_paused(ss));
	return slideshow_paused(ss);
}
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
