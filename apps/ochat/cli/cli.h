/*
 * cli.h - ochat command line: shared declarations.
 *
 * main.c       - argument dispatch
 * cli/cmd.c    - send, recv, who, ping, status
 * cli/shell.c  - interactive line chat
 */
#ifndef OCHAT_CLI_H
#define OCHAT_CLI_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "link/link.h"

extern struct ochat_link g_link;

/* Opens the RPC bus, printing a clear error (busy, silent, no tower). */
int cli_connect(void);
/* Address or alias to a normalised address; prints the reason on failure. */
int cli_resolve(const char *target, char out[OCHAT_ADDR_SIZE]);
/* Ping once and wait; returns OCHAT_PING_* or -1 on transport error. */
int cli_ping_once(const char *addr, long timeout_ms, long *rtt_us);

int cmd_send(int argc, char **argv);
int cmd_recv(int argc, char **argv);
int cmd_who(int argc, char **argv);
int cmd_ping(int argc, char **argv);
int cmd_status(int argc, char **argv);
int cmd_shell(int argc, char **argv);
#endif
