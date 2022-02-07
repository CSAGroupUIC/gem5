#ifndef __MINIRANK_DRAM_INTERFACE_HH__
#define __MINIRANK_DRAM_INTERFACE_HH__

#include <vector>

// #include "mem/channel_dram_interface.hh"
#include "mem/channel_mem_ctrl.hh"
#include "mem/mem_interface.hh"
// #include "mem/minirank_mem_ctrl.hh"
#include "params/MinirankDRAMInterface.hh"
#include "sim/eventq.hh"

namespace gem5
{

namespace memory
{

class MinirankMemCtrl;
// class DRAMInterface;
class ChannelDRAMInterface;
class MinirankDRAMInterface : public DRAMInterface
{
    private:
    /**
     * A pointer to the parent MemCtrl instance
     */
    MinirankMemCtrl* ctrl;
    unsigned numOfChannel;

    std::vector<ChannelDRAMInterface* > channels;

    protected:

    struct MinirankDRAMStats : public statistics::Group
    {
        MinirankDRAMStats(MinirankDRAMInterface &_dram);

        void regStats() override;
        void resetStats() override;

        MinirankDRAMInterface &dram;
        Stats::Scalar parityDRAM;
    };

    MinirankDRAMStats stats;

    public:

    void setMRCtrl(MinirankMemCtrl* _ctrl);
    void startup() override;
    void init() override;

    void setupRank(const uint8_t rank, const bool is_read) override;
    bool allRanksDrained() const override;

    // std::pair<MemPacketQueue::iterator, Tick>
    // chooseNextFRFCFS(MemPacketQueue& queue, Tick min_col_at) const override;

    // Tick commandOffset() const override { return (tRP + tRCD); }
    // Tick accessLatency() const override { return (tRP + tRCD + tCL); }

    // bool burstReady(MemPacket *pkt) const override;
    // void addRankToRankDelay(Tick cmd_at) override{}

    Addr mapAddr(Addr addr);
    MinirankDRAMInterface(const MinirankDRAMInterfaceParams &_p);
    ChannelDRAMInterface* getChannelDRAMInterface(uint8_t channel_number){
        return channels[channel_number];
    }
};
}
}
#endif
