#include "unity.h"
#include "examples/systemcalls/systemcalls.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void test_nonzero_and_signal_exit_fail(void)
{
    TEST_ASSERT_FALSE(do_system("exit 7"));
    TEST_ASSERT_FALSE(do_system("kill -TERM $$"));
    TEST_ASSERT_FALSE(do_exec(3, "/bin/sh", "-c", "exit 7"));
    TEST_ASSERT_FALSE(do_exec(3, "/bin/sh", "-c", "kill -TERM $$"));
}

void test_invalid_commands_fail(void)
{
    TEST_ASSERT_FALSE(do_system(NULL));
    TEST_ASSERT_FALSE(do_exec(0));
    TEST_ASSERT_FALSE(do_exec(1, (char *)NULL));
    TEST_ASSERT_FALSE(do_exec(1, "/nonexistent/aeld-command"));
    TEST_ASSERT_FALSE(do_exec(1, "echo"));
}

void test_redirect_reports_failures(void)
{
    TEST_ASSERT_FALSE(do_exec_redirect(NULL, 1, "/bin/echo"));
    TEST_ASSERT_FALSE(do_exec_redirect("/nonexistent/aeld/output", 1, "/bin/echo"));
    TEST_ASSERT_FALSE(do_exec_redirect(".", 1, "/bin/echo"));
}

void test_redirect_truncates_existing_file(void)
{
    const char *path = "systemcalls-redirect-test.txt";
    TEST_ASSERT_TRUE(do_exec_redirect(path, 2, "/bin/echo", "long initial contents"));
    TEST_ASSERT_TRUE(do_exec_redirect(path, 2, "/bin/echo", "new"));
    FILE *file = fopen(path, "r");
    TEST_ASSERT_NOT_NULL(file);
    char output[64] = {0};
    size_t length = fread(output, 1, sizeof(output) - 1, file);
    fclose(file);
    unlink(path);
    TEST_ASSERT_EQUAL_UINT(4, length);
    TEST_ASSERT_EQUAL_STRING("new\n", output);
}
