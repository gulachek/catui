#include "msgstream.h"
#include "unixsocket.h"

#include <cjson/cJSON.h>

#include <stdio.h>

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

volatile sig_atomic_t sig_interrupted = 0;
volatile sig_atomic_t lb_socket = 0;
void sig_interrupt_handler(int signo) {
  sig_interrupted = 1;

  if (lb_socket) {
    close(lb_socket);
  }
}

int child_main(int sock) {
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", sock);
  setenv("CATUI_LOAD_BALANCER_FD", buf, 1);
  const char *arg0 = "./build/echo_server";
  execl(arg0, arg0, 0);
  perror("execl");
  return 1;
}

const char *default_addr_path() {
  return "~/Library/Caches/TemporaryItems/catui/load_balancer.sock";
}

int main(int argc, const char **argv) {
  const char *addr_path = default_addr_path();
  if (argc > 1) {
    addr_path = argv[1];
  }

  int sockets[2];
  if (unix_socketpair(SOCK_STREAM, sockets) == -1) {
    perror("unix_socketpair");
    return 1;
  }

  int child = sockets[0];
  int child_parent = sockets[1];

  pid_t pid = fork();
  if (pid == 0) {
    // child
    close(child);
    return child_main(child_parent);
  } else if (pid == -1) {
    perror("fork");
    return 1;
  }
  // else parent process (load balancer)

  signal(SIGINT, sig_interrupt_handler);

  close(child_parent);

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
    perror("fcntl");
    return 1;
  }

  int exit_code = 0;

  while (!sig_interrupted) {
    fd_set readfds;
    FD_ZERO(&readfds);

    FD_SET(sock, &readfds);

    int max_fd = sock;

    if (select(max_fd + 1, &readfds, NULL, NULL, NULL) == -1) {
      perror("select");
      return 1;
    }

    if (FD_ISSET(sock, &readfds)) {
      struct sockaddr client_addr;
      socklen_t client_addr_len = 0;
      int conn = accept(sock, &client_addr, &client_addr_len);

      if (conn == -1) {
        if (errno == EAGAIN)
          continue;
        perror("accept");
        exit_code = 1;
        break;
      }

      char buf[1024];

      msgstream_size nread = msgstream_recv(conn, buf, sizeof(buf), stderr);
      if (nread == -1) {
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

      printf("Received JSON object:\n");
      for (cJSON *child = json->child; child != NULL; child = child->next) {
        printf("\t%s: %s\n", child->string, child->valuestring);
      }

      cJSON_Delete(json);

      if (unix_send_fd(child, conn) == -1) {
        perror("unix_send_fd");
        close(conn);
        exit_code = 1;
        break;
      }

      close(conn);
    }
  }

  waitpid(pid, NULL, 0);
  unlink(addr_path);

  return exit_code;
}
