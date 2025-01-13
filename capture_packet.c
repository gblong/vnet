#include <stdio.h>
#include <pcap.h>
#include <netinet/ip.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// 以太网头部结构
struct ethernet_header {
    u_char  ether_dhost[6];  // 目的MAC地址
    u_char  ether_shost[6];  // 源MAC地址
    u_short ether_type;      // 以太网类型
};

// 回调函数，用于处理每个捕获的数据包
void packet_handler(u_char *user_data, const struct pcap_pkthdr *pkthdr, const u_char *packet) {
    struct ethernet_header *eth_hdr;
    struct ip *ip_hdr;

    // 以太网头部
    eth_hdr = (struct ethernet_header*)packet;
    
    printf("Ethernet Header:\n");
    printf("  Destination MAC: %s\n", ether_ntoa((struct ether_addr*)eth_hdr->ether_dhost));
    printf("  Source MAC: %s\n", ether_ntoa((struct ether_addr*)eth_hdr->ether_shost));
    printf("  EtherType: 0x%04x\n", ntohs(eth_hdr->ether_type));

    // 如果是 IPv4 数据包，则进一步解析 IP 包头
    if (ntohs(eth_hdr->ether_type) == ETHERTYPE_IP) {
        ip_hdr = (struct ip*)(packet + sizeof(struct ethernet_header));

        printf("IP Header:\n");
        printf("  Source IP: %s\n", inet_ntoa(ip_hdr->ip_src));
        printf("  Destination IP: %s\n", inet_ntoa(ip_hdr->ip_dst));
        printf("  Protocol: %u\n", ip_hdr->ip_p);
    }
    printf("\n");
}

int main(int argc, char** argv) {
    pcap_t *handle;
    char errbuf[PCAP_ERRBUF_SIZE];
    printf("capture on dev: %s\n", argv[1]);

    // 打开网络设备进行捕获
    handle = pcap_open_live(argv[1], BUFSIZ, 1, 1000, errbuf);
    if (handle == NULL) {
        printf("Error opening pcap: %s\n", errbuf);
        return 1;
    }

    // 捕获数据包并处理
    if (pcap_loop(handle, 0, packet_handler, NULL) < 0) {
        printf("Error during capture: %s\n", pcap_geterr(handle));
        return 1;
    }

    // 关闭 pcap
    pcap_close(handle);

    return 0;
}

