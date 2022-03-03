/*
 * Copyright (c) 2012-2020 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2013 Amin Farmahini-Farahani
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * MemCtrl declaration
 */

#ifndef __CHANNEL_MEM_CTRL_HH__
#define __CHANNEL_MEM_CTRL_HH__

#include <deque>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/callback.hh"
#include "base/statistics.hh"
#include "enums/MemSched.hh"
#include "mem/channel_dram_interface.hh"
#include "mem/mem_ctrl.hh"
#include "mem/mem_interface.hh"

// #include "mem/minirank_dram_interface.hh"
#include "mem/qport.hh"
#include "params/MemCtrl.hh"
#include "params/MinirankMemCtrl.hh"
#include "sim/eventq.hh"

namespace gem5
{

namespace memory
{

class DRAMInterface;
class NVMInterface;
class BurstHelpler;
class MemPacket;
class MinirankMemCtrl;
class MinirankDRAMInterface;
class ChannelDRAMInterface;

// The memory packets are store in a multiple dequeue structure,
// based on their QoS priority
typedef std::deque<MemPacket*> MemPacketQueue;


/**
 * The memory controller is a single-channel memory controller capturing
 * the most important timing constraints associated with a
 * contemporary controller. For multi-channel memory systems, the controller
 * is combined with a crossbar model, with the channel address
 * interleaving taking part in the crossbar.
 *
 * As a basic design principle, this controller
 * model is not cycle callable, but instead uses events to: 1) decide
 * when new decisions can be made, 2) when resources become available,
 * 3) when things are to be considered done, and 4) when to send
 * things back. The controller interfaces to media specific interfaces
 * to enable flexible topoloties.
 * Through these simple principles, the model delivers
 * high performance, and lots of flexibility, allowing users to
 * evaluate the system impact of a wide range of memory technologies.
 *
 * For more details, please see Hansson et al, "Simulating DRAM
 * controllers for future system architecture exploration",
 * Proc. ISPASS, 2014. If you use this model as part of your research
 * please cite the paper.
 *
 */
class ChannelMemCtrl : public MemCtrl
{
  friend class MinirankMemCtrl;
  private:

    // // For now, make use of a queued response port to avoid dealing with
    // // flow control for the responses being sent back
    // class MemoryPort : public QueuedResponsePort
    // {

    //     RespPacketQueue queue;
    //     ChannelMemCtrl& ctrl;

    //   public:

    //     MemoryPort(const std::string& name, ChannelMemCtrl& _ctrl);

    //   protected:

    //     Tick recvAtomic(PacketPtr pkt) override;
    //     Tick recvAtomicBackdoor(
    //             PacketPtr pkt, MemBackdoorPtr &backdoor) override;

    //     void recvFunctional(PacketPtr pkt) override;

    //     bool recvTimingReq(PacketPtr) override;

    //     AddrRangeList getAddrRanges() const override;

    // };

    /**
     * Bunch of things requires to setup "events" in gem5
     * When event "respondEvent" occurs for example, the method
     * processRespondEvent is called; no parameters are allowed
     * in these methods
     */
    virtual void processNextReqEvent() override;
    // EventFunctionWrapper nextReqEvent;

    virtual void processRespondEvent() override;
    // EventFunctionWrapper respondEvent;

    /**
     * Check if the read queue has room for more entries
     *
     * @param pkt_count The number of entries needed in the read queue
     * @return true if read queue is full, false otherwise
     */
    bool readQueueFull(unsigned int pkt_count) const;

    /**
     * Check if the write queue has room for more entries
     *
     * @param pkt_count The number of entries needed in the write queue
     * @return true if write queue is full, false otherwise
     */
    bool writeQueueFull(unsigned int pkt_count) const;

    /**
     * When a new read comes in, first check if the write q has a
     * pending request to the same address.\ If not, decode the
     * address to populate rank/bank/row, create one or mutliple
     * "mem_pkt", and push them to the back of the read queue.\
     * If this is the only
     * read request in the system, schedule an event to start
     * servicing it.
     *
     * @param pkt The request packet from the outside world
     * @param pkt_count The number of memory bursts the pkt
     * @param is_dram Does this packet access DRAM?
     * translate to. If pkt size is larger then one full burst,
     * then pkt_count is greater than one.
     */
    void addToReadQueue(PacketPtr pkt, unsigned int pkt_count, bool is_dram);

    /**
     * Decode the incoming pkt, create a mem_pkt and push to the
     * back of the write queue. \If the write q length is more than
     * the threshold specified by the user, ie the queue is beginning
     * to get full, stop reads, and start draining writes.
     *
     * @param pkt The request packet from the outside world
     * @param pkt_count The number of memory bursts the pkt
     * @param is_dram Does this packet access DRAM?
     * translate to. If pkt size is larger then one full burst,
     * then pkt_count is greater than one.
     */
    void addToWriteQueue(PacketPtr pkt, unsigned int pkt_count, bool is_dram);

    /**
     * Actually do the burst based on media specific access function.
     * Update bus statistics when complete.
     *
     * @param mem_pkt The memory packet created from the outside world pkt
     */
    void doBurstAccess(MemPacket* mem_pkt);

    /**
     * When a packet reaches its "readyTime" in the response Q,
     * use the "access()" method in AbstractMemory to actually
     * create the response packet, and send it back to the outside
     * world requestor.
     *
     * @param pkt The packet from the outside world
     * @param static_latency Static latency to add before sending the packet
     */
    void accessAndRespond(PacketPtr pkt, Tick static_latency);

    /**
     * Determine if there is a packet that can issue.
     *
     * @param pkt The packet to evaluate
     */
    bool packetReady(MemPacket* pkt);

    /**
     * Calculate the minimum delay used when scheduling a read-to-write
     * transision.
     * @param return minimum delay
     */
    Tick minReadToWriteDataGap();

    /**
     * Calculate the minimum delay used when scheduling a write-to-read
     * transision.
     * @param return minimum delay
     */
    Tick minWriteToReadDataGap();

    /**
     * The memory schduler/arbiter - picks which request needs to
     * go next, based on the specified policy such as FCFS or FR-FCFS
     * and moves it to the head of the queue.
     * Prioritizes accesses to the same rank as previous burst unless
     * controller is switching command type.
     *
     * @param queue Queued requests to consider
     * @param extra_col_delay Any extra delay due to a read/write switch
     * @return an iterator to the selected packet, else queue.end()
     */
    MemPacketQueue::iterator chooseNext(MemPacketQueue& queue,
        Tick extra_col_delay);

    /**
     * For FR-FCFS policy reorder the read/write queue depending on row buffer
     * hits and earliest bursts available in memory
     *
     * @param queue Queued requests to consider
     * @param extra_col_delay Any extra delay due to a read/write switch
     * @return an iterator to the selected packet, else queue.end()
     */
    MemPacketQueue::iterator chooseNextFRFCFS(MemPacketQueue& queue,
            Tick extra_col_delay);
    /**
     * Used for debugging to observe the contents of the queues.
     */
    void printQs() const;

    /**
     * Burst-align an address.
     *
     * @param addr The potentially unaligned address
     * @param is_dram Does this packet access DRAM?
     *
     * @return An address aligned to a memory burst
     */
    Addr burstAlign(Addr addr, bool is_channel_dram) const;

    // /**
    //  * The following are basic design parameters of the memory
    //  * controller, and are initialized based on parameter values.
    //  * The rowsPerBank is determined based on the capacity, number of
    //  * ranks and banks, the burst size, and the row buffer size.
    //  */
    // const uint32_t readBufferSize;
    // const uint32_t writeBufferSize;
    // const uint32_t writeHighThreshold;
    // const uint32_t writeLowThreshold;
    // const uint32_t minWritesPerSwitch;
    // uint32_t writesThisTime;
    // uint32_t readsThisTime;

    struct CtrlStats : public statistics::Group
    {
        CtrlStats(MinirankMemCtrl &ctrl, uint8_t minirankChannel);

        void regStats() override;

        MinirankMemCtrl &ctrl;

        // All statistics that the model needs to capture
        statistics::Scalar readReqs;
        statistics::Scalar writeReqs;
        statistics::Scalar readBursts;
        statistics::Scalar writeBursts;
        statistics::Scalar servicedByWrQ;
        statistics::Scalar mergedWrBursts;
        statistics::Scalar neitherReadNorWriteReqs;
        // Average queue lengths
        statistics::Average avgRdQLen;
        statistics::Average avgWrQLen;

        statistics::Scalar numRdRetry;
        statistics::Scalar numWrRetry;
        statistics::Vector readPktSize;
        statistics::Vector writePktSize;
        statistics::Vector rdQLenPdf;
        statistics::Vector wrQLenPdf;
        statistics::Histogram rdPerTurnAround;
        statistics::Histogram wrPerTurnAround;

        statistics::Scalar bytesReadWrQ;
        statistics::Scalar bytesReadSys;
        statistics::Scalar bytesWrittenSys;
        // Average bandwidth
        statistics::Formula avgRdBWSys;
        statistics::Formula avgWrBWSys;

        statistics::Scalar totGap;
        statistics::Formula avgGap;

        // per-requestor bytes read and written to memory
        statistics::Vector requestorReadBytes;
        statistics::Vector requestorWriteBytes;

        // per-requestor bytes read and written to memory rate
        statistics::Formula requestorReadRate;
        statistics::Formula requestorWriteRate;

        // per-requestor read and write serviced memory accesses
        statistics::Vector requestorReadAccesses;
        statistics::Vector requestorWriteAccesses;

        // per-requestor read and write total memory access latency
        statistics::Vector requestorReadTotalLat;
        statistics::Vector requestorWriteTotalLat;

        // per-requestor raed and write average memory access latency
        statistics::Formula requestorReadAvgLat;
        statistics::Formula requestorWriteAvgLat;
    };

    uint8_t minirankChannel;
    MinirankDRAMInterface* minirankDRAM;
    MinirankMemCtrl* minirank;

    ChannelDRAMInterface* channel_dram;
    CtrlStats stats;

  public:

    ChannelMemCtrl(const MinirankMemCtrlParams &p, bool _subranked = false,
        MinirankMemCtrl* _minirank = nullptr, uint8_t minirank_channel = 0,
        unsigned numOfChannels = 0);

    /**
     * Ensure that all interfaced have drained commands
     *
     * @return bool flag, set once drain complete
     */
    bool allIntfDrained() const;

    DrainState drain() override;
    virtual void init() override;
    virtual void startup() override;
    virtual void drainResume() override;

    MinirankMemCtrl* getMinirankMemCtrl(){  return minirank;}
  protected:

    bool recvTimingReq(PacketPtr pkt);
};

} // namespace memory
} // namespace gem5

#endif //__MEM_CTRL_HH__
