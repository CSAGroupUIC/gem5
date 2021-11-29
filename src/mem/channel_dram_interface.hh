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
#include "mem/abstract_mem.hh"
#include "mem/channel_mem_ctrl.hh"
#include "mem/drampower.hh"
#include "mem/mem_interface.hh"
#include "params/MinirankDRAMInterface.hh"
#include "sim/eventq.hh"

namespace gem5
{

namespace memory
{

class ChannelMemCtrl;
class ChannelDRAMInterface : public MemInterface
{
    protected:
    /**
     * A pointer to the parent MemCtrl instance
     */
    ChannelMemCtrl* ctrl;
    unsigned numOfChannel;

    private:
    /**
     * Simple structure to hold the values needed to keep track of
     * commands for DRAMPower
     */
    struct Command
    {
       Data::MemCommand::cmds type;
       uint8_t bank;
       Tick timeStamp;

       constexpr Command(Data::MemCommand::cmds _type, uint8_t _bank,
                         Tick time_stamp)
            : type(_type), bank(_bank), timeStamp(time_stamp)
        { }
    };

    enum PowerState
    {
        PWR_IDLE = 0,
        PWR_REF,
        PWR_SREF,
        PWR_PRE_PDN,
        PWR_ACT,
        PWR_ACT_PDN
    };

    enum RefreshState
    {
        REF_IDLE = 0,
        REF_DRAIN,
        REF_PD_EXIT,
        REF_SREF_EXIT,
        REF_PRE,
        REF_START,
        REF_RUN
    };

    class Rank;
    struct RankStats : public statistics::Group
    {
        RankStats(ChannelDRAMInterface &channel_dram, Rank &rank);

        void regStats() override;
        void resetStats() override;
        void preDumpStats() override;

        Rank &rank;

        /*
         * Command energies
         */
        statistics::Scalar actEnergy;
        statistics::Scalar preEnergy;
        statistics::Scalar readEnergy;
        statistics::Scalar writeEnergy;
        statistics::Scalar refreshEnergy;

        /*
         * Active Background Energy
         */
        statistics::Scalar actBackEnergy;

        /*
         * Precharge Background Energy
         */
        statistics::Scalar preBackEnergy;

        /*
         * Active Power-Down Energy
         */
        statistics::Scalar actPowerDownEnergy;
        /*
         * Precharge Power-Down Energy
         */
        statistics::Scalar prePowerDownEnergy;

        /*
         * self Refresh Energy
         */
        statistics::Scalar selfRefreshEnergy;

        statistics::Scalar totalEnergy;
        statistics::Scalar averagePower;

        /**
         * Stat to track total DRAM idle time
         *
         */
        statistics::Scalar totalIdleTime;

        /**
         * Track time spent in each power state.
         */
        statistics::Vector pwrStateTime;
    };

    /**
     * Rank class includes a vector of banks. Refresh and Power state
     * machines are defined per rank. Events required to change the
     * state of the refresh and power state machine are scheduled per
     * rank. This class allows the implementation of rank-wise refresh
     * and rank-wise power-down.
     */
    class Rank : public EventManager
    {
      private:

        /**
         * A reference to the parent DRAMInterface instance
         */
        ChannelDRAMInterface& channel_dram;

        /**
         * Since we are taking decisions out of order, we need to keep
         * track of what power transition is happening at what time
         */
        PowerState pwrStateTrans;

        /**
         * Previous low-power state, which will be re-entered after refresh.
         */
        PowerState pwrStatePostRefresh;

        /**
         * Track when we transitioned to the current power state
         */
        Tick pwrStateTick;

        /**
         * Keep track of when a refresh is due.
         */
        Tick refreshDueAt;

        /**
         * Function to update Power Stats
         */
        void updatePowerStats();

        /**
         * Schedule a power state transition in the future, and
         * potentially override an already scheduled transition.
         *
         * @param pwr_state Power state to transition to
         * @param tick Tick when transition should take place
         */
        void schedulePowerEvent(PowerState pwr_state, Tick tick);

      public:

        /**
         * Current power state.
         */
        PowerState pwrState;

       /**
         * current refresh state
         */
        RefreshState refreshState;

        /**
         * rank is in or transitioning to power-down or self-refresh
         */
        bool inLowPowerState;

        /**
         * Current Rank index
         */
        uint8_t rank;

       /**
         * Track number of packets in read queue going to this rank
         */
        uint32_t readEntries;

       /**
         * Track number of packets in write queue going to this rank
         */
        uint32_t writeEntries;

        /**
         * Number of ACT, RD, and WR events currently scheduled
         * Incremented when a refresh event is started as well
         * Used to determine when a low-power state can be entered
         */
        uint8_t outstandingEvents;

        /**
         * delay low-power exit until this requirement is met
         */
        Tick wakeUpAllowedAt;

        /**
         * One DRAMPower instance per rank
         */
        ChannelDRAMPower power;

        /**
         * List of commands issued, to be sent to DRAMPpower at refresh
         * and stats dump.  Keep commands here since commands to different
         * banks are added out of order.  Will only pass commands up to
         * curTick() to DRAMPower after sorting.
         */
        std::vector<Command> cmdList;

        /**
         * Vector of Banks. Each rank is made of several devices which in
         * term are made from several banks.
         */
        std::vector<Bank> banks;

        /**
         *  To track number of banks which are currently active for
         *  this rank.
         */
        unsigned int numBanksActive;

        /** List to keep track of activate ticks */
        std::deque<Tick> actTicks;

        /**
         * Track when we issued the last read/write burst
         */
        Tick lastBurstTick;

        Rank(const MinirankDRAMInterfaceParams &_p, int _rank,
             ChannelDRAMInterface& _dram);

        const std::string name() const { return csprintf("%d", rank); }

        /**
         * Kick off accounting for power and refresh states and
         * schedule initial refresh.
         *
         * @param ref_tick Tick for first refresh
         */
        void startup(Tick ref_tick);

        /**
         * Stop the refresh events.
         */
        void suspend();

        /**
         * Check if there is no refresh and no preparation of refresh ongoing
         * i.e. the refresh state machine is in idle
         *
         * @param Return true if the rank is idle from a refresh point of view
         */
        bool inRefIdleState() const { return refreshState == REF_IDLE; }

        /**
         * Check if the current rank has all banks closed and is not
         * in a low power state
         *
         * @param Return true if the rank is idle from a bank
         *        and power point of view
         */
        bool inPwrIdleState() const { return pwrState == PWR_IDLE; }

        /**
         * Trigger a self-refresh exit if there are entries enqueued
         * Exit if there are any read entries regardless of the bus state.
         * If we are currently issuing write commands, exit if we have any
         * write commands enqueued as well.
         * Could expand this in the future to analyze state of entire queue
         * if needed.
         *
         * @return boolean indicating self-refresh exit should be scheduled
         */
        bool forceSelfRefreshExit() const;

        /**
         * Check if the command queue of current rank is idle
         *
         * @param Return true if the there are no commands in Q.
         *                    Bus direction determines queue checked.
         */
        bool isQueueEmpty() const;

        /**
         * Let the rank check if it was waiting for requests to drain
         * to allow it to transition states.
         */
        void checkDrainDone();

        /**
         * Push command out of cmdList queue that are scheduled at
         * or before curTick() to DRAMPower library
         * All commands before curTick are guaranteed to be complete
         * and can safely be flushed.
         */
        void flushCmdList();

        /**
         * Computes stats just prior to dump event
         */
        void computeStats();

        /**
         * Reset stats on a stats event
         */
        void resetStats();

        /**
         * Schedule a transition to power-down (sleep)
         *
         * @param pwr_state Power state to transition to
         * @param tick Absolute tick when transition should take place
         */
        void powerDownSleep(PowerState pwr_state, Tick tick);

       /**
         * schedule and event to wake-up from power-down or self-refresh
         * and update bank timing parameters
         *
         * @param exit_delay Relative tick defining the delay required between
         *                   low-power exit and the next command
         */
        void scheduleWakeUpEvent(Tick exit_delay);

        void processWriteDoneEvent();
        EventFunctionWrapper writeDoneEvent;

        void processActivateEvent();
        EventFunctionWrapper activateEvent;

        void processPrechargeEvent();
        EventFunctionWrapper prechargeEvent;

        void processRefreshEvent();
        EventFunctionWrapper refreshEvent;

        void processPowerEvent();
        EventFunctionWrapper powerEvent;

        void processWakeUpEvent();
        EventFunctionWrapper wakeUpEvent;

      protected:
        RankStats stats;
    };

    /**
     * Function for sorting Command structures based on timeStamp
     *
     * @param a Memory Command
     * @param next Memory Command
     * @return true if timeStamp of Command 1 < timeStamp of Command 2
     */
    static bool
    sortTime(const Command& cmd, const Command& cmd_next)
    {
        return cmd.timeStamp < cmd_next.timeStamp;
    }
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

    /** The time when stats were last reset used to calculate average power */
    Tick lastStatsResetTick;

    void activateBank(Rank& rank_ref, Bank& bank_ref, Tick act_tick,
                uint32_t row);

    void prechargeBank(Rank& rank_ref, Bank& bank_ref,
                Tick pre_tick, bool auto_or_preall = false,
                bool trace = true);

    struct DRAMStats : public statistics::Group
    {
        DRAMStats(ChannelDRAMInterface &dram);

        void regStats() override;
        void resetStats() override;

        ChannelDRAMInterface &dram;

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

    DRAMStats stats;
    /**
      * Vector of dram ranks
      */
    std::vector<Rank*> ranks;

    /*
     * @return delay between write and read commands
     */
    Tick writeToReadDelay() const override { return tBURST + tWTR + tCL; }

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
    std::pair<std::vector<uint32_t>, bool>
    minBankPrep(const MemPacketQueue& queue, Tick min_col_at) const;

    Tick
    burstDelay() const
    {
        return (burstInterleave ? tBURST_MAX / 2 : tBURST);
    }

    public:
    /**
     * Initialize the DRAM interface and verify parameters
     */
    void init() override;

    /**
     * Iterate through dram ranks and instantiate per rank startup routine
     */
    void startup() override;

    /**
     * Setup the rank based on packet received
     *
     * @param integer value of rank to be setup. used to index ranks vector
     * @param are we setting up rank for read or write packet?
     */
    void setupRank(const uint8_t rank, const bool is_read) override;

    /**
     * Iterate through dram ranks to exit self-refresh in order to drain
     */
    void drainRanks();

    bool allRanksDrained() const override;

    /**
     * Iterate through DRAM ranks and suspend them
     */
    void suspend();

    /*
     * @return time to offset next command
     */
    Tick commandOffset() const override { return (tRP + tRCD); }

    /*
     * Function to calulate unloaded, closed bank access latency
     */
    Tick accessLatency() const override { return (tRP + tRCD + tCL); }

    std::pair<MemPacketQueue::iterator, Tick>
    chooseNextFRFCFS(MemPacketQueue& queue, Tick min_col_at) const override;

    std::pair<Tick, Tick>
    doBurstAccess(MemPacket* mem_pkt, Tick next_burst_at,
                  const std::vector<MemPacketQueue>& queue);

    bool
    burstReady(MemPacket* pkt) const override
    {
        return ranks[pkt->rank]->inRefIdleState();
    }

    bool isBusy();
    void addRankToRankDelay(Tick cmd_at) override;
    void respondEvent(uint8_t rank);
    void checkRefreshState(uint8_t rank);
    void setCtrl(ChannelMemCtrl* _ctrl);
    ChannelDRAMInterface(const MinirankDRAMInterfaceParams &_p,
        uint8_t raim_channel = 0, MinirankDRAMInterface* raim = nullptr);
};
}
}
#endif
