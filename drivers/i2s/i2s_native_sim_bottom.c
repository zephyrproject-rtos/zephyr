/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef NS_I2S_ALSA
#include <alsa/asoundlib.h>
#endif

#include <nsi_errno.h>
#include <nsi_tracing.h>

#include "i2s_native_sim_bottom.h"

#define NS_I2S_ALSA_PREFIX "alsa:"

struct ns_i2s_dev {
	bool used;
	int fd;
#ifdef NS_I2S_ALSA
	snd_pcm_t *pcm;
	uint32_t frame_bytes;
	int rx;
	/* Frames of the block in progress already moved to or from the device */
	uint32_t done;
#endif
};

/* Grows as streams are opened. The index into it is the handle returned to the driver. */
static struct ns_i2s_dev *ns_i2s_devs;
static int ns_i2s_dev_count;

static int ns_i2s_dev_alloc(void)
{
	struct ns_i2s_dev *dev;
	int idx;

	for (idx = 0; idx < ns_i2s_dev_count; idx++) {
		if (!ns_i2s_devs[idx].used) {
			break;
		}
	}
	if (idx == ns_i2s_dev_count) {
		dev = realloc(ns_i2s_devs, (ns_i2s_dev_count + 1) * sizeof(*dev));
		if (dev == NULL) {
			return -NSI_ERRNO_MID_ENOMEM;
		}
		ns_i2s_devs = dev;
		ns_i2s_dev_count++;
	}
	memset(&ns_i2s_devs[idx], 0, sizeof(ns_i2s_devs[idx]));
	ns_i2s_devs[idx].used = true;

	return idx;
}

#ifdef NS_I2S_ALSA
/* Returns idx, or a negative error after releasing the slot */
static int alsa_open(int idx, const char *name, int rx, unsigned int rate, unsigned int frame_bytes,
		     unsigned int word_size)
{
	struct ns_i2s_dev *dev = &ns_i2s_devs[idx];
	snd_pcm_format_t fmt;
	snd_pcm_t *pcm = NULL;
	uint32_t sample_bytes;
	int ret;

	switch (word_size) {
	case 8:
		fmt = SND_PCM_FORMAT_S8;
		sample_bytes = 1;
		break;
	case 16:
		fmt = SND_PCM_FORMAT_S16_LE;
		sample_bytes = 2;
		break;
	case 24:
		/* 24-bit words are LSB-justified in 32-bit samples */
		fmt = SND_PCM_FORMAT_S24_LE;
		sample_bytes = 4;
		break;
	case 32:
		fmt = SND_PCM_FORMAT_S32_LE;
		sample_bytes = 4;
		break;
	default:
		nsi_print_warning("ALSA %s: %u-bit words are not supported\n", name, word_size);
		dev->used = false;
		return -NSI_ERRNO_MID_EINVAL;
	}
	if (frame_bytes == 0 || frame_bytes % sample_bytes != 0) {
		dev->used = false;
		return -NSI_ERRNO_MID_EINVAL;
	}

	ret = snd_pcm_open(&pcm, name, rx ? SND_PCM_STREAM_CAPTURE : SND_PCM_STREAM_PLAYBACK,
			   SND_PCM_NONBLOCK);
	if (ret == 0) {
		ret = snd_pcm_set_params(pcm, fmt, SND_PCM_ACCESS_RW_INTERLEAVED,
					 frame_bytes / sample_bytes, rate, 1, 200000);
	}
	if (ret < 0) {
		nsi_print_warning("ALSA %s could not be opened (%s)\n", name, snd_strerror(ret));
		if (pcm != NULL) {
			snd_pcm_close(pcm);
		}
		dev->used = false;
		return -nsi_errno_to_mid(-ret);
	}

	dev->pcm = pcm;
	dev->frame_bytes = frame_bytes;
	dev->rx = rx;

	return idx;
}

/*
 * Moves one block to or from the device without blocking. ALSA may move only
 * part of it, so the progress is kept in dev->done and the same block is
 * completed over retries: returns -NSI_ERRNO_MID_EAGAIN until the device has
 * taken or provided the whole block, and len once it has.
 */
static long alsa_xfer(struct ns_i2s_dev *dev, char *buf, long len)
{
	long frames = len / dev->frame_bytes;
	snd_pcm_sframes_t n;

	while (dev->done < frames) {
		char *pos = buf + dev->done * dev->frame_bytes;

		if (dev->rx && snd_pcm_state(dev->pcm) == SND_PCM_STATE_PREPARED) {
			(void)snd_pcm_start(dev->pcm);
		}
		n = dev->rx ? snd_pcm_readi(dev->pcm, pos, frames - dev->done)
			    : snd_pcm_writei(dev->pcm, pos, frames - dev->done);
		if (n == -EAGAIN || n == 0) {
			return -NSI_ERRNO_MID_EAGAIN;
		}
		if (n < 0) {
			/* xrun or suspend: recover and go on with the block */
			if (snd_pcm_recover(dev->pcm, (int)n, 1) < 0) {
				dev->done = 0;
				return -NSI_ERRNO_MID_EIO;
			}
			continue;
		}
		dev->done += (uint32_t)n;
	}
	dev->done = 0;

	return len;
}

static void alsa_close(struct ns_i2s_dev *dev)
{
	if (!dev->rx) {
		(void)snd_pcm_nonblock(dev->pcm, 0);
		(void)snd_pcm_drain(dev->pcm);
	}
	snd_pcm_close(dev->pcm);
	dev->pcm = NULL;
	dev->used = false;
}
#endif /* NS_I2S_ALSA */

/* Returns idx, or a negative error after releasing the slot */
static int file_open(int idx, const char *pathname, int rx)
{
	int fd;

	if (rx) {
		fd = open(pathname, O_RDONLY);
	} else {
		fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	}

	if (fd < 0) {
		nsi_print_warning("%s could not be opened (%s)\n", pathname, strerror(errno));
		ns_i2s_devs[idx].used = false;
		return -nsi_errno_to_mid(errno);
	}

	ns_i2s_devs[idx].fd = fd;

	return idx;
}

int ns_i2s_open_bottom(const char *pathname, int rx, unsigned int rate, unsigned int frame_bytes,
		       unsigned int word_size)
{
	int idx = ns_i2s_dev_alloc();

	if (idx < 0) {
		return idx;
	}

#ifdef NS_I2S_ALSA
	if (strncmp(pathname, NS_I2S_ALSA_PREFIX, strlen(NS_I2S_ALSA_PREFIX)) == 0) {
		return alsa_open(idx, pathname + strlen(NS_I2S_ALSA_PREFIX), rx, rate, frame_bytes,
				 word_size);
	}
#else
	(void)rate;
	(void)frame_bytes;
	(void)word_size;
#endif

	return file_open(idx, pathname, rx);
}

long ns_i2s_write_bottom(int idx, const void *buf, long len)
{
#ifdef NS_I2S_ALSA
	if (ns_i2s_devs[idx].pcm != NULL) {
		return alsa_xfer(&ns_i2s_devs[idx], (char *)buf, len);
	}
#endif

	return write(ns_i2s_devs[idx].fd, buf, len);
}

long ns_i2s_read_bottom(int idx, void *buf, long len)
{
#ifdef NS_I2S_ALSA
	if (ns_i2s_devs[idx].pcm != NULL) {
		return alsa_xfer(&ns_i2s_devs[idx], buf, len);
	}
#endif

	return read(ns_i2s_devs[idx].fd, buf, len);
}

void ns_i2s_close_bottom(int idx)
{
#ifdef NS_I2S_ALSA
	if (ns_i2s_devs[idx].pcm != NULL) {
		alsa_close(&ns_i2s_devs[idx]);
		return;
	}
#endif

	(void)close(ns_i2s_devs[idx].fd);
	ns_i2s_devs[idx].used = false;
}
