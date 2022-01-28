
#include "mem/minirank_dram_interface.hh"

#include "base/bitfield.hh"
#include "base/cprintf.hh"
#include "base/trace.hh"
#include "debug/MinirankDRAM.hh"
#include "mem/channel_dram_interface.hh"
#include "mem/mem_interface.hh"

namespace gem5
{

using namespace Data;
namespace memory
{

MinirankDRAMInterface::MinirankDRAMInterface(
            const MinirankDRAMInterfaceParams &_p)
    : DRAMInterface(_p)
{
    DPRINTF(MinirankDRAM, "Setting up minirank DRAM Interface\n");

    fatal_if(!isPowerOf2(burstSize), "DRAM burst size %d is not allowed, "
             "must be a power of two\n", burstSize);

    // sanity check the ranks since we rely on bit slicing for the
    // address decoding
    fatal_if(!isPowerOf2(ranksPerChannel), "DRAM rank count of %d is "
             "not allowed, must be a power of two\n", ranksPerChannel);

    // numOfChannel = 8;
    // for (int i = 0; i < numOfChannel; i++) {
        // DPRINTF(MinirankDRAM, "Creating DRAM rank %d \n", i);
        ChannelDRAMInterface* channel_dram
                = new ChannelDRAMInterface(_p, 0, this,true);
        channels.push_back(channel_dram);
    // }
}

void
MinirankDRAMInterface::startup()
{
    DPRINTF(MinirankDRAM,"Minirank dram interface startup\n");

    for (auto m : channels){
        m->startup();
    }
}

void
MinirankDRAMInterface::setMRCtrl(MinirankMemCtrl* _ctrl)
{
    ctrl = _ctrl;
}

Addr
MinirankDRAMInterface::mapAddr(Addr addr)
{
    Addr ret;

    ret = (addr >> 9) << 6;
    return ret;
}

void
MinirankDRAMInterface::init()
{
    AbstractMemory::init();
    for (ChannelDRAMInterface* c : channels) {
        c->system(_system);
    }
    // a bit of sanity checks on the interleaving, save it for here to
    // ensure that the system pointer is initialised
    if (range.interleaved()) {
        if (addrMapping == enums::RoRaBaChCo) {
            if (rowBufferSize != range.granularity()) {
                fatal("Channel interleaving of %s doesn't match RoRaBaChCo "
                      "address map\n", name());
            }
        } else if (addrMapping == enums::RoRaBaCoCh ||
                   addrMapping == enums::RoCoRaBaCh) {
            // for the interleavings with channel bits in the bottom,
            // if the system uses a channel striping granularity that
            // is larger than the DRAM burst size, then map the
            // sequential accesses within a stripe to a number of
            // columns in the DRAM, effectively placing some of the
            // lower-order column bits as the least-significant bits
            // of the address (above the ones denoting the burst size)
            assert(burstsPerStripe >= 1);

            // channel striping has to be done at a granularity that
            // is equal or larger to a cache line
            if (system()->cacheLineSize() > range.granularity()) {
                fatal("Channel interleaving of %s must be at least as large "
                      "as the cache line size\n", name());
            }

            // ...and equal or smaller than the row-buffer size
            if (rowBufferSize < range.granularity()) {
                fatal("Channel interleaving of %s must be at most as large "
                      "as the row-buffer size\n", name());
            }
            // this is essentially the check above, so just to be sure
            assert(burstsPerStripe <= burstsPerRowBuffer);
        }
    }
}

void MinirankDRAMInterface::setupRank(const uint8_t rank, const bool is_read)
{
    channels[0]->setupRank(rank,is_read);
}

// DEBUG need to code differently?
// std::pair<MemPacketQueue::iterator, Tick>
// MinirankDRAMInterface::chooseNextFRFCFS(MemPacketQueue& queue,
//         Tick min_col_at) const
// {
//     auto selected_pkt_it = queue.end();
//     Tick selected_col_at = MaxTick;
//     std::tie(selected_pkt_it, selected_col_at) =
//     channels[0]->chooseNextFRFCFS(queue, min_col_at);
//     return std::make_pair(selected_pkt_it, selected_col_at);
// }

// DEBUG burst ready need to be ready seperately for each sub channel
// bool MinirankDRAMInterface::burstReady(MemPacket *pkt) const {
//         return channels[0]->burstReady(pkt);
// }

bool
MinirankDRAMInterface::allRanksDrained() const
{
    // true until proven false
    bool all_ranks_drained = true;
    all_ranks_drained = channels[0]->allRanksDrained();
    return all_ranks_drained;
}

} // namespce memory

} // namespace gem5
