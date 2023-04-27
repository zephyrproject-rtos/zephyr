/*
 * Copyright 2023 Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/instrumentation/instrumentation_ringbuffer.h>

#define RING_BUFFER_SIZE CONFIG_INSTRUMENTATION_MODE_CALLGRAPH_TRACE_BUFFER_SIZE
static uint8_t ring_buffer[RING_BUFFER_SIZE];

static int head;
static int tail;
static int wrapped;

static size_t type_size_table[INSTR_EVENT_NUM];

__no_instrumentation__
size_t instr_rb_get_item_size(enum instr_event_types type)
{
	return type_size_table[type];
}

__no_instrumentation__
int instr_rb_increment_pos(int pos, int offset)
{
	pos += offset;
	pos %= RING_BUFFER_SIZE;

	return pos;
}

__no_instrumentation__
struct instr_record *instr_rb_put_item_claim(enum instr_event_types item_type)
{
	int item_size;
	int new_head;
	int tail_record_size;
	struct instr_record *tail_record;

	item_size = instr_rb_get_item_size(item_type);
	new_head = head + item_size;
	/* If there is space to put item, adjust tail accordingly. */
	if ((head + item_size) <= RING_BUFFER_SIZE) {
		if (head == tail && wrapped) {
			tail_record = (struct instr_record *)&ring_buffer[tail];
			tail_record_size = instr_rb_get_item_size(tail_record->header.type);
			tail = instr_rb_increment_pos(tail, tail_record_size);
		}

		while (head < tail && new_head > tail) {
			tail_record = (struct instr_record *)&ring_buffer[tail];
			tail_record_size = instr_rb_get_item_size(tail_record->header.type);
			tail = instr_rb_increment_pos(tail, tail_record_size);
		}

		return (struct instr_record *)&ring_buffer[head];
	}

	/* No space to put item, buffer will wrap. */
	return NULL;
}

__no_instrumentation__
int instr_rb_put_item_wrapping(struct instr_record *rec)
{
	int new_head;
	int next_tail;
	struct instr_record *tail_record;
	size_t first_chunk;
	size_t second_chunk;

	assert(rec != NULL);

	/* Adjust head first */
	new_head = instr_rb_increment_pos(head, instr_rb_get_item_size(rec->header.type));

	/*
	 * Handle case when tail points to an item which is wrapped itself,
	 * like, for instance, when buffer size = 41, head = 39, tail = 40, and
	 * a new item of size 20 will be inserted.
	 */
	tail_record = (struct instr_record *)&ring_buffer[tail];
	next_tail = instr_rb_increment_pos(tail, instr_rb_get_item_size(tail_record->header.type));
	if (next_tail < tail) {
		tail = next_tail;
	}

	/*
	 * Advance tail until there is space to put the new item, but not beyond
	 * the head.
	 */
	while (new_head > tail && tail != head) {
		tail_record = (struct instr_record *)&ring_buffer[tail];
		tail = instr_rb_increment_pos(tail,
						instr_rb_get_item_size(tail_record->header.type));
	}

	/* First, write the number of bytes available before buffer wraps. */
	first_chunk = RING_BUFFER_SIZE - head;
	memcpy(&ring_buffer[head], (uint8_t *)rec, first_chunk);

	/* Wrap buffer. */
	head = 0;

	/* Second, write remaining item bytes not written yet. */
	second_chunk = instr_rb_get_item_size(rec->header.type) - first_chunk;
	memcpy(&ring_buffer[head], (uint8_t *)rec + first_chunk, second_chunk);

	/* Adjust final head position. */
	head += second_chunk;

	wrapped = 1;

	return 0;
}

/* TODO(gromero): can be optimized to find out 'item_type' automatically? */
__no_instrumentation__
void instr_rb_put_item_finish(enum instr_event_types item_type)
{
	int item_size;

	item_size = instr_rb_get_item_size(item_type);

	head += item_size;
	head %= RING_BUFFER_SIZE;

	if (head == tail) {
		wrapped = 1;
	}
}

__no_instrumentation__
struct instr_record *instr_rb_get_item_claim(void)
{
	struct instr_record *tail_record;
	size_t item_size;

	if (!wrapped && tail == head) {
		/* Buffer is empty. */
		return NULL;
	}

	/* If type size is more than 1 byte it means that its value can wrap
	 * around the buffer boundaries, hence a corner case will exist, i.e.
	 * it will be necessary to "unwrap" type before we can reference to it.
	 * Since having more than 256 types is quite unlike, asserting type size
	 * is exactly 1 byte is enough for now. N.B.: that type member is always
	 * the first in the header struct.
	 */
	assert(sizeof(tail_record->header.type) == 1);

	tail_record = (struct instr_record *)&ring_buffer[tail];
	item_size = instr_rb_get_item_size(tail_record->header.type);

	if (tail + item_size <= RING_BUFFER_SIZE) {
		return tail_record;
	}

	/* Buffer wraps, so can't access the item directly. */
	return NULL;
}

__no_instrumentation__
int instr_rb_get_item_wrapping(struct instr_record *record)
{
	size_t item_size;
	size_t first_chunk;
	size_t second_chunk;
	struct instr_record *tail_record;

	/* See comment above in get_item_claim(). */
	assert(sizeof(record->header.type) == 1);

	if (!wrapped && tail == head) {
		/* Buffer is empty. */
		return -1;
	}

	/* Find the record type to find out the total item size. */
	tail_record = (struct instr_record *)&ring_buffer[tail];
	item_size = instr_rb_get_item_size(tail_record->header.type);

	first_chunk = RING_BUFFER_SIZE - tail;
	memcpy((uint8_t *)record, &ring_buffer[tail], first_chunk);

	second_chunk = item_size - first_chunk;
	memcpy((uint8_t *)(record + first_chunk), &ring_buffer[0], second_chunk);

	/* Pop item from tail. */
	tail += item_size;
	tail %= RING_BUFFER_SIZE;

	wrapped = 0;

	/* TODO(gromero): revisit API, maybe this can return no value. */
	return 0;
}

__no_instrumentation__
void instr_rb_get_item_finish(struct instr_record *record)
{
	int item_size;

	assert(record != NULL);

	/*
	 * TODO(gromero): add check for claim being called before this func ?
	 * i.e. it's forbidden to call this function before calling
	 * get_item_claim().
	 */

	/*
	 * TODO(gromero): can item size be deduced from item at 'head', so arg
	 * 'record' can be eliminated?
	 */

	item_size = instr_rb_get_item_size(record->header.type);
	tail += item_size;
	tail %= RING_BUFFER_SIZE;

	if (tail == head) {
		wrapped = 0;
	}
}

__no_instrumentation__
void instr_rb_init_type_size_table(void)
{
	size_t header_size;
	size_t record_size;

	header_size = sizeof(struct instr_header);
	header_size += sizeof(void *);
	header_size += sizeof(void *);
	header_size += sizeof(uint64_t);

	/* Entry/Exit record with context record */
	record_size = header_size + sizeof(struct instr_event_context);
	type_size_table[INSTR_EVENT_ENTRY] = record_size;
	type_size_table[INSTR_EVENT_EXIT] = record_size;

	/* Schedule in/out record. Same size as entry/exit recs. w/ context */
	record_size = header_size + sizeof(struct instr_event_context);
	type_size_table[INSTR_EVENT_SCHED_IN] = record_size;
	type_size_table[INSTR_EVENT_SCHED_OUT] = record_size;

	/* ... */
	/* Add more record sizes here */

	/*
	 * The minimum buffer size allows recording at least 1 record of the
	 * largest declared record. Assert this condition.
	 */
	for (size_t i = 0; i < INSTR_EVENT_NUM; i++) {
		assert(type_size_table[i] <= RING_BUFFER_SIZE);
	}
}

__no_instrumentation__
void instr_rb_init(void)
{
	head = 0;
	tail = 0;
	wrapped = 0;
	instr_rb_init_type_size_table();
}

__no_instrumentation__
struct instr_record *instr_rb_get_item(struct instr_record *record)
{
	assert(record != NULL);

	struct instr_record *rec_ptr;
	int ret;

	/* Try to get item directly. */
	rec_ptr = instr_rb_get_item_claim();
	if (rec_ptr != NULL) {
		memcpy(record, rec_ptr, instr_rb_get_item_size(rec_ptr->header.type));
		instr_rb_get_item_finish(rec_ptr);

		return record;
	}

	/* Try to get item wrapping. */
	ret = instr_rb_get_item_wrapping(record);
	if (ret != -1) {
		return record;
	}

	/* If first and second attempts failed, then buffer is empty. */
	return NULL;
}
