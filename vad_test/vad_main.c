#ifdef C4A_HIFI4
int main(int argc, char* argv[])
{
    printf("Not support such case\n");
    return 0;
}
#else
#include <tinyalsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include "generic_macro.h"
#define ALSA_CARD_PATH  "/proc/asound/cards"
#define ALSA_PCM_PATH   "/proc/asound/pcm"

/* auge sound card */
#define CARD_AUGE       "AMLAUGESOUND"
#define BUF_SIZE     (512)

#define MIXER_VAD_EN        "VAD enable"
#define MIXER_VAD_TEST      "VAD Test"
#define MIXER_PDM_LOWPOWER  "PDM Low Power mode"

#define VAD_SRC_PDM     "PDM"
#define VAD_SRC_LB      "LOOPBACK"

/*
 * eg.
 * cat /proc/asound/cards
 * 0 [AMLAUGESOUND   ]: AML-AUGESOUND - AML-AUGESOUND
 *                     AML-AUGESOUND
 */
int get_card(void)
{
	FILE *p_card = NULL;
	char tmp_buf[BUF_SIZE];
	int card_idx = -1;

	p_card = fopen(ALSA_CARD_PATH, "r");
	if (!p_card) {
		printf("No Node %s. Please check to setup sound card.\n", ALSA_CARD_PATH);
		return -ENXIO;
	}
	while (!feof(p_card)) {
		fgets(tmp_buf, BUF_SIZE, p_card);

		/* this line contain '[' character */
		if (strchr(tmp_buf, '[')) {
			char *Rch = strtok(tmp_buf, "[");
			int tmp_idx = atoi(Rch);
			Rch = strtok(NULL, " ]");
			if (strcmp(Rch, CARD_AUGE) == 0) {
				card_idx = tmp_idx;
				printf("\t %s, card index = %d\n", Rch, card_idx);
				break;
			}
		}
		memset((void *)tmp_buf, 0, BUF_SIZE);
	}
	fclose(p_card);

	return card_idx;
}

/*
 * eg.
 * cat /proc/asound/pcm
 * 00-00: TDM-A-tas5805m-amplifier-alsaPORT-i2s multicodec-0 :  : playback 1 : capture 1
 * 00-01: TDM-B-dummy-alsaPORT-pcm dummy-1 :  : playback 1 : capture 1
 * 00-02: TDM-C-dummy dummy-2 :  : playback 1 : capture 1
 * 00-03: PDM-dummy-alsaPORT-pdm dummy-3 :  : capture 1
 * 00-04: SPDIF-dummy-alsaPORT-spdif dummy-4 :  : playback 1 : capture 1
 * 00-05: SPDIF-B-dummy-alsaPORT-spdifb dummy-5 :  : playback 1
 * 00-06: EXTN-dummy-alsaPORT-tv dummy-6 :  : playback 1 : capture 1
 * 00-07: EARC/ARC-dummy-alsaPORT-earc dummy-7 :  : playback 1 : capture 1
 * 00-08: LOOPBACK-A-dummy-alsaPORT-loopback dummy-8 :  : capture 1
 */
int get_device(char *src_name)
{
	FILE *p_pcms = NULL;
	char tmp_buf[BUF_SIZE];
	char *tmp_str;
	int device_idx = -1;

	printf("%s, device name:%s\n", __FUNCTION__, src_name);

	if (!src_name)
		return device_idx;

	p_pcms = fopen(ALSA_PCM_PATH, "r");
	if (!p_pcms) {
		printf("No device %s. Please check to setup sound device.\n", ALSA_PCM_PATH);
		return -ENXIO;
	}
	while (!feof(p_pcms)) {
		int tmp_cardidx, tmp_pcmidx;
		fgets(tmp_buf, BUF_SIZE, p_pcms);

		tmp_str = strtok(tmp_buf, "-");
		tmp_cardidx = atoi(tmp_str);
		tmp_str = strtok(NULL, ":");
		tmp_pcmidx = atoi(tmp_str);
		tmp_str = strtok(NULL, ": ");

		if (tmp_str) {
			char *PortName = strstr(tmp_str, src_name);

			if (!strncmp(tmp_str, src_name, strlen(src_name))) {
				printf("\t %s, card index:%d, device index:%d\n",
					__FUNCTION__, tmp_cardidx, tmp_pcmidx);
				device_idx = tmp_pcmidx;
				break;
			}
		}
		memset((void *)tmp_buf, 0, BUF_SIZE);
	}
	fclose(p_pcms);

	return device_idx;
}

int capturing = 1;

void sigint_handler(int signum)
{
	AMX_UNUSED(signum);
    capturing = 0;
}

unsigned int capturing_audio(unsigned int card, unsigned int device,
                            unsigned int channels, unsigned int rate,
                            enum pcm_format format, unsigned int period_size,
                            unsigned int period_count)
{
    struct pcm_config config;
    struct pcm *pcm;
    char *buffer;
    unsigned int size;
    unsigned int bytes_read = 0;
    unsigned int frames = 0;

    memset(&config, 0, sizeof(config));
    config.channels = channels;
    config.rate = rate;
    config.period_size = period_size;
    config.period_count = period_count;
    config.format = format;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;

    pcm = pcm_open(card, device, PCM_IN, &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        fprintf(stderr, "Unable to open PCM device (%s)\n",
                pcm_get_error(pcm));
        return 0;
    }

    size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
    buffer = malloc(size);
    if (!buffer) {
        fprintf(stderr, "Unable to allocate %u bytes\n", size);
        free(buffer);
        pcm_close(pcm);
        return 0;
    }

    printf("Capturing sample: %u ch, %u hz, %u bit\n", channels, rate,
           pcm_format_to_bits(format));

    while (capturing /*&& !pcm_read(pcm, buffer, size)*/) {
		/* ignore read failed */
		pcm_read(pcm, buffer, size);
        bytes_read += size;
    }

    frames = pcm_bytes_to_frames(pcm, bytes_read);
    free(buffer);
    pcm_close(pcm);
    return frames;
}

int main(int argc, char *argv[])
{
	int card_idx, device_idx;
	struct mixer *pmixer = NULL;
	struct mixer_ctl *pctrl = NULL;
	enum pcm_format format = PCM_FORMAT_S32_LE;
	unsigned int channels = 2;
	unsigned int rate = 16000;
	unsigned int period_size = 256;
	unsigned int period_count = 4;
	unsigned int bits;
    unsigned int frames = 0;
	int last_vad_sts = 0;
	char *vad_src = NULL;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s [-s vad source:%s or %s]"
                " [-c channels] [-r rate] [-b bits] [-p period_size]"
                " [-n n_periods]\n",
                argv[0],
                VAD_SRC_PDM,
                VAD_SRC_LB);
        return 1;
    }

    /* parse command line arguments */
    argv += 1;
    while (*argv) {
        if (strcmp(*argv, "-c") == 0) {
            argv++;
            if (*argv)
                channels = atoi(*argv);
        } else if (strcmp(*argv, "-r") == 0) {
            argv++;
            if (*argv)
                rate = atoi(*argv);
        } else if (strcmp(*argv, "-b") == 0) {
            argv++;
            if (*argv)
                bits = atoi(*argv);
        } else if (strcmp(*argv, "-p") == 0) {
            argv++;
            if (*argv)
                period_size = atoi(*argv);
        } else if (strcmp(*argv, "-n") == 0) {
            argv++;
            if (*argv)
                period_count = atoi(*argv);
        } else if (strcmp(*argv, "-s") == 0) {
            argv++;
            if (*argv) {
				if (!strncmp(*argv, VAD_SRC_PDM, strlen(VAD_SRC_PDM)) ||
					!strncmp(*argv, VAD_SRC_LB, strlen(VAD_SRC_LB))) {
					vad_src = strdup(*argv);
					printf("vad_src:%s\n", vad_src);
				} else {
					fprintf(stderr,
						"only supported VAD source :%s or %s\n",
						VAD_SRC_PDM, VAD_SRC_LB);
					return 1;
				}
            }
        }
        if (*argv)
            argv++;
    }

    switch (bits) {
    case 32:
        format = PCM_FORMAT_S32_LE;
        break;
    case 24:
        format = PCM_FORMAT_S24_LE;
        break;
    case 16:
        format = PCM_FORMAT_S16_LE;
        break;
    default:
        break;
    }

	card_idx = get_card();
	device_idx = get_device(vad_src);
	pmixer = mixer_open(card_idx);

	if (card_idx < 0 || device_idx < 0 || !pmixer)
		return -ENXIO;

	/* mixer config */
	/* vad enable */
	pctrl = mixer_get_ctl_by_name(pmixer, MIXER_VAD_EN);
	last_vad_sts = mixer_ctl_get_value(pctrl, 0);
	if (!last_vad_sts)
		mixer_ctl_set_value(pctrl, 0, 1);

    /* install signal handler and begin capturing */
    signal(SIGINT, sigint_handler);
    signal(SIGHUP, sigint_handler);
    signal(SIGTERM, sigint_handler);

	/* record audio data */
    frames = capturing_audio(card_idx, device_idx, channels,
				rate, format, period_size, period_count);
    printf("Captured %u frames\n", frames);

	/* restore mixer vad enable */
	if (!last_vad_sts)
		mixer_ctl_set_value(pctrl, 0, 0);

	mixer_close(pmixer);

	if (vad_src)
		free(vad_src);

	return 0;
}
#endif
