#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define ANSI_COLOR_RED          "\x1b[31m"
#define ANSI_COLOR_GREEN        "\x1b[32m"
#define ANSI_COLOR_PURPLE       "\x1b[35m"


#define TASK_COMM_LEN		16
#define MAX_PAGE_BYTES		32

char *strreplace(char *s, char old, char new)
{
	for (; *s; ++s)
		if (*s == old)
			*s = new;
	return s;
}

static inline int is_pid_valid(pid_t pid)
{
	if (pid < 0) {
		fprintf(stderr, ANSI_COLOR_PURPLE
				"[ DEBUG ] invalid pid %d\n", pid);
		goto fail;
	}

	return 0;
fail:
	return -1;
}

static inline int is_pid_running(pid_t pid)
{
	int ret;
	FILE *f;
	char path[50];

	snprintf(path, sizeof(path), "/proc/%d", pid);
	/* check if pid is running */
	f = fopen(path, "r");
	if (!f) {
		fprintf(stderr, ANSI_COLOR_RED
				"[ FAILURE ] %s: No Such File or Directory\n",
				path);
		goto fail;
	}

	ret = fclose(f);
	if (ret) {
		fprintf(stderr, ANSI_COLOR_RED
				"[ ERROR ] %s", strerror(errno));
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static inline char *get_process_name(pid_t pid)
{
	FILE *f;
	char *buf;
	int ret;

	buf = malloc(sizeof(char) * 50);

	snprintf(buf, 50, "/proc/%d/comm", pid);
	/* check if pid is running */
	f = fopen(buf, "r");
	if (!f) {
		fprintf(stderr, ANSI_COLOR_RED
				"[ FAILURE ] %s: No Such File or Directory\n",
				buf);
		goto fail;
	}

	ret = fread(buf, 1, TASK_COMM_LEN, f);
	if (!ret && ferror(f)) {
		fprintf(stderr, ANSI_COLOR_RED
				"[ ERROR ] %s\n", strerror(errno));
		fclose(f);
		goto fail;
	}

	/* rewrite '/n' to '\0' and escape '/' */
	buf[ret - 1] = '\0';
	strreplace(buf, '/', '-');
	return buf;
fail:
	exit(1);
}

static inline unsigned long get_pages(const char *path, const char *file)
{
	char buf[50];
	FILE *f;
	int ret;
	long long pages;

	snprintf(buf, sizeof(buf), "%s/%s", path, file);
	f = fopen(buf, "r");
	if (!f) {
		fprintf(stderr, ANSI_COLOR_RED
				"[ FAILURE ] %s: No Such File or Directory\n",
				buf);
		goto fail;
	}

	ret = fread(buf, 1, TASK_COMM_LEN, f);
	if (!ret && ferror(f)) {
		fprintf(stderr, ANSI_COLOR_RED "[ ERROR ] %s\n", strerror(errno));
		fclose(f);
		goto fail;
	}

	/* rewrite '/n' to '\0' */
	buf[ret - 2] = '\0';

	pages = atoll(buf);
	return (unsigned long) pages;

fail:
	exit(1);
}

int main(int argc, char **argv)
{
	pid_t pid;
	char *name, ppagefs_path[50];
	unsigned long zero, total;


	if (argc != 2) {
		fprintf(stderr, "<usage>: inspect_pages <pid>\n");
		goto fail;
	}

	pid = atoi(argv[1]);
	
	if (is_pid_valid(pid))
		goto fail;

	if (is_pid_running(pid))
		goto fail;

	/* pid is running */
	name = get_process_name(pid);

	snprintf(ppagefs_path, sizeof(ppagefs_path),
			"/ppagefs/%d.%s", pid, name);
	zero = get_pages(ppagefs_path, "zero");
	total = get_pages(ppagefs_path, "total");

	fprintf(stderr, ANSI_COLOR_GREEN
			"[ RESULT ] pid: %d, pid->comm: %s, "
			"zero: %lu, total: %lu\n",
			pid, name, zero, total);
	
	return 0;

fail:
	return 0;
}
