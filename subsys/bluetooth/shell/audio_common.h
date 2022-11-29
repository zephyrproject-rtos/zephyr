
#include <stdio.h>

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>

static inline void print_qos(const struct shell *sh,
			     const struct bt_codec_qos *qos)
{
	shell_print(ctx_shell, "QoS: interval %u framing 0x%02x "
		    "phy 0x%02x sdu %u rtn %u latency %u pd %u",
		    qos->interval, qos->framing, qos->phy, qos->sdu,
		    qos->rtn, qos->latency, qos->pd);
}

static inline void print_codec(const struct shell *sh,
			       const struct bt_codec *codec)
{
	shell_print(sh, "codec 0x%02x cid 0x%04x vid 0x%04x count %u",
		    codec->id, codec->cid, codec->vid, codec->data_count);

	for (size_t i = 0U; i < codec->data_count; i++) {
		shell_print(sh, "data #%u: type 0x%02x len %u", i,
			    codec->data[i].data.type,
			    codec->data[i].data.data_len);
		shell_hexdump(sh, codec->data[i].data.data,
			      codec->data[i].data.data_len -
				sizeof(codec->data[i].data.type));
	}

	for (size_t i = 0U; i < codec->meta_count; i++) {
		shell_print(sh, "meta #%u: type 0x%02x len %u", i,
			    codec->meta[i].data.type,
			    codec->meta[i].data.data_len);
		shell_hexdump(sh, codec->meta[i].data.data,
			      codec->data[i].data.data_len -
				sizeof(codec->meta[i].data.type));
	}
}

static inline void print_base(const struct shell *sh,
			      const struct bt_audio_base *base)
{
	uint8_t bis_indexes[BROADCAST_SNK_STREAM_CNT] = { 0 };
	/* "0xXX " requires 5 characters */
	char bis_indexes_str[5 * ARRAY_SIZE(bis_indexes) + 1];
	size_t index_count = 0;

	for (size_t i = 0U; i < base->subgroup_count; i++) {
		const struct bt_audio_base_subgroup *subgroup;

		subgroup = &base->subgroups[i];

		shell_print(sh, "Subgroup[%d]:", i);
		print_codec(sh, &subgroup->codec);

		for (int j = 0; j < subgroup->bis_count; j++) {
			const struct bt_audio_base_bis_data *bis_data;

			bis_data = &subgroup->bis_data[j];

			shell_print(sh, "BIS[%d] index 0x%02x",
				    i, bis_data->index);
			bis_indexes[index_count++] = bis_data->index;

			for (int k = 0; k < bis_data->data_count; k++) {
				const struct bt_codec_data *codec_data;

				codec_data = &bis_data->data[k];

				shell_print(sh,
					    "data #%u: type 0x%02x len %u",
					    i, codec_data->data.type,
					    codec_data->data.data_len);
				shell_hexdump(sh, codec_data->data.data,
					      codec_data->data.data_len -
						sizeof(codec_data->data.type));
			}
		}
	}

	(void)memset(bis_indexes_str, 0, sizeof(bis_indexes_str));

	/* Create space separated list of indexes as hex values */
	for (int i = 0; i < index_count; i++) {
		char bis_index_str[6];

		sprintf(bis_index_str, "0x%02x ", bis_indexes[i]);

		strcat(bis_indexes_str, bis_index_str);
		shell_print(sh, "[%d]: %s", i, bis_index_str);
	}

	shell_print(sh, "Possible indexes: %s", bis_indexes_str);
}
