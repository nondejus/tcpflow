/**
 * address_histogram.cpp: 
 * Show packets received vs port
 *
 * This source file is public domain, as it is not based on the original tcpflow.
 *
 * Author: Michael Shick <mike@shick.in>
 *
 */

#include "config.h"
#include "tcpflow.h"
#include "tcpip.h"

#include <iomanip>
#include <algorithm>

#include "address_histogram.h"

void address_histogram::ingest_packet(const packet_info &pi)
{
    struct ip4_dgram ip4;
    struct ip6_dgram ip6;
    std::stringstream ss;

    // IPv4?
    if(tcpip::ip4_from_bytes(pi.ip_data, pi.ip_datalen, ip4)) {
        // recording source addresses?
        if(relationship == SOURCE || relationship == SRC_OR_DST) {
            parent_count_histogram.increment(std::string(inet_ntoa(ip4.header->ip_src)), 1);
        }
        if(relationship == DESTINATION || relationship == SRC_OR_DST) {
            parent_count_histogram.increment(std::string(inet_ntoa(ip4.header->ip_dst)), 1);
        }
    }
    // IPv6?
    else if(tcpip::ip6_from_bytes(pi.ip_data, pi.ip_datalen, ip6)) {
        ss.str(std::string());
        ss << std::hex << std::setfill('0');
        // recording source addresses?
        if(relationship == SOURCE || relationship == SRC_OR_DST) {
            for(int ii = 0; ii < 8; ii++) {
                ss << std::setw(4) << ip6.header->ip6_src.__u6_addr.__u6_addr8[ii];
                if(ii < 7) {
                    ss << ":";
                }
            }
            parent_count_histogram.increment(ss.str(), 1);
            ss.str(std::string());
            ss << std::hex << std::setfill('0');
        }
        // recording destination addresses?
        if(relationship == DESTINATION || relationship == SRC_OR_DST) {
            for(int ii = 0; ii < 8; ii++) {
                ss << std::setw(4) << ip6.header->ip6_dst.__u6_addr.__u6_addr8[ii];
                if(ii < 7) {
                    ss << ":";
                }
            }
            parent_count_histogram.increment(ss.str(), 1);
        }
    }
}

void address_histogram::render(cairo_t *cr, const plot::bounds_t &bounds)
{
#ifdef CAIRO_PDF_AVAILABLE
    parent_count_histogram.render(cr, bounds);
#endif
}

void address_histogram::from_iptree(const iptree &tree)
{
    // convert iptree to suitable vector for count histogram
    std::vector<iptree::addr_elem> addresses;

    tree.get_histogram(addresses);

    std::sort(addresses.begin(), addresses.end(), iptree_node_comparator());

    std::vector<count_histogram::count_pair> bars;

    std::vector<iptree::addr_elem>::const_iterator it = addresses.begin();
    int ii = 0;
    while(ii < parent_count_histogram.max_bars && it != addresses.end()) {
        bars.push_back(count_histogram::count_pair((*it).str(), it->count));
        ii++;
        it++;
    }

    bars.resize(parent_count_histogram.max_bars);

    parent_count_histogram.set_top_list(bars);
    parent_count_histogram.set_count_sum(tree.sum());
}

bool address_histogram::iptree_node_comparator::operator()(const iptree::addr_elem &a,
        const iptree::addr_elem &b)
{
    if(a.count > b.count) {
        return true;
    }
    else if(a.count < b.count) {
        return false;
    }
    for(size_t ii = 0; ii < sizeof(a.addr); ii++) {
        if(a.addr[ii] > b.addr[ii]) {
            return true;
        }
        else if(a.addr[ii] < b.addr[ii]) {
            return false;
        }
    }
    return false;
}

void address_histogram::quick_config(relationship_t relationship_,
        std::string title_, std::string subtitle_)
{
    relationship = relationship_;
    parent_count_histogram.parent_plot.title = title_;
    parent_count_histogram.parent_plot.subtitle = subtitle_;
    parent_count_histogram.parent_plot.title_on_bottom = true;
    parent_count_histogram.parent_plot.pad_left_factor = 0.0;
    parent_count_histogram.parent_plot.pad_right_factor = 0.0;
    parent_count_histogram.parent_plot.x_label = "";
    parent_count_histogram.parent_plot.y_label = "";
}
