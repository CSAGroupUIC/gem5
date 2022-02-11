#include "mem/minirank_dram_interface.hh"
#include "minirank_mem_ctrl.hh"

namespace gem5
{

namespace memory
{

MinirankMemCtrl::MinirankMemCtrl(const MinirankMemCtrlParams &p) :
    MemCtrl(p),
    port(name() + ".port", *this), isTimingMode(false),
    retryRdReq(false), retryWrReq(false),
    minirankDRAM(p.minirank_dram),
    stats(*this)
{
    // perform a basic check of the write thresholds
    if (p.write_low_thresh_perc >= p.write_high_thresh_perc)
        fatal("Write buffer low threshold %d must be smaller than the "
              "high threshold %d\n", p.write_low_thresh_perc,
              p.write_high_thresh_perc);
    if (minirankDRAM)
        minirankDRAM->setMRCtrl(this);
    assert(minirankDRAM);

    numOfChannels = 1;
    ChannelMemCtrl* channel_ctrl = new ChannelMemCtrl(p, true, this,
            0, numOfChannels);
    minirankChannels.push_back(channel_ctrl);
}

void
MinirankMemCtrl::init()
{
    for (ChannelMemCtrl* c : minirankChannels) {
        c->_system = _system;
    }
   if (!port.isConnected()) {
        fatal("Minirank port  %s is unconnected!\n", name());
    } else {
        port.sendRangeChange();
    }

}

void
MinirankMemCtrl::startup()
{
    isTimingMode = system()->isTimingMode();
    for (ChannelMemCtrl* c : minirankChannels) {
        c->startup();
    }
}

MinirankMemCtrl::MinirankCtrlStats::MinirankCtrlStats(
          MinirankMemCtrl &_ctrl)
    : Stats::Group(&_ctrl),
    ctrl(_ctrl),
    ADD_STAT(parityUpdates, statistics::units::Count::get(),
             "Total number of parity updates")
{
}

void
MinirankMemCtrl::MinirankCtrlStats::regStats()
{
    using namespace statistics;
}

Tick
MinirankMemCtrl::recvAtomic(PacketPtr pkt)
{
    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");

    Tick latency = 0;
    // do the actual memory access and turn the packet into a response
    if (minirankDRAM && minirankDRAM->getAddrRange().contains(
                pkt->getAddr())) {

        minirankDRAM->access(pkt);

        if (pkt->hasData()) {
            // this value is not supposed to be accurate, just enough to
            // keep things going, mimic a closed page
            latency = minirankDRAM->accessLatency();
        }
    } else {
        panic("Can't handle address range for packet %s\n",
              pkt->print());
    }

    return latency;
}

uint8_t
MinirankMemCtrl::mapChannel(PacketPtr pkt)
{
    // map into different sub level channel ctrls
    // FIXME add mapping scheme
    uint8_t channel;
    channel = 0;
    return channel;
}

bool
MinirankMemCtrl::recvTimingReq(PacketPtr pkt)
{
    // This is where we enter from the outside world

    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");

    panic_if(!(pkt->isRead() || pkt->isWrite()),
             "Should only see read and writes at memory controller\n");

    // Get the channel number, send the packet to the channel
    uint8_t channel = mapChannel(pkt);
    bool ret_code = minirankChannels[channel]->recvTimingReq(pkt);
    assert(ret_code);

    return true;
}

Tick
MinirankMemCtrl::scheduleAddrBus(Tick req_time)
{
    auto& reserve_map = this->mrAddrBusReserveMap;

    // Purse the reserve map every N = 1000 calls
    static int call_count = 0;
    if (++call_count > 1000) {
        Tick now = divCeil(curTick(), minirankDRAM->checkTCK());
        for (auto it = reserve_map.begin(); it != reserve_map.end(); ++it) {
            if (*it < now) // Fixme
                reserve_map.erase(it);
        }
        call_count = 0;
    }

    // Find the first available bus slot from the request time
    // Note: Reserve map usage is based on DRAM bus clock, one slot for each
    // bus clock cycle
    // FIXME Using std::bitset is much more efficient than using std::set
    Tick bus_slot = divCeil(req_time, minirankDRAM->checkTCK());
    while (reserve_map.find(bus_slot) != reserve_map.end()) {
        ++bus_slot;
    }
    reserve_map.insert(bus_slot);

    // Return the granted time
    Tick granted_time = bus_slot * minirankDRAM->checkTCK();
    return granted_time;
}

void
MinirankMemCtrl::recvFunctional(PacketPtr pkt)
{
    if (minirankDRAM &&
              minirankDRAM->getAddrRange().contains(pkt->getAddr()))
    {
        // rely on the abstract memory
        minirankDRAM->functionalAccess(pkt);
    } else {
        panic("Can't handle address range for packet %s\n",
              pkt->print());
   }
}


DrainState
MinirankMemCtrl::drain()
{
    DrainState state = DrainState::Drained;

    for (MemCtrl* c : minirankChannels) {
        if (c->drain() == DrainState::Draining)
            state = DrainState::Draining;
    }

    return state;
}

void
MinirankMemCtrl::drainResume()
{
    for (MemCtrl* c :minirankChannels) {
        c->drainResume();
    }
}

Port &
MinirankMemCtrl::getPort(const std::string &if_name, PortID idx)
{
    if (if_name != "port") {
        return qos::MemCtrl::getPort(if_name, idx);
    } else {
        return port;
    }
}

MinirankMemCtrl::MemoryPort::MemoryPort(const std::string& name, MinirankMemCtrl& _ctrl)
    : QueuedResponsePort(name, &_ctrl, queue), queue(_ctrl, *this, true),
      ctrl(_ctrl)
{ }

AddrRangeList
MinirankMemCtrl::MemoryPort::getAddrRanges() const
{
    AddrRangeList ranges;
    // FIXME return extended range ranges
    if (ctrl.minirankDRAM) {
        ranges.push_back(ctrl.minirankDRAM->getAddrRange());
    }
    return ranges;
}

void
MinirankMemCtrl::MemoryPort::recvFunctional(PacketPtr pkt)
{
    pkt->pushLabel(ctrl.name());

    if (!queue.trySatisfyFunctional(pkt)) {
        // Default implementation of SimpleTimingPort::recvFunctional()
        // calls recvAtomic() and throws away the latency; we can save a
        // little here by just not calculating the latency.
        ctrl.recvFunctional(pkt);
    }

    pkt->popLabel();
}


Tick
MinirankMemCtrl::recvAtomicBackdoor(PacketPtr pkt, MemBackdoorPtr &backdoor)
{
    Tick latency = recvAtomic(pkt);

    minirankDRAM->getBackdoor(backdoor);
    return latency;
}

Tick
MinirankMemCtrl::MemoryPort::recvAtomic(PacketPtr pkt)
{
    return ctrl.recvAtomic(pkt);
}


Tick
MinirankMemCtrl::MemoryPort::recvAtomicBackdoor(
        PacketPtr pkt, MemBackdoorPtr &backdoor)
{
    return ctrl.recvAtomicBackdoor(pkt, backdoor);
}

bool
MinirankMemCtrl::MemoryPort::recvTimingReq(PacketPtr pkt)
{
    // pass it to the memory controller
    return ctrl.recvTimingReq(pkt);
}

} // namespce memory

} // namespace gem5
