#include "ft_traceroute.h"

static char	*resolve_hostname(const char *hostname, t_traceroute *t)
{
	(void)t;
	struct addrinfo in_info, *result;
	char ip_str[INET6_ADDRSTRLEN];

	memset(&in_info, 0, sizeof(in_info));
	in_info.ai_family = AF_INET;
	in_info.ai_socktype = SOCK_RAW;

	int ret;
	if ((ret = getaddrinfo(hostname, NULL, &in_info, &result)) != 0)
	{
		dprintf(2, "ft_traceroute: unknown host\n");
		exit(EXIT_FAILURE);
	}

	void	*addr;

	if (result->ai_family == AF_INET)
	{
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)result->ai_addr;
		addr = &(ipv4->sin_addr);
	}
	else
	{
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)result->ai_addr;
		addr = &(ipv6->sin6_addr);
	}
	inet_ntop(result->ai_family, addr, ip_str, sizeof(ip_str));

	freeaddrinfo(result);
	return (strdup(ip_str));
}

# define INVALID_FD -1

void create_server_socket(t_traceroute *t)
{
	int server_fd = INVALID_FD;
	int socket_flags = 0;

	server_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (server_fd != INVALID_FD)
	{
		t->socket_type = TYPE_RAW;
		goto set_socket_flags;
	}
	server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
	if (server_fd != INVALID_FD)
	{
		t->socket_type = TYPE_DGRAM;
		goto set_socket_flags;
	}
	dprintf(2, "ft_traceroute: network capabilities disabled\n");
	dprintf(2, "Try 'sudo setcap cap_net_raw+ep %s' or 'sudo %s'\n", t->flags->argv[0], t->flags->argv[0]);
	exit(EXIT_FAILURE);

	set_socket_flags:
		socket_flags = fcntl(server_fd, F_GETFL, 0);

		fcntl(server_fd, F_SETFL, socket_flags & ~O_NONBLOCK); // Disable non-blocking

		struct timeval tv = {.tv_sec = 2, .tv_usec = 0};
		setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)); // set 2 secs timeout
	t->server_sock = server_fd;
}

void set_count(t_traceroute *t, int pos_flag)
{
	int argc = t->flags->flags[pos_flag].args_count - 1;

	t->send_limit = atoi(t->flags->flags[pos_flag].args[argc]);
	if (t->send_limit == 0)
		t->send_limit = UINT32_MAX;
}

void set_ttl(t_traceroute *t, int pos_flag)
{
	int argc = t->flags->flags[pos_flag].args_count - 1;

	t->custom_ttl = atoi(t->flags->flags[pos_flag].args[argc]);
	if (t->custom_ttl < MIN_TTL)
	{
		dprintf(2, "ft_traceroute: value too smal: %d\n", t->custom_ttl);
		cleanup_parser(t->flags);
		exit(EXIT_FAILURE);
	}
	if (t->custom_ttl > MAX_TTL)
	{
		dprintf(2, "ft_traceroute: value too big: %d\n", t->custom_ttl);
		cleanup_parser(t->flags);
		exit(EXIT_FAILURE);
	}
}

void    check_flags(t_traceroute *t)
{
	int	pos_flag;

	if ((pos_flag = check_flag(t->flags, '?', "help"))       != -1) {
		printf("%s\n", HELP);
		cleanup_parser(t->flags);
		exit(EXIT_SUCCESS);
	} if ((pos_flag = check_flag(t->flags, 0, "usage"))      != -1) {
		printf("%s\n", USAGE);
		cleanup_parser(t->flags);
		exit(EXIT_SUCCESS);
	} if ((pos_flag = check_flag(t->flags, 'c', "count"))    != -1) {
		set_count(t, pos_flag);
	} if ((pos_flag = check_flag(t->flags, 0, "ttl"))        != -1) {
		set_ttl(t, pos_flag);
	}

	if (t->flags->extra_args_count < 1)
	{
		dprintf(2, "ft_traceroute: missing host operand\n");
		dprintf(2, "Try '%s --help' or '%s --usage' for more information.", t->flags->argv[0], t->flags->argv[0]);
		cleanup_parser(t->flags);
		exit(EXIT_FAILURE);
	}

	if (check_flag(t->flags, 'f', "flood") != -1 && check_flag(t->flags, 'i', "interval") != -1)
	{
		dprintf(2, "ft_traceroute: -f and -i incompatible options\n");
		cleanup_parser(t->flags);
		exit(EXIT_FAILURE);
	}
}

void    init_t_traceroute(t_traceroute *t, char *host, t_flag_parser *flags)
{
	t->custom_ttl = -1;
	t->send_limit = UINT32_MAX;
	t->flags = flags;

	check_flags(t);

	t->hostname = host;
	t->ip_addr = resolve_hostname(host, t);

	create_server_socket(t);

	gettimeofday(&t->time_start, NULL);
	t->rtt_s.max = (double)INT64_MIN;
	t->rtt_s.min = (double)INT64_MAX;

	printf("t %s (%s): 56 data bytes\n", t->hostname, t->ip_addr);
}
