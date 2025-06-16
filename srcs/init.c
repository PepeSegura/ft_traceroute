#include "ft_traceroute.h"

static char	*resolve_hostname(const char *hostname, t_traceroute *ping)
{
	struct addrinfo in_info, *result;
	char ip_str[INET6_ADDRSTRLEN];

	memset(&in_info, 0, sizeof(in_info));
	in_info.ai_family = AF_INET;
	in_info.ai_socktype = SOCK_RAW;

	int ret;
	if ((ret = getaddrinfo(hostname, NULL, &in_info, &result)) != 0)
	{
		if (ping->verbose_mode == true)
			dprintf(2, "ft_ping: %s: %s\n", hostname, gai_strerror(ret));
		else
			dprintf(2, "ft_ping: unknown host\n");
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

void create_server_socket(t_traceroute *ping)
{
	int server_fd = INVALID_FD;
	int socket_flags = 0;

	server_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (server_fd != INVALID_FD)
	{
		ping->socket_type = TYPE_RAW;
		goto set_socket_flags;
	}
	server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
	if (server_fd != INVALID_FD)
	{
		ping->socket_type = TYPE_DGRAM;
		goto set_socket_flags;
	}
	dprintf(2, "ft_traceroute: network capabilities disabled\n");
	dprintf(2, "Try 'sudo setcap cap_net_raw+ep %s' or 'sudo %s'\n", ping->flags->argv[0], ping->flags->argv[0]);
	exit(EXIT_FAILURE);

	set_socket_flags:
		socket_flags = fcntl(server_fd, F_GETFL, 0);

		fcntl(server_fd, F_SETFL, socket_flags & ~O_NONBLOCK); // Disable non-blocking

		struct timeval tv = {.tv_sec = 2, .tv_usec = 0};
		setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)); // set 2 secs timeout
	ping->server_sock = server_fd;
}

void set_count(t_traceroute *ping, int pos_flag)
{
	int argc = ping->flags->flags[pos_flag].args_count - 1;

	ping->send_limit = atoi(ping->flags->flags[pos_flag].args[argc]);
	if (ping->send_limit == 0)
		ping->send_limit = UINT32_MAX;
}

void set_interval(t_traceroute *ping, int pos_flag)
{
	int argc = ping->flags->flags[pos_flag].args_count - 1;

	ping->send_interval = atof(ping->flags->flags[pos_flag].args[argc]);
	if (ping->send_interval < MIN_INTERVAL)
	{
		dprintf(2, "ft_ping: option value too smal: %0.2f\n", ping->send_interval);
		cleanup_parser(ping->flags);
		exit(EXIT_FAILURE);
	}
}

void set_ttl(t_traceroute *ping, int pos_flag)
{
	int argc = ping->flags->flags[pos_flag].args_count - 1;

	ping->custom_ttl = atoi(ping->flags->flags[pos_flag].args[argc]);
	if (ping->custom_ttl < MIN_TTL)
	{
		dprintf(2, "ft_ping: value too smal: %d\n", ping->custom_ttl);
		cleanup_parser(ping->flags);
		exit(EXIT_FAILURE);
	}
	if (ping->custom_ttl > MAX_TTL)
	{
		dprintf(2, "ft_ping: value too big: %d\n", ping->custom_ttl);
		cleanup_parser(ping->flags);
		exit(EXIT_FAILURE);
	}
}

void set_timeout(t_traceroute *ping, int pos_flag)
{
	int argc = ping->flags->flags[pos_flag].args_count - 1;

	ping->total_runtime = atol(ping->flags->flags[pos_flag].args[argc]);
	if (ping->total_runtime == 0)
	{
		dprintf(2, "ft_ping: value too small: %ld\n", ping->total_runtime);
		cleanup_parser(ping->flags);
		exit(EXIT_FAILURE);
	}
	if (ping->total_runtime < 0 || ping->total_runtime > INT32_MAX)
	{
		dprintf(2, "ft_ping: value too big: %ld\n", ping->total_runtime);
		cleanup_parser(ping->flags);
		exit(EXIT_FAILURE);
	}
}

void    check_flags(t_traceroute *ping)
{
	int	pos_flag;

	if ((pos_flag = check_flag(ping->flags, '?', "help"))       != -1) {
		printf("%s\n", HELP);
		cleanup_parser(ping->flags);
		exit(EXIT_SUCCESS);
	} if ((pos_flag = check_flag(ping->flags, 0, "usage"))      != -1) {
		printf("%s\n", USAGE);
		cleanup_parser(ping->flags);
		exit(EXIT_SUCCESS);
	} if ((pos_flag = check_flag(ping->flags, 'v', "verbose"))  != -1) {
		ping->verbose_mode = true;
	} if ((pos_flag = check_flag(ping->flags, 'c', "count"))    != -1) {
		set_count(ping, pos_flag);
	} if ((pos_flag = check_flag(ping->flags, 'i', "interval")) != -1) {
		set_interval(ping, pos_flag);
	} if ((pos_flag = check_flag(ping->flags, 0, "ttl"))        != -1) {
		set_ttl(ping, pos_flag);
	} if ((pos_flag = check_flag(ping->flags, 'w', "timeout"))  != -1) {
		set_timeout(ping, pos_flag);
	} if ((pos_flag = check_flag(ping->flags, 'q', "quiet"))    != -1) {
		ping->quiet_mode = true;
	} if ((pos_flag = check_flag(ping->flags, 'f', "flood"))    != -1) {
		ping->flood_mode = true;
	}

	if (ping->flags->extra_args_count < 1)
	{
		dprintf(2, "ft_ping: missing host operand\n");
		dprintf(2, "Try '%s --help' or '%s --usage' for more information.", ping->flags->argv[0], ping->flags->argv[0]);
		cleanup_parser(ping->flags);
		exit(EXIT_FAILURE);
	}

	if (check_flag(ping->flags, 'f', "flood") != -1 && check_flag(ping->flags, 'i', "interval") != -1)
	{
		dprintf(2, "ft_ping: -f and -i incompatible options\n");
		cleanup_parser(ping->flags);
		exit(EXIT_FAILURE);
	}
}

void    init_t_ping(t_traceroute *ping, char *host, t_flag_parser *flags)
{
	ping->custom_ttl = -1;
	ping->send_limit = UINT32_MAX;
	ping->total_runtime = INT32_MAX;
	ping->send_interval = 1.0;

	ping->flags = flags;
	check_flags(ping);

	ping->time_end = ping->ping_start + (ping->total_runtime * 1000);

	if (get_time_in_ms() > ping->time_end)
		finish = true;

	ping->hostname = host;
	ping->ip_addr = resolve_hostname(host, ping);

	create_server_socket(ping);

	gettimeofday(&ping->time_start, NULL);
	ping->rtt_s.max = (double)INT64_MIN;
	ping->rtt_s.min = (double)INT64_MAX;

	if (ping->verbose_mode == false) {
		printf("PING %s (%s): 56 data bytes\n", ping->hostname, ping->ip_addr);
	} else {
		size_t pid = getpid();
		printf("PING %s (%s): 56 data bytes, id %p = %ld\n", ping->hostname, ping->ip_addr, (void *)pid, pid);
	}
}
