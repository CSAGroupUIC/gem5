#ifndef __CHANNEL_DRAM_INTERFACE_HH__
#define __CHANNEL_DRAM_INTERFACE_HH__

#include <deque>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/compiler.hh"
#include "base/statistics.hh"
#include "enums/AddrMap.hh"
#include "enums/PageManage.hh"
#include "mem/channel_mem_ctrl.hh"
// #include "mem/minirank_dram_interface.hh"
#include "mem/drampower.hh"
#include "mem/mem_interface.hh"
#include "params/MinirankDRAMInterface.hh"
#include "sim/eventq.hh"

namespace gem5
{

namespace memory
{

class ChannelMemCtrl;
class DRAMInterface;
class MinirankDRAMInterface;
class ChannelDRAMInterface : public DRAMInterface
{
    friend class Rank;
    protected:

    /**
     * A pointer to the parent MemCtrl instance
     */
    ChannelMemCtrl* channelCtrl;

    unsigned numOfChannel;

    MinirankDRAMInterface* minirankInt;

    virtual void activateBank(Rank& rank_ref, Bank& bank_ref, Tick act_tick,
                uint32_t row) override;

    virtual void prechargeBank(Rank& rank_ref, Bank& bank_ref,
                Tick pre_tick, bool auto_or_preall = false,
                bool trace = true) override;

    struct ChannelDRAMStats : public statistics::Group
    {
        ChannelDRAMStats(MinirankDRAMInterface &dram, uint8_t raim_channel);

        void regStats() override;
        void resetStats() override;

        MinirankDRAMInterface &dram;

        /** total number of DRAM bursts serviced */
        statistics::Scalar readBursts;
        statistics::Scalar writeBursts;

        /** DRAM per bank stats */
        statistics::Vector perBankRdBursts;
        statistics::Vector perBankWrBursts;

        // Latencies summed over all requests
        statistics::Scalar totQLat;
        statistics::Scalar totBusLat;
        statistics::Scalar totMemAccLat;

        // Average latencies per request
        statistics::Formula avgQLat;
        statistics::Formula avgBusLat;
        statistics::Formula avgMemAccLat;

        // Row hit count and rate
        statistics::Scalar readRowHits;
        statistics::Scalar writeRowHits;
        statistics::Formula readRowHitRate;
        statistics::Formula writeRowHitRate;
        statistics::Histogram bytesPerActivate;
        // Number of bytes transferred to/from DRAM
        statistics::Scalar bytesRead;
        statistics::Scalar bytesWritten;

        // Average bandwidth
        statistics::Formula avgRdBW;
        statistics::Formula avgWrBW;
        statistics::Formula peakBW;
        // bus utilization
        statistics::Formula busUtil;
        statistics::Formula busUtilRead;
        statistics::Formula busUtilWrite;
        statistics::Formula pageHitRate;
    };

    ChannelDRAMStats stats;

    /**
     * Find which are the earliest banks ready to issue an activate
     * for the enqueued requests. Assumes maximum of 32 banks per rank
     * Also checks if the bank is already prepped.
     *
     * @param queue Queued requests to consider
     * @param min_col_at time of seamless burst command
     * @return One-hot encoded mask of bank indices
     * @return boolean indicating burst can issue seamlessly, with no gaps
     */
    virtual std::pair<std::vector<uint32_t>, bool>
    minBankPrep(const MemPacketQueue& queue, Tick min_col_at) const override;

    public:
    /**
     * Initialize the DRAM interface and verify parameters
     */
    void init() override;

    std::pair<Tick, Tick>
    doBurstAccess(MemPacket* mem_pkt, Tick next_burst_at,
                  const std::vector<MemPacketQueue>& queue);

    void setChannelCtrl(ChannelMemCtrl* _ctrl);
    ChannelDRAMInterface(const MinirankDRAMInterfaceParams &_p,
        uint8_t raim_channel = 0, MinirankDRAMInterface* raim = nullptr,
        bool is_raim = false);
};
}
}
#endif
