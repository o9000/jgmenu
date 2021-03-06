/*
 * xdgapps.c creates cache for .desktop- and .directory-files
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "xdgapps.h"
#include "xdgdirs.h"
#include "util.h"
#include "sbuf.h"
#include "list.h"

struct list_head desktop_files_all;
struct list_head desktop_files_filtered;
struct list_head directory_files;

/* "${HOME}/.local/share", "/usr/share", etc. */
static struct list_head xdg_data_dirs;

static void parse_directory_file(FILE *fp, const char *filename)
{
	char line[8192];
	char *p;
	struct directory_file_data *tmp;

	tmp = xcalloc(1, sizeof(struct directory_file_data));

	tmp->filename = strdup(filename);

	while (fgets(line, sizeof(line), fp)) {
		p = strchr(line, '\n');
		if (p)
			*p = '\0';

		if (!strncmp("Name=", line, 5))
			tmp->name = strdup(line + 5);
		else if (!strncmp("Icon=", line, 5))
			tmp->icon = strdup(line + 5);
	}

	list_add_tail(&tmp->list, &directory_files);
}

static void parse_desktop_file(FILE *fp)
{
	char line[8192];
	char *p;
	struct desktop_file_data *tmp;

	tmp = xcalloc(1, sizeof(struct desktop_file_data));

	while (fgets(line, sizeof(line), fp)) {
		p = strchr(line, '\n');
		if (p)
			*p = '\0';

		/* A bit crude, but prevents loading other Exec values */
		if ((line[0] == '[') && strcmp(line, "[Desktop Entry]"))
			break;

		if (!strncmp("Name=", line, 5))
			tmp->name = strdup(line + 5);
		else if (!strncmp("Exec=", line, 5))
			tmp->exec = strdup(line + 5);
		else if (!strncmp("Icon=", line, 5))
			tmp->icon = strdup(line + 5);
		else if (!strncmp("Categories=", line, 11))
			tmp->categories = strdup(line + 11);
	}

	/* Remove %U, %f, etc at the end of Exec cmd */
	if (tmp->exec) {
		p = strchr(tmp->exec, '%');
		if (p)
			*p = '\0';
	}

	list_add_tail(&tmp->full_list, &desktop_files_all);
}

static void process_file(char *filename, const char *path, int isdir)
{
	FILE *fp;
	char fullname[8192];

	strncpy(fullname, path, strlen(path));
	strncpy(fullname + strlen(path), filename, strlen(filename) + 1);

	fp = fopen(fullname, "r");
	if (!fp)
		die("could not open file %s", filename);

	if (isdir)
		parse_directory_file(fp, filename);
	else
		parse_desktop_file(fp);

	fclose(fp);
}

/*
 * isdir=0 will process .desktop files
 * isdir=1 will process .directory files
 */
static void populate_desktop_and_directory_lists(const char *path, int isdir)
{
	struct dirent *entry;
	DIR *dp;

	dp = opendir(path);
	if (!dp)
		goto close_dir;

	while ((entry = readdir(dp))) {
		if (!strncmp(entry->d_name, ".", 1) ||
		    !strncmp(entry->d_name, "..", 2))
			continue;
		process_file(entry->d_name, path, isdir);
	}

close_dir:
	closedir(dp);
}

void xdgapps_init_lists(void)
{
	struct sbuf *dir;
	struct sbuf s;

	INIT_LIST_HEAD(&desktop_files_all);
	INIT_LIST_HEAD(&desktop_files_filtered);
	INIT_LIST_HEAD(&directory_files);
	INIT_LIST_HEAD(&xdg_data_dirs);

	xdgdirs_get_basedirs(&xdg_data_dirs);

	sbuf_init(&s);
	list_for_each_entry(dir, &xdg_data_dirs, list) {
		sbuf_cpy(&s, dir->buf);
		sbuf_addstr(&s, "/applications/");
		populate_desktop_and_directory_lists(s.buf, 0);
	}
	list_for_each_entry(dir, &xdg_data_dirs, list) {
		sbuf_cpy(&s, dir->buf);
		sbuf_addstr(&s, "/desktop-directories/");
		populate_desktop_and_directory_lists(s.buf, 1);
	}
}

void xdgapps_filter_desktop_files_on_category(const char *category)
{
	struct desktop_file_data *a, *pos;

	list_for_each_entry_safe(pos, a, &desktop_files_filtered, filtered_list)
		list_del(&pos->filtered_list);

	list_for_each_entry(a, &desktop_files_all, full_list) {
		if (!a->name || !a->categories)
			continue;

		if (strstr(a->categories, category))
			list_add_tail(&a->filtered_list,
				      &desktop_files_filtered);
	}
}
