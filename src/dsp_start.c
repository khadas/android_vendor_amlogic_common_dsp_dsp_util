#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <log/log.h>

#define DSP_UTIL "/vendor/bin/dsp_util"

int main(void)
{
    pid_t pid;
    int status, i, dsp_cnt = 0;
    char *arg_load_a[] = {"dsp_util", "-d", "hifi4a", "-f", "dspboota.bin", "-l", NULL};
    char *arg_start_a[] = {"dsp_util", "-d", "hifi4a", "-S", NULL};
    char *arg_load_b[] = {"dsp_util", "-d", "hifi4b", "-f", "dspbootb.bin", "-l", NULL};
    char *arg_start_b[] = {"dsp_util", "-d", "hifi4b", "-S", NULL};
    char **arg[4];
    char *cmd_info[4] = {"load dspa firmware", "start dspa", "load dspb firmware", "start dspb"};

    arg[0] = arg_load_a;
    arg[2] = arg_load_b;
    arg[1] = arg_start_a;
    arg[3] = arg_start_b;

#ifdef TWODSP
    dsp_cnt = 2;
#elif ONEDSP
    dsp_cnt = 1;
#endif

    ALOGI("[%d]native service begin to start %d dsp...\n", getpid(), dsp_cnt);

    for (i = 0; i < 2 * dsp_cnt; i++) {
        pid = fork();
        if (pid == 0) {
            ALOGI("[%d]child process begin to %s...\n", getpid(), cmd_info[i]);
            if (execv(DSP_UTIL, arg[i]) < 0)
            {
                ALOGI("execv error\n");
                exit(1);
            }
        }
        waitpid(pid, &status, WUNTRACED | WCONTINUED);
        //sleep(1);
        ALOGI("[%d]parent process  %s succeed...\n", getpid(), cmd_info[i]);
    }

    return 0;
}
