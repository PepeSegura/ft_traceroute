#include "ft_traceroute.h"

#include <netinet/ip.h>

void	handle_timeout(t_traceroute *t)
{
	if (errno != EINTR)
	{
		if (t->current_try == 1) {
			printf("%4d   *", t->current_hop);
		} else
			printf("  *");
		if (t->current_try == t->tries)
			printf("\n");
	}
}

void	print_hop_info(t_traceroute *t, char *ip_str, struct timeval *time)
{
	char	*dns_str = NULL;
	double	rtt_ms = time_diff(time);

	if (t->resolve_hostnames)
		dns_str = reverse_dns_lookup(ip_str);
    if (t->current_try == 1)
	{
        if (t->resolve_hostnames && dns_str)
            printf("%4d   %s (%s)", t->current_hop, ip_str, dns_str);
        else
            printf("%4d   %s", t->current_hop, ip_str);
    }
    printf("  %.3fms", rtt_ms);
    if (t->current_try == t->tries)
		printf("\n");
	free(dns_str);
}

void recv_packet(t_traceroute *t)
{
	while (1)
	{
		byte response[1024] = {0};

		struct sockaddr_in	src_addr;
		socklen_t			src_len = sizeof(src_addr);

		int ret_recv = recvfrom(
			t->server_sock, response, sizeof(response), 0,
			(struct sockaddr *)&src_addr, &src_len
		);
		if (ret_recv < 0)
		{
			handle_timeout(t);
			break ;
		}

		struct icmphdr	*icmp_hdr = (struct icmphdr *)response;
		struct iphdr	*iphdr = (struct iphdr *)response;
	
		if (t->socket_type == TYPE_RAW)
		{
			icmp_hdr = (struct icmphdr *)(response + (iphdr->ihl << 2));
		}

		char *ip_dest_str = inet_ntoa(src_addr.sin_addr);

		if (icmp_hdr->type == ICMP_TIME_EXCEEDED)
		{
			print_hop_info(t, ip_dest_str, &t->time_send);
			break;
		}
		if (icmp_hdr->type == ICMP_ECHOREPLY && t->current_try == t->tries) finish = true;

		if (t->socket_type == TYPE_RAW && icmp_hdr->un.echo.id != htons(getpid())) continue ;

		t_payload *payload = (t_payload *)(icmp_hdr + 1);

		print_hop_info(t, ip_dest_str, &payload->timestamp);
		break ;
	}
}
