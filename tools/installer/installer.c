/* One-shot installer: writes the HBC app files (boot.dol, meta.xml,
 * icon.png) to sd:/apps/meshtasticd/. Send it once with wiiload (install.sh
 * does); it does the rest. */
#include <stdio.h>
#include <string.h>

#include <fat.h>
#include <gccore.h>
#include <sys/stat.h>
#include <wiiuse/wpad.h>

#include "install_data.h"

#define INSTALL_APP "meshtasticd"
#define INSTALL_TITLE "Meshtastic"

static void *xfb;

static void video_start(void)
{
    GXRModeObj *r;
    VIDEO_Init();
    r = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(r));
    console_init(xfb, 20, 20, r->fbWidth, r->xfbHeight, r->fbWidth * VI_DISPLAY_PIX_SZ);
    VIDEO_Configure(r);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();
}

int main(void)
{
    int i, ok = 0;

    video_start();
    WPAD_Init();
    printf("\x1b[2;1H" INSTALL_TITLE " installer\n");
    printf("%u files, %u KiB payload\n\n",
           (unsigned)INSTALL_NFILES, (unsigned)(INSTALL_TOTAL_BYTES / 1024));

    if (!fatInitDefault()) {
        printf("No SD card found. Push Home to exit.\n");
        goto wait;
    }
    for (i = 0; i < INSTALL_NFILES; i++) {
        char path[128], dir[128];
        char *slash, *p;
        FILE *f;
        size_t n = 0;
        snprintf(path, sizeof(path), "sd:/apps/" INSTALL_APP "/%s", INSTALL_FILES[i].name);
        /* Create every parent directory of this file. */
        snprintf(dir, sizeof(dir), "%s", path);
        slash = strrchr(dir, '/');
        if (slash != NULL) {
            *slash = '\0';
            for (p = dir + 4; *p; p++) /* skip "sd:/" */
                if (*p == '/') { *p = '\0'; mkdir(dir, 0777); *p = '/'; }
            mkdir(dir, 0777);
        }
        f = fopen(path, "wb");
        if (f != NULL) {
            n = fwrite(INSTALL_FILES[i].data, 1, INSTALL_FILES[i].len, f);
            fclose(f);
        }
        if (n == INSTALL_FILES[i].len)
            ok++;
        else
            printf("  FAILED %s\n", INSTALL_FILES[i].name);
        if ((i % 20) == 0 || i == INSTALL_NFILES - 1) {
            printf("\x1b[6;1H  installing... %d / %d files   ", i + 1, INSTALL_NFILES);
        }
    }

    printf("\n\n%s. %d files written to sd:/apps/" INSTALL_APP "/.\n",
           ok == INSTALL_NFILES ? "Done" : "Some files failed", ok);
wait:
    printf("\nPush Home to exit.\n");
    for (;;) {
        WPAD_ScanPads();
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)
            break;
        VIDEO_WaitVSync();
    }
    return 0;
}
