#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "util.h"
#include "config.h"
#include "list.h"
#include "sbuf.h"
#include "xdgdirs.h"

struct config config;
static struct sbuf jgmenurc_file;

void config_set_defaults(void)
{
	config.spawn		   = 1;	/* not in jgmenurc */
	config.stay_alive	   = 1;
	config.hide_on_startup	   = 0;
	/* jgmenurc has a csv_cmd variable here */
	config.tint2_look	   = 1;
	config.at_pointer	   = 0;
	config.multi_window	   = 1;
	config.terminal_exec	   = xstrdup("x-terminal-emulator");
	config.terminal_args	   = xstrdup("-e");

	config.menu_margin_x	   = 0;
	config.menu_margin_y	   = 31;
	config.menu_width	   = 200;
	config.menu_padding_top	   = 5;
	config.menu_padding_right  = 5;
	config.menu_padding_bottom = 5;
	config.menu_padding_left   = 5;
	config.menu_radius	   = 1;
	config.menu_border	   = 0;
	config.menu_halign	   = LEFT;
	config.menu_valign	   = BOTTOM;

	config.sub_spacing	   = 1;
	config.sub_padding_top	   = -1;
	config.sub_padding_right   = -1;
	config.sub_padding_bottom  = -1;
	config.sub_padding_left	   = -1;
	config.sub_hover_action    = 1;

	config.item_margin_x	   = 3;
	config.item_margin_y	   = 3;
	config.item_height	   = 25;
	config.item_padding_x	   = 4;
	config.item_radius	   = 1;
	config.item_border	   = 0;
	config.item_halign	   = LEFT;
	config.sep_height	   = 5;

	config.font		   = NULL; /* Leave as NULL (see font.c) */
	config.font_fallback	   = xstrdup("xtg");
	config.icon_size	   = 22;
	config.icon_text_spacing   = 10;
	config.icon_theme	   = NULL; /* Leave as NULL (see theme.c) */
	config.icon_theme_fallback = xstrdup("xtg");

	config.arrow_string	   = xstrdup("▸");
	config.arrow_width	   = 15;

	parse_hexstr("#000000 70", config.color_menu_bg);
	parse_hexstr("#eeeeee 20", config.color_menu_fg);
	parse_hexstr("#eeeeee 8", config.color_menu_border);
	parse_hexstr("#000000 00", config.color_norm_bg);
	parse_hexstr("#eeeeee 100", config.color_norm_fg);
	parse_hexstr("#ffffff 20", config.color_sel_bg);
	parse_hexstr("#eeeeee 100", config.color_sel_fg);
	parse_hexstr("#eeeeee 8", config.color_sel_border);
	parse_hexstr("#ffffff 20", config.color_sep_fg);
}

void config_cleanup(void)
{
	xfree(config.terminal_exec);
	xfree(config.terminal_args);
	xfree(config.font);
	xfree(config.font_fallback);
	xfree(config.icon_theme);
	xfree(config.icon_theme_fallback);
	xfree(config.arrow_string);
	xfree(jgmenurc_file.buf);
}

static void process_line(char *line)
{
	char *option, *value;

	if (!parse_config_line(line, &option, &value))
		return;

	if (!strcmp(option, "stay_alive")) {
		xatoi(&config.stay_alive, value, XATOI_NONNEG, "config.stay_alive");
	} else if (!strcmp(option, "hide_on_startup")) {
		xatoi(&config.hide_on_startup, value, XATOI_NONNEG, "config.hide_on_startup");
	} else if (!strcmp(option, "tint2_look")) {
		xatoi(&config.tint2_look, value, XATOI_NONNEG, "config.tint2_look");
	} else if (!strcmp(option, "at_pointer")) {
		xatoi(&config.at_pointer, value, XATOI_NONNEG, "config.at_pointer");
	} else if (!strcmp(option, "multi_window")) {
		xatoi(&config.multi_window, value, XATOI_NONNEG, "config.multi_window");
	} else if (!strcmp(option, "terminal_exec")) {
		xfree(config.terminal_exec);
		config.terminal_exec = xstrdup(value);
	} else if (!strcmp(option, "terminal_args")) {
		xfree(config.terminal_args);
		config.terminal_args = xstrdup(value);

	} else if (!strcmp(option, "menu_margin_x")) {
		xatoi(&config.menu_margin_x, value, XATOI_NONNEG, "config.margin_x");
	} else if (!strcmp(option, "menu_margin_y")) {
		xatoi(&config.menu_margin_y, value, XATOI_NONNEG, "config.margin_y");
	} else if (!strcmp(option, "menu_width")) {
		xatoi(&config.menu_width, value, XATOI_GT_0, "config.menu_width");
	} else if (!strcmp(option, "menu_padding_top")) {
		xatoi(&config.menu_padding_top, value, XATOI_NONNEG, "config.menu_padding_top");
	} else if (!strcmp(option, "menu_padding_right")) {
		xatoi(&config.menu_padding_right, value, XATOI_NONNEG, "config.menu_padding_right");
	} else if (!strcmp(option, "menu_padding_bottom")) {
		xatoi(&config.menu_padding_bottom, value, XATOI_NONNEG, "config.menu_padding_bottom");
	} else if (!strcmp(option, "menu_padding_left")) {
		xatoi(&config.menu_padding_left, value, XATOI_NONNEG, "config.menu_padding_left");
	} else if (!strcmp(option, "menu_radius")) {
		xatoi(&config.menu_radius, value, XATOI_NONNEG, "config.menu_radius");
	} else if (!strcmp(option, "menu_border")) {
		xatoi(&config.menu_border, value, XATOI_NONNEG, "config.menu_border");
	} else if (!strcmp(option, "menu_halign")) {
		if (!value)
			return;
		if (!strcasecmp(value, "left"))
			config.menu_halign = LEFT;
		else if (!strcasecmp(value, "right"))
			config.menu_halign = RIGHT;
	} else if (!strcmp(option, "menu_valign")) {
		if (!value)
			return;
		if (!strcasecmp(value, "top"))
			config.menu_valign = TOP;
		else if (!strcasecmp(value, "bottom"))
			config.menu_valign = BOTTOM;

	} else if (!strcmp(option, "sub_spacing")) {
		xatoi(&config.sub_spacing, value, 0, "config.sub_spacing");
	} else if (!strcmp(option, "sub_padding_top")) {
		xatoi(&config.sub_padding_top, value, XATOI_NONNEG, "config.sub_padding_top");
	} else if (!strcmp(option, "sub_padding_right")) {
		xatoi(&config.sub_padding_right, value, XATOI_NONNEG, "config.sub_padding_right");
	} else if (!strcmp(option, "sub_padding_bottom")) {
		xatoi(&config.sub_padding_bottom, value, XATOI_NONNEG, "config.sub_padding_bottom");
	} else if (!strcmp(option, "sub_padding_left")) {
		xatoi(&config.sub_padding_left, value, XATOI_NONNEG, "config.sub_padding_left");
	} else if (!strcmp(option, "sub_hover_action")) {
		xatoi(&config.sub_hover_action, value, XATOI_NONNEG, "config.sub_hover_action");

	} else if (!strcmp(option, "item_margin_x")) {
		xatoi(&config.item_margin_x, value, XATOI_NONNEG, "config.item_margin_x");
	} else if (!strcmp(option, "item_margin_y")) {
		xatoi(&config.item_margin_y, value, XATOI_NONNEG, "config.item_margin_y");
	} else if (!strcmp(option, "item_height")) {
		xatoi(&config.item_height, value, XATOI_GT_0, "config.item_height");
	} else if (!strcmp(option, "item_padding_x")) {
		xatoi(&config.item_padding_x, value, XATOI_NONNEG, "config.item_padding_x");
	} else if (!strcmp(option, "item_radius")) {
		xatoi(&config.item_radius, value, XATOI_NONNEG, "config.item_radius");
	} else if (!strcmp(option, "item_border")) {
		xatoi(&config.item_border, value, XATOI_NONNEG, "config.item_border");
	} else if (!strcmp(option, "item_halign")) {
		if (!value)
			return;
		if (!strcasecmp(value, "left"))
			config.item_halign = LEFT;
		else if (!strcasecmp(value, "right"))
			config.item_halign = RIGHT;
	} else if (!strcmp(option, "sep_height")) {
		xatoi(&config.sep_height, value, XATOI_NONNEG, "config.sep_height");

	} else if (!strcmp(option, "font")) {
		xfree(config.font);
		config.font = xstrdup(value);
	} else if (!strcmp(option, "font_fallback")) {
		xfree(config.font_fallback);
		config.font_fallback = xstrdup(value);
	} else if (!strcmp(option, "icon_size")) {
		xatoi(&config.icon_size, value, XATOI_NONNEG, "config.icon_size");
	} else if (!strcmp(option, "icon_text_spacing")) {
		xatoi(&config.icon_text_spacing, value, XATOI_NONNEG, "config.icon_text_spacing");
	} else if (!strcmp(option, "icon_theme")) {
		xfree(config.icon_theme);
		config.icon_theme = xstrdup(value);
	} else if (!strcmp(option, "icon_theme_fallback")) {
		xfree(config.icon_theme_fallback);
		config.icon_theme_fallback = xstrdup(value);

	} else if (!strcmp(option, "arrow_string")) {
		xfree(config.arrow_string);
		config.arrow_string = xstrdup(value);
	} else if (!strcmp(option, "arrow_width")) {
		xatoi(&config.arrow_width, value, XATOI_NONNEG, "config.arrow_width");

	} else if (!strcmp(option, "color_menu_bg")) {
		parse_hexstr(value, config.color_menu_bg);
	} else if (!strcmp(option, "color_menu_fg")) {
		parse_hexstr(value, config.color_menu_fg);
	} else if (!strcmp(option, "color_menu_border")) {
		parse_hexstr(value, config.color_menu_border);
	} else if (!strcmp(option, "color_norm_bg")) {
		parse_hexstr(value, config.color_norm_bg);
	} else if (!strcmp(option, "color_norm_fg")) {
		parse_hexstr(value, config.color_norm_fg);
	} else if (!strcmp(option, "color_sel_bg")) {
		parse_hexstr(value, config.color_sel_bg);
	} else if (!strcmp(option, "color_sel_fg")) {
		parse_hexstr(value, config.color_sel_fg);
	} else if (!strcmp(option, "color_sel_border")) {
		parse_hexstr(value, config.color_sel_border);
	} else if (!strcmp(option, "color_sep_fg")) {
		parse_hexstr(value, config.color_sep_fg);
	}
}

static void read_file(FILE *fp)
{
	char line[1024];

	while (fgets(line, sizeof(line), fp))
		process_line(line);
}

static void parse_file(char *filename)
{
	FILE *fp;

	fp = fopen(filename, "r");
	if (!fp)
		return;
	read_file(fp);
	fclose(fp);
}

void config_read_jgmenurc(const char *filename)
{
	struct stat sb;
	static int initiated;
	LIST_HEAD(config_dirs);
	struct sbuf *tmp;

	/* use default values for --config-file= without file specified */
	if (filename && filename[0] == '\0')
		return;
	if (initiated)
		goto parse;
	sbuf_init(&jgmenurc_file);
	if (filename) {
		sbuf_cpy(&jgmenurc_file, filename);
		sbuf_expand_tilde(&jgmenurc_file);
	}
	/*
	 * We look for jgmenurc in the following order:
	 *   --config-file=
	 *   ${XDG_CONFIG_HOME:-$HOME/.config}
	 *   ${XDG_CONFIG_DIRS:-/etc/xdg}
	 */
	if (!jgmenurc_file.len) {
		xdgdirs_get_configdirs(&config_dirs);
		list_for_each_entry(tmp, &config_dirs, list) {
			sbuf_addstr(tmp, "/jgmenu/jgmenurc");
			if (!stat(tmp->buf, &sb)) {
				sbuf_cpy(&jgmenurc_file, tmp->buf);
				break;
			}
		}
		sbuf_list_free(&config_dirs);
	}
	initiated = 1;
	if (stat(jgmenurc_file.buf, &sb) < 0)
		return;
	info("using config file %s", jgmenurc_file.buf);
parse:
	parse_file(jgmenurc_file.buf);
}

static void set_floor(int *var, int floor)
{
	if (floor > *var)
		*var = floor;
}

void config_post_process(void)
{
	/*
	 * The menu-border is drawn 'inside' the menu. Therefore, padding_* has
	 * to allow for the border thickness.
	 */
	set_floor(&config.menu_padding_bottom, config.menu_border);
	set_floor(&config.menu_padding_left, config.menu_border);
	set_floor(&config.menu_padding_right, config.menu_border);
	set_floor(&config.menu_padding_top, config.menu_border);

	if (config.sub_padding_top < 0)
		config.sub_padding_top = config.menu_padding_top;
	if (config.sub_padding_right < 0)
		config.sub_padding_right = config.menu_padding_right;
	if (config.sub_padding_bottom < 0)
		config.sub_padding_bottom  = config.menu_padding_bottom;
	if (config.sub_padding_left < 0)
		config.sub_padding_left = config.menu_padding_left;
}
