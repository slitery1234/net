#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <pcap/pcap.h>
#include <time.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <string.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/ipv6.h>

void print_packet_info(
		u_char *args,
		const struct pcap_pkthdr *header,
		const u_char *packet
		){
	printf("-----------|Packet Start|-----------\n\n");
    // Get Time Stamp
	printf("Time Stamp: %s", ctime((const time_t*)&header->ts.tv_sec));	

	// Get MAC and type
	struct ether_header *eth = (struct ether_header*) packet;
	printf("Source MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",eth->ether_shost[0], eth->ether_shost[1], eth->ether_shost[2],	eth->ether_shost[3], eth->ether_shost[4], eth->ether_shost[5]);
	printf("Dest MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",eth->ether_dhost[0], eth->ether_dhost[1], eth->ether_dhost[2], eth->ether_dhost[3], eth->ether_dhost[4], eth->ether_dhost[5]);

	u_int16_t type = ntohs(eth->ether_type);
	printf("Type: %04X\n",type);

	// Locate IP or TCP/UDP
	if((type == ETHERTYPE_IP) || (type == ETHERTYPE_IPV6)){
		struct iphdr *iphd = (struct iphdr*)(packet + sizeof(struct ethhdr));
		printf("IP verison: %d\n", iphd->version);
		// IPv6, addr_length=16
		if(iphd->version == 6){
			struct ipv6hdr *ipv6hd = (struct ipv6hdr*)(packet + sizeof(struct ethhdr));
			printf("Source IP: ");
			for(int i=0;i<16;i++)
				if(i % 2 == 1 && i != 15)
					printf("%02X:",ipv6hd->saddr.s6_addr[i]);
				else
					printf("%02X",ipv6hd->saddr.s6_addr[i]);
			printf("\n");
			printf("Dest IP: ");
			for(int i=0;i<16;i++)
				if(i % 2 == 1 && i != 15)
					printf("%02X:",ipv6hd->daddr.s6_addr[i]);
				else
					printf("%02X",ipv6hd->daddr.s6_addr[i]);
			printf("\n");
		}
		else{	//IPv4
			printf("Source IP: %s\n", inet_ntoa(*(struct in_addr*)\
						&iphd->saddr));
			printf("Dest IP: %s\n", inet_ntoa(*(struct in_addr*)\
						&iphd->daddr));
		}
		if(iphd->protocol == 6){	// TCP
			struct tcphdr *tcphd = (struct tcphdr*)\
			(packet + sizeof(struct ethhdr) + sizeof(struct iphdr));
			printf("TCP ports:\n");
			printf("\tSource: %d\n", ntohs(tcphd->source));
			printf("\tDest: %d\n", ntohs(tcphd->dest));
		}else if(iphd->protocol == 17){		//UDP
			struct udphdr *udphd = (struct udphdr*)\
			(packet + sizeof(struct ethhdr) + sizeof(struct iphdr));
			printf("UDP ports:\n");
			printf("\tSource: %d\n", ntohs(udphd->source));
			printf("\tDest: %d\n", ntohs(udphd->dest));
		}
	}else{
		printf("Not IP packet\n");
	}
	printf("\n------------|Packet End|------------\n\n");
}

int main(int argc, char *argv[])  
{  
    char *pcapFile;
	const u_char *packet;
	struct pcap_pkthdr header;
	if(argc < 2){
		perror("Error: No Input File\nError:");
		exit(0);
	}else{
		pcapFile = argv[1];
	}
	char error_buffer[PCAP_ERRBUF_SIZE];
	pcap_t *handle = pcap_open_offline_with_tstamp_precision(pcapFile, PCAP_TSTAMP_PRECISION_MICRO, error_buffer);
	if(handle == NULL){
		perror("Error: The pcap File can't open\n Error:");
		exit(0);
	}
	pcap_loop(handle, 0, print_packet_info, NULL);
	return 0;
}



