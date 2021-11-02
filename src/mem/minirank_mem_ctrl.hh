#ifndef __MINIRANK_MEM_CTRL_HH__
#define __MINIRANK_MEM_CTRL_HH__

#include <string>
#include <vector>

#include "mem/channel_mem_ctrl.hh"
#include "mem/mem_ctrl.hh"
#include "mem/qport.hh"
#include "params/MinirankMemCtrl.hh"

namespace gem5
{

namespace memory
{

class ChannelMemCtrl;

class MinirankMemCtrl : public ChannelMemCtrl
{
    protected:

    class MemoryPort : public QueuedResponsePort
    {
        RespPacketQueue queue;
        MinirankMemCtrl& ctrl;

        public:

        MemoryPort(const std::string& name, MinirankMemCtrl& _ctrl);

      protected:

        Tick recvAtomic(PacketPtr pkt) override;
        Tick recvAtomicBackdoor(
                PacketPtr pkt, MemBackdoorPtr &backdoor) override;

        void recvFunctional(PacketPtr pkt) override;

        bool recvTimingReq(PacketPtr) override;

        AddrRangeList getAddrRanges() const override;

    };

    /**
     * Sub-channels, channels in short
     */
    std::vector<ChannelMemCtrl *> minirankChannels;

    /**
     * Our incoming port, for a multi-ported controller add a crossbar
     * in front of it
     */
    MemoryPort port;

    /**
     * Remember if the memory system is in timing mode
     */
    bool isTimingMode;

    /**
     * Number of channels (sub-channels)
     */
    unsigned numOfChannels;

    /**
     * Address bus reserved slot map
     * Note: In real hardware design, it can be implemented in an
     * efficient way, e.g. as a bitmap in forms of shift register
     */
    std::set<Tick> addrBusReserveMap;

    /**
     * Remember if we have to retry a request when available.
     */
    bool retryRdReq;
    bool retryWrReq;

    public:

    MinirankMemCtrl(const MinirankMemCtrlParams &p);

    Port &getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;

    virtual void init() override;
    virtual void startup() override;
    virtual void drainResume() override;
    Tick scheduleAddrBus(Tick req_time);

    protected:

    Tick recvAtomic(PacketPtr pkt);
    void recvFunctional(PacketPtr pkt);
    bool recvTimingReq(PacketPtr pkt);

    private:
    uint8_t mapChannel(PacketPtr pkt);

    friend class ChannelMemCtrl;

};


} // namespace memory

} // namespace gem5
#endif // __MINIRANK_MEM_CTRL_HH__
