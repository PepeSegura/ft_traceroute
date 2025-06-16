#include "ft_traceroute.h"

bool finish;

void sig_handler(int signum)
{
	if (signum == SIGINT)
	{
		finish = true;
	}
}

void print_stats(t_traceroute *p)
{
	int	packet_loss = 100;
	if (p->send_count)
		packet_loss = 100 - ((p->read_count * 100) / p->send_count);
	

	printf("--- %s ping statistics ---\n", p->hostname);
	printf("%d packets transmitted, %d packets received, %d%% packet loss\n",
		p->send_count, p->read_count, packet_loss
	);
	if (packet_loss == 100)
		return ;
	printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
		p->rtt_s.min, p->rtt_s.mean, p->rtt_s.max, p->rtt_s.stddev
	);
}

void	check_help_usage(t_flag_parser *flags)
{
	int	pos_flag;

	if (flags->argc < 2) {
		dprintf(2, "%s: missing host operand\n", flags->argv[0]);
		dprintf(2, "Try '%s --help' or '%s --usage' for more information.\n", flags->argv[0], flags->argv[0]);
		exit(EXIT_FAILURE);
	} if ((pos_flag = check_flag(flags, '?', "help"))	!= -1) {
		printf("%s\n", HELP);
		cleanup_parser(flags);
		exit(EXIT_SUCCESS);
	} if ((pos_flag = check_flag(flags, 0, "usage"))	!= -1) {
		printf("%s\n", USAGE);
		cleanup_parser(flags);
		exit(EXIT_SUCCESS);
	} if (flags->extra_args_count < 1) {
		dprintf(2, "%s: missing host operand\n", flags->argv[0]);
		dprintf(2, "Try '%s --help' or '%s --usage' for more information.\n", flags->argv[0], flags->argv[0]);
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char **argv)
{

	finish = false;

	signal(SIGINT, sig_handler);

	t_flag available_flags[] = {
		/* Mandatory */
		INIT_FLAG('?',	"help",				NO_ARG),
		INIT_FLAG(0,	"usage",			NO_ARG),
		/* Bonus */
		INIT_FLAG('q',	"tries",			NEED_ARG),	// send NUM probe packets per hop (default: 3)
		INIT_FLAG('p',	"port",				NEED_ARG),	// use destination PORT port (default: 33434)
		INIT_FLAG('m',	"max-hop",			NEED_ARG),	// set maximal hop count (default: 64)
		INIT_FLAG('f',	"first-hop",		NEED_ARG),	// set initial hop distance, i.e., time-to-live
		INIT_FLAG('w',	"wait",				NEED_ARG),	// wait NUM seconds for response (default: 3)
		INIT_FLAG(0,	"resolve-hostnames",NO_ARG	),	// resolve hostnames
	};

	t_flag_parser flags = parser_init(available_flags, FLAGS_COUNT(available_flags), argc, argv);

	parse(&flags);
	check_help_usage(&flags);
	print_parsed_flags(&flags);

	t_traceroute t;

	memset(&t, 0, sizeof(t_traceroute));

	init_t_traceroute(&t, flags.extra_args[flags.extra_args_count - 1], &flags);

	for (size_t hops = t.start_hop; hops < t.max_hops; hops++)
	{
		for (size_t times = 0; times < t.send_limit; times++)
		{
			send_packet(&t);
			recv_packet(&t);
			if (finish == false)
				usleep(t.wait_time * 1000 * 1000);
		}
	}

	print_stats(&t);
	close(t.server_sock);
	free(t.ip_addr);
	cleanup_parser(&flags);
	return (0);
}
