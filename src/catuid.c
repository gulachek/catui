#include <cjson/cJSON.h>
#include <msgstream.h>
#include <unixsocket.h>

#include <ctype.h>
#include <stdio.h>

#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

struct semver {
  uint16_t major;
  uint16_t minor;
  uint32_t patch;
};

struct server_entry {
  int is_active;
  char proto[128];
  struct semver version;
  int sock_fd;
  pid_t pid;
};

volatile sig_atomic_t sig_interrupted = 0;
volatile sig_atomic_t lb_socket = 0;
void sig_interrupt_handler(int signo) {
  sig_interrupted = 1;

  if (lb_socket) {
    close(lb_socket);
  }
}

void sig_child_handler(int signo) {
  int status;
  pid_t pid = wait(&status);
  if (pid == -1) {
    perror("sig_child_handler:wait");
    return;
  }

  if (WIFEXITED(status)) {
    fprintf(stderr, "Process %d exited with status %d\n", pid,
            WEXITSTATUS(status));
  } else if (WIFSIGNALED(status)) {
    fprintf(stderr, "Process %d exited with signal %d\n", pid,
            WTERMSIG(status));
  } else if (WIFSTOPPED(status)) {
    fprintf(stderr, "Process %d paused with signal %d\n", pid,
            WSTOPSIG(status));
  }
}

int child_main(int sock, char **argv) {
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", sock);
  setenv("CATUI_LOAD_BALANCER_FD", buf, 1);
  execv(argv[0], argv);
  perror("execv");
  return 1;
}

const char *default_addr_path() {
  return "~/Library/Caches/TemporaryItems/catui/load_balancer.sock";
}

int semver_parse_cstr(const char *str, struct semver *version);
int semver_parse(const void *str, size_t len, struct semver *version);
int semver_can_use(struct semver *consumer, struct semver *api);

typedef char path_type[MAXPATHLEN];

struct server_entry *fork_server(const char *proto, struct semver *version,
                                 struct server_entry *servers, int nservers,
                                 path_type *paths, int npaths, FILE *err);

#define NSERVERS 128

int main(int argc, const char **argv) {
  const char *addr_path = default_addr_path();
  if (argc > 1) {
    addr_path = argv[1];
  }

  signal(SIGINT, sig_interrupt_handler);
  signal(SIGCHLD, sig_child_handler);

  struct server_entry servers[NSERVERS] = {};
  char search_paths[32][MAXPATHLEN] = {};
  strncpy(search_paths[0], "./build/catui", MAXPATHLEN);

  int sock = unix_socket(SOCK_STREAM);
  lb_socket = sock;
  if (sock == -1) {
    perror("socket");
    return 1;
  }

  if (unix_bind(sock, addr_path) == -1) {
    perror("bind");
  }

  if (listen(sock, 32) == -1) {
    perror("listen");
    return 1;
  }

  if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1) {
    perror("fcntl O_NONBLOCK");
    return 1;
  }

  if (fcntl(sock, F_SETFD, O_CLOEXEC) == -1) {
    perror("fcntl O_CLOEXEC");
    return 1;
  }

  int exit_code = 0;
  sigset_t child_sigmask;
  sigemptyset(&child_sigmask);
  sigaddset(&child_sigmask, SIGCHLD);

  while (!sig_interrupted) {
    fd_set readfds;
    FD_ZERO(&readfds);

    FD_SET(sock, &readfds);

    int max_fd = sock;

    for (int i = 0; i < NSERVERS; ++i) {
      struct server_entry *server = &servers[i];
      if (!server->is_active)
        continue;

      FD_SET(server->sock_fd, &readfds);
      max_fd = MAX(max_fd, server->sock_fd);
    }

    sigprocmask(SIG_BLOCK, &child_sigmask, NULL);

    if (select(max_fd + 1, &readfds, NULL, NULL, NULL) == -1) {
      perror("select");
      return 1;
    }

    sigprocmask(SIG_UNBLOCK, &child_sigmask, NULL);

    if (FD_ISSET(sock, &readfds)) {
      struct sockaddr client_addr;
      socklen_t client_addr_len = 0;
      int conn = accept(sock, &client_addr, &client_addr_len);

      if (fcntl(conn, F_SETFL, 0) == -1) {
        perror("fcntl clear flags");
        continue;
      }

      if (conn == -1) {
        if (errno == EAGAIN)
          continue;

        perror("accept");
        exit_code = 1;
        break;
      }

      char buf[1024];

      msgstream_size nread = msgstream_recv(conn, buf, sizeof(buf), stderr);
      if (nread < 0) {
        fprintf(stderr, "failed to read initial message from connection\n");
        close(conn);
        break;
      }

      cJSON *json = cJSON_ParseWithLength(buf, nread);

      if (!json) {
        // TODO - inform invalid json
        close(conn);
      }

      if (!cJSON_IsObject(json)) {
        // TODO - inform not object
        close(conn);
      }

      cJSON *catui_version_value = cJSON_GetObjectItem(json, "catui-version");
      if (!catui_version_value) {
        // TODO - inform
        close(conn);
      }

      char *catui_version_cstr = cJSON_GetStringValue(catui_version_value);
      if (!catui_version_cstr) {
        // TODO - inform
        close(conn);
      }

      struct semver catui_version;
      if (semver_parse_cstr(catui_version_cstr, &catui_version) == -1) {
        // TODO - inform
        close(conn);
      }

      struct semver my_version = {.major = 0, .minor = 1, .patch = 0};
      if (!semver_can_use(&catui_version, &my_version)) {
        // TODO - inform
        close(conn);
      }

      cJSON *proto_value = cJSON_GetObjectItem(json, "protocol");
      if (!proto_value) {
        // TODO - inform
        close(conn);
      }

      char *proto = cJSON_GetStringValue(proto_value);
      if (!proto) {
        // TODO - inform
        close(conn);
      }

      cJSON *version_value = cJSON_GetObjectItem(json, "version");
      if (!version_value) {
        // TODO - inform
        close(conn);
      }

      char *version_cstr = cJSON_GetStringValue(version_value);
      if (!version_cstr) {
        // TODO - inform
        close(conn);
      }

      struct semver version;
      if (semver_parse_cstr(version_cstr, &version) == -1) {
        // TODO - inform
        close(conn);
      }

      struct server_entry *server = fork_server(
          proto, &version, servers, NSERVERS, search_paths, 1, stderr);

      cJSON_Delete(json);

      if (server) {
        if (unix_send_fd(server->sock_fd, conn) == -1) {
          perror("unix_send_fd");
          close(conn);
          exit_code = 1;
          break;
        }
      }

      close(conn);
    }

    // Clear closed servers
    for (int i = 0; i < NSERVERS; ++i) {
      struct server_entry *server = &servers[i];
      if (FD_ISSET(server->sock_fd, &readfds)) {
        char *null;
        int nread = read(server->sock_fd, &null, 1);
        if (nread == -1) {
          perror("read");
          continue;
        } else if (nread > 0) {
          fprintf(stderr, "Unexpected info from server '%s'\n", server->proto);
          continue;
        } else {
          close(server->sock_fd);
          fprintf(stderr, "Connection with server '%s' (pid=%d) closed\n",
                  server->proto, server->pid);
          memset(server, 0, sizeof(struct server_entry));
        }
      }
    }
  }

  unlink(addr_path);

  return exit_code;
}

int semver_can_use(struct semver *consumer, struct semver *api) {
  if (consumer->major != api->major)
    return 0;

  if (consumer->major == 0) {
    if (consumer->minor != api->minor)
      return 0;
    return consumer->patch <= api->patch;
  } else {
    if (consumer->minor == api->minor)
      return consumer->patch <= api->patch;
    else
      return consumer->minor < api->minor;
  }
}

int semver_parse_cstr(const char *str, struct semver *version) {
  unsigned int major, minor, patch;
  if (sscanf(str, "%u.%u.%u", &major, &minor, &patch) != 3)
    return -1;

  if (major > UINT16_MAX || minor > UINT16_MAX || patch > UINT32_MAX)
    return -1;

  version->major = (uint16_t)major;
  version->minor = (uint16_t)minor;
  version->patch = (uint32_t)patch;
  return 0;
}

int semver_parse(const void *str, size_t len, struct semver *version) {
  char safebuf[256];
  safebuf[255] = 0;

  if (len > 255)
    return -1;
  strncpy(safebuf, str, len); // null terminated for sure

  return semver_parse_cstr(safebuf, version);
}

/*
 * Fork process if necessary. Return index in servers of
 * forked server. Return -1 on error
 */
struct server_entry *fork_server(const char *proto, struct semver *version,
                                 struct server_entry *servers, int nservers,
                                 path_type *paths, int npaths, FILE *err) {

  int first_inactive = -1;

  for (int i = 0; i < nservers; ++i) {
    struct server_entry *server = &servers[i];
    if (server->is_active) {
      if (strcmp(proto, server->proto) == 0 &&
          semver_can_use(version, &server->version)) {
        return server;
      }
    } else if (first_inactive < 0) {
      first_inactive = i;
    }
  }

  if (first_inactive < 0) {
    fprintf(err,
            "Cannot fork server for '%s' because max number of servers "
            "are in "
            "use\n",
            proto);
    return NULL;
  }

  int root_index = -1;
  char version_str[256];
  struct semver server_version;

  for (int i = 0; i < npaths && root_index == -1; ++i) {
    path_type server_config;

    const char *root = paths[i];
    strncpy(server_config, root, MAXPATHLEN);
    int pathlen = strlen(root);

    server_config[pathlen] = '/';
    pathlen += 1;

    // TODO - this needs to be better to not have negative len
    strncpy(server_config + pathlen, proto, MAXPATHLEN - pathlen);
    pathlen += strlen(proto);

    DIR *dir = opendir(server_config);
    if (dir == NULL)
      continue;

    for (struct dirent *entry = readdir(dir); entry != NULL;
         entry = readdir(dir)) {
      if (entry->d_type != DT_DIR)
        continue;

      if (semver_parse(entry->d_name, entry->d_namlen, &server_version) == -1)
        continue;

      if (semver_can_use(version, &server_version)) {
        root_index = i;

        int len = MIN(entry->d_namlen, sizeof(version_str) - 1);
        strncpy(version_str, entry->d_name, len);
        version_str[len] = '\0';

        break;
      }
    }
  }

  if (root_index == -1) {
    fprintf(err, "No compatible server found for protocol %s (%u.%u.%u)\n",
            proto, version->major, version->minor, version->patch);
    return NULL;
  }

  path_type config;
  snprintf(config, MAXPATHLEN, "%s/%s/%s/config.json", paths[root_index], proto,
           version_str);
  // TODO error handling above

  FILE *config_file = fopen(config, "r");
  if (config_file == NULL) {
    char cwd[MAXPATHLEN + 1];
    getcwd(cwd, MAXPATHLEN);
    cwd[MAXPATHLEN] = 0;

    fprintf(err,
            "Could not open config file '%s' (filename len=%lu, "
            "cwd='%s')\n",
            config, strlen(config), cwd);
    perror("fopen");
    return NULL;
  }

  int fno = fileno(config_file);
  struct stat config_stat;
  if (fstat(fno, &config_stat) == -1) {
    perror("stat");
    fprintf(err, "Failed to get stat for file %s\n", config);
    return NULL;
  }

  char buf[4096];
  if (config_stat.st_size > sizeof(buf)) {
    fprintf(err, "config file '%s' too big (max 4Kb)\n", config);
    return NULL;
  }

  fread(buf, 1, config_stat.st_size, config_file);
  fclose(config_file);

  cJSON *json = cJSON_ParseWithLength(buf, sizeof(buf));
  if (!json) {
    fprintf(err, "Failed to parse JSON file '%s': %s\n", config,
            cJSON_GetErrorPtr());
    return NULL;
  }

  if (!cJSON_IsObject(json)) {
    fprintf(err,
            "Top level JSON object in file '%s' is not a JSON "
            "object.\n",
            config);
    return NULL;
  }

  // TODO validate config version

  cJSON *exec_value = cJSON_GetObjectItemCaseSensitive(json, "exec");
  if (exec_value == NULL) {
    fprintf(err, "Config file '%s' is missing 'exec' object key.\n", config);
    return NULL;
  }

  if (!cJSON_IsArray(exec_value)) {
    fprintf(err, "'exec' is not an array in file '%s'\n", config);
    return NULL;
  }

  int array_len = cJSON_GetArraySize(exec_value);
  if (array_len < 1) {
    fprintf(err, "'exec' array is empty in file '%s'\n", config);
    return NULL;
  }

  if (array_len > 255) {
    fprintf(err,
            "'exec' array has more than max (255) arguments in "
            "file '%s'\n",
            config);
    return NULL;
  }

  typedef char *arg_type;
  arg_type argv[256];

  for (int i = 0; i < array_len; ++i) {
    cJSON *arg_val = cJSON_GetArrayItem(exec_value, i);
    if (!cJSON_IsString(arg_val)) {
      fprintf(err,
              "'exec' item at index '%d' is not a string in "
              "file '%s'\n",
              i, config);
      return NULL;
    }

    argv[i] = cJSON_GetStringValue(arg_val);
  }

  argv[array_len] = NULL;

  int sockets[2];
  if (unix_socketpair(SOCK_STREAM, sockets) == -1) {
    perror("unix_socketpair");
    return NULL;
  }

  int child = sockets[0];
  int child_parent = sockets[1];

  pid_t pid = fork();
  if (pid == 0) {
    // child
    close(child);
    exit(child_main(child_parent, argv));
  } else if (pid == -1) {
    perror("fork");
    return NULL;
  }
  // else parent process (load balancer)

  fprintf(stderr, "Forked server '%s' (%u.%u.%u) with pid %d\n", proto,
          server_version.major, server_version.minor, server_version.patch,
          pid);

  close(child_parent);
  cJSON_Delete(json);

  struct server_entry *server = &servers[first_inactive];
  server->is_active = 1;
  server->pid = pid;
  server->sock_fd = child;
  memcpy(&server->version, &server_version, sizeof(struct semver));
  strncpy(server->proto, proto, sizeof(server->proto));

  return server;
}
