#include <common.h>
#include <devices.h>
#include <klib.h>
#include <kmt.h>
#include <procfs.h>
#include <util.h>

int parse_path(const char* str);
filesystem_t* parse_fs(const char* str);

static char buf[1024];

void mount_test() {
    util_log("vfs_test mount: start.", 0, LOG_WARNING, LOG_NNONE);

    char* paths[16] = {
        "/mnt1",  "/mnt2",  "/mnt3",  "/mnt4",  "/mnt5",  "/mnt6",
        "/mnt7",  "/mnt8",  "/mnt9",  "/mnt10", "/mnt11", "/mnt12",
        "/mnt13", "/mnt14", "/mnt15", "/mnt16",
    };

    for (int i = 0; i < 16; i++) {
        assert(vfs->mount(paths[i], (filesystem_t*)(i + 1)) == 0);
    }

    for (int i = 0; i < 16; i++) {
        assert((long)parse_fs(paths[i]) == i + 1);
    }

    // for (int i = 0; i < 16; i++) {
    //     assert(vfs->mount(paths[i], NULL) != 0);
    // }

    // for (int i = 1; i < 16; i++) {
    //     assert((long)parse_fs(paths[i]) == i);
    // }

    for (int i = 0; i < 16; i++) {
        assert(vfs->unmount(paths[i]) == 0);
    }

    util_log("vfs_test mount: pass.", 0, LOG_SUCCESS, LOG_NNONE);
}

void parse_test() {
    util_log("vfs_test parse: start.", 0, LOG_WARNING, LOG_NNONE);

    assert(parse_path("") == 0);
    assert(parse_path("/") == 1);
    assert(parse_path("/mnt") == 4);
    assert(parse_path("/mnt/") == 4);
    assert(parse_path("/mnt/fuck1") == 4);
    assert(parse_path("/mnt/fuck1/fuck") == 4);

    util_log("vfs_test parse: pass.", 0, LOG_SUCCESS, LOG_NNONE);
}

static const char* test_hello = "Hello from vfs test!\n";
static const char* test_hello2 =
    "Type \"continue\" in QEMU's terminal to continue the test. (Please type "
    "exactly, or you will get assert 0.)(DEPRECATED)\n";
static const char* test_continue = "continue\n";

void dev_test() {
    util_log("vfs_test dev: start.", 0, LOG_WARNING, LOG_NNONE);

    // tty
    int fd_tty = vfs->open("/dev/tty1", 0);
    assert(fd_tty != -1);
    // Check qemu's screen.
    assert(vfs->write(fd_tty, (void*)test_hello, strlen(test_hello)) ==
           strlen(test_hello));
    assert(vfs->write(fd_tty, (void*)test_hello2, strlen(test_hello2)) ==
           strlen(test_hello2));
    util_log(test_hello2, 0, LOG_WARNING, LOG_NNONE);

    // DEPRECATED
    // assert(vfs->read(fd_tty, buf, 1024) == strlen(test_continue));
    // assert(strcmp(buf, test_continue) == 0);

    assert(vfs->write(fd_tty + 1, "fuck", 4) == -1);
    assert(vfs->write(NOFILE + 1, "fuck", 4) == -1);

    // ramdisk
    // DEPRECATED: It will destory the filesystem at /mnt
    // int fd_ram = vfs->open("/dev/ramdisk1", 0);
    // assert(fd_ram != -1);
    // assert(vfs->write(fd_ram, "hahaha", 6) == 6);
    // assert(vfs->lseek(fd_ram, 0, SEEK_SET) == 0);
    // assert(vfs->read(fd_ram, buf, 6) == 6);
    // assert(strncmp(buf, "hahaha", 6) == 0);

    // assert(vfs->lseek(fd_ram, -8, SEEK_END) == RD_SIZE - 8);
    // assert(vfs->write(fd_ram, "12345678", 8) == 8);
    // assert(vfs->lseek(fd_ram, -4, SEEK_CUR) == RD_SIZE - 4);
    // assert(vfs->read(fd_ram, buf, 4) == 4);
    // assert(strncmp(buf, "5678", 4) == 0);

    // assert(vfs->mkdir("/dev/ramdisk1/dir1") == -1);
    // assert(vfs->rmdir("/dev/ramdisk1/dir1") == -1);

    // util
    int rand1, rand2;
    int fd_rand = vfs->open("/dev/random", 0);
    assert(fd_rand != -1);
    assert(vfs->read(fd_rand, buf, 4) == 4);
    rand1 = *((uint32_t*)buf);
    assert(vfs->read(fd_rand, buf, 4) == 4);
    rand2 = *((uint32_t*)buf);
    assert(rand1 != rand2);
    assert(vfs->write(fd_rand, buf, 8) == 8);

    // zero
    int fd_zero = vfs->open("/dev/zero", 0);
    assert(fd_zero != -1);
    assert(vfs->read(fd_zero, buf, 4) == 4);
    assert(*((uint32_t*)buf) == 0);
    assert(vfs->write(fd_zero, buf, 8) == 8);

    // null
    int fd_null = vfs->open("/dev/null", 0);
    assert(fd_null != -1);
    assert(vfs->read(fd_null, buf, 4) == EOF);
    assert(vfs->write(fd_null, buf, 8) == 8);

    assert(vfs->close(fd_tty) == 0);
    // assert(vfs->close(fd_ram) == 0);
    assert(vfs->close(fd_rand) == 0);
    assert(vfs->close(fd_zero) == 0);
    assert(vfs->close(fd_null) == 0);

    util_log("vfs_test dev: pass.", 0, LOG_SUCCESS, LOG_NNONE);
}

void proc_test() {
    util_log("vfs_test proc: start.", 0, LOG_WARNING, LOG_NNONE);

    assert(util_s2u32("123", 0) == 123);
    assert(util_s2u32("123", 3) == 123);
    assert(util_s2u32("123///", 3) == 123);
    assert(util_s2u32("123///", 0) == (uint32_t)-1);

    // broken
    int fd_broken = vfs->open("/proc/nosuchchild", 0);
    assert(fd_broken == -1);

    int fd_tty = vfs->open("/dev/tty1", 0);
    assert(fd_tty != -1);

    // cpuinfo
    int fd_cpuinfo = vfs->open("/proc/cpuinfo", 0);
    assert(fd_cpuinfo != -1);
    assert(((procfs_t*)task_current[_cpu()]->files[fd_cpuinfo]->data)->type ==
           PROCFS_CPUINFO);
    assert(vfs->read(fd_cpuinfo, buf, 1024) >= 0);
    util_log(buf, 0, LOG_INFO, LOG_NNONE);
    vfs->write(fd_tty, buf, strlen(buf));

    // meminfo
    int fd_meminfo = vfs->open("/proc/meminfo", 0);
    assert(fd_meminfo != -1);
    assert(((procfs_t*)task_current[_cpu()]->files[fd_meminfo]->data)->type ==
           PROCFS_MEMINFO);
    assert(vfs->read(fd_meminfo, buf, 1024) >= 0);
    util_log(buf, 0, LOG_INFO, LOG_NNONE);
    vfs->write(fd_tty, buf, strlen(buf));

    // proc
    int fd_proc = vfs->open("/proc/1", 0);
    assert(fd_proc != -1);
    assert(((procfs_t*)task_current[_cpu()]->files[fd_proc]->data)->type ==
           PROCFS_PROC);
    assert(((procfs_t*)task_current[_cpu()]->files[fd_proc]->data)->pid == 1);
    assert(vfs->read(fd_proc, buf, 1024) >= 0);
    util_log(buf, 0, LOG_INFO, LOG_NNONE);

    assert(vfs->close(fd_tty) == 0);
    assert(vfs->close(fd_cpuinfo) == 0);
    assert(vfs->close(fd_meminfo) == 0);
    assert(vfs->close(fd_proc) == 0);

    util_log("vfs_test proc: pass.", 0, LOG_SUCCESS, LOG_NNONE);
}

void diskfs_test() {
    util_log("vfs_test disk: start.", 0, LOG_WARNING, LOG_NNONE);

    int fd_dir = -1;
    assert(vfs->mkdir("/mnt/fuckdir") == 0);
    assert(vfs->mkdir("/mnt/fuckdir/fuckdir2") == 0);
    assert(vfs->mkdir("/mnt/fuckdir1/fuckdir2") != 0);
    assert(vfs->rmdir("/mnt/fuckdir1/fuckdir2") != 0);
    assert(vfs->rmdir("/mnt/fuckdir/fuckdir2") == 0);
    assert((fd_dir = vfs->open("/mnt/fuckdir", MODE_R)) != -1);
    assert(vfs->close(fd_dir) == 0);
    assert(vfs->rmdir("/mnt/fuckdir") == 0);
    assert(vfs->open("/mnt/fuckdir", MODE_R) == -1);
    assert((fd_dir = vfs->open("/mnt/fuckdir", MODE_W)) != -1);
    assert(vfs->close(fd_dir) == 0);
    assert(vfs->rmdir("/mnt/fuckdir") == 0);

    int fd_file = -1;
    assert((fd_file = vfs->open("/mnt/main.cpp", MODE_W)) != -1);
    vfs->write(fd_file, (void*)test_continue, strlen(test_continue));
    vfs->lseek(fd_file, 0, SEEK_SET);
    vfs->read(fd_file, buf, 10);
    util_log(buf, 0, LOG_SUCCESS, LOG_NNONE);
    assert(vfs->close(fd_file) == 0);

    util_log("vfs_test disk: pass.", 0, LOG_SUCCESS, LOG_NNONE);
}

void vfs_test(void* arg) {
    parse_test();
    mount_test();

    dev_test();
    proc_test();
    diskfs_test();

    util_log("vfs_test all pass.", 0, LOG_SUCCESS, LOG_NNONE);

    while (1)
        ;
}