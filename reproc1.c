// autogenerated by syzkaller (http://github.com/google/syzkaller)

#define _GNU_SOURCE

#include <sys/syscall.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_tun.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <net/if_arp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

__attribute__((noreturn)) static void doexit(int status)
{
		volatile unsigned i;
		syscall(__NR_exit_group, status);
		for (i = 0;; i++) {
		}
}
#define NORETURN __attribute__((noreturn))

#include <stdint.h>
#include <string.h>

const int kFailStatus = 67;
const int kRetryStatus = 69;

NORETURN static void fail(const char* msg, ...)
{
		int e = errno;
		fflush(stdout);
		va_list args;
		va_start(args, msg);
		vfprintf(stderr, msg, args);
		va_end(args);
		fprintf(stderr, " (errno %d)\n", e);
		doexit((e == ENOMEM || e == EAGAIN) ? kRetryStatus : kFailStatus);
}

#define BITMASK_LEN(type,bf_len) (type)((1ull << (bf_len)) - 1)

#define BITMASK_LEN_OFF(type,bf_off,bf_len) (type)(BITMASK_LEN(type, (bf_len)) << (bf_off))

#define STORE_BY_BITMASK(type,addr,val,bf_off,bf_len) if ((bf_off) == 0 && (bf_len) == 0) { *(type*)(addr) = (type)(val); } else { type new_val = *(type*)(addr); new_val &= ~BITMASK_LEN_OFF(type, (bf_off), (bf_len)); new_val |= ((type)(val)&BITMASK_LEN(type, (bf_len))) << (bf_off); *(type*)(addr) = new_val; }

struct csum_inet {
		uint32_t acc;
};

static void csum_inet_init(struct csum_inet* csum)
{
		csum->acc = 0;
}

static void csum_inet_update(struct csum_inet* csum, const uint8_t* data, size_t length)
{
		if (length == 0)
				return;

		size_t i;
		for (i = 0; i < length - 1; i += 2)
				csum->acc += *(uint16_t*)&data[i];

		if (length & 1)
				csum->acc += (uint16_t)data[length - 1];

		while (csum->acc > 0xffff)
				csum->acc = (csum->acc & 0xffff) + (csum->acc >> 16);
}

static uint16_t csum_inet_digest(struct csum_inet* csum)
{
		return ~csum->acc;
}

static void vsnprintf_check(char* str, size_t size, const char* format, va_list args)
{
		int rv;

		rv = vsnprintf(str, size, format, args);
		if (rv < 0)
				fail("tun: snprintf failed");
		if ((size_t)rv >= size)
				fail("tun: string '%s...' doesn't fit into buffer", str);
}

static void snprintf_check(char* str, size_t size, const char* format, ...)
{
		va_list args;

		va_start(args, format);
		vsnprintf_check(str, size, format, args);
		va_end(args);
}

#define COMMAND_MAX_LEN 128

static void execute_command(const char* format, ...)
{
		va_list args;
		char command[COMMAND_MAX_LEN];
		int rv;

		va_start(args, format);

		vsnprintf_check(command, sizeof(command), format, args);
		rv = system(command);
		if (rv != 0)
				fail("tun: command \"%s\" failed with code %d", &command[0], rv);

		va_end(args);
}

static int tunfd = -1;
static int tun_frags_enabled;

#define SYZ_TUN_MAX_PACKET_SIZE 1000

#define MAX_PIDS 32
#define ADDR_MAX_LEN 32

#define LOCAL_MAC "aa:aa:aa:aa:aa:%02hx"
#define REMOTE_MAC "bb:bb:bb:bb:bb:%02hx"

#define LOCAL_IPV4 "172.20.%d.170"
#define REMOTE_IPV4 "172.20.%d.187"

#define LOCAL_IPV6 "fe80::%02hxaa"
#define REMOTE_IPV6 "fe80::%02hxbb"

#define IFF_NAPI 0x0010
#define IFF_NAPI_FRAGS 0x0020

static void initialize_tun(uint64_t pid)
{
		if (pid >= MAX_PIDS)
				fail("tun: no more than %d executors", MAX_PIDS);
		int id = pid;

		tunfd = open("/dev/net/tun", O_RDWR | O_NONBLOCK);
		if (tunfd == -1)
				fail("tun: can't open /dev/net/tun");

		char iface[IFNAMSIZ];
		snprintf_check(iface, sizeof(iface), "syz%d", id);

		struct ifreq ifr;
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, iface, IFNAMSIZ);
		ifr.ifr_flags = IFF_TAP | IFF_NO_PI | IFF_NAPI | IFF_NAPI_FRAGS;
		if (ioctl(tunfd, TUNSETIFF, (void*)&ifr) < 0) {
				ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
				if (ioctl(tunfd, TUNSETIFF, (void*)&ifr) < 0)
						fail("tun: ioctl(TUNSETIFF) failed");
		}
		if (ioctl(tunfd, TUNGETIFF, (void*)&ifr) < 0)
				fail("tun: ioctl(TUNGETIFF) failed");
		tun_frags_enabled = (ifr.ifr_flags & IFF_NAPI_FRAGS) != 0;

		char local_mac[ADDR_MAX_LEN];
		snprintf_check(local_mac, sizeof(local_mac), LOCAL_MAC, id);
		char remote_mac[ADDR_MAX_LEN];
		snprintf_check(remote_mac, sizeof(remote_mac), REMOTE_MAC, id);

		char local_ipv4[ADDR_MAX_LEN];
		snprintf_check(local_ipv4, sizeof(local_ipv4), LOCAL_IPV4, id);
		char remote_ipv4[ADDR_MAX_LEN];
		snprintf_check(remote_ipv4, sizeof(remote_ipv4), REMOTE_IPV4, id);

		char local_ipv6[ADDR_MAX_LEN];
		snprintf_check(local_ipv6, sizeof(local_ipv6), LOCAL_IPV6, id);
		char remote_ipv6[ADDR_MAX_LEN];
		snprintf_check(remote_ipv6, sizeof(remote_ipv6), REMOTE_IPV6, id);

		execute_command("sysctl -w net.ipv6.conf.%s.accept_dad=0", iface);

		execute_command("sysctl -w net.ipv6.conf.%s.router_solicitations=0", iface);

		execute_command("ip link set dev %s address %s", iface, local_mac);
		execute_command("ip addr add %s/24 dev %s", local_ipv4, iface);
		execute_command("ip -6 addr add %s/120 dev %s", local_ipv6, iface);
		execute_command("ip neigh add %s lladdr %s dev %s nud permanent", remote_ipv4, remote_mac, iface);
		execute_command("ip -6 neigh add %s lladdr %s dev %s nud permanent", remote_ipv6, remote_mac, iface);
		execute_command("ip link set dev %s up", iface);
}

static void setup_tun(uint64_t pid, bool enable_tun)
{
		if (enable_tun)
				initialize_tun(pid);
}

#define MAX_FRAGS 4
struct vnet_fragmentation {
		uint32_t full;
		uint32_t count;
		uint32_t frags[MAX_FRAGS];
};

static uintptr_t syz_emit_ethernet(uintptr_t a0, uintptr_t a1, uintptr_t a2)
{
		if (tunfd < 0)
				return (uintptr_t)-1;

		uint32_t length = a0;
		char* data = (char*)a1;

		struct vnet_fragmentation* frags = (struct vnet_fragmentation*)a2;
		struct iovec vecs[MAX_FRAGS + 1];
		uint32_t nfrags = 0;
		if (!tun_frags_enabled || frags == NULL) {
				vecs[nfrags].iov_base = data;
				vecs[nfrags].iov_len = length;
				nfrags++;
		} else {
				bool full = true;
				uint32_t i, count = 0;
				full = frags->full;
				count = frags->count;
				if (count > MAX_FRAGS)
						count = MAX_FRAGS;
				for (i = 0; i < count && length != 0; i++) {
						uint32_t size = 0;
						size = frags->frags[i];
						if (size > length)
								size = length;
						vecs[nfrags].iov_base = data;
						vecs[nfrags].iov_len = size;
						nfrags++;
						data += size;
						length -= size;
				}
				if (length != 0 && (full || nfrags == 0)) {
						vecs[nfrags].iov_base = data;
						vecs[nfrags].iov_len = length;
						nfrags++;
				}
		}
		return writev(tunfd, vecs, nfrags);
}

static void test();

void loop()
{
		while (1) {
				test();
		}
}

#ifndef __NR_mmap
#define __NR_mmap 222
#endif
#ifndef __NR_socket
#define __NR_socket 198
#endif
#ifndef __NR_bind
#define __NR_bind 200
#endif
#ifndef __NR_sendto
#define __NR_sendto 206
#endif

long r[80];
void test()
{
		memset(r, -1, sizeof(r));
		r[0] = syscall(__NR_mmap, 0x20000000ul, 0x26000ul, 0x3ul, 0x32ul, 0xfffffffffffffffful, 0x0ul);
		r[1] = syscall(__NR_socket, 0x2ul, 0x1ul, 0x0ul);
		*(uint16_t*)0x2000bff0 = (uint16_t)0x2;
		*(uint16_t*)0x2000bff2 = (uint16_t)0x204e;
		*(uint8_t*)0x2000bff4 = (uint8_t)0xac;
		*(uint8_t*)0x2000bff5 = (uint8_t)0x14;
		*(uint8_t*)0x2000bff6 = (uint8_t)0x0;
		*(uint8_t*)0x2000bff7 = (uint8_t)0xaa;
		*(uint8_t*)0x2000bff8 = (uint8_t)0x0;
		*(uint8_t*)0x2000bff9 = (uint8_t)0x0;
		*(uint8_t*)0x2000bffa = (uint8_t)0x0;
		*(uint8_t*)0x2000bffb = (uint8_t)0x0;
		*(uint8_t*)0x2000bffc = (uint8_t)0x0;
		*(uint8_t*)0x2000bffd = (uint8_t)0x0;
		*(uint8_t*)0x2000bffe = (uint8_t)0x0;
		*(uint8_t*)0x2000bfff = (uint8_t)0x0;
		r[16] = syscall(__NR_bind, r[1], 0x2000bff0ul, 0x10ul);
		*(uint16_t*)0x20017ff0 = (uint16_t)0x2;
		*(uint16_t*)0x20017ff2 = (uint16_t)0x204e;
		*(uint32_t*)0x20017ff4 = (uint32_t)0x0;
		*(uint8_t*)0x20017ff8 = (uint8_t)0x0;
		*(uint8_t*)0x20017ff9 = (uint8_t)0x0;
		*(uint8_t*)0x20017ffa = (uint8_t)0x0;
		*(uint8_t*)0x20017ffb = (uint8_t)0x0;
		*(uint8_t*)0x20017ffc = (uint8_t)0x0;
		*(uint8_t*)0x20017ffd = (uint8_t)0x0;
		*(uint8_t*)0x20017ffe = (uint8_t)0x0;
		*(uint8_t*)0x20017fff = (uint8_t)0x0;
		r[28] = syscall(__NR_sendto, r[1], 0x20017ffful, 0x0ul, 0x20000844ul, 0x20017ff0ul, 0x10ul);
		memcpy((void*)0x20016000, "\xe1\xb9\xbd\xc0\x8e\x1f\xd4\x81\x51\xaa\x07\x1a\xd9\x65\x83\x8f\xfe\xea\x8d\x21\x74\x7a\x78\x00\x9a\xbd\xa4\x38\x01\x36\x88\xab\x89\xa0\xd2\xf2\xd7\x52\x76\xe8\xcc\xf6\xbe\xa4\x78\xa3\x98\xa6\x60\x44\x05\x04\xe0\xa1\x8a\x38\x1d\x6a\x12\xeb\x24\xa0\x12\xd8\x14\xb5\x2d\x6d\xae\x8e\x82\xb5\x62\xe8\xd0\x25\x67\x58\xcf\x53\xc3\x78\x4b\x37\x7c\x88\x27\x63\xe4\x74\x72\x1c\xb8\x7c\x4e\x59\x05\x29\x5b\xda\x4a\xa1\x96\x44\xca\xa1\x04\xfd\x75\xf2\xb1\x5a\x3e\x43\x76\x67\xd2\x22\x55\xe7\xed\x8a\x56\x53\x9f\xcd\xaf\x03\x3f\xe7\xcc\xd0\xf4\x03\xf2\xdd\x7f\xba\xbd\x97\x49\xd6\x4e\x75\x14\x10\x69\x16\x95\x49\xb4\x6a\x31\xeb\x10\xe7\xf1\x69\xf0\x51\xc2\x48\x26\x7a\xa3\x4f\x29\xcf\x49\x99\xdc\x8c\x94\x6d\x83\xbb\x0e\x8b\x0e\x7a\xe9\xc2\x33\xdf\x7e\x1f\xb6\xcb\x1b\xb0\x47\x74\x22\x42\xe3\x06\x5f\x42\xee\xac\xf9\xb2\xdb\x25\xae\xfd\x90\x14\x69\x70\x42\x84\xff\x59\xd0\x41\x91\x46\xb1\x8c\xb2\xed\xdc\xf7\xcf\x15\xca\x9b\x80\x4e\x58\xfe\x6f\xb4\x8f\x11\x3b\xff\x89\x89\x0d\xdc\xe6\xa7\xaf\x93\xdc\x97\x91\x83\x08\xed\xbc\xe0\x10\x33\xfd\xac\xba\xa9\x27\xc2\x26\xc0\x44\x37\xec\xd6\x79\x46\xc9\xee\x60\x46\x9f\xf6\x7d\x75\x09\xde\x21\xc7\xd7\xb9\xe5\x1b\x0f\xb0\xa3\x72\x40\x36\xa0\x1a\xea\x50\x7d\xbb\x9e\x21\x77\xba\x92\xd6\xa2\x27\xeb\xce\xb9\x20\xf4\xb7\x89\x76\xef\x40\xf6\x2a\xc6\x70\x56\x58\xce\xaf\xbc\x3d\x07\xdb\x22\x8a\xb7\x26\xf5\xb4\xbd\x83\x59\xd1\xb3\xec\x03\xf2\x87\x59\xf4\x3f\xeb\x3e\xfa\x67\x3f\x6f\x92\x7d\x74\x0e\xf5\x4d\x14\xf7\x6d\x64\x10\x42\x9e\xf8\x35\xf4\x6c\x4f\x13\x80\x32\x84\x81\xc8\xf3\xd0\x71\x94\xc9\x55\x52\x90\x1e\xf6\x6b\x0f\x8e\x35\x2c\xe8\xa1\x1c\x5b\xa8\x72\xf1\x72\x29\xeb\xca\xb9\xb2\xf0\x8b\x3f\x57\xf1\x54\xc6\x7f\xcb\xfb\xe9\x9e\xfa\xd8\xe1\x75\xe3\xbf\x86\x69\x10\x61\x42\x5f\xff\xd6\x50\x23\xaa\x6f\x9f\x9a\xdb\x14\x87\xfa\xa2\xc1\x70\x3c\xad\x61\xd1\x7d\xac\x47\x34\x0f\x65\x2b\x92\xe4\x1a\xc9\x37\xe1\x95\x02\x6c\x66\x3a\x9f\x80\xa8\x17\xa2\xa9\x1e\x10\x94\xd2\xa9\x8e\x33\xed\xbc\xe1\x86\xd4\xf0\x5d\xf7\x77\x39\x0a\xfa\xd4\xc2\xf0\x8d\x73\xdc\xb9\x91\x6e\x79\x62\x2b\xd5\x92\x7d\xcb\xd5\xe4\x4e\xe1\xa8\x5b\x00\x39\x98\x0c\x00\x9d\x64\x27\x06\xbe\xe8\x50\x6e\x10\x16\x0b\x08\x04\x6a\x78\xaf\xda\x7f\x2e\xc7\xb3\xc2\x11\xc5\xd6\x83\x3b\x30\x0a\xc9\xee\x3a\x7e\x5e\x23\x4d\xc3\xb2\x57\x6f\x4b\x18\x6f\x24\xdf\x17\xdc\x16\xfb\xd3\xc2\x1e\xc2\x21\x44\xda\x0e\xfa\x56\xe9\x23\xc1\xdb\x87\xc7\x1b\xb4\xdf\xa3\xaa\x29\x8a\xde\x48\xde\xc3\x2e\xd4\x6e\xc8\x64\x19\x0c\x74\xac\x6c\x91\x4e\xdb\xfa\x9e\x3d\x17\x91\x42\x0b\xd7\x73\xd8\x03\xfc\x5a\xc4\xa4\x7e\xf6\x3d\x5a\x20\x9e\xe8\x88\x6e\x4e\xae\xb4\x11\x9c\x11\x78\x27\xa4\xbf\x94\x3e\x41\xeb\xd2\xa9\x6d\x4e\xbf\xb2\x74\xe9\x37\x61\xfc\xb3\xd4\x24\xfb\x4c\xb1\x19\x6e\x4a\x53\xb3\x07\x7f\x49\x85\xca\x87\xdf\x19\x37\x68\x41\x73\x41\xa2\x40\xc1\x7a\x2e\xd8\x9b\x4e\xad\x17\x45\x0f\x5d\xfd\x0b\x23\x6a\xdd\x82\x7d\xfa\xdb\x4e\x2f\x56\x4c\x05\x4a\xe9\x3b\x72\x58\x51\x9c\xa7\xbf\xe5\x2e\x5f\xbb\x0f\xc1\x23\xaf\x07\x32\xba\x12\x7f\x6f\x71\x1d\xd7\xae\x45\x05\xab\x4d\x07\x6f\x75\xac\x09\x80\x29\xd8\xce\x2c\x1c\x7f\xdc\xa8\x01\xcc\x4a\x9c\x6a\x3a\x12\x62\xa9\x25\x1a\xe2\xc6\x2b\xe1\xdf\x35\x25\xe0\x0c\x11\xc9\x8b\xfa\x1f\x2c\x59\x58\x6a\x51\xd2\x9d\xb6\xe6\x0f\xb4\x82\xce\x34\x7d\xe2\x47\xbc\x8a\x03\x01\xb5\x85\x39\x4d\xec\x0b\xca\xba\x9e\x86\x78\xe1\xa3\xd9\x70\x0f\x56\xeb\xed\xab\x72\x8f\xc8\x48\x0b\xbf\xef\x6a\xff\x50\x65\x44\x4b\x43\xe8\x85\xef\x7b\x98\x2b\x6e\xfa\x43\x77\xad\xd4\x5c\x43\x63\x98\x08\x32\xd0\xcb\x70\x6f\x64\x5f\xad\x37\xe7\x91\x66\x09\x9b\x12\x46\x8b\x7d\xd7\xfb\x31\xee\x24\x5e\x44\x61\x83\xe6\xd7\x18\xca\x5b\xe5\xdd\x0e\x0f\xb8\x15\xb6\x95\xa0\xdb\xb6\x3f\xe0\xb3\x2e\x9a\xb0\x9a\x57\x3e\x02\x72\xbf\xcc\x6a\x53\x33\x14\xcc\x0b\xfd\x46\xe6\x80\x0d\xfa\x0b\xc6\x60\xf7\xc4\xae\xf2\x6a\x13\xfd\xd1\x53\x0b\xa3\xd4\xb7\x8a\xb8\xf4\x90\xd4\x0a\xd9\xfd\x34\x06\x09\x23\x58\x92\x06\x3b\x5a\xc8\xcc\x35\xe1\x2b\x00\x75\x96\x17\x0a\xe1\x2a\x8b\x54\x05\xaa\x12\x02\x91\xce\xed\xc5\x1c\xf3\x10\x42\x9c\xa5\x60\x86\x7d\x6e\xbd\x3a\x4b\x22\x0d\x3b\x88\x3b\x80\xef\xda\x06\x2d\x32\x3d\x3d\x5f\xa3\xb3\x9b\xcb\x19\x8f\xc8\x21\x6f\xf1\x76\x06\xba\x5f\x00\xeb\x61\x71\x7e\x9f\x4e\xf7\x18\x69\x7b\xa2\xd9\xad\xbf\x80\x2a\xc0\x7b\x72\x0a\x67\x79\xe1\x9c\x7c\x04\x89\xa5\xc4\x7a\xe7\x25\x50\x86\x9c\xe1\x9c\x95\x06\xcc\x22\x50\x49\xfe\x1b\x1f\x19\xad\x60\x18\x98\xac\x7e\x9d\x20\x05\x6d\x1b\xc0\xd8\xcd\x94\x22\x6c\x64\xbc\x97\x0d\xb4\x4c\x67\xa4\xfc\x12\xcc\x73\x7f\x63\xed\xc0\xc0\x45\xc9\xe9\x3f\xc1\x40\xf2\xb7\x28\xfc\x79\xf4\x69\xdd\xc3\x01\x01\x77\x0e\x4d\x91\x22\x3e\xeb\xbf\x99\x41\xcb\xdc\xcc\x0e\xd3\x8c\x65\x6c\x1f\x38\xff\x77\x20\xa6\x1b\x4a\xf5\xf5\x87\xd5\xb0\x1d\xec\x12\x06\x72\x48\xbd\x5a\x53\xfa\x65\xae\xcc\x48\x8d\x16\x38\x03\x58\x52\x32\x25\x8b\xf8\x23\x52\x79\xc8\x77\x6d\x3a\xd6\x35\xdd\x8e\x8b\x97\x0d\x22\xc7\x61\xe7\x6a\xa7\x5c\x95\xc2\x27\x7b\x3f\x9a\x6e\xd7\x7d\x49\x5c\xdb\x95\x81\xdf\x11\x9c\x1b\xa4\xc4\x69\x3d\x41\x9b\x28\xd8\x2e\xa5\x47\x3b\xea\xd4\x11\x13\xce\xa1\xd0\x31\x3c\xf0\xa4\xf3\x8d\x54\x65\x01\xc8\x74\xdc\x74\xf8\xc0\xb7\xa2\x6b\xff\x3d\x43\x97\xcf\xd7\xdb\xe6\x4b\x58\x26\xdb\x0c\xf9\xfd\x96\xf6\x72\xe9\x38\x33\xc4\xff\x01\x7a\xe3\x57\xab\xa9\xf6\x67\xa3\x4a\x62\x14\x92\xa9\x4e\x0a\xae\x6f\xfb\xd3\x42\x8d\xa2\x33\xf9\xce\x26\x92\x67\x16\x93\xe8\xe9\x0a\xbd\x8b\x4d\x2b\x58\xf4\x88\x62\x55\x2f\xcc\xd2\x51\x8f\xf0\xae\xb6\x5a\x7e\xa2\x22\x44\xa0\x82\xc3\x65\x87\x32\x12\x6c\xad\x22\x51\xd3\xbf\xdf\x88\x09\xc9\x56\x4c\x66\x0b\x19\x68\xc7\x47\x43\x40\x2c\xec\x8e\x95\x65\x3a\xd0\xcc\x44\x4c\xfd\xde\x16\x10\xe9\x9d\xff\x22\x69\x6a\xe4\xcb\x05\xec\x21\xd2\xed\x4e\xdd\x85\x06\x32\x68\xbe\x20\x2e\xd1\x67\xb7\xe3\xf5\xdb\xa2\xa6\xa8\xf5\x4d\xab\x00\x8b\xb6\xd3\x21\xf7\x23\x39\xb4\xfc\x59\xf4\x07\x56\xc0\xb0\x25\x5b\x29\xfe\x90\xeb\x76\x82\x9e\xa9\x47\xff\x6d\xf5\x77\x14\x54\x4d\x05\xa3\x02\xa9\xb7\x77\x71\xce\xe0\xa9\x42\xf4\x60\xa3\xdc\x35\x19\xad\x8b\x62\xe7\xdb\x71\xbb\xcf\x94\xb8\x35\x18\xcb\xf9\x22\xd5\xb2\x27\x2b\xd5\xe1\x18\x5b\xa2\x66\x13\x59\xda\x43\x8a\x40\x03\x08\xcb\xc0\x28\x72\x41\xa2\x8a\x8a\x24\x49\xfa\x8d\x26\xc9\x63\x41\xac\xe7\x7b\xd0\x58\x21\x27\x05\xff\x59\x07\x77\x4f\x34\xc4\x2a\x0a\xd5\x80\xcc\x49\x41\xde\xb5\x57\xe8\xe1\x57\x1f\x86\xd3\xd0\xa7\xf0\x00\x0a\x6f\xbb\x8b\x3f\x67\xd3\xb9\x05\x9c\x4c\x3c\x7e\x01\x41\xa0\x7f\x68\x1e\xb6\x02\xe1\x10\xe9\xca\xc4\x9f\x77\x6f\xce\x33\x6e\x64\x3d\x9d\x48\x40\x1f\x3c\xef\xb5\xcf\x8e\xde\xfa\x46\xb9\xa5\x55\x65\x72\x68\x77\xb2\x15\x6d\x1c\x74\x94\xca\xc4\x2f", 1593);
		*(uint16_t*)0x2000cff0 = (uint16_t)0x2;
		*(uint16_t*)0x2000cff2 = (uint16_t)0x214e;
		*(uint32_t*)0x2000cff4 = (uint32_t)0x100007f;
		*(uint8_t*)0x2000cff8 = (uint8_t)0x0;
		*(uint8_t*)0x2000cff9 = (uint8_t)0x0;
		*(uint8_t*)0x2000cffa = (uint8_t)0x0;
		*(uint8_t*)0x2000cffb = (uint8_t)0x0;
		*(uint8_t*)0x2000cffc = (uint8_t)0x0;
		*(uint8_t*)0x2000cffd = (uint8_t)0x0;
		*(uint8_t*)0x2000cffe = (uint8_t)0x0;
		*(uint8_t*)0x2000cfff = (uint8_t)0x0;
		r[41] = syscall(__NR_sendto, r[1], 0x20016000ul, 0x639ul, 0x0ul, 0x2000cff0ul, 0x10ul);
		*(uint8_t*)0x20019000 = (uint8_t)0xaa;
		*(uint8_t*)0x20019001 = (uint8_t)0xaa;
		*(uint8_t*)0x20019002 = (uint8_t)0xaa;
		*(uint8_t*)0x20019003 = (uint8_t)0xaa;
		*(uint8_t*)0x20019004 = (uint8_t)0xaa;
		*(uint8_t*)0x20019005 = (uint8_t)0x0;
		memcpy((void*)0x20019006, "\x8b\x34\xc1\x75\x6b\x40", 6);
		*(uint16_t*)0x2001900c = (uint16_t)0x8;
		STORE_BY_BITMASK(uint8_t, 0x2001900e, 0x7, 0, 4);
		STORE_BY_BITMASK(uint8_t, 0x2001900e, 0x4, 4, 4);
		STORE_BY_BITMASK(uint8_t, 0x2001900f, 0x0, 0, 2);
		STORE_BY_BITMASK(uint8_t, 0x2001900f, 0x0, 2, 6);
		*(uint16_t*)0x20019010 = (uint16_t)0x2400;
		*(uint16_t*)0x20019012 = (uint16_t)0x6400;
		*(uint16_t*)0x20019014 = (uint16_t)0x0;
		*(uint8_t*)0x20019016 = (uint8_t)0x0;
		*(uint8_t*)0x20019017 = (uint8_t)0x0;
		*(uint16_t*)0x20019018 = (uint16_t)0x0;
		*(uint8_t*)0x2001901a = (uint8_t)0xac;
		*(uint8_t*)0x2001901b = (uint8_t)0x14;
		*(uint8_t*)0x2001901c = (uint8_t)0x0;
		*(uint8_t*)0x2001901d = (uint8_t)0xaa;
		*(uint8_t*)0x2001901e = (uint8_t)0xac;
		*(uint8_t*)0x2001901f = (uint8_t)0x14;
		*(uint8_t*)0x20019020 = (uint8_t)0x0;
		*(uint8_t*)0x20019021 = (uint8_t)0xbb;
		*(uint8_t*)0x20019022 = (uint8_t)0x94;
		*(uint8_t*)0x20019023 = (uint8_t)0x6;
		*(uint32_t*)0x20019024 = (uint32_t)0x9000000;
		*(uint16_t*)0x2001902a = (uint16_t)0x204e;
		*(uint16_t*)0x2001902c = (uint16_t)0x204e;
		*(uint16_t*)0x2001902e = (uint16_t)0x800;
		*(uint16_t*)0x20019030 = (uint16_t)0x0;
		*(uint32_t*)0x20011000 = (uint32_t)0x0;
		*(uint32_t*)0x20011004 = (uint32_t)0x0;
		struct csum_inet csum_77;
		csum_inet_init(&csum_77);
		csum_inet_update(&csum_77, (const uint8_t*)0x2001901a, 4);
		csum_inet_update(&csum_77, (const uint8_t*)0x2001901e, 4);
		uint16_t csum_77_chunk_2 = 0x1100;
		csum_inet_update(&csum_77, (const uint8_t*)&csum_77_chunk_2, 2);
		uint16_t csum_77_chunk_3 = 0x800;
		csum_inet_update(&csum_77, (const uint8_t*)&csum_77_chunk_3, 2);
		csum_inet_update(&csum_77, (const uint8_t*)0x2001902a, 8);
		*(uint16_t*)0x20019030 = csum_inet_digest(&csum_77);
		struct csum_inet csum_78;
		csum_inet_init(&csum_78);
		csum_inet_update(&csum_78, (const uint8_t*)0x2001900e, 28);
		*(uint16_t*)0x20019018 = csum_inet_digest(&csum_78);
		r[79] = syz_emit_ethernet(0x32ul, 0x20019000ul, 0x20011000ul);
}

int main()
{
		setup_tun(0, true);
		loop();
		return 0;
}

