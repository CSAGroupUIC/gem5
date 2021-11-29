#ifndef __MINIRANK_DRAM_INTERFACE_HH__
#define __MINIRANK_DRAM_INTERFACE_HH__

#include <vector>

#include "mem/channel_dram_interface.hh"
#include "mem/channel_mem_ctrl.hh"
#include "mem/mem_interface.hh"
#include "mem/minirank_mem_ctrl.hh"
#include "params/MinirankDRAMInterface.hh"
#include "sim/eventq.hh"

namespace gem5
{

namespace memory
{

class MinirankMemCtrl;
class ChannelDRAMInterface;
class MinirankDRAMInterface : public MemInterface
{
    private:
    /**
     * A pointer to the parent MemCtrl instance
     */
    MinirankMemCtrl* ctrl;
    unsigned numOfChannel;

    std::vector<ChannelDRAMInterface* > channels;
    /**
     * DRAM specific device characteristics
     */
    const uint32_t bankGroupsPerRank;
    const bool bankGroupArch;

    /**
     * DRAM specific timing requirements
     */
    const Tick tCL;
    const Tick tBURST_MIN;
    const Tick tBURST_MAX;
    const Tick tCCD_L_WR;
    const Tick tCCD_L;
    const Tick tRCD;
    const Tick tRP;
    const Tick tRAS;
    const Tick tWR;
    const Tick tRTP;
    const Tick tRFC;
    const Tick tREFI;
    const Tick tRRD;
    const Tick tRRD_L;
    const Tick tPPD;
    const Tick tAAD;
    const Tick tXAW;
    const Tick tXP;
    const Tick tXS;
    const Tick clkResyncDelay;
    const bool dataClockSync;
    const bool burstInterleave;
    const uint8_t twoCycleActivate;
    const uint32_t activationLimit;
    const Tick wrToRdDlySameBG;
    const Tick rdToWrDlySameBG;


    enums::PageManage pageMgmt;
    /**
     * Max column accesses (read and write) per row, before forefully
     * closing it.
     */
    const uint32_t maxAccessesPerRow;

    // timestamp offset
    uint64_t timeStampOffset;

    // Holds the value of the DRAM rank of burst issued
    uint8_t activeRank;

    /** Enable or disable DRAM powerdown states. */
    bool enableDRAMPowerdown;

    public:

    void setCtrl(MinirankMemCtrl* _ctrl);
    void startup() override;
    void init() override;

    void setupRank(const uint8_t rank, const bool is_read) override;
    bool allRanksDrained() const override;

    std::pair<MemPacketQueue::iterator, Tick>
    chooseNextFRFCFS(MemPacketQueue& queue, Tick min_col_at) const override;

    Tick commandOffset() const override { return (tRP + tRCD); }
    Tick accessLatency() const override { return (tRP + tRCD + tCL); }

    bool burstReady(MemPacket *pkt) const override;
    void addRankToRankDelay(Tick cmd_at) override{}

    Addr mapAddr(Addr addr);
    MinirankDRAMInterface(const MinirankDRAMInterfaceParams &_p);
    ChannelDRAMInterface* getChannelDRAMInterface(uint8_t channel_number){
        return channels[channel_number];
    }
};
}
}
#endif
