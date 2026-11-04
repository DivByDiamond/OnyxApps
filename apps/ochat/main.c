/*
 * ochat - chat and network tools for the OC2R radio network.
 *
 *   ochat send <address|alias> <text>     ochat recv [--follow]
 *   ochat shell                           ochat who
 *   ochat ping <address|alias> [-c N]     ochat status [address|alias]
 *
 * Talks to the GPS tower device over the OC2R RPC bus (/dev/hvc0). Only one
 * process can own that bus: close oc2ctl/ochat-gui/other ochat first.
 */
#include "cli/cli.h"

static int usage(void) {
    puts("usage: ochat send <address|alias> <text>");
    puts("       ochat recv [--follow]");
    puts("       ochat shell");
    puts("       ochat who");
    puts("       ochat ping <address|alias> [-c N]");
    puts("       ochat status [address|alias]");
    puts("Addresses look like 7K3F:02Q9. Uses the RPC bus (/dev/hvc0): one program at a time.");
    return 2;
}

int main(int argc, char **argv) {
    const char *c = argc > 1 ? argv[1] : "help";
    if (!strcmp(c, "send")) return cmd_send(argc, argv);
    if (!strcmp(c, "recv")) return cmd_recv(argc, argv);
    if (!strcmp(c, "who")) return cmd_who(argc, argv);
    if (!strcmp(c, "ping")) return cmd_ping(argc, argv);
    if (!strcmp(c, "status")) return cmd_status(argc, argv);
    if (!strcmp(c, "shell")) return cmd_shell(argc, argv);
    return usage();
}
