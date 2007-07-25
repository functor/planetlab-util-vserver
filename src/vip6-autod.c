#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>

#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
/*XXX #include <linux/ipv6.h>*/
struct in6_ifreq {
	struct in6_addr	ifr6_addr;
	__u32		ifr6_prefixlen;
	int		ifr6_ifindex;
};

typedef unsigned int nid_t;
typedef unsigned int xid_t;
#include <vserver.h>

#define HAS_ADDRESS	0x01
#define HAS_PREFIX	0x02
struct prefix_list {
	struct prefix_list *prev;
	struct prefix_list *next;
	uint32_t mask;
	int ifindex;
	struct {
		struct in6_addr addr;
		int prefix_len;
		time_t valid_until;
	} prefix;
	struct {
		struct in6_addr addr;
		int prefix_len;
		time_t valid_until;
	} address;
};

/* from linux/include/net/ipv6.h */
static inline int ipv6_prefix_equal(struct in6_addr *prefix,
				    struct in6_addr *addr, int prefixlen)
{
	uint32_t *a1 = prefix->s6_addr32, *a2 = addr->s6_addr32;
	unsigned pdw, pbi;

	/* check complete u32 in prefix */
	pdw = prefixlen >> 5;
	if (pdw && memcmp(a1, a2, pdw << 2))
		return 0;

	/* check incomplete u32 in prefix */
	pbi = prefixlen & 0x1f;
	if (pbi && ((a1[pdw] ^ a2[pdw]) & htonl((0xffffffff) << (32 - pbi))))
		return 0;

	return 1;
}

static int add_address_to_interface(int ifindex, struct in6_addr *address, int prefix)
{
	struct in6_ifreq ireq;
	int sock;

	ireq.ifr6_ifindex = ifindex;
	ireq.ifr6_prefixlen = prefix;
	memcpy(&ireq.ifr6_addr, address, sizeof(*address));

	/* XXX should use netlink */
	sock = socket(PF_INET6, SOCK_DGRAM, IPPROTO_IP);
	if (sock == -1)
		return -1;

	if (ioctl(sock, SIOCSIFADDR, &ireq) == -1 && errno != EEXIST) {
		close(sock);
		return -1;
	}

	close(sock);
	return 0;
}

static void do_slices_autoconf(struct prefix_list *head)
{
	DIR *dp;
	struct dirent *de;
	nid_t nid;
	struct vc_net_nx addr;
	struct prefix_list *i, *expired = NULL;

	if ((dp = opendir("/proc/virtnet")) == NULL)
		return;
	while ((de = readdir(dp)) != NULL) {
		if (!isdigit(de->d_name[0]))
			continue;

		nid = strtoul(de->d_name, NULL, 10);
		addr.type = vcNET_IPV6;
		addr.count = -1;
		if (vc_net_remove(nid, &addr) == -1)
			continue;

		for (i = head->next; i;) {
			/* expired */
			if (i->mask & HAS_PREFIX && i->prefix.valid_until < time(NULL)) {
				struct prefix_list *tmp;
				if (i->next)
					i->next->prev = i->prev;
				if (i->prev)
					i->prev->next = i->next;
				
				tmp = i->next;
				free(i);
				i = tmp;
				continue;
			}
			if (i->mask != (HAS_ADDRESS|HAS_PREFIX))
				goto next;

			addr.count = 1;
			addr.mask[0] = i->prefix.prefix_len;
			memcpy(addr.ip, &i->address.addr, sizeof(struct in6_addr));
			addr.ip[2] = htonl((ntohl(addr.ip[2]) & 0xffffff00) | ((nid & 0x7f00) >> 7));
			addr.ip[3] = htonl((ntohl(addr.ip[3]) & 0x00ffffff) | ((nid & 0xff) << 25));
			if (vc_net_add(nid, &addr) == -1) {
				perror("vc_net_add");
				exit(1);
			}
			if (add_address_to_interface(i->ifindex, (struct in6_addr *) addr.ip, i->prefix.prefix_len) == -1) {
				perror("add_address_to_interface");
				exit(1);
			}
next:
			i = i->next;
		}
	}
	closedir(dp);
}

static int add_prefix(struct prefix_list *head, struct prefixmsg *msg,
		      struct in6_addr *prefix, struct prefix_cacheinfo *cache)
{
	struct prefix_list *i = head;
	if (!msg || !prefix || !cache)
		return -1;

	do {
		if (i->next != NULL)
			i = i->next;
		if (ipv6_prefix_equal(prefix, &i->prefix.addr, msg->prefix_len) ||
		    ipv6_prefix_equal(prefix, &i->address.addr, msg->prefix_len)) {
			i->mask |= HAS_PREFIX;
			i->ifindex = msg->prefix_ifindex;
			memcpy(&i->prefix.addr, prefix, sizeof(*prefix));
			i->prefix.prefix_len = msg->prefix_len;
			i->prefix.valid_until = time(NULL) + cache->preferred_time;
			return 0;
		}
	} while (i->next);

	i->next = calloc(1, sizeof(*i));
	if (!i->next)
		return -1;
	i->next->prev = i;
	i = i->next;
	i->mask = HAS_PREFIX;
	memcpy(&i->prefix.addr, prefix, sizeof(*prefix));
	i->prefix.prefix_len = msg->prefix_len;
	i->prefix.valid_until = time(NULL) + cache->preferred_time;

	return 1;
}

static inline int add_address(struct prefix_list *head, struct ifaddrmsg *msg,
			      struct in6_addr *address, struct ifa_cacheinfo *cache)
{
	struct prefix_list *i = head;
	if (!msg || !address || !cache)
		return -1;

	if (address->s6_addr[11] != 0xFF || address->s6_addr[12] != 0xFE)
		return 0;

	do {
		if (i->next != NULL)
			i = i->next;
		if (ipv6_prefix_equal(address, &i->prefix.addr, msg->ifa_prefixlen) ||
		    ipv6_prefix_equal(address, &i->address.addr, msg->ifa_prefixlen)) {
			i->mask |= HAS_ADDRESS;
			memcpy(&i->address.addr, address, sizeof(*address));
			i->address.prefix_len = msg->ifa_prefixlen;
			i->address.valid_until = time(NULL) + cache->ifa_prefered;
			return 0;
		}
	} while (i->next);

	i->next = calloc(1, sizeof(*i));
	if (!i->next)
		return -1;
	i->next->prev = i;
	i = i->next;
	i->mask = HAS_ADDRESS;
	memcpy(&i->address.addr, address, sizeof(*address));
	i->address.prefix_len = msg->ifa_prefixlen;
	i->address.valid_until = time(NULL) + cache->ifa_prefered;

	return 1;
}

int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_nl sa;
	struct prefix_list head = { .prev = NULL, .next = NULL };

	sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
	if (sock == -1) {
		perror("socket()");
		exit(1);
	}
	sa.nl_family = AF_NETLINK;
	sa.nl_groups = RTMGRP_IPV6_PREFIX|RTMGRP_IPV6_IFADDR;
	if (bind(sock, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
		perror("bind()");
		exit(1);
	}

	while (1) {
		char buf[4000], *payload;
		struct nlmsghdr *nlh;
		ssize_t len, this_len;
		socklen_t socklen = sizeof(sa);

		if ((len = recvfrom(sock, buf, sizeof(buf), 0, 
				    (struct sockaddr *) &sa, &socklen)) <= 0)
			break;
		for (nlh = (struct nlmsghdr *) buf; NLMSG_OK(nlh, len); nlh = NLMSG_NEXT(nlh, len)) {
			struct nlattr *nla;

			if (nlh->nlmsg_type == NLMSG_DONE)
				break;
			else if (nlh->nlmsg_type == NLMSG_ERROR)
				break;

			this_len = NLMSG_ALIGN((nlh)->nlmsg_len) - NLMSG_LENGTH(0);
			payload = NLMSG_DATA(nlh);
			if (sa.nl_groups == RTMGRP_IPV6_PREFIX) {
				struct prefixmsg *prefixmsg;
				struct in6_addr *prefix;
				struct prefix_cacheinfo *cacheinfo;

				prefixmsg = (struct prefixmsg *) payload;
				prefix = NULL;
				cacheinfo = NULL;
				for (nla = (struct nlattr *)(payload + sizeof(*prefixmsg)); nla < payload + this_len; nla = (char *) nla + nla->nla_len) {
					if (nla->nla_type == PREFIX_ADDRESS)
						prefix = (struct in6_addr *)(((char *) nla) + sizeof(*nla));
					else if (nla->nla_type == PREFIX_CACHEINFO)
						cacheinfo = (struct prefix_cacheinfo *)(((char *) nla) + sizeof(*nla));
				}
				if (add_prefix(&head, prefixmsg, prefix, cacheinfo) == -1) {
					printf("Adding prefix failed!\n");
				}
			}
			else if (sa.nl_groups == RTMGRP_IPV6_IFADDR) {
				struct ifaddrmsg *ifaddrmsg;
				struct in6_addr *address;
				struct ifa_cacheinfo *cacheinfo;

				ifaddrmsg = (struct ifaddrmsg *) payload;
				address = NULL;
				cacheinfo = NULL;
				for (nla = (struct nlattr *)(payload + sizeof(*ifaddrmsg)); nla < payload + this_len; nla = (char *) nla + nla->nla_len) {
					if (nla->nla_type == IFA_ADDRESS)
						address = (struct in6_addr *)(((char *) nla) + sizeof(*nla));
					else if (nla->nla_type == IFA_CACHEINFO)
						cacheinfo = (struct ifa_cacheinfo *)(((char *) nla) + sizeof(*nla));
				}
				if (add_address(&head, ifaddrmsg, address, cacheinfo) == -1) {
					printf("Adding address failed!\n");
				}
			}
		}
		do_slices_autoconf(&head);
	}

	close(sock);
	return 0;
}
