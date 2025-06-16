#include "ft_traceroute.h"

void mean_and_stddev(t_traceroute *p, double new_value)
{
	t_rtt_stats *r = (t_rtt_stats *)&p->rtt_s;

	r->total_count = p->read_count;

	double delta = new_value - r->mean;
	r->mean += delta / r->total_count;

	double delta2 = new_value - r->mean;

	r->m2 += delta * delta2;

	double variance = 0.0;
	if (r->total_count > 1)
		variance = (r->m2 / (r->total_count - 1));
	r->stddev = sqrt(variance);
}

#include <netinet/ip.h>

void dump_ip_header(struct iphdr *ip)
{
	byte *bytes = (byte *)ip;

	struct icmphdr	*response_icmp = (struct icmphdr *)ip;

	printf("IP Hdr Dump:\n ");
	for (size_t i = 0; i < sizeof(struct iphdr); i+=4)
	{
		printf("%02x%02x %02x%02x ", bytes[i], bytes[i+1], bytes[i+2], bytes[i+3]);
	}

	char ip_src[INET_ADDRSTRLEN];
	char ip_dst[INET_ADDRSTRLEN];

	inet_ntop(AF_INET, &(ip->saddr), ip_src, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &(ip->daddr), ip_dst, INET_ADDRSTRLEN);

	printf("\n");
	printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src      Dst     Data\n");
	printf(" %1x  %1x  %02x %04x %04x   %1x %04x  %02x  %02x %04x %s %s\n",
		   ip->version, ip->ihl, ip->tos, ntohs(ip->tot_len), ntohs(ip->id),
		   ntohs(ip->frag_off) >> 13, 
		   ntohs(ip->frag_off) & 0x1FFF,
		   ip->ttl, ip->protocol, ntohs(ip->check),
		   ip_src, ip_dst
	);
	static int8_t seq_it;
	printf("ICMP: type %d, code %d, size %d, id 0x%x, seq 0x%x\n",
		response_icmp->type, response_icmp->code, ip->tot_len, ntohs(ip->id), (int32_t)seq_it 
	);
	seq_it++;
}

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
			if (p->verbose_mode == true)
				dump_ip_header(iphdr);
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
		
		mean_and_stddev(p, rtt_ms);

		if (p->flood_mode == true) { write(1, "\r", 1); break; }
		if (p->quiet_mode == true) break;

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
