/*
 * Copyright (c) 2021 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(npf_base, CONFIG_NET_PKT_FILTER_LOG_LEVEL);

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt_filter.h>
#include <zephyr/spinlock.h>

/*
 * Our actual rule lists for supported test points
 */

struct npf_rule_list npf_send_rules = {
	.rule_head = SYS_SLIST_STATIC_INIT(&send_rules.rule_head),
	.lock = { },
};

struct npf_rule_list npf_recv_rules = {
	.rule_head = SYS_SLIST_STATIC_INIT(&recv_rules.rule_head),
	.lock = { },
};

#ifdef CONFIG_NET_PKT_FILTER_LOCAL_IN_HOOK
struct npf_rule_list npf_local_in_recv_rules = {
	.rule_head = SYS_SLIST_STATIC_INIT(&local_in_recv_rules.rule_head),
	.lock = { },
};
#endif /* CONFIG_NET_PKT_FILTER_LOCAL_IN_HOOK */

#ifdef CONFIG_NET_PKT_FILTER_IPV4_HOOK
struct npf_rule_list npf_ipv4_recv_rules = {
	.rule_head = SYS_SLIST_STATIC_INIT(&ipv4_recv_rules.rule_head),
	.lock = { },
};
#endif /* CONFIG_NET_PKT_FILTER_IPV4_HOOK */

#ifdef CONFIG_NET_PKT_FILTER_IPV6_HOOK
struct npf_rule_list npf_ipv6_recv_rules = {
	.rule_head = SYS_SLIST_STATIC_INIT(&ipv6_recv_rules.rule_head),
	.lock = { },
};
#endif /* CONFIG_NET_PKT_FILTER_IPV6_HOOK */

/*
 * Helper function
 */
static struct npf_rule_list *get_ip_rules(uint8_t pf)
{
	switch (pf) {
	case PF_INET:
#ifdef CONFIG_NET_PKT_FILTER_IPV4_HOOK
		return &npf_ipv4_recv_rules;
#endif
		break;
	case PF_INET6:
#ifdef CONFIG_NET_PKT_FILTER_IPV6_HOOK
		return &npf_ipv6_recv_rules;
#endif
		break;
	default:
		return NULL;
	}

	return NULL;
}

/*
 * Rule application
 */

/*
 * All tests must be true to return true.
 * If no tests then it is true.
 */
static bool apply_tests(struct npf_rule *rule, struct net_pkt *pkt)
{
	struct npf_test *test;
	unsigned int i;
	bool result;

	for (i = 0; i < rule->nb_tests; i++) {
		test = rule->tests[i];
		result = test->fn(test, pkt);
		NET_DBG("test %p result %d", test, result);
		if (result == false) {
			return false;
		}
	}

	return true;
}

/*
 * We return the specified result for the first rule whose tests are all true.
 */
static enum net_verdict evaluate(sys_slist_t *rule_head, struct net_pkt *pkt)
{
	struct npf_rule *rule;

	NET_DBG("rule_head %p on pkt %p", rule_head, pkt);

	if (sys_slist_is_empty(rule_head)) {
		NET_DBG("no rules");
		return NET_OK;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(rule_head, rule, node) {
		if (apply_tests(rule, pkt) == true) {
			return rule->result;
		}
	}

	NET_DBG("no matching rules from rule_head %p", rule_head);
	return NET_DROP;
}

static enum net_verdict lock_evaluate(struct npf_rule_list *rules, struct net_pkt *pkt)
{
	k_spinlock_key_t key = k_spin_lock(&rules->lock);
	enum net_verdict result = evaluate(&rules->rule_head, pkt);

	k_spin_unlock(&rules->lock, key);
	return result;
}

bool net_pkt_filter_send_ok(struct net_pkt *pkt)
{
	enum net_verdict result = lock_evaluate(&npf_send_rules, pkt);

	return result == NET_OK;
}

bool net_pkt_filter_recv_ok(struct net_pkt *pkt)
{
	enum net_verdict result = lock_evaluate(&npf_recv_rules, pkt);

	return result == NET_OK;
}

#ifdef CONFIG_NET_PKT_FILTER_LOCAL_IN_HOOK
bool net_pkt_filter_local_in_recv_ok(struct net_pkt *pkt)
{
	enum net_verdict result = lock_evaluate(&npf_local_in_recv_rules, pkt);

	return result == NET_OK;
}
#endif /* CONFIG_NET_PKT_FILTER_LOCAL_IN_HOOK */

#if defined(CONFIG_NET_PKT_FILTER_IPV4_HOOK) || defined(CONFIG_NET_PKT_FILTER_IPV6_HOOK)
bool net_pkt_filter_ip_recv_ok(struct net_pkt *pkt)
{
	struct npf_rule_list *rules = get_ip_rules(net_pkt_family(pkt));

	if (!rules) {
		NET_DBG("no rules");
		return true;
	}

	enum net_verdict result = lock_evaluate(rules, pkt);

	return result == NET_OK;
}
#endif /* CONFIG_NET_PKT_FILTER_IPV4_HOOK || CONFIG_NET_PKT_FILTER_IPV6_HOOK */

/*
 * Rule management
 */

void npf_insert_rule(struct npf_rule_list *rules, struct npf_rule *rule)
{
	k_spinlock_key_t key = k_spin_lock(&rules->lock);

	NET_DBG("inserting rule %p into %p", rule, rules);
	sys_slist_prepend(&rules->rule_head, &rule->node);

	k_spin_unlock(&rules->lock, key);
}

void npf_append_rule(struct npf_rule_list *rules, struct npf_rule *rule)
{
	__ASSERT(sys_slist_peek_tail(&rules->rule_head) != &npf_default_ok.node, "");
	__ASSERT(sys_slist_peek_tail(&rules->rule_head) != &npf_default_drop.node, "");

	k_spinlock_key_t key = k_spin_lock(&rules->lock);

	NET_DBG("appending rule %p into %p", rule, rules);
	sys_slist_append(&rules->rule_head, &rule->node);

	k_spin_unlock(&rules->lock, key);
}

bool npf_remove_rule(struct npf_rule_list *rules, struct npf_rule *rule)
{
	k_spinlock_key_t key = k_spin_lock(&rules->lock);
	bool result = sys_slist_find_and_remove(&rules->rule_head, &rule->node);

	k_spin_unlock(&rules->lock, key);
	NET_DBG("removing rule %p from %p: %d", rule, rules, result);
	return result;
}

bool npf_remove_all_rules(struct npf_rule_list *rules)
{
	k_spinlock_key_t key = k_spin_lock(&rules->lock);
	bool result = !sys_slist_is_empty(&rules->rule_head);

	if (result) {
		sys_slist_init(&rules->rule_head);
		NET_DBG("removing all rules from %p", rules);
	}

	k_spin_unlock(&rules->lock, key);
	return result;
}

/*
 * Default rule list terminations.
 */

struct npf_rule npf_default_ok = {
	.result = NET_OK,
};

struct npf_rule npf_default_drop = {
	.result = NET_DROP,
};

/*
 * Some simple generic conditions
 */

bool npf_iface_match(struct npf_test *test, struct net_pkt *pkt)
{
	struct npf_test_iface *test_iface =
			CONTAINER_OF(test, struct npf_test_iface, test);

	return test_iface->iface == net_pkt_iface(pkt);
}

bool npf_iface_unmatch(struct npf_test *test, struct net_pkt *pkt)
{
	return !npf_iface_match(test, pkt);
}

bool npf_orig_iface_match(struct npf_test *test, struct net_pkt *pkt)
{
	struct npf_test_iface *test_iface =
			CONTAINER_OF(test, struct npf_test_iface, test);

	return test_iface->iface == net_pkt_orig_iface(pkt);
}

bool npf_orig_iface_unmatch(struct npf_test *test, struct net_pkt *pkt)
{
	return !npf_orig_iface_match(test, pkt);
}

bool npf_size_inbounds(struct npf_test *test, struct net_pkt *pkt)
{
	struct npf_test_size_bounds *bounds =
			CONTAINER_OF(test, struct npf_test_size_bounds, test);
	size_t pkt_size = net_pkt_get_len(pkt);

	return pkt_size >= bounds->min && pkt_size <= bounds->max;
}

bool npf_ip_src_addr_match(struct npf_test *test, struct net_pkt *pkt)
{
	struct npf_test_ip *test_ip =
			CONTAINER_OF(test, struct npf_test_ip, test);
	uint8_t pkt_family = net_pkt_family(pkt);

	for (uint32_t ip_it = 0; ip_it < test_ip->ipaddr_num; ip_it++) {
		if (IS_ENABLED(CONFIG_NET_IPV4) && pkt_family == AF_INET) {
			struct in_addr *addr = (struct in_addr *)NET_IPV4_HDR(pkt)->src;

			if (net_ipv4_addr_cmp(addr, &((struct in_addr *)test_ip->ipaddr)[ip_it])) {
				return true;
			}
		} else if (IS_ENABLED(CONFIG_NET_IPV6) && pkt_family == AF_INET6) {
			struct in6_addr *addr = (struct in6_addr *)NET_IPV6_HDR(pkt)->src;

			if (net_ipv6_addr_cmp(addr, &((struct in6_addr *)test_ip->ipaddr)[ip_it])) {
				return true;
			}
		}
	}
	return false;
}

bool npf_ip_src_addr_unmatch(struct npf_test *test, struct net_pkt *pkt)
{
	return !npf_ip_src_addr_match(test, pkt);
}
