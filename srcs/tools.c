#include "ft_traceroute.h"

uint16_t checksum(void *buffer, size_t len)
{
	uint8_t		*bytes = (uint8_t *)buffer;
	uint32_t	sum = 0;

	for (size_t i = 0; i < len; i += 2)
	{
		uint16_t two_bytes;
		if (i + 1 < len)
			two_bytes = (bytes[i] << 8) | bytes[i + 1];
		else
			two_bytes = (bytes[i] << 8);
		sum += two_bytes;
	}

	while (sum >> 16 != 0)
	{
		sum = (sum & 0xFFFF) + (sum >> 16);
	}
	sum = (uint16_t)(~sum);
	return (sum);
}

double time_diff(struct timeval *prev)
{
	struct timeval now;

	gettimeofday(&now, NULL);
	return (((now.tv_sec - prev->tv_sec) * 1000.0) + (now.tv_usec - prev->tv_usec) / 1000.0);
}

int64_t get_time_in_ms(void)
{
	struct timeval now;

	gettimeofday(&now, NULL);
	return ((now.tv_sec * 1000.0) + (now.tv_usec / 1000.0));
}

char *reverse_dns_lookup(char *ip_address_str)
{
	struct sockaddr_in sa;
	char host[NI_MAXHOST];

	memset(&sa, 0, sizeof(struct sockaddr_in));
	sa.sin_family = AF_INET;
	if (inet_pton(AF_INET, ip_address_str, &sa.sin_addr) != 1) {
		perror("inet_pton failed");
		return (NULL);
	}

	int res = getnameinfo((struct sockaddr *)&sa, sizeof(sa), host, sizeof(host), NULL, 0, NI_NAMEREQD);
	if (res != 0)
		return (strdup(ip_address_str));
	return (strdup(host));
}
