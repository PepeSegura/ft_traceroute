#include "ft_traceroute.h"

#include <netinet/ip.h>

void recv_packet(t_traceroute *p)
{
	while (1)
	{
		byte response[1024] = {0};

		struct sockaddr_in	src_addr;
		socklen_t			src_len = sizeof(src_addr);

		int ret_recv = recvfrom(
			p->server_sock, response, sizeof(response), 0,
			(struct sockaddr *)&src_addr, &src_len
		);
		if (ret_recv < 0)
		{
			if (errno != EINTR)
				perror("recvfrom ret < 0");
			break ;
		}

		uint8_t			ttl = 0;
		struct icmphdr	*icmp_hdr = (struct icmphdr *)response;
		struct iphdr	*iphdr = (struct iphdr *)response;
	
		int	pkt_size = DEFAULT_READ;

		if (p->socket_type == TYPE_RAW)
		{
			ttl = iphdr->ttl;
			icmp_hdr = (struct icmphdr *)(response + (iphdr->ihl << 2));
			pkt_size = ntohs(iphdr->tot_len) - (iphdr->ihl << 2) - (sizeof(struct icmphdr) << 1);
		}

		if (icmp_hdr->type == ICMP_TIME_EXCEEDED)
		{
			char *ip_dest_str = inet_ntoa(src_addr.sin_addr);
			char *dns_lookup_str = reverse_dns_lookup(ip_dest_str);

			if (pkt_size < 32) pkt_size = 32;
	
			printf("%d bytes from: %s (%s): Time to live exceeded\n", pkt_size, dns_lookup_str, ip_dest_str);
			free(dns_lookup_str);
			break;
		}
		if (icmp_hdr->type != ICMP_ECHOREPLY) continue ;

		if (p->socket_type == TYPE_RAW && icmp_hdr->un.echo.id != htons(getpid()))
		{
			dprintf(2, "Invalid ID\n");
			continue ;
		}

		p->read_count++;

		t_payload *payload = (t_payload *)(icmp_hdr + 1);

		double rtt_ms = time_diff(&payload->timestamp);

		if (rtt_ms > p->rtt_s.max) p->rtt_s.max = rtt_ms;
		if (rtt_ms < p->rtt_s.min) p->rtt_s.min = rtt_ms;
		
		printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
			pkt_size,
			inet_ntoa(src_addr.sin_addr),
			ntohs(icmp_hdr->un.echo.sequence),
			ttl,
			rtt_ms
		);
		break ;
	}
}
