#include <signal.h>

#include "util.h"

void info(const char *err, ...)
{
	va_list params;

	fprintf(stderr, "info: ");
	va_start(params, err);
	vfprintf(stderr, err, params);
	va_end(params);
	fprintf(stderr, "\n");
}

void warn(const char *err, ...)
{
	va_list params;

	fprintf(stderr, "warning: ");
	va_start(params, err);
	vfprintf(stderr, err, params);
	va_end(params);
	fprintf(stderr, "\n");
}

void die(const char *err, ...)
{
	va_list params;

	fputs("fatal: ", stderr);
	va_start(params, err);
	vfprintf(stderr, err, params);
	va_end(params);
	fputc('\n', stderr);

	exit(1);
}

/* voids zombie processes */
static void set_no_child_wait(void)
{
	static int done;
	static struct sigaction sigchld_action = {
		.sa_handler = SIG_DFL,
		.sa_flags = SA_NOCLDWAIT
	};

	if (done)
		return;
	sigaction(SIGCHLD, &sigchld_action, NULL);
	done = 1;
}

void spawn(const char *arg)
{
	const char default_shell[] = "/bin/sh";
	const char *shell = NULL;

	if (!arg)
		return;
	set_no_child_wait();
	shell = getenv("SHELL");
	if (!shell)
		shell = default_shell;
	switch (fork()) {
	case -1:
		die("unable to fork()");
		break;
	case 0:
		setsid();
		execl(shell, shell, "-c", arg, (char *)NULL);
		exit(0);
	default:
		break;
	}
}

void safe_free(void **ptr)
{
	if (!ptr || !*ptr)
		return;
	free(*ptr);
	*ptr = NULL;
}

char *xstrdup(const char *s)
{
	char *ret = NULL;

	if (!s)
		return NULL;
	ret = strdup(s);
	if (!ret)
		die("Out of memory; strdup failed");
	return ret;
}

void *xmalloc(size_t size)
{
	void *ret = malloc(size);

	if (!ret)
		die("Out of memory; malloc failed");
	return ret;
}

void *xrealloc(void *ptr, size_t size)
{
	void *ret = realloc(ptr, size);

	if (!ret)
		die("Out of memory; realloc failed");
	return ret;
}

void *xcalloc(size_t nb, size_t size)
{
	void *ret = calloc(nb, size);

	if (!ret)
		die("Out of memory; calloc failed");
	return ret;
}

char *expand_tilde(char *s)
{
	char *tmp;

	tmp = xmalloc(strlen(s) + strlen(getenv("HOME")) + 1);
	strcpy(tmp, getenv("HOME"));
	strcat(tmp, s + 1);

	free(s);
	s = NULL;

	return tmp;
}

char *strstrip(char *s)
{
	size_t len;
	char *end;

	len = strlen(s);
	if (!len)
		return s;

	end = s + len - 1;
	while (end >= s && isspace(*end))
		end--;
	*(end + 1) = '\0';

	while (isspace(*s))
		s++;

	return s;
}

int parse_config_line(char *line, char **option, char **value)
{
	char *p;

	p = line;
	while ((p[0] == ' ') || (p[0] == '\t'))
		p++;
	if ((p[0] == '#') || (p[0] == '\n'))
		return 0;

	p = strchr(line, '=');
	if (!p)
		return 0;
	p[0] = 0;

	*option = strstrip(line);
	*value  = strstrip(++p);

	p = strchr(p, '\n');
	if (p)
		p[0] = 0;

	return 1;
}

int hex_to_dec(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0;
}

int parse_hexstr(char *hex, double *rgba)
{
	if (!hex || hex[0] != '#' || strlen(hex) < 7)
		return 0;

	rgba[0] = (hex_to_dec(hex[1]) * 16 + hex_to_dec(hex[2])) / 255.0;
	rgba[1] = (hex_to_dec(hex[3]) * 16 + hex_to_dec(hex[4])) / 255.0;
	rgba[2] = (hex_to_dec(hex[5]) * 16 + hex_to_dec(hex[6])) / 255.0;

	if (strlen(hex) > 7)
		rgba[3] = atoi(hex + 7) / 100.0;
	else
		rgba[3] = 1.0;

	return 1;
}

int get_first_num_from_str(const char *s)
{
	int i = 0, num = 0, has_found_number = 0;

	if (!s)
		return 0;

	for (;;) {
		unsigned char c = s[i++];

		if (!c)
			return num;

		if (!has_found_number && isdigit(c)) {
			num = c - '0';
			has_found_number = 1;
			continue;
		}

		if (has_found_number && isdigit(c))
			num = 10 * num + c - '0';

		if (has_found_number && !isdigit(c))
			return num;
	}
}

static void xatoi_warn(const char *msg, const char *val, const char *key)
{
	fprintf(stderr, "warning: %s; value='%s'; key='%s'\n", msg, val, key);
}

/*
 * xatoi() is an alternative to 'var = atoi(value)'. It was inspired by
 * http://man7.org/tlpi/code/online/dist/lib/get_num.c.html
 *
 * Variable 'var' is only touched if 'value' is a valid integer.
 *
 * The variable 'key' is only there for debugging purposes; it would typically
 * contain the variable name;
 *
 * The flags are defined in the header-file (XATOI_*)
 */
void xatoi(int *var, const char *value, int flags, const char *key)
{
	long res;
	char *endptr;

	if (!value || *value == '\0') {
		xatoi_warn("null or empty string", value, key);
		return;
	}

	errno = 0;
	res = strtol(value, &endptr, 10);
	if (errno != 0)
		xatoi_warn("strtol() failed", value, key);
	else if (*endptr != '\0')
		xatoi_warn("nonnumeric characters", value, key);
	else if ((flags & XATOI_NONNEG) && res < 0)
		xatoi_warn("negative value not allowed", value, key);
	else if ((flags & XATOI_GT_0) && res <= 0)
		xatoi_warn("value must be > 0", value, key);
	else if (res > INT_MAX || res < INT_MIN)
		xatoi_warn("integer out of range", value, key);
	else
		*var = (int)res;
}
