#ifndef CATUI_H
#define CATUI_H

#include <stdio.h>
#include <stdlib.h>

#define CATUI_ACK_SIZE 1024

int catui_connect(const char *proto, const char *semver, FILE *err);

#endif
