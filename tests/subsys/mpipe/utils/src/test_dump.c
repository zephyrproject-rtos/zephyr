/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>

#include <zephyr/mpipe/mpipe.h>
#include <zephyr/mpipe/mpipe_fake_src.h>
#include <zephyr/mpipe/utils/mpipe_dump.h>

/* Element IDs (values are arbitrary; only uniqueness within the pipeline matters) */
enum {
	DUMP_PIPE_ID,
	DUMP_SRC_ID,
	DUMP_TRANSFORM_ID,
	DUMP_SINK_ID,
};

/* 'R', 'G', 'B', 'P': the fourcc VIDEO_PIX_FMT_RGB565 is spelled with */
#define TEST_FOURCC_RGBP 0x50424752U

#define TEST_DUMP_BUF_SIZE 2048

struct test_dump_capture {
	char buf[TEST_DUMP_BUF_SIZE];
	size_t len;
};

struct test_dump_fixture {
	struct mpipe pipeline;
	struct mpipe_fake_src fake_src;
	struct mpipe_transform transform;
	struct mpipe_sink sink;
	struct test_dump_capture capture;
};

/* Collects the dump into a buffer, which is what lets a test read it */
static void test_dump_print(void *ctx, const char *str)
{
	struct test_dump_capture *cap = ctx;
	size_t room = sizeof(cap->buf) - 1U - cap->len;
	size_t len = MIN(strlen(str), room);

	memcpy(&cap->buf[cap->len], str, len);
	cap->len += len;
	cap->buf[cap->len] = '\0';
}

static int test_dump_pipeline(struct test_dump_fixture *fix)
{
	return mpipe_dump_bin((struct mpipe_bin *)&fix->pipeline, test_dump_print, &fix->capture);
}

static void test_dump_reset(struct test_dump_fixture *fix)
{
	fix->capture.len = 0;
	fix->capture.buf[0] = '\0';
}

static void *dump_suite_setup(void)
{
	static struct test_dump_fixture fixture;

	return &fixture;
}

static void dump_before(void *f)
{
	struct test_dump_fixture *fix = f;

	memset(fix, 0, sizeof(*fix));

	zassert_ok(mpipe_pipeline_init(&fix->pipeline, DUMP_PIPE_ID));
	zassert_ok(mpipe_fake_src_init(&fix->fake_src, DUMP_SRC_ID));
	zassert_ok(mpipe_transform_init(&fix->transform, DUMP_TRANSFORM_ID));
	zassert_ok(mpipe_sink_init(&fix->sink, DUMP_SINK_ID));

	zassert_ok(mpipe_bin_add((struct mpipe_bin *)&fix->pipeline,
				 (struct mpipe_element *)&fix->fake_src,
				 (struct mpipe_element *)&fix->transform,
				 (struct mpipe_element *)&fix->sink, NULL),
		   "Failed to add elements");
}

ZTEST_SUITE(test_dump, NULL, dump_suite_setup, dump_before, NULL, NULL);

static void dump_link_chain(struct test_dump_fixture *fix)
{
	zassert_ok(mpipe_element_link((struct mpipe_element *)&fix->fake_src,
				      (struct mpipe_element *)&fix->transform,
				      (struct mpipe_element *)&fix->sink, NULL),
		   "Failed to link elements");
}

/* An element is named after its init function, with the prefix and suffix dropped */
ZTEST_F(test_dump, test_dump_names_every_element)
{
	dump_link_chain(fixture);

	zassert_ok(test_dump_pipeline(fixture));

	zassert_not_null(strstr(fixture->capture.buf, "pipeline #0"), "Bin not named:\n%s",
			 fixture->capture.buf);
	zassert_not_null(strstr(fixture->capture.buf, "fake_src #1"), "Source not named:\n%s",
			 fixture->capture.buf);
	zassert_not_null(strstr(fixture->capture.buf, "transform #2"), "Transform not named:\n%s",
			 fixture->capture.buf);
	zassert_not_null(strstr(fixture->capture.buf, "sink #3"), "Sink not named:\n%s",
			 fixture->capture.buf);

	/*
	 * Neither the "mpipe_" prefix nor the "_init" suffix belongs in a name.
	 * Checked against the names themselves rather than the whole rendering,
	 * which legitimately carries "mpipe_" in the graph's own name.
	 */
	zassert_is_null(strstr(fixture->capture.buf, "mpipe_fake_src"), "Prefix not stripped:\n%s",
			fixture->capture.buf);
	zassert_is_null(strstr(fixture->capture.buf, "_init"), "Suffix not stripped:\n%s",
			fixture->capture.buf);
}

/* Every link becomes an edge between the two ports it actually joins */
ZTEST_F(test_dump, test_dump_draws_an_edge_per_link)
{
	dump_link_chain(fixture);

	zassert_ok(test_dump_pipeline(fixture));

	zassert_not_null(strstr(fixture->capture.buf, "e0:src0 -> e1:sink0"),
			 "Edge from the source is missing or lands on the wrong pad:\n%s",
			 fixture->capture.buf);
	zassert_not_null(strstr(fixture->capture.buf, "e1:src1 -> e2:sink0"),
			 "Edge to the sink is missing or lands on the wrong pad:\n%s",
			 fixture->capture.buf);

	/* Every pad is linked, so nothing is marked */
	zassert_is_null(strstr(fixture->capture.buf, "#cc0000"),
			"A fully linked chain marked an element as unlinked:\n%s",
			fixture->capture.buf);
}

/* An element with a pad left over is marked, which is what a broken graph looks like */
ZTEST_F(test_dump, test_dump_marks_an_element_with_an_unlinked_pad)
{
	/* Deliberately left unlinked */
	zassert_ok(test_dump_pipeline(fixture));

	zassert_not_null(strstr(fixture->capture.buf, "#cc0000"),
			 "An unlinked element was not marked:\n%s", fixture->capture.buf);
	zassert_is_null(strstr(fixture->capture.buf, " -> "),
			"Nothing is linked, so no edge should be drawn:\n%s", fixture->capture.buf);
}

/* The capability negotiated on each pad is what the dump exists to show */
ZTEST_F(test_dump, test_dump_shows_negotiated_caps)
{
	dump_link_chain(fixture);

	zassert_ok(test_dump_pipeline(fixture));

	/* Nothing has negotiated yet, so every pad still constrains nothing */
	zassert_not_null(strstr(fixture->capture.buf, "<any>"),
			 "An un-negotiated pad should render as <any>:\n%s", fixture->capture.buf);

	zassert_ok(mpipe_element_set_state((struct mpipe_element *)&fixture->pipeline,
					   MPIPE_STATE_PAUSED),
		   "Pipeline failed to reach PAUSED");

	test_dump_reset(fixture);
	zassert_ok(test_dump_pipeline(fixture));

	/* Every element, not just the bin, has to report the state it reached */
	zassert_not_null(strstr(fixture->capture.buf, "pipeline #0 PAUSED"),
			 "Bin state not shown:\n%s", fixture->capture.buf);
	zassert_not_null(strstr(fixture->capture.buf, "fake_src #1\\nPAUSED"),
			 "Source still reports its initial state:\n%s", fixture->capture.buf);
	zassert_is_null(strstr(fixture->capture.buf, "READY"),
			"An element was left behind at READY:\n%s", fixture->capture.buf);

	zassert_ok(mpipe_element_set_state((struct mpipe_element *)&fixture->pipeline,
					   MPIPE_STATE_READY),
		   "Pipeline failed to return to READY");
}

/* The rendering has to be a graph dot(1) can actually lay out */
ZTEST_F(test_dump, test_dump_is_a_well_formed_digraph)
{
	dump_link_chain(fixture);

	zassert_ok(test_dump_pipeline(fixture));

	zassert_equal(strncmp(fixture->capture.buf, "digraph mpipe_pipeline {", 21), 0,
		      "DOT does not open a digraph:\n%s", fixture->capture.buf);
	zassert_not_null(strstr(fixture->capture.buf, "rankdir=LR;"), "DOT lacks its layout:\n%s",
			 fixture->capture.buf);

	/* Every brace opened has to close, or dot(1) rejects the whole graph */
	int depth = 0;

	for (size_t i = 0; i < fixture->capture.len; i++) {
		if (fixture->capture.buf[i] == '{') {
			depth++;
		} else if (fixture->capture.buf[i] == '}') {
			depth--;
		}

		zassert_true(depth >= 0, "DOT closes a brace it never opened:\n%s",
			     fixture->capture.buf);
	}

	zassert_equal(depth, 0, "DOT leaves %d brace(s) open:\n%s", depth, fixture->capture.buf);
}

/* A capability renders its media type, its field names and a fourcc as characters */
ZTEST_F(test_dump, test_dump_caps_renders_fields_by_name)
{
	struct mpipe_structure caps;

	zassert_ok(mpipe_structure_init_fields(
		&caps, MPIPE_MEDIA_VIDEO, MPIPE_CAPS_PIXEL_FORMAT, MPIPE_TYPE_UINT,
		TEST_FOURCC_RGBP, MPIPE_CAPS_IMAGE_WIDTH, MPIPE_TYPE_UINT, 640,
		MPIPE_CAPS_IMAGE_HEIGHT, MPIPE_TYPE_UINT_RANGE, 16, 480, 2, MPIPE_CAPS_END));

	zassert_ok(mpipe_dump_caps(&caps, test_dump_print, &fixture->capture));

	zassert_equal(strcmp(fixture->capture.buf,
			     "<video, format=RGBP, width=640, height=[16, 480, 2]>"),
		      0, "Unexpected rendering: '%s'", fixture->capture.buf);
}

/* An audio capability uses the same table, so its field names have to be there too */
ZTEST_F(test_dump, test_dump_caps_renders_audio_fields)
{
	struct mpipe_structure caps;

	zassert_ok(mpipe_structure_init_fields(&caps, MPIPE_MEDIA_AUDIO_PCM, MPIPE_CAPS_SAMPLE_RATE,
					       MPIPE_TYPE_UINT, 48000, MPIPE_CAPS_NUM_OF_CHANNEL,
					       MPIPE_TYPE_UINT, 2, MPIPE_CAPS_INTERLEAVED,
					       MPIPE_TYPE_BOOLEAN, true, MPIPE_CAPS_END));

	zassert_ok(mpipe_dump_caps(&caps, test_dump_print, &fixture->capture));

	zassert_equal(strcmp(fixture->capture.buf,
			     "<audio/pcm, rate=48000, channels=2, interleaved=true>"),
		      0, "Unexpected rendering: '%s'", fixture->capture.buf);
}

/*
 * Constraining nothing because it intersects with everything is not the same as
 * constraining nothing because it intersects with nothing.
 */
ZTEST_F(test_dump, test_dump_caps_tells_any_from_empty)
{
	struct mpipe_structure caps;

	zassert_ok(mpipe_structure_init_any(&caps));
	zassert_ok(mpipe_dump_caps(&caps, test_dump_print, &fixture->capture));
	zassert_equal(strcmp(fixture->capture.buf, "<any>"), 0, "Unexpected rendering: '%s'",
		      fixture->capture.buf);

	test_dump_reset(fixture);

	zassert_ok(mpipe_structure_init(&caps, MPIPE_MEDIA_VIDEO));
	zassert_ok(mpipe_dump_caps(&caps, test_dump_print, &fixture->capture));
	zassert_equal(strcmp(fixture->capture.buf, "<empty>"), 0, "Unexpected rendering: '%s'",
		      fixture->capture.buf);
}

/* A dump with no callback goes to the console, which is how the player renders one */
ZTEST_F(test_dump, test_dump_to_the_console_is_accepted)
{
	dump_link_chain(fixture);

	zassert_ok(mpipe_dump_bin((struct mpipe_bin *)&fixture->pipeline, NULL, NULL),
		   "A dump to the console should be accepted");
}
