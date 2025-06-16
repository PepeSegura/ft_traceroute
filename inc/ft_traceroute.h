#ifndef FT_TRACEROUTE_H
# define FT_TRACEROUTE_H

# include "flag_parser.h"

# include <math.h>
# include <signal.h>
# include <stdbool.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <errno.h>

# include <sys/socket.h>
# include <sys/time.h>
# include <sys/types.h>

# include <arpa/inet.h>

# include <netdb.h>
# include <netinet/in.h>
# include <netinet/ip.h>
# include <netinet/ip_icmp.h>

# include <fcntl.h>
# include <poll.h>

# include "help.h"

enum sock_type {
	TYPE_RAW,
	TYPE_DGRAM,
};

# define DEFAULT_READ 64
# define DEFAULT_ERR 92

typedef struct s_rtt_stats {
	int		total_count;

	double	min;
	double	max;
	double	m2;

	double	mean;
	double	stddev;
} t_rtt_stats;

typedef struct s_ping {
	t_flag_parser	*flags;

	char			*hostname;
	char			*ip_addr;
	int				socket_type;
	int				server_sock;

	int				send_limit;
	int				dest_port;
	int				max_hops;
	int				start_hop;
	int				wait_time;
	int				resolve_hostnames;

	int     		sequence;
	int				custom_ttl;

	t_rtt_stats		rtt_s;

	struct timeval	time_start;
	struct timeval	time_last;

}	t_traceroute;


extern bool finish;

# define PING_PKT_SIZE 64
# define MIN_INTERVAL 0.2
# define MIN_TTL 1
# define MAX_TTL 255
# define MIN_RUNTIME 0
# define MAX_RUNTIME INT32_MAX

typedef struct s_payload{
	uint16_t		sequence;
	struct timeval	timestamp;
	char			data[PING_PKT_SIZE - sizeof(uint16_t) - sizeof(struct timeval)];
} t_payload;

typedef struct s_packet {
	struct icmphdr	hdr;
	t_payload		payload;
}   t_packet;

typedef uint8_t byte;

/* init.c */
void		init_t_traceroute(t_traceroute *trace, char *host, t_flag_parser *flags);

/* tools.c */
uint16_t	checksum(void *buffer, size_t len);
double		time_diff(struct timeval *prev);
int64_t		get_time_in_ms(void);
char		*reverse_dns_lookup(char *ip_address_str);

/* send.c */
void		send_packet(t_traceroute *trace);
void		recv_packet(t_traceroute *trace);

#endif
