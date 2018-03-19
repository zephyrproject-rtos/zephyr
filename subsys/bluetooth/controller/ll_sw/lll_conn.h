/* NOTE: Definitions used between ULL and LLL implementations */

struct lll_tx {
	u16_t handle;
	void *node;
};

struct node_tx {
	union {
		void        *next;
		void        *pool;
		memq_link_t *link;
	};

	u8_t pdu[];
};
