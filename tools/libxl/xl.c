/*
 * Copyright (C) 2009      Citrix Ltd.
 * Author Stefano Stabellini <stefano.stabellini@eu.citrix.com>
 * Author Vincent Hanquez <vincent.hanquez@eu.citrix.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; version 2.1 only. with the special
 * exception on linking described in file LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 */

#include "libxl_osdeps.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <inttypes.h>
#include <regex.h>
#include <sched.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <pthread.h>
#include <xencall.h>

#include "libxl.h"
#include "libxl_utils.h"
#include "libxlutil.h"
#include "xl.h"

#include "eval.h"

xentoollog_logger_stdiostream *logger;
int dryrun_only;
int force_execution;
int autoballoon = -1;
char *blkdev_start;
int run_hotplug_scripts = 1;
char *lockfile;
char *default_vifscript = NULL;
char *default_bridge = NULL;
char *default_gatewaydev = NULL;
char *default_vifbackend = NULL;
char *default_remus_netbufscript = NULL;
char *default_colo_proxy_script = NULL;
enum output_format default_output_format = OUTPUT_FORMAT_JSON;
int claim_mode = 1;
bool progress_use_cr = 0;

xentoollog_level minmsglevel = minmsglevel_default;

/* Get autoballoon option based on presence of dom0_mem Xen command
 line option. */
static int auto_autoballoon(void) {
	const libxl_version_info *info;
	regex_t regex;
	int ret;

	info = libxl_get_version_info(ctx);
	if (!info)
		return 1; /* default to on */

	ret = regcomp(&regex,
			"(^| )dom0_mem=((|min:|max:)[0-9]+[bBkKmMgG]?,?)+($| )",
			REG_NOSUB | REG_EXTENDED);
	if (ret)
		return 1;

	ret = regexec(&regex, info->commandline, 0, NULL, 0);
	regfree(&regex);
	return ret == REG_NOMATCH;
}

static void parse_global_config(const char *configfile,
		const char *configfile_data, int configfile_len) {
	long l;
	XLU_Config *config;
	int e;
	const char *buf;

	config = xlu_cfg_init(stderr, configfile);
	if (!config) {
		fprintf(stderr, "Failed to allocate for configuration\n");
		exit(1);
	}

	e = xlu_cfg_readdata(config, configfile_data, configfile_len);
	if (e) {
		fprintf(stderr, "Failed to parse config file: %s\n", strerror(e));
		exit(1);
	}

	if (!xlu_cfg_get_string(config, "autoballoon", &buf, 0)) {
		if (!strcmp(buf, "on") || !strcmp(buf, "1"))
			autoballoon = 1;
		else if (!strcmp(buf, "off") || !strcmp(buf, "0"))
			autoballoon = 0;
		else if (!strcmp(buf, "auto"))
			autoballoon = -1;
		else
			fprintf(stderr, "invalid autoballoon option");
	}
	if (autoballoon == -1)
		autoballoon = auto_autoballoon();

	if (!xlu_cfg_get_long(config, "run_hotplug_scripts", &l, 0))
		run_hotplug_scripts = l;

	if (!xlu_cfg_get_string(config, "lockfile", &buf, 0))
		lockfile = strdup(buf);
	else {
		lockfile = strdup(XL_LOCK_FILE);
	}

	if (!lockfile) {
		fprintf(stderr, "failed to allocate lockfile\n");
		exit(1);
	}

	/*
	 * For global options that are related to a specific type of device
	 * we use the following nomenclature:
	 *
	 * <device type>.default.<option name>
	 *
	 * This allows us to keep the default options classified for the
	 * different device kinds.
	 */

	if (!xlu_cfg_get_string(config, "vifscript", &buf, 0)) {
		fprintf(stderr, "the global config option vifscript is deprecated, "
				"please switch to vif.default.script\n");
		free(default_vifscript);
		default_vifscript = strdup(buf);
	}

	if (!xlu_cfg_get_string(config, "vif.default.script", &buf, 0)) {
		free(default_vifscript);
		default_vifscript = strdup(buf);
	}

	if (!xlu_cfg_get_string(config, "defaultbridge", &buf, 0)) {
		fprintf(stderr, "the global config option defaultbridge is deprecated, "
				"please switch to vif.default.bridge\n");
		free(default_bridge);
		default_bridge = strdup(buf);
	}

	if (!xlu_cfg_get_string(config, "vif.default.bridge", &buf, 0)) {
		free(default_bridge);
		default_bridge = strdup(buf);
	}

	if (!xlu_cfg_get_string(config, "vif.default.gatewaydev", &buf, 0))
		default_gatewaydev = strdup(buf);

	if (!xlu_cfg_get_string(config, "vif.default.backend", &buf, 0))
		default_vifbackend = strdup(buf);

	if (!xlu_cfg_get_string(config, "output_format", &buf, 0)) {
		if (!strcmp(buf, "json"))
			default_output_format = OUTPUT_FORMAT_JSON;
		else if (!strcmp(buf, "sxp"))
			default_output_format = OUTPUT_FORMAT_SXP;
		else {
			fprintf(stderr, "invalid default output format \"%s\"\n", buf);
		}
	}
	if (!xlu_cfg_get_string(config, "blkdev_start", &buf, 0))
		blkdev_start = strdup(buf);

	if (!xlu_cfg_get_long(config, "claim_mode", &l, 0))
		claim_mode = l;

	xlu_cfg_replace_string(config, "remus.default.netbufscript",
			&default_remus_netbufscript, 0);
	xlu_cfg_replace_string(config, "colo.default.proxyscript",
			&default_colo_proxy_script, 0);

	xlu_cfg_destroy(config);
}

void postfork(void) {
	libxl_postfork_child_noexec(ctx); /* in case we don't exit/exec */
	ctx = 0;

	xl_ctx_alloc();
}

pid_t xl_fork(xlchildnum child, const char *description) {
	xlchild *ch = &children[child];
	int i;

	assert(!ch->pid);
	ch->reaped = 0;
	ch->description = description;

	ch->pid = fork();
	if (ch->pid == -1) {
		perror("fork failed");
		exit(-1);
	}

	if (!ch->pid) {
		/* We are in the child now.  So all these children are not ours. */
		for (i = 0; i < child_max; i++)
			children[i].pid = 0;
	}

	return ch->pid;
}

pid_t xl_waitpid(xlchildnum child, int *status, int flags) {
	xlchild *ch = &children[child];
	pid_t got = ch->pid;
	assert(got);
	if (ch->reaped) {
		*status = ch->status;
		ch->pid = 0;
		return got;
	}
	for (;;) {
		got = waitpid(ch->pid, status, flags);
		if (got < 0 && errno == EINTR)
			continue;
		if (got > 0) {
			assert(got == ch->pid);
			ch->pid = 0;
		}
		return got;
	}
}

int xl_child_pid(xlchildnum child) {
	xlchild *ch = &children[child];
	return ch->pid;
}

void xl_report_child_exitstatus(xentoollog_level level, xlchildnum child,
		pid_t pid, int status) {
	libxl_report_child_exitstatus(ctx, level, children[child].description, pid,
			status);
}

static int xl_reaped_callback(pid_t got, int status, void *user) {
	int i;
	assert(got);
	for (i = 0; i < child_max; i++) {
		xlchild *ch = &children[i];
		if (ch->pid == got) {
			ch->reaped = 1;
			ch->status = status;
			return 0;
		}
	}
	return ERROR_UNKNOWN_CHILD;
}

static const libxl_childproc_hooks childproc_hooks = { .chldowner =
		libxl_sigchld_owner_libxl, .reaped_callback = xl_reaped_callback, };

void xl_ctx_alloc(void) {
	if (libxl_ctx_alloc(&ctx, LIBXL_VERSION, 0, (xentoollog_logger*) logger)) {
		fprintf(stderr, "cannot init xl context\n");
		exit(1);
	}

	libxl_childproc_setmode(ctx, &childproc_hooks, 0);
}

static void xl_ctx_free(void) {
	if (ctx) {
		libxl_ctx_free(ctx);
		ctx = NULL;
	}
	if (logger) {
		xtl_logger_destroy((xentoollog_logger*) logger);
		logger = NULL;
	}
	if (lockfile) {
		free(lockfile);
		lockfile = NULL;
	}
}
extern void log_time(char *msg);
int main(int argc, char **argv) {
	int opt = 0;
	char *cmd = 0;
	struct cmd_spec *cspec;
	int ret;
	void *config_data = 0;
	int config_len = 0;

	while ((opt = getopt(argc, argv, "+vftN")) >= 0) {
		switch (opt) {
		case 'v':
			if (minmsglevel > 0)
				minmsglevel--;
			break;
		case 'N':
			dryrun_only = 1;
			break;
		case 'f':
			force_execution = 1;
			break;
		case 't':
			progress_use_cr = 1;
			break;
		default:
			fprintf(stderr, "unknown global option\n");
			exit(EXIT_FAILURE);
		}
	}

	cmd = argv[optind];

	if (!cmd) {
		help(NULL);
		exit(EXIT_FAILURE);
	}
	opterr = 0;

	logger = xtl_createlogger_stdiostream(stderr, minmsglevel,
			(progress_use_cr ? XTL_STDIOSTREAM_PROGRESS_USE_CR : 0));
	if (!logger)
		exit(EXIT_FAILURE);

	atexit(xl_ctx_free);

	xl_ctx_alloc();

	ret = libxl_read_file_contents(ctx, XL_GLOBAL_CONFIG, &config_data,
			&config_len);
	if (ret)
		fprintf(stderr, "Failed to read config file: %s: %s\n",
		XL_GLOBAL_CONFIG, strerror(errno));
	parse_global_config(XL_GLOBAL_CONFIG, config_data, config_len);
	free(config_data);

	/* Reset options for per-command use of getopt. */
	argv += optind;
	argc -= optind;
	optind = 1;

	cspec = cmdtable_lookup(cmd);
	if (cspec) {
		if (dryrun_only && !cspec->can_dryrun) {
			fprintf(stderr, "command does not implement -N (dryrun) option\n");
			ret = EXIT_FAILURE;
			goto xit;
		}
		ret = cspec->cmd_impl(argc, argv);
	} else if (!strcmp(cmd, "help")) {
		help(argv[1]);
		ret = EXIT_SUCCESS;
	} else {
		fprintf(stderr, "command not implemented\n");
		ret = EXIT_FAILURE;
	}

	xit: return ret;
}
/*
 void printCPUAffinity()
 {
 char str[80];
 strcpy(str," ");
 int count = 0;
 int j;
 CPU_ZERO(&set);
 pid = sched_getaffinity(0, sizeof(set), &set);
 for (j = 0; j < CPU_SETSIZE; ++j)
 {
 if (CPU_ISSET(j, &set))
 {
 ++count;
 char cpunum[3];
 sprintf(cpunum, "%d ", j);
 strcat(str, cpunum);
 }
 }
 printf("------------------------------ affinity has %d CPUs ... %s\n", count, str);
 }
 */
static int get_socket(const char *connect_to) {
	struct sockaddr_un addr;
	int sock, saved_errno;

	sock = socket(PF_UNIX, SOCK_SEQPACKET, 0);
	if (sock < 0)
		return -1;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	if (strlen(connect_to) >= sizeof(addr.sun_path)) {
		errno = EINVAL;
		goto error;
	}
	strcpy(addr.sun_path, connect_to);

	if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) != 0)
		goto error;

	return sock;

	error: saved_errno = errno;
	close(sock);
	errno = saved_errno;
	return -1;
}

int main_create_client(int argc, char **argv) {
	int socket, i, written;
	const char *path = "/var/run/xen/server";

	if (argc < 2)
		return -1;

	socket = get_socket(path);
	if (socket < 0)
		return -1;

	written = write(socket, argv[1], strlen(argv[1]));
	for (i = 2; i < argc; ++i)
		written = write(socket, argv[i], strlen(argv[i]));
	written = write(socket, "end", 3);

	return 0;
}

static void init_server_socket(int *sock, const char *soc_str) {
	struct sockaddr_un addr;
	int flags;

	/* Create sockets for them to listen to. */
	*sock = socket(PF_UNIX, SOCK_SEQPACKET, 0);
	if (*sock < 0) {
		fprintf(stderr, "Could not create socket\n");
		return;
	}

	if ((flags = fcntl(*sock, F_GETFD)) < 0) {
		fprintf(stderr, "Could not get flags\n");
		return;
	}
	flags |= FD_CLOEXEC;
	if (fcntl(*sock, F_SETFD, flags) < 0) {
		fprintf(stderr, "Could not set flags\n");
		return;
	}

	/* FIXME: Be more sophisticated, don't mug running daemon. */
	unlink(soc_str);

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;

	if (strlen(soc_str) >= sizeof(addr.sun_path))
		fprintf(stderr, "socket string '%s' too long\n", soc_str);
	strcpy(addr.sun_path, soc_str);
	if (bind(*sock, (struct sockaddr *) &addr, sizeof(addr)) != 0)
		fprintf(stderr, "Could not bind socket to %s\n", soc_str);

	if (chmod(soc_str, 0600) != 0)
		fprintf(stderr, "Could not chmod socket\n");

	/*make the socket a server with a 10 places queue*/
	if (listen(*sock, 10) != 0)
		fprintf(stderr, "Could not listen on sockets\n");
}

static inline void *main_create_callback(void *cmd_ptr) {
		main_create(2, cmd_ptr);

	return NULL;
}

static inline void *create_callback(void *cmd_ptr) {
	char **cmd = cmd_ptr;
	
#ifdef ENABLE_EVAL
	START_TIMER
#endif /* ENABLE_EVAL */

	main_create_callback(cmd_ptr);

#ifdef ENABLE_EVAL
	STOP_TIMER_W(EVAL_OP_FILE_CREATE)
#endif /* ENABLE_EVAL */

	free(cmd[1]);
	free(cmd);
	return NULL;
}

static inline void *main_destroy_callback(void *cmd_ptr) {
	main_destroy(2, cmd_ptr);

	return NULL;
}

static inline void *destroy_callback(void *cmd_ptr) {
	char **cmd = cmd_ptr;

#ifdef ENABLE_EVAL
        START_TIMER
#endif /* ENABLE_EVAL */

	main_destroy_callback(cmd_ptr);

#ifdef ENABLE_EVAL
        STOP_TIMER_W(EVAL_OP_FILE_DESTROY)
#endif /* ENABLE_EVAL */


	free(cmd[1]);
	free(cmd);
	return NULL;
}

static inline void *main_shutdown_reboot_callback(void *cmd_ptr) {
	if (!strcmp( *(char **)cmd_ptr, "shutdown")) {
		main_shutdown(2, cmd_ptr);
	} else if (!strcmp( *(char **)cmd_ptr, "reboot")){
		main_reboot(2, cmd_ptr);
	}

	return NULL;
}

static inline void *shutdown_reboot_callback(void *cmd_ptr) {
	char **cmd = cmd_ptr;

	main_shutdown_reboot_callback(cmd_ptr);

	free(cmd[1]);
	free(cmd);
	return NULL;
}

#define MAX_DOM_NAME 20
static void *rename_domain(void *cmd_ptr){
	char **cmd=cmd_ptr;

	main_rename(3, cmd);

	optind=1;
	free(cmd[1]);
	free(cmd[2]);
	free(cmd[3]);
	free(cmd);
	return NULL;
}

#define MAX_PARALLEL_DOMAINS 70
#define MAX_CFG_FILE_NAME 50
#define MAX_CMD_LENGTH 10
int main_create_server(int argc, char **argv) {
	int socket, i;
	int fd;
	int last_i;
	bool reread;
	void *cb;
	cpu_set_t set;

	/* this variable is our reference to the threads */
	pthread_t xl_thread[MAX_PARALLEL_DOMAINS +1];

	int nprocs_max, max_parralel;
	pthread_attr_t attr;

	/*the first is the xl command*/
	char **cmd[MAX_PARALLEL_DOMAINS + 1];
	char c[MAX_CMD_LENGTH];
	const char *path = "/var/run/xen/server";

	//CPU_ZERO(&set);
	//CPU_SET(0, &set);
	//if (sched_setaffinity(getpid(), sizeof(set), &set) == -1) {
	//	fprintf(stderr, "Failed to sched_setaffinitied.\n");
	//	return -1;
	//}

	pthread_attr_init(&attr);
	//pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	//All processus should run on CPU0. The remaining processors are dedicated to VM creation
	nprocs_max = sysconf(_SC_NPROCESSORS_ONLN) - 1;
	if (nprocs_max < 1) {
		fprintf(stderr, "Failed to get the number of online processors.\n");
		return -1;
	}
	if (nprocs_max > MAX_PARALLEL_DOMAINS) {
		fprintf(stderr, "Please increase MAX_PARALLEL_DOMAINS.\n");
		max_parralel = MAX_PARALLEL_DOMAINS;
	} else {
		max_parralel = nprocs_max;
	}

	init_server_socket(&socket, path);

	pthread_atfork(NULL, NULL, &xencall_cache_unlock_simple);

	for (;;) {
		fd = accept(socket, NULL, NULL);
		if (fd < 0)
			return -1;

		memset(&c, 0, MAX_CMD_LENGTH);
		if (read(fd, &c, MAX_CMD_LENGTH) == -1) {
			fprintf(stderr, "read failed\n");
			return -1;
		}

		if (!strcmp(c, "create"))
			cb = create_callback;
		else if (!strcmp(c, "destroy"))
			cb = destroy_callback;
		else if (!strcmp(c, "shutdown") || !strcmp(c, "reboot"))
			cb = shutdown_reboot_callback;
		else if (!strcmp(c, "rename")){
			cmd[0] = malloc(4 * sizeof(char*));
			cmd[0][0]=c;
			for (i = 1; i <= 3; ++i) {
				cmd[0][i]=calloc(MAX_DOM_NAME, sizeof(char));
				if (read(fd, cmd[0][i], MAX_DOM_NAME) == -1) {
					fprintf(stderr, "read failed\n");
					return -1;
				}
			}
			if (pthread_create(&xl_thread[0], &attr, rename_domain, (void*) cmd[0])) {
				fprintf(stderr, "Error creating rename thread\n");
			}
			continue;
		}else {
			fprintf(stderr, "unknown command\n");
			continue;
		}

		read_loop: for (i = 0;; i++) {
			cmd[i] = malloc(2 * sizeof(char*));
			cmd[i][0] = c;
			cmd[i][1] = calloc(MAX_CFG_FILE_NAME, sizeof(char));

			if (read(fd, cmd[i][1], MAX_CFG_FILE_NAME) == -1) {
				fprintf(stderr, "read failed\n");
				return -1;
			}
			if (!strcmp(cmd[i][1], "end")) {
				reread = false;
				last_i = i - 1;
				free(cmd[i][1]);
				free(cmd[i]);
				break;
			}
			if (i >= max_parralel) {
				reread = true;
				last_i = i;
				break;
			}
		}

		for (i = 0; i <= last_i; i++) {
			CPU_ZERO(&set);
			CPU_SET((i % nprocs_max) + 1, &set);
			if (pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set)) {
				fprintf(stderr, "Failed to pthread_attr_setaffinity_np.\n");
				continue;
			}

			if (pthread_create(&xl_thread[i], &attr, cb, (void*) cmd[i])) {
				fprintf(stderr, "Error creating thread\n");
			}
		}

		for (i = 0; i <= last_i; i++){
			if (pthread_join(xl_thread[i], NULL)){
				fprintf(stderr, "Error creating thread\n");
			}
		}

		if (reread)
			goto read_loop;

	}

}

int main_multiple(int argc, char **argv) {
	char ***cmd = (char***) malloc((argc - 1) * sizeof(char**));
	void *cb;

	/*this variable is our reference to the threads*/
	pthread_t xl_threads[argc - 1];

	cpu_set_t set;
	int nprocs_max;
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	//All processus should run on CPU0. The remaining processors are dedicated to VM creation
	nprocs_max = sysconf(_SC_NPROCESSORS_ONLN) - 1;
	if (nprocs_max < 1) {
		fprintf(stderr, "Failed to sysconfed.\n");
		goto out;
	}

	if (!strcmp(argv[1], "create"))
		cb = main_create_callback;
	else if (!strcmp(argv[1], "destroy"))
		cb = main_destroy_callback;
	else {
		fprintf(stderr, "unknown command\n");
		return -1;
	}

	pthread_atfork(NULL, NULL, &xencall_cache_unlock_simple);

	for (int i = 0; i < argc - 2; i++) {
		cmd[i] = (char**) malloc(2 * sizeof(char*));
		cmd[i][0] = argv[1];
		cmd[i][1] = argv[i + 2];

		printf("%s %s\n", argv[1], argv[i + 2]);

		CPU_ZERO(&set);
		CPU_SET((i % nprocs_max) + 1, &set);
		if (pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set)) {
			fprintf(stderr, "Failed to pthread_attr_setaffinity_np.\n");
			goto out;
		}

		if (pthread_create(&xl_threads[i], &attr, cb, (void*) cmd[i])) {
			fprintf(stderr, "Error creating thread\n");
			goto out;
		}
	}

	for (int i = 0; i < argc - 2; i++) {
		pthread_join(xl_threads[i], NULL);
	}

	/*those two free rise errors
	 cmd is freed somewhere on the road*/
	out: for (int i = 0; i < argc - 2; i++) {
		free(cmd[i]);
	}
	free(cmd);
	return 0;
}

/*
 * Local variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
