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

		struct timeval tv = {.tv_sec = t->wait_time, .tv_usec = 0};
		setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)); // set 2 secs timeout
	t->server_sock = server_fd;
}

# define HELP_MODE 1
# define USAGE_MODE 2

void	handle_help_and_usage(t_traceroute *t, int mode)
{
	switch (mode)
	{
		case HELP_MODE:
			printf("%s\n", HELP);
			break;
		case USAGE_MODE:
			printf("%s\n", USAGE);
			break;
		default:
			break;
	}
	cleanup_parser(t->flags);
	exit(EXIT_SUCCESS);
}

void	set_first_hop(t_traceroute *t, int pos_flag)
{
	int last_arg_pos = t->flags->flags[pos_flag].args_count - 1;

	char *arg = t->flags->flags[pos_flag].args[last_arg_pos];
	t->first_hop = atoi(arg);
	if (t->first_hop < MIN_HOPS || t->first_hop > MAX_HOPS)
	{
		dprintf(2, "ft_traceroute: imposible distance `%s'\n", arg);
		cleanup_parser(t->flags);
		exit(EXIT_FAILURE);
	}
}

void	set_max_hop(t_traceroute *t, int pos_flag)
{
	int last_arg_pos = t->flags->flags[pos_flag].args_count - 1;

	char *arg = t->flags->flags[pos_flag].args[last_arg_pos];
	t->max_hop = atoi(arg);
	if (t->max_hop < MIN_HOPS || t->max_hop > MAX_HOPS)
	{
		dprintf(2, "ft_traceroute: invalid hops value `%s'\n", arg);
		cleanup_parser(t->flags);
		exit(EXIT_FAILURE);
	}
}

void	set_dest_port(t_traceroute *t, int pos_flag)
{
	int last_arg_pos = t->flags->flags[pos_flag].args_count - 1;

	char *arg = t->flags->flags[pos_flag].args[last_arg_pos];
	t->dest_port = atoi(arg);
	if (t->dest_port <= 0 || t->dest_port >= 65536)
	{
		dprintf(2, "ft_traceroute: invalid port number `%s'\n", arg);
		cleanup_parser(t->flags);
		exit(EXIT_FAILURE);
	}
}

void	set_tries(t_traceroute *t, int pos_flag)
{
	int last_arg_pos = t->flags->flags[pos_flag].args_count - 1;

	char *arg = t->flags->flags[pos_flag].args[last_arg_pos];
	t->tries = atoi(arg);
	if (t->tries < MIN_TRIES || t->tries > MAX_TRIES)
	{
		dprintf(2, "ft_traceroute: number of tries should be between 1 and 10\n");
		cleanup_parser(t->flags);
		exit(EXIT_FAILURE);
	}
}

void set_resolve_hostnames(t_traceroute *t)
{
	t->resolve_hostnames = true;
}

void	set_wait(t_traceroute *t, int pos_flag)
{
	int last_arg_pos = t->flags->flags[pos_flag].args_count - 1;

	char *arg = t->flags->flags[pos_flag].args[last_arg_pos];
	t->wait_time = atoi(arg);
	if (t->wait_time < MIN_WAIT || t->wait_time > MAX_WAIT)
	{
		dprintf(2, "ft_traceroute: ridiculous waiting time `%s'\n", arg);
		cleanup_parser(t->flags);
		exit(EXIT_FAILURE);
	}
}

void    check_flags(t_traceroute *t)
{
	int	pos_flag;

	if (check_flag(t->flags, '?', "help")  != -1) {
		handle_help_and_usage(t, HELP_MODE);
	} if (check_flag(t->flags, 0, "usage") != -1) {
		handle_help_and_usage(t, USAGE_MODE);
	} if ((pos_flag = check_flag(t->flags, 'f', "first-hop")) != -1) {
		set_first_hop(t, pos_flag);
	} if ((pos_flag = check_flag(t->flags, 'm', "max-hop")) != -1) {
		set_max_hop(t, pos_flag);
	} if ((pos_flag = check_flag(t->flags, 'p', "port")) != -1) {
		set_dest_port(t, pos_flag);
	} if ((pos_flag = check_flag(t->flags, 'q', "tries")) != -1) {
		set_tries(t, pos_flag);
	} if (check_flag(t->flags, 0, "resolve-hostnames") != -1) {
		set_resolve_hostnames(t);
	} if ((pos_flag = check_flag(t->flags, 'w', "wait")) != -1) {
		set_wait(t, pos_flag);
	}

	if (t->flags->extra_args_count < 1)
	{
		dprintf(2, "ft_traceroute: missing host operand\n");
		dprintf(2, "Try '%s --help' or '%s --usage' for more information.", t->flags->argv[0], t->flags->argv[0]);
		cleanup_parser(t->flags);
		exit(EXIT_FAILURE);
	}
}

void    init_t_traceroute(t_traceroute *t, char *host, t_flag_parser *flags)
{
	setbuf(stdout, NULL);
	t->flags = flags;

	t->current_hop = 1;
	t->first_hop = 1;
	t->max_hop = 64;
	t->dest_port = 33434;
	t->tries = 3;
	t->resolve_hostnames = false;
	t->wait_time = 3;

	check_flags(t);

	t->hostname = host;
	t->ip_addr = resolve_hostname(host, t);

	create_server_socket(t);

	printf("traceroute to %s (%s), %d hops max\n", t->hostname, t->ip_addr, t->max_hop);
}
