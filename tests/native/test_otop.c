/*
 * test_otop.c - host tests for the pure otop module
 * (apps/otop/probe/parse.c). Runs on any Linux with gcc:
 *
 *   make -C tests/native test_otop && ./tests/native/test_otop
 *
 * The sample buffers below reproduce the exact /proc formats emitted
 * by OnyxKernel kernel/src/fs/procfs (meminfo, load, cpuinfo, uptime).
 */
#include <stdio.h>
#include <string.h>
#include "otop.h"

static int failures = 0;
static int checks = 0;

#define CHECK(cond)                                                      \
    do {                                                                 \
        checks++;                                                        \
        if (cond) {                                                      \
            printf("  ok  %s\n", #cond);                                 \
        } else {                                                         \
            printf("  FAIL %s (%s:%d)\n", #cond, __FILE__, __LINE__);    \
            failures++;                                                  \
        }                                                                \
    } while (0)

/* Exact kernel procfs layouts (see OnyxKernel fs/procfs/content.rs). */
static const char MEMINFO[] =
    "MemTotal\t: 262144 kB\n"
    "MemFree\t\t: 131072 kB\n"
    "MemUsed\t\t: 131072 kB\n"
    "HeapTotal\t: 4096 kB\n"
    "HeapUsed\t: 1024 kB\n"
    "HeapFree\t: 3072 kB\n";

static const char LOAD[] =
    "processes\t: 7\n"
    "procs_running\t: 1\n"
    "procs_blocked\t: 0\n";

static const char CPUINFO[] =
    "harts\t\t: 2\n"
    "processor\t: 0\n"
    "processor\t: 1\n"
    "model name\t: rv64gc\n"
    "isa\t\t: rv64imafdc\n";

static const char UPTIME[] = "123.45\n";

static void test_parse_num(void) {
    long v;

    printf("parse_num over /proc/meminfo:\n");
    v = 0;
    CHECK(parse_num(MEMINFO, "MemTotal", &v) == 1);
    CHECK(v == 262144);
    v = 0;
    CHECK(parse_num(MEMINFO, "MemFree", &v) == 1);
    CHECK(v == 131072);
    v = 0;
    CHECK(parse_num(MEMINFO, "HeapUsed", &v) == 1);
    CHECK(v == 1024);
    CHECK(parse_num(MEMINFO, "SwapTotal", &v) == 0); /* absent key */
    CHECK(parse_num(MEMINFO, "Total", &v) == 0); /* matches only mid-line */

    printf("parse_num over /proc/load:\n");
    v = 0;
    CHECK(parse_num(LOAD, "processes", &v) == 1);
    CHECK(v == 7);
    v = 0;
    CHECK(parse_num(LOAD, "procs_running", &v) == 1);
    CHECK(v == 1);
    v = 0;
    CHECK(parse_num(LOAD, "procs_blocked", &v) == 1);
    CHECK(v == 0);

    printf("parse_num over /proc/cpuinfo:\n");
    v = 0;
    CHECK(parse_num(CPUINFO, "harts", &v) == 1);
    CHECK(v == 2);
}

static void test_parse_uptime(void) {
    printf("parse_uptime_s:\n");
    CHECK(parse_uptime_s(UPTIME) == 123);
    CHECK(parse_uptime_s("0.00\n") == 0);
    CHECK(parse_uptime_s("98765.43\n") == 98765);
}

static void test_permille(void) {
    printf("permille edge cases:\n");
    CHECK(permille(500, 1000) == 500);
    CHECK(permille(0, 1000) == 0);
    CHECK(permille(1, 0) == 0);      /* zero total */
    CHECK(permille(-5, 10) == 0);    /* negative used */
    CHECK(permille(2000, 1000) == 1000); /* overflow clamp */
}

static void test_fmt_uptime(void) {
    char buf[16];

    printf("fmt_uptime:\n");
    fmt_uptime(3723, buf, sizeof(buf));
    CHECK(strcmp(buf, "01:02:03") == 0);
    fmt_uptime(59, buf, sizeof(buf));
    CHECK(strcmp(buf, "00:00:59") == 0);
    fmt_uptime(3600, buf, sizeof(buf));
    CHECK(strcmp(buf, "01:00:00") == 0);
    fmt_uptime(86399, buf, sizeof(buf));
    CHECK(strcmp(buf, "23:59:59") == 0);
}

static void test_hist(void) {
    struct otop_hist h;
    int i, ok;

    printf("hist_push ring behavior:\n");
    memset(&h, 0, sizeof(h));
    for (i = 1; i <= 60; i++) hist_push(&h, i);
    CHECK(h.n == OTOP_HIST);
    CHECK(h.v[0] == 60 - OTOP_HIST + 1); /* oldest survivor */
    CHECK(h.v[OTOP_HIST - 1] == 60);     /* newest */
    ok = 1;
    for (i = 1; i < OTOP_HIST; i++) {
        if (h.v[i] != h.v[i - 1] + 1) ok = 0;
    }
    CHECK(ok == 1); /* strictly ascending after the shift */
}

static void test_spark(void) {
    char out[OTOP_HIST + 1];
    struct otop_hist h;
    int i, ok;
    static char levels[10] = " .:-=+*#@";

    printf("spark_ch levels:\n");
    CHECK(spark_ch(0) == ' ');
    CHECK(spark_ch(1000) == '@');
    CHECK(strchr(levels, spark_ch(500)) != NULL);

    printf("spark_line flat zero input:\n");
    memset(&h, 0, sizeof(h));
    for (i = 0; i < 10; i++) hist_push(&h, 0);
    spark_line(&h, out, 10);
    CHECK(out[10] == 0);
    ok = 1;
    for (i = 0; i < 10; i++) {
        if (out[i] != ' ') ok = 0;
    }
    CHECK(ok == 1);

    printf("spark_line ascending input:\n");
    memset(&h, 0, sizeof(h));
    for (i = 0; i < 10; i++) hist_push(&h, (i + 1) * 100);
    spark_line(&h, out, 10);
    CHECK(out[10] == 0);
    ok = 1;
    for (i = 0; i < 10; i++) {
        if (strchr(levels, out[i]) == NULL) ok = 0;
    }
    CHECK(ok == 1);
    CHECK(out[0] == ' ' && out[1] == '.' && out[9] == '@');
    CHECK(out[8] == '@'); /* 900 and 1000 share the top level */

    printf("spark_line shorter than width fills left to right:\n");
    memset(&h, 0, sizeof(h));
    hist_push(&h, 500);
    spark_line(&h, out, 8);
    CHECK(out[8] == 0);
    CHECK(out[0] == '@' && out[7] == ' ');
}

int main(void) {
    test_parse_num();
    test_parse_uptime();
    test_permille();
    test_fmt_uptime();
    test_hist();
    test_spark();
    printf("\ntest_otop: %d checks, %d failures\n", checks, failures);
    return failures > 0;
}
