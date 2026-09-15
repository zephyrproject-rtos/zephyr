/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#include <zephyr/video/formats.h>

#include <zephyr/mpipe/mpipe_bin.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_object.h>
#include <zephyr/mpipe/mpipe_pad.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_value.h>
#include <zephyr/mpipe/utils/mpipe_dump.h>

/**
 * Longest line held before it is written out; a longer line is split, and a
 * mid-line split lets an active shell inject its prompt into the graph, so
 * this fits a node line whose ports carry caps.
 */
#define DUMP_LINE_MAX 256

/**
 * A sink with one line of output held in front of it. Writing fragment by
 * fragment floods the deferred log buffer behind printk() and drops lines;
 * holding a line brings a graph down to a dozen writes.
 */
struct mpipe_dump_writer {
	/** Where a completed line is written */
	const struct mpipe_dump_sink *sink;
	/** Number of characters held in @ref line */
	size_t len;
	/** The line being built, not NUL-terminated until it is written */
	char line[DUMP_LINE_MAX];
};

/**
 * State of one dump. Elements are indexed up front: a node's DOT name is its
 * position in this array, and an edge names its peer's node by it.
 */
struct mpipe_dump_ctx {
	/** Where the rendering is written */
	struct mpipe_dump_writer writer;
	/** Every element the dumped bin holds, nested bins included */
	struct mpipe_element *elements[CONFIG_MPIPE_DUMP_MAX_ELEMENTS];
	/** Number of slots in use at the front of @ref elements */
	int num_elements;
	/** True once the bin held more elements than @ref elements can index */
	bool truncated;
};

/*
 * Names for the caps vocabulary, indexed by the identifier itself; the
 * assertions fail the build if an identifier lacks a name.
 */
/* clang-format off */
static const char *const dump_field_names[] = {
	[MPIPE_CAPS_PIXEL_FORMAT] = "format",
	[MPIPE_CAPS_IMAGE_WIDTH] = "width",
	[MPIPE_CAPS_IMAGE_HEIGHT] = "height",
	[MPIPE_CAPS_SAMPLE_RATE] = "rate",
	[MPIPE_CAPS_BITWIDTH] = "bitwidth",
	[MPIPE_CAPS_NUM_OF_CHANNEL] = "channels",
	[MPIPE_CAPS_INTERLEAVED] = "interleaved",
	[MPIPE_CAPS_FRAME_INTERVAL] = "frame-interval",
};
/* clang-format on */
BUILD_ASSERT(ARRAY_SIZE(dump_field_names) == MPIPE_CAPS_END,
	     "A caps field identifier has no name in dump_field_names");

static const char *const dump_media_names[] = {
	[MPIPE_MEDIA_UNKNOWN] = "unknown",
	[MPIPE_MEDIA_AUDIO_PCM] = "audio/pcm",
	[MPIPE_MEDIA_VIDEO] = "video",
};
BUILD_ASSERT(ARRAY_SIZE(dump_media_names) == MPIPE_MEDIA_END,
	     "A media type has no name in dump_media_names");

static void __printf_like(2, 3) dump_emit(const struct mpipe_dump_sink *sink, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	if (sink == NULL) {
		vprintk(fmt, ap);
	} else {
		sink->vprint(sink->ctx, fmt, ap);
	}

	va_end(ap);
}

/* Write out whatever line has been built so far, if any */
static void dump_flush(struct mpipe_dump_writer *w)
{
	if (w->len == 0) {
		return;
	}

	w->line[w->len] = '\0';
	w->len = 0;

	dump_emit(w->sink, "%s", w->line);
}

/*
 * Append a fragment, writing the line out once complete. A fragment that does
 * not fit flushes the held line first, so an overlong line splits, not drops.
 */
static void __printf_like(2, 3) dump_print(struct mpipe_dump_writer *w, const char *fmt, ...)
{
	va_list ap;
	va_list retry;
	size_t room = sizeof(w->line) - w->len;
	int written;

	va_start(ap, fmt);
	va_copy(retry, ap);

	written = vsnprintk(&w->line[w->len], room, fmt, ap);
	if (written >= 0 && (size_t)written >= room) {
		w->line[w->len] = '\0';
		dump_flush(w);
		written = vsnprintk(w->line, sizeof(w->line), fmt, retry);
	}

	va_end(retry);
	va_end(ap);

	if (written < 0) {
		return;
	}

	w->len = MIN(w->len + (size_t)written, sizeof(w->line) - 1);

	if (memchr(w->line, '\n', w->len) != NULL) {
		dump_flush(w);
	}
}

const char *mpipe_dump_state_str(enum mpipe_state state)
{
	switch (state) {
	case MPIPE_STATE_READY:
		return "READY";
	case MPIPE_STATE_PAUSED:
		return "PAUSED";
	case MPIPE_STATE_PLAYING:
		return "PLAYING";
	default:
		return "?";
	}
}

/* A NULL entry is a mid-enum hole the size assertions cannot see */
static const char *dump_name(const char *const *names, size_t count, uint8_t id)
{
	if (id >= count || names[id] == NULL) {
		return "?";
	}

	return names[id];
}

/* A pixel format is a fourcc, so show the four characters rather than a number */
static void dump_fourcc(struct mpipe_dump_writer *w, uint32_t fourcc)
{
	const char *str = VIDEO_FOURCC_TO_STR(fourcc);

	for (int i = 0; i < 4; i++) {
		if (isprint((unsigned char)str[i]) == 0) {
			dump_print(w, "0x%08x", fourcc);
			return;
		}
	}

	dump_print(w, "%s", str);
}

static void dump_value(struct mpipe_dump_writer *w, uint8_t field_id,
		       const struct mpipe_value *value)
{
	switch (value->type) {
	case MPIPE_TYPE_BOOLEAN:
		dump_print(w, "%s", value->v_boolean ? "true" : "false");
		break;
	case MPIPE_TYPE_INT:
		dump_print(w, "%d", value->v_int);
		break;
	case MPIPE_TYPE_UINT:
		if (field_id == MPIPE_CAPS_PIXEL_FORMAT) {
			dump_fourcc(w, value->v_uint);
		} else {
			dump_print(w, "%u", value->v_uint);
		}
		break;
	case MPIPE_TYPE_INT_RANGE:
		dump_print(w, "[%d, %d, %d]", value->range.min.v_int, value->range.max.v_int,
			   value->range.step.v_int);
		break;
	case MPIPE_TYPE_UINT_RANGE:
		dump_print(w, "[%u, %u, %u]", value->range.min.v_uint, value->range.max.v_uint,
			   value->range.step.v_uint);
		break;
	default:
		dump_print(w, "?");
		break;
	}
}

/* The media name and field list, without the framing the context chooses */
static void dump_caps_fields(struct mpipe_dump_writer *w, const struct mpipe_structure *caps)
{
	uint8_t num_fields;

	dump_print(w, "%s",
		   dump_name(dump_media_names, ARRAY_SIZE(dump_media_names), caps->media_type_id));

	/* Nothing locks the pad against a negotiation running underneath us */
	num_fields = MIN(caps->num_fields, (uint8_t)CONFIG_MPIPE_STRUCTURE_MAX_FIELDS);

	for (uint8_t i = 0; i < num_fields; i++) {
		dump_print(w, ", %s=",
			   dump_name(dump_field_names, ARRAY_SIZE(dump_field_names), caps->ids[i]));
		dump_value(w, caps->ids[i], &caps->values[i]);
	}
}

/* Render a capability inline: it joins the caller's line, not one of its own */
static void dump_caps(struct mpipe_dump_writer *w, const struct mpipe_structure *caps)
{
	/* Both constrain nothing, but ANY intersects with anything, empty with nothing */
	if (mpipe_structure_is_any(caps)) {
		dump_print(w, "<any>");
		return;
	}

	if (mpipe_structure_is_empty(caps)) {
		dump_print(w, "<empty>");
		return;
	}

	dump_print(w, "<");
	dump_caps_fields(w, caps);
	dump_print(w, ">");
}

int mpipe_dump_caps(const struct mpipe_structure *caps, const struct mpipe_dump_sink *sink)
{
	struct mpipe_dump_writer w = {.sink = sink};

	__ASSERT_NO_MSG(caps != NULL);

	dump_caps(&w, caps);
	dump_flush(&w);

	return 0;
}

/*
 * Render an element as "vid_src #1". Every init function names its element
 * after its type, so the name is printed as it was set.
 */
static void dump_element_name(struct mpipe_dump_ctx *ctx, struct mpipe_element *element)
{
	const char *name = element->name;

	if (name == NULL) {
		dump_print(&ctx->writer, "element #%u", element->object.id);
		return;
	}

	dump_print(&ctx->writer, "%s #%u", name, element->object.id);
}

static int dump_element_index(struct mpipe_dump_ctx *ctx, struct mpipe_element *element)
{
	for (int i = 0; i < ctx->num_elements; i++) {
		if (ctx->elements[i] == element) {
			return i;
		}
	}

	return -1;
}

/*
 * Index every element the bin holds, descending into nested bins: a bin is a
 * container, not a node of the graph.
 */
static void dump_index_bin(struct mpipe_dump_ctx *ctx, struct mpipe_bin *bin)
{
	struct mpipe_object *obj;

	SYS_DLIST_FOR_EACH_CONTAINER(&bin->children, obj, node) {
		struct mpipe_element *element = (struct mpipe_element *)obj;

		if ((element->object.flags & MPIPE_OBJECT_FLAG_BIN) != 0) {
			dump_index_bin(ctx, (struct mpipe_bin *)element);
			continue;
		}

		if (ctx->num_elements >= (int)ARRAY_SIZE(ctx->elements)) {
			ctx->truncated = true;
			return;
		}

		ctx->elements[ctx->num_elements] = element;
		ctx->num_elements++;
	}
}

/* Pads are record-label ports, so an edge can land on the pad it uses */
static void dump_dot_ports(struct mpipe_dump_ctx *ctx, sys_dlist_t *pads, const char *side)
{
	struct mpipe_object *obj;
	bool first = true;

	dump_print(&ctx->writer, "{");

	/* The port name is an identifier an edge refers to, the text is the label */
	SYS_DLIST_FOR_EACH_CONTAINER(pads, obj, node) {
		struct mpipe_pad *pad = (struct mpipe_pad *)obj;

		dump_print(&ctx->writer, "%s<%s%u> %s #%u", first ? "" : "|", side, obj->id, side,
			   obj->id);

		/*
		 * A linked pad's caps ride its edge; an unlinked pad has no edge, so
		 * its caps show here - in parentheses, a record label reserves <>.
		 */
		if (pad->peer == NULL && !mpipe_structure_is_any(&pad->caps)) {
			if (mpipe_structure_is_empty(&pad->caps)) {
				dump_print(&ctx->writer, "\\n(empty)");
			} else {
				dump_print(&ctx->writer, "\\n(");
				dump_caps_fields(&ctx->writer, &pad->caps);
				dump_print(&ctx->writer, ")");
			}
		}

		first = false;
	}

	dump_print(&ctx->writer, "}");
}

static const char *dump_dot_fill(enum mpipe_state state)
{
	switch (state) {
	case MPIPE_STATE_PLAYING:
		return "#d7f0d7";
	case MPIPE_STATE_PAUSED:
		return "#fdf3d0";
	default:
		return "#e6e6e6";
	}
}

/* True when any pad of the element has no peer */
static bool dump_has_unlinked_pad(struct mpipe_element *element)
{
	struct mpipe_object *obj;

	SYS_DLIST_FOR_EACH_CONTAINER(&element->sink_pads, obj, node) {
		if (((struct mpipe_pad *)obj)->peer == NULL) {
			return true;
		}
	}

	SYS_DLIST_FOR_EACH_CONTAINER(&element->src_pads, obj, node) {
		if (((struct mpipe_pad *)obj)->peer == NULL) {
			return true;
		}
	}

	return false;
}

static void dump_dot_nodes(struct mpipe_dump_ctx *ctx)
{
	for (int i = 0; i < ctx->num_elements; i++) {
		struct mpipe_element *element = ctx->elements[i];

		dump_print(&ctx->writer, "  e%d [", i);

		/* An unlinked pad marks the element rather than growing a dangling edge */
		if (dump_has_unlinked_pad(element)) {
			dump_print(&ctx->writer, "color=\"#cc0000\", penwidth=2, ");
		}

		dump_print(&ctx->writer, "fillcolor=\"%s\", label=\"{",
			   dump_dot_fill(element->current_state));

		if (!sys_dlist_is_empty(&element->sink_pads)) {
			dump_dot_ports(ctx, &element->sink_pads, "sink");
			dump_print(&ctx->writer, "|");
		}

		dump_element_name(ctx, element);
		dump_print(&ctx->writer, "\\n%s", mpipe_dump_state_str(element->current_state));

		if (!sys_dlist_is_empty(&element->src_pads)) {
			dump_print(&ctx->writer, "|");
			dump_dot_ports(ctx, &element->src_pads, "src");
		}

		dump_print(&ctx->writer, "}\"];\n");
	}
}

static void dump_dot_edges(struct mpipe_dump_ctx *ctx)
{
	for (int i = 0; i < ctx->num_elements; i++) {
		struct mpipe_object *obj;

		SYS_DLIST_FOR_EACH_CONTAINER(&ctx->elements[i]->src_pads, obj, node) {
			struct mpipe_pad *src_pad = (struct mpipe_pad *)obj;
			/* Read once: a concurrent relink may clear it between uses */
			struct mpipe_pad *peer = src_pad->peer;
			int peer_index;

			/* An unlinked pad has no edge to draw; the node carries the mark */
			if (peer == NULL) {
				continue;
			}

			peer_index = dump_element_index(
				ctx, (struct mpipe_element *)peer->object.container);
			if (peer_index < 0) {
				continue;
			}

			dump_print(&ctx->writer, "  e%d:src%u -> e%d:sink%u [label=\"", i, obj->id,
				   peer_index, peer->object.id);
			dump_caps(&ctx->writer, &src_pad->caps);
			dump_print(&ctx->writer, "\"];\n");
		}
	}
}

int mpipe_dump_bin(struct mpipe_bin *bin, const struct mpipe_dump_sink *sink)
{
	struct mpipe_dump_ctx ctx = {
		.writer = {.sink = sink},
	};
	struct mpipe_element *self = (struct mpipe_element *)bin;

	__ASSERT_NO_MSG(bin != NULL);

	dump_index_bin(&ctx, bin);

	dump_print(&ctx.writer, "digraph mpipe_pipeline {\n");
	dump_print(&ctx.writer, "  rankdir=LR;\n");
	dump_print(&ctx.writer, "  node [shape=record, style=filled, fontname=\"sans\"];\n");
	dump_print(&ctx.writer, "  label=\"");
	dump_element_name(&ctx, self);
	dump_print(&ctx.writer, " %s%s\";\n", mpipe_dump_state_str(self->current_state),
		   ctx.truncated ? " (truncated)" : "");

	dump_dot_nodes(&ctx);
	dump_dot_edges(&ctx);

	dump_print(&ctx.writer, "}\n");

	/* The rendering ends on a newline, so this only catches a truncated line */
	dump_flush(&ctx.writer);

	return 0;
}
