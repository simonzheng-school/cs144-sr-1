/**********************************************************************
 * file:  sr_router.c
 * date:  Mon Feb 18 12:50:42 PST 2002
 * Contact: casado@stanford.edu
 *
 * Description:
 *
 * This file contains all the functions that interact directly
 * with the routing table, as well as the main entry method
 * for routing.
 *
 **********************************************************************/

#include <stdio.h>
#include <assert.h>


#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"
#include "sr_arpcache.h"
#include "sr_utils.h"

/*---------------------------------------------------------------------
 * Method: sr_init(void)
 * Scope:  Global
 *
 * Initialize the routing subsystem
 *
 *---------------------------------------------------------------------*/

void sr_init(struct sr_instance* sr)
{
    /* REQUIRES */
    assert(sr);

    /* Initialize cache and cache cleanup thread */
    sr_arpcache_init(&(sr->cache));

    pthread_attr_init(&(sr->attr));
    pthread_attr_setdetachstate(&(sr->attr), PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_t thread;

    pthread_create(&thread, &(sr->attr), sr_arpcache_timeout, sr);
    
    /* Add initialization code here! */

} /* -- sr_init -- */

/*---------------------------------------------------------------------
 * Method: sr_handlepacket(uint8_t* p,char* interface)
 * Scope:  Global
 *
 * This method is called each time the router receives a packet on the
 * interface.  The packet buffer, the packet length and the receiving
 * interface are passed in as parameters. The packet is complete with
 * ethernet headers.
 *
 * Note: Both the packet buffer and the character's memory are handled
 * by sr_vns_comm.c that means do NOT delete either.  Make a copy of the
 * packet instead if you intend to keep it around beyond the scope of
 * the method call.
 *
 *---------------------------------------------------------------------*/

void sr_handlepacket(struct sr_instance* sr,
        uint8_t * packet/* lent */,
        unsigned int len,
        char* interface/* lent */)
{
  /* REQUIRES */
  assert(sr);
  assert(packet);
  assert(interface);
  printf("*** -> Received packet of length %d \n",len);



  printf("~*~*~*~ Printing Packet Headers. ~*~*~*~\n\n");
  print_hdrs(packet, len);
	printf("~*~*~*~ Header finished, Starting printing packet processing. ~*~*~*~\n\n");


  /* Ethernet */
  int minlength = sizeof(sr_ethernet_hdr_t);
  if (len < minlength) {
    printf("Failed ETHERNET header, insufficient length\n");
    return;
  }

  uint16_t ethtype = ethertype(packet);
	if (ethtype == ethertype_ip) { /* If this is an IP packet */

    printf ("This is an IP Packet!\n");
    sr_ip_hdr_t *ip_hdr = (sr_ip_hdr_t *) (packet + sizeof(sr_ethernet_hdr_t));
		
    if (sr_if_list_contains_ip(sr, ip_hdr->ip_dst)) { /* If the packet is for the router */
      printf ("The IP packet is for me!\n");

      /* Check the ip_protocol */
      uint8_t ip_proto = ip_protocol((uint8_t *)ip_hdr);

      if (ip_proto == ip_protocol_icmp) { /* ICMP */
      minlength += sizeof(sr_icmp_hdr_t);
      if (len < minlength)
        fprintf(stderr, "Failed to print ICMP header, insufficient length\n");
        return;
      }
      
      /* If it's ICMP echo req, send echo reply. */
      sr_icmp_hdr_t *icmp_hdr = (sr_icmp_hdr_t *)(packet + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t)); 
      if (icmp_hdr->icmp_type == 8 && icmp_hdr->icmp_code == 0) { /* If this is an echo request */
        /* fill in code here */
        printf ("This is an ICMP Echo request!\n");
      }

      /* Else if it's TCP/UDP, send ICMP port unreachable */
        /* fill in code here */

		} else { /* If the packet is not for the router */
			printf ("This IP packet is not for me!\n");

      /* Check if the IP header has not been truncated  */
      minlength += sizeof(sr_ip_hdr_t);
      if (len < minlength) {
        fprintf(stderr, "Failed IP header, insufficient length\n");
        return;
      }

      /* Perform checksums */
      if (!ip_hdr_checksum_valid(ip_hdr)) {
        fprintf(stderr, "Checksum is not valid -- Packet is probably corrupt\n");
        return;
      }

      /* If the packet is not for the router, check routing table, perform LPM */
      sr_routing_table_lpm_forwarding(sr, ip_hdr->ip_dst);


		}
	} else if (ethtype == ethertype_arp) { /* If this is an ARP packet */
    printf ("This is an ARP Packet!\n");

    minlength += sizeof(sr_arp_hdr_t);
    if (len < minlength) {
      fprintf(stderr, "Failed to print ARP header, insufficient length\n");
      return;
    }
    /* If it's a reply to me: Cache it, go through my request queue and send outstanding packets */
      /* fill in code here */

    /* If it's a request to me: Construct an ARP reply and send it back */
      /* fill in code here */
  } else {
    fprintf(stderr, "Unrecognized Ethernet Type: %d\n", ethtype);
  }

  printf("~*~*~*~ Finished printing packet processing. ~*~*~*~\n\n");

}/* end sr_ForwardPacket */

int ip_hdr_checksum_valid (sr_ip_hdr_t *ip_hdr) {
  printf("Checking if checksum is valid...\n\n");
  uint16_t initial_checksum = ip_hdr->ip_sum;
  ip_hdr->ip_sum = 0;
  if (cksum(ip_hdr, sizeof(sr_ip_hdr_t)) == initial_checksum) {
    printf("Checksum is valid!\n\n");
    return 1;
  }
  printf("Checksum is NOT valid...\n\n");
  return 0;
}


void sr_routing_table_lpm_forwarding(struct sr_instance* sr, uint32_t ip_addr)
{
  printf("~*~*~*~ Starting Routing Table LPM Forwarding with the following rtable on IP == %x: ~*~*~*~\n\n", ip_addr);
  sr_print_routing_table(sr);
  printf("~*~*~*~\n\n");
  struct sr_rt* rt_walker = 0;

  if(sr->routing_table == 0)
  {
    printf(" *warning* Routing table empty \n");
    return;
  }

  /* Traverse the routing table searching for the gateway address with the greatest match */
  rt_walker = sr->routing_table;

  struct in_addr next_gw = rt_walker->gw;
  uint32_t longest_mask = rt_walker->mask.s_addr;

  printf("LPM: The first routing entry is as follows and has mask == %x: \n\t", rt_walker->mask.s_addr);
  sr_print_routing_entry(rt_walker);
  printf("\tAfter applying the subnet mask we have ip&mask == %x\n", rt_walker->mask.s_addr & ip_addr);
  uint32_t masked_ip = rt_walker->mask.s_addr & ip_addr;
  if (masked_ip == rt_walker->dest.s_addr) {
    printf("\twe have a match with \n");
    sr_print_routing_entry(rt_walker);
  }

  /* variables to hold the current rtable entry */
  /* 
  int max_match = 0;
  struct in_addr dest = rt_walker->dest;
  struct in_addr gw = rt_walker->gw;
  struct in_addr mask = rt_walker->mask;
  */

  


  /* e.g.: uint32_t gateway_addr = calculate_prefix_match(rt_walker, max_match); */
  while(rt_walker->next)
  {
      rt_walker = rt_walker->next; 
      /* do something */
      printf("LPM: The next routing entry is as follows and has mask == %x: \n\t", rt_walker->mask.s_addr);
      sr_print_routing_entry(rt_walker);
      printf("\tAfter applying the subnet mask we have ip&mask == %x\n", rt_walker->mask.s_addr & ip_addr);
      masked_ip = rt_walker->mask.s_addr & ip_addr;
      if (masked_ip == rt_walker->dest.s_addr) {
        printf("\twe have a match with \n");
        sr_print_routing_entry(rt_walker);
        if (rt_walker->mask.s_addr > longest_mask) {
          printf("\t *** we have a new longest match! *** \n");
          next_gw = rt_walker->gw;
          longest_mask = rt_walker->mask.s_addr;
        }
      }
      /* e.g.: uint32_t gateway_addr = calculate_prefix_match(rt_walker, max_match); */
  }
  printf("The Next gateway_addr is %s\n", inet_ntoa(entry->gw));
  printf("~*~*~*~ Finished with Routing Table LPM Forwarding ~*~*~*~\n\n");
}
 
/* Check TTL, ARP, etc and the stub functions kurt talked about and add these in your function. */
/* Check if your minlength calculations are working out, especially around ARP but it should be since it won't add anything else if it's ARP */
/* Check if you need to see if the datagram has been truncated to < than length specified in IP hdr */