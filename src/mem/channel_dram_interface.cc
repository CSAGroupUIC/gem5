#include "mem/channel_dram_interface.hh"

#include "base/bitfield.hh"
#include "base/cprintf.hh"
#include "base/trace.hh"
#include "debug/ChannelDRAM.hh"
#include "debug/DRAMPower.hh"
#include "debug/DRAMState.hh"
#include "mem/minirank_dram_interface.hh"
#include "mem/minirank_mem_ctrl.hh"
#include "sim/system.hh"

namespace gem5
{

using namespace Data;
namespace memory
{

ChannelDRAMInterface::ChannelDRAMInterface(
        const MinirankDRAMInterfaceParams &_p,
        uint8_t raim_channel, MinirankDRAMInterface* raim, bool is_raim)
    : DRAMInterface(_p, is_raim),
      raimChannel(raim_channel),
      minirankInt(raim),
      stats(*minirankInt)
{
    DPRINTF(ChannelDRAM, "Setting up channel DRAM Interface\n");

    fatal_if(!isPowerOf2(burstSize), "DRAM burst size %d is not allowed, "
             "must be a power of two\n", burstSize);

    // sanity check the ranks since we rely on bit slicing for the
    // address decoding
    fatal_if(!isPowerOf2(ranksPerChannel), "DRAM rank count of %d is "
             "not allowed, must be a power of two\n", ranksPerChannel);

    isRaim = is_raim;
    for (int i = 0; i < ranksPerChannel; i++) {
        DPRINTF(ChannelDRAM, "Creating channel DRAM rank %d \n", i);
        ChannelRank* rank = new ChannelRank(_p, i,*this, true);
        ranks.push_back(rank);
    }

    // determine the dram actual capacity from the DRAM config in Mbytes+
    uint64_t deviceCapacity = deviceSize / (1024 * 1024) * devicesPerRank *
                              ranksPerChannel;

    uint64_t capacity = deviceCapacity * 1024 * 1024;

    DPRINTF(ChannelDRAM, "rowbuffer size %d banksPerRank %d ranksPerChannel"
                "%d\n", rowBufferSize, banksPerRank, ranksPerChannel);
    DPRINTF(ChannelDRAM, "t burst %d devicesPerRaimChannel\n",
            tBURST, devicesPerRank);

    rowsPerBank = capacity / (rowBufferSize * banksPerRank * ranksPerChannel);

    // some basic sanity checks
    if (tREFI <= tRP || tREFI <= tRFC) {
        fatal("tREFI (%d) must be larger than tRP (%d) and tRFC (%d)\n",
              tREFI, tRP, tRFC);
    }

    // basic bank group architecture checks ->
    if (bankGroupArch) {
        // must have at least one bank per bank group
        if (bankGroupsPerRank > banksPerRank) {
            fatal("banks per rank (%d) must be equal to or larger than "
                  "banks groups per rank (%d)\n",
                  banksPerRank, bankGroupsPerRank);
        }
        // must have same number of banks in each bank group
        if ((banksPerRank % bankGroupsPerRank) != 0) {
            fatal("Banks per rank (%d) must be evenly divisible by bank "
                  "groups per rank (%d) for equal banks per bank group\n",
                  banksPerRank, bankGroupsPerRank);
        }
        // tCCD_L should be greater than minimal, back-to-back burst delay
        if (tCCD_L < tBURST) {
            fatal("tCCD_L (%d) should be larger than the minimum bus delay "
                  "(%d) when bank groups per rank (%d) is greater than 1\n",
                  tCCD_L, tBURST, bankGroupsPerRank);
        }
        // tCCD_L_WR should be greater than minimal, back-to-back burst delay
        if (tCCD_L_WR < tBURST) {
            fatal("tCCD_L_WR (%d) should be larger than the minimum bus delay "
                  " (%d) when bank groups per rank (%d) is greater than 1\n",
                  tCCD_L_WR, tBURST, bankGroupsPerRank);
        }
        // tRRD_L is greater than minimal, same bank group ACT-to-ACT delay
        // some datasheets might specify it equal to tRRD
        if (tRRD_L < tRRD) {
            fatal("tRRD_L (%d) should be larger than tRRD (%d) when "
                  "bank groups per rank (%d) is greater than 1\n",
                  tRRD_L, tRRD, bankGroupsPerRank);
        }
    }
}

void
ChannelDRAMInterface::init()
{
    // init should not be called in the lower level
    DPRINTF(ChannelDRAM,"channel dram interface init\n");
}
void
ChannelDRAMInterface::setChannelCtrl(ChannelMemCtrl* _ctrl)
{
    channelCtrl = _ctrl;
}

void
ChannelDRAMInterface::activateBank(Rank& rank_ref, Bank& bank_ref,
                       Tick act_tick, uint32_t row)
{
    DPRINTF(ChannelDRAM,"channel dram interface activate bank\n");

    assert(rank_ref.actTicks.size() == activationLimit);

    // verify that we have command bandwidth to issue the activate
    // if not, shift to next burst window
    Tick act_at;
    if (twoCycleActivate)
    {
        // act_at = channelCtrl->verifyMultiCmd(act_tick,
                // maxCommandsPerWindow, tAAD);
    }else{
        // act_at = channelCtrl->verifySingleCmd(act_tick,
                // maxCommandsPerWindow);
        act_at = (channelCtrl->getMinirankMemCtrl())
                        ->scheduleAddrBus(act_tick);
    }
    DPRINTF(ChannelDRAM, "Activate at tick %d\n", act_at);

    // update the open row
    assert(bank_ref.openRow == Bank::NO_ROW);
    bank_ref.openRow = row;

    // start counting anew, this covers both the case when we
    // auto-precharged, and when this access is forced to
    // precharge
    bank_ref.bytesAccessed = 0;
    bank_ref.rowAccesses = 0;

    ++rank_ref.numBanksActive;
    assert(rank_ref.numBanksActive <= banksPerRank);

    DPRINTF(ChannelDRAM, "Activate bank %d, "
            "rank %d at tick %lld, now got "
            "%d active\n", bank_ref.bank, rank_ref.rank, act_at,
            ranks[rank_ref.rank]->numBanksActive);

    rank_ref.cmdList.push_back(Command(MemCommand::ACT, bank_ref.bank,
                               act_at));

    DPRINTF(DRAMPower, "%llu,ACT,%d,%d\n", divCeil(act_at, tCK) -
            timeStampOffset, bank_ref.bank, rank_ref.rank);

    // The next access has to respect tRAS for this bank
    bank_ref.preAllowedAt = act_at + tRAS;

    // Respect the row-to-column command delay for both read and write cmds
    bank_ref.rdAllowedAt = std::max(act_at + tRCD, bank_ref.rdAllowedAt);
    bank_ref.wrAllowedAt = std::max(act_at + tRCD, bank_ref.wrAllowedAt);

    // start by enforcing tRRD
    for (int i = 0; i < banksPerRank; i++) {
        // next activate to any bank in this rank must not happen
        // before tRRD
        if (bankGroupArch && (bank_ref.bankgr == rank_ref.banks[i].bankgr)) {
            // bank group architecture requires longer delays between
            // ACT commands within the same bank group.  Use tRRD_L
            // in this case
            rank_ref.banks[i].actAllowedAt = std::max(act_at + tRRD_L,
                                             rank_ref.banks[i].actAllowedAt);
        } else {
            // use shorter tRRD value when either
            // 1) bank group architecture is not supportted
            // 2) bank is in a different bank group
            rank_ref.banks[i].actAllowedAt = std::max(act_at + tRRD,
                                             rank_ref.banks[i].actAllowedAt);
        }
    }

    // next, we deal with tXAW, if the activation limit is disabled
    // then we directly schedule an activate power event
    if (!rank_ref.actTicks.empty()) {
        // sanity check
        if (rank_ref.actTicks.back() &&
           (act_at - rank_ref.actTicks.back()) < tXAW) {
            panic("Got %d activates in window %d (%llu - %llu) which "
                  "is smaller than %llu\n", activationLimit, act_at -
                  rank_ref.actTicks.back(), act_at,
                  rank_ref.actTicks.back(), tXAW);
        }

        // shift the times used for the book keeping, the last element
        // (highest index) is the oldest one and hence the lowest value
        rank_ref.actTicks.pop_back();

        // record an new activation (in the future)
        rank_ref.actTicks.push_front(act_at);

        // cannot activate more than X times in time window tXAW, push the
        // next one (the X + 1'st activate) to be tXAW away from the
        // oldest in our window of X
        if (rank_ref.actTicks.back() &&
           (act_at - rank_ref.actTicks.back()) < tXAW) {
            DPRINTF(ChannelDRAM, "Enforcing tXAW with X = %d, next activate "
                    "no earlier than %llu\n", activationLimit,
                    rank_ref.actTicks.back() + tXAW);
            for (int j = 0; j < banksPerRank; j++)
                // next activate must not happen before end of window
                rank_ref.banks[j].actAllowedAt =
                    std::max(rank_ref.actTicks.back() + tXAW,
                             rank_ref.banks[j].actAllowedAt);
        }
    }

    // at the point when this activate takes place, make sure we
    // transition to the active power state
    if (!rank_ref.activateEvent.scheduled())
        schedule(rank_ref.activateEvent, act_at);
    else if (rank_ref.activateEvent.when() > act_at)
        // move it sooner in time
        reschedule(rank_ref.activateEvent, act_at);
}


void
ChannelDRAMInterface::prechargeBank(Rank& rank_ref, Bank& bank, Tick pre_tick,
                             bool auto_or_preall, bool trace)
{
    // make sure the bank has an open row
    assert(bank.openRow != Bank::NO_ROW);

    // sample the bytes per activate here since we are closing
    // the page
    // stats.bytesPerActivate.sample(bank.bytesAccessed);

    bank.openRow = Bank::NO_ROW;

    Tick pre_at = pre_tick;
    if (auto_or_preall) {
        // no precharge allowed before this one
        bank.preAllowedAt = pre_at;
    } else {
        // Issuing an explicit PRE command
        // Verify that we have command bandwidth to issue the precharge
        // if not, shift to next burst window
        pre_at = channelCtrl->getMinirankMemCtrl()->scheduleAddrBus(pre_tick);
        // enforce tPPD
        for (int i = 0; i < banksPerRank; i++) {
            rank_ref.banks[i].preAllowedAt = std::max(pre_at + tPPD,
                                             rank_ref.banks[i].preAllowedAt);
        }
    }

    Tick pre_done_at = pre_at + tRP;

    bank.actAllowedAt = std::max(bank.actAllowedAt, pre_done_at);

    assert(rank_ref.numBanksActive != 0);
    --rank_ref.numBanksActive;

    DPRINTF(ChannelDRAM, "Precharging bank %d, rank %d at tick %lld, now got "
            "%d active\n", bank.bank, rank_ref.rank, pre_at,
            rank_ref.numBanksActive);

    if (trace) {

        rank_ref.cmdList.push_back(Command(MemCommand::PRE, bank.bank,
                                   pre_at));
        DPRINTF(DRAMPower, "%llu,PRE,%d,%d\n", divCeil(pre_at, tCK) -
                timeStampOffset, bank.bank, rank_ref.rank);
    }

    // if we look at the current number of active banks we might be
    // tempted to think the DRAM is now idle, however this can be
    // undone by an activate that is scheduled to happen before we
    // would have reached the idle state, so schedule an event and
    // rather check once we actually make it to the point in time when
    // the (last) precharge takes place
    if (!rank_ref.prechargeEvent.scheduled()) {
        schedule(rank_ref.prechargeEvent, pre_done_at);
        // New event, increment count
        ++rank_ref.outstandingEvents;
    } else if (rank_ref.prechargeEvent.when() < pre_done_at) {
        reschedule(rank_ref.prechargeEvent, pre_done_at);
    }
}

std::pair<Tick, Tick>
ChannelDRAMInterface::doBurstAccess(MemPacket* mem_pkt, Tick next_burst_at,
                             const std::vector<MemPacketQueue>& queue)
{
    DPRINTF(ChannelDRAM, "Timing access to addr %#x, rank/bank/row %d %d %d\n",
            mem_pkt->addr, mem_pkt->rank, mem_pkt->bank, mem_pkt->row);

    // get the rank
    Rank& rank_ref = *ranks[mem_pkt->rank];

    assert(rank_ref.inRefIdleState());

    // are we in or transitioning to a low-power state and have not scheduled
    // a power-up event?
    // if so, wake up from power down to issue RD/WR burst
    if (rank_ref.inLowPowerState) {
        assert(rank_ref.pwrState != PWR_SREF);
        rank_ref.scheduleWakeUpEvent(tXP);
    }

    // get the bank
    Bank& bank_ref = rank_ref.banks[mem_pkt->bank];

    // for the state we need to track if it is a row hit or not
    // bool row_hit = true;

    // Determine the access latency and update the bank state
    if (bank_ref.openRow == mem_pkt->row) {
        // nothing to do
    } else {
        // row_hit = false;

        // If there is a page open, precharge it.
        if (bank_ref.openRow != Bank::NO_ROW) {
            prechargeBank(rank_ref, bank_ref, std::max(bank_ref.preAllowedAt,
                                                   curTick()));
        }

        // next we need to account for the delay in activating the page
        Tick act_tick = std::max(bank_ref.actAllowedAt, curTick());

        // Record the activation and deal with all the global timing
        // constraints caused be a new activation (tRRD and tXAW)
        activateBank(rank_ref, bank_ref, act_tick, mem_pkt->row);
    }

    // respect any constraints on the command (e.g. tRCD or tCCD)
    const Tick col_allowed_at = mem_pkt->isRead() ?
                                bank_ref.rdAllowedAt : bank_ref.wrAllowedAt;

    // we need to wait until the bus is available before we can issue
    // the command; need to ensure minimum bus delay requirement is met
    Tick cmd_at = std::max({col_allowed_at, next_burst_at, curTick()});

    // verify that we have command bandwidth to issue the burst
    // if not, shift to next burst window
    if (dataClockSync && ((cmd_at - rank_ref.lastBurstTick) > clkResyncDelay))
    {
        // cmd_at = channelCtrl->verifyMultiCmd(cmd_at,
                    // maxCommandsPerWindow, tCK);
    }
    else
        cmd_at = channelCtrl->getMinirankMemCtrl()->scheduleAddrBus(cmd_at);

    // if we are interleaving bursts, ensure that
    // 1) we don't double interleave on next burst issue
    // 2) we are at an interleave boundary; if not, shift to next boundary
    Tick burst_gap = tBURST_MIN;
    if (burstInterleave) {
        if (cmd_at == (rank_ref.lastBurstTick + tBURST_MIN)) {
            // already interleaving, push next command to end of full burst
            burst_gap = tBURST;
        } else if (cmd_at < (rank_ref.lastBurstTick + tBURST)) {
            // not at an interleave boundary after bandwidth check
            // Shift command to tBURST boundary to avoid data contention
            // Command will remain in the same burst window given that
            // tBURST is less than tBURST_MAX
            cmd_at = rank_ref.lastBurstTick + tBURST;
        }
    }
    DPRINTF(ChannelDRAM, "Schedule RD/WR burst at tick %d\n", cmd_at);

    // update the packet ready time
    mem_pkt->readyTime = cmd_at + tCL + tBURST;

    rank_ref.lastBurstTick = cmd_at;


    // update the time for the next read/write burst for each
    // bank (add a max with tCCD/tCCD_L/tCCD_L_WR here)
    Tick dly_to_rd_cmd;
    Tick dly_to_wr_cmd;
    for (int j = 0; j < ranksPerChannel; j++) {
        for (int i = 0; i < banksPerRank; i++) {
            if (mem_pkt->rank == j) {
                if (bankGroupArch &&
                   (bank_ref.bankgr == ranks[j]->banks[i].bankgr)) {
                    // bank group architecture requires longer delays between
                    // RD/WR burst commands to the same bank group.
                    // tCCD_L is default requirement for same BG timing
                    // tCCD_L_WR is required for write-to-write
                    // Need to also take bus turnaround delays into account
                    dly_to_rd_cmd = mem_pkt->isRead() ?
                                    tCCD_L : std::max(tCCD_L, wrToRdDlySameBG);
                    dly_to_wr_cmd = mem_pkt->isRead() ?
                                    std::max(tCCD_L, rdToWrDlySameBG) :
                                    tCCD_L_WR;
                } else {
                    // tBURST is default requirement for diff BG timing
                    // Need to also take bus turnaround delays into account
                    dly_to_rd_cmd = mem_pkt->isRead() ? burst_gap :
                                                       writeToReadDelay();
                    dly_to_wr_cmd = mem_pkt->isRead() ? readToWriteDelay() :
                                                       burst_gap;
                }
            } else {
                // different rank is by default in a different bank group and
                // doesn't require longer tCCD or additional RTW, WTR delays
                // Need to account for rank-to-rank switching
                dly_to_wr_cmd = rankToRankDelay();
                dly_to_rd_cmd = rankToRankDelay();
            }
            ranks[j]->banks[i].rdAllowedAt = std::max(cmd_at + dly_to_rd_cmd,
                                             ranks[j]->banks[i].rdAllowedAt);
            ranks[j]->banks[i].wrAllowedAt = std::max(cmd_at + dly_to_wr_cmd,
                                             ranks[j]->banks[i].wrAllowedAt);
        }
    }

    // Save rank of current access
    activeRank = mem_pkt->rank;

    // If this is a write, we also need to respect the write recovery
    // time before a precharge, in the case of a read, respect the
    // read to precharge constraint
    bank_ref.preAllowedAt = std::max(bank_ref.preAllowedAt,
                                 mem_pkt->isRead() ? cmd_at + tRTP :
                                 mem_pkt->readyTime + tWR);

    // increment the bytes accessed and the accesses per row
    bank_ref.bytesAccessed += burstSize;
    ++bank_ref.rowAccesses;

    // if we reached the max, then issue with an auto-precharge
    bool auto_precharge = pageMgmt == enums::close ||
        bank_ref.rowAccesses == maxAccessesPerRow;

    // if we did not hit the limit, we might still want to
    // auto-precharge
    if (!auto_precharge &&
        (pageMgmt == enums::open_adaptive ||
         pageMgmt == enums::close_adaptive)) {
        // a twist on the open and close page policies:
        // 1) open_adaptive page policy does not blindly keep the
        // page open, but close it if there are no row hits, and there
        // are bank conflicts in the queue
        // 2) close_adaptive page policy does not blindly close the
        // page, but closes it only if there are no row hits in the queue.
        // In this case, only force an auto precharge when there
        // are no same page hits in the queue
        bool got_more_hits = false;
        bool got_bank_conflict = false;

        for (uint8_t i = 0; i < channelCtrl->numPriorities(); ++i) {
            auto p = queue[i].begin();
            // keep on looking until we find a hit or reach the end of the
            // queue
            // 1) if a hit is found, then both open and close adaptive
            //    policies keep the page open
            // 2) if no hit is found, got_bank_conflict is set to true if a
            //    bank conflict request is waiting in the queue
            // 3) make sure we are not considering the packet that we are
            //    currently dealing with
            while (!got_more_hits && p != queue[i].end()) {
                if (mem_pkt != (*p)) {
                    bool same_rank_bank = (mem_pkt->rank == (*p)->rank) &&
                                          (mem_pkt->bank == (*p)->bank);

                    bool same_row = mem_pkt->row == (*p)->row;
                    got_more_hits |= same_rank_bank && same_row;
                    got_bank_conflict |= same_rank_bank && !same_row;
                }
                ++p;
            }

            if (got_more_hits)
                break;
        }

        // auto pre-charge when either
        // 1) open_adaptive policy, we have not got any more hits, and
        //    have a bank conflict
        // 2) close_adaptive policy and we have not got any more hits
        auto_precharge = !got_more_hits &&
            (got_bank_conflict || pageMgmt == enums::close_adaptive);
    }

    // DRAMPower trace command to be written
    std::string mem_cmd = mem_pkt->isRead() ? "RD" : "WR";

    // MemCommand required for DRAMPower library
    MemCommand::cmds command = (mem_cmd == "RD") ? MemCommand::RD :
                                                   MemCommand::WR;

    rank_ref.cmdList.push_back(Command(command, mem_pkt->bank, cmd_at));

    DPRINTF(DRAMPower, "%llu,%s,%d,%d\n", divCeil(cmd_at, tCK) -
            timeStampOffset, mem_cmd, mem_pkt->bank, mem_pkt->rank);

    // if this access should use auto-precharge, then we are
    // closing the row after the read/write burst
    if (auto_precharge) {
        // if auto-precharge push a PRE command at the correct tick to the
        // list used by DRAMPower library to calculate power
        prechargeBank(rank_ref, bank_ref, std::max(curTick(),
                      bank_ref.preAllowedAt), true);

        DPRINTF(ChannelDRAM, "Auto-precharged bank: %d\n", mem_pkt->bankId);
    }

    // Update the stats and schedule the next request
    if (mem_pkt->isRead()) {
        // Every respQueue which will generate an event, increment count
        ++rank_ref.outstandingEvents;

        // stats.readBursts++;
        // if (row_hit)
        //     stats.readRowHits++;
        // stats.bytesRead += burstSize;
        // stats.perBankRdBursts[mem_pkt->bankId]++;

        // // Update latency stats
        // stats.totMemAccLat += mem_pkt->readyTime - mem_pkt->entryTime;
        // stats.totQLat += cmd_at - mem_pkt->entryTime;
        // stats.totBusLat += tBURST;
    } else {
        // Schedule write done event to decrement event count
        // after the readyTime has been reached
        // Only schedule latest write event to minimize events
        // required; only need to ensure that final event scheduled covers
        // the time that writes are outstanding and bus is active
        // to holdoff power-down entry events
        if (!rank_ref.writeDoneEvent.scheduled()) {
            schedule(rank_ref.writeDoneEvent, mem_pkt->readyTime);
            // New event, increment count
            ++rank_ref.outstandingEvents;

        } else if (rank_ref.writeDoneEvent.when() < mem_pkt->readyTime) {
            reschedule(rank_ref.writeDoneEvent, mem_pkt->readyTime);
        }
        // will remove write from queue when returned to parent function
        // decrement count for DRAM rank
        --rank_ref.writeEntries;

        // stats.writeBursts++;
        // if (row_hit)
        //     stats.writeRowHits++;
        // stats.bytesWritten += burstSize;
        // stats.perBankWrBursts[mem_pkt->bankId]++;

    }
    // Update bus state to reflect when previous command was issued
    return std::make_pair(cmd_at, cmd_at + burst_gap);
}


// void
// ChannelDRAMInterface::addRankToRankDelay(Tick cmd_at)
// {
//     // update timing for DRAM ranks due to bursts issued
//     // to ranks on other media interfaces
//     for (auto n : ranks) {
//         for (int i = 0; i < banksPerRank; i++) {
//             // different rank by default
//             // Need to only account for rank-to-rank switching
//             n->banks[i].rdAllowedAt = std::max(cmd_at + rankToRankDelay(),
//                                              n->banks[i].rdAllowedAt);
//             n->banks[i].wrAllowedAt = std::max(cmd_at + rankToRankDelay(),
//                                              n->banks[i].wrAllowedAt);
//         }
//     }
// }


// void
// ChannelDRAMInterface::startup()
// {
//     if (system()->isTimingMode()) {
//         // timestamp offset should be in clock cycles for DRAMPower
//         timeStampOffset = divCeil(curTick(), tCK);

//         for (auto r : ranks) {
//             r->startup(curTick() + tREFI - tRP);
//         }
//     }
// }


bool
ChannelDRAMInterface::isBusy()
{
    int busy_ranks = 0;
    for (auto r : ranks) {
        if (!r->inRefIdleState()) {
            if (r->pwrState != PWR_SREF) {
                // rank is busy refreshing
                DPRINTF(DRAMState, "Rank %d is not available\n", r->rank);
                busy_ranks++;

                // let the rank know that if it was waiting to drain, it
                // is now done and ready to proceed
                r->checkDrainDone();
            }

            // check if we were in self-refresh and haven't started
            // to transition out
            if ((r->pwrState == PWR_SREF) && r->inLowPowerState) {
                DPRINTF(DRAMState, "Rank %d is in self-refresh\n", r->rank);
                // if we have commands queued to this rank and we don't have
                // a minimum number of active commands enqueued,
                // exit self-refresh
                if (r->forceSelfRefreshExit()) {
                    DPRINTF(DRAMState, "rank %d was in self refresh and"
                           " should wake up\n", r->rank);
                    //wake up from self-refresh
                    r->scheduleWakeUpEvent(tXS);
                    // things are brought back into action once a refresh is
                    // performed after self-refresh
                    // continue with selection for other ranks
                }
            }
        }
    }
    return (busy_ranks == ranksPerChannel);
}


void ChannelDRAMInterface::setupRank(const uint8_t rank, const bool is_read)
{
    // increment entry count of the rank based on packet type
    if (is_read) {
        ++ranks[rank]->readEntries;
    } else {
        ++ranks[rank]->writeEntries;
    }
}


void
ChannelDRAMInterface::respondEvent(uint8_t rank)
{
    Rank& rank_ref = *ranks[rank];

    // if a read has reached its ready-time, decrement the number of reads
    // At this point the packet has been handled and there is a possibility
    // to switch to low-power mode if no other packet is available
    --rank_ref.readEntries;
    DPRINTF(ChannelDRAM, "number of read entries for rank %d is %d\n",
            rank, rank_ref.readEntries);

    // counter should at least indicate one outstanding request
    // for this read
    assert(rank_ref.outstandingEvents > 0);
    // read response received, decrement count
    --rank_ref.outstandingEvents;

    // at this moment should not have transitioned to a low-power state
    assert((rank_ref.pwrState != PWR_SREF) &&
           (rank_ref.pwrState != PWR_PRE_PDN) &&
           (rank_ref.pwrState != PWR_ACT_PDN));

    // track if this is the last packet before idling
    // and that there are no outstanding commands to this rank
    if (rank_ref.isQueueEmpty() && rank_ref.outstandingEvents == 0 &&
        rank_ref.inRefIdleState() && enableDRAMPowerdown) {
        // verify that there are no events scheduled
        assert(!rank_ref.activateEvent.scheduled());
        assert(!rank_ref.prechargeEvent.scheduled());

        // if coming from active state, schedule power event to
        // active power-down else go to precharge power-down
        DPRINTF(DRAMState, "Rank %d sleep at tick %d; current power state is "
                "%d\n", rank, curTick(), rank_ref.pwrState);

        // default to ACT power-down unless already in IDLE state
        // could be in IDLE if PRE issued before data returned
        PowerState next_pwr_state = PWR_ACT_PDN;
        if (rank_ref.pwrState == PWR_IDLE) {
            next_pwr_state = PWR_PRE_PDN;
        }

        rank_ref.powerDownSleep(next_pwr_state, curTick());
    }
}


void
ChannelDRAMInterface::checkRefreshState(uint8_t rank)
{
    Rank& rank_ref = *ranks[rank];

    if ((rank_ref.refreshState == REF_PRE) &&
        !rank_ref.prechargeEvent.scheduled()) {
          // kick the refresh event loop into action again if banks already
          // closed and just waiting for read to complete
          schedule(rank_ref.refreshEvent, curTick());
    }
}


void
ChannelDRAMInterface::drainRanks()
{
    // also need to kick off events to exit self-refresh
    for (auto r : ranks) {
        // force self-refresh exit, which in turn will issue auto-refresh
        if (r->pwrState == PWR_SREF) {
            DPRINTF(ChannelDRAM,
                    "Rank%d: Forcing self-refresh wakeup in drain\n",
                    r->rank);
            r->scheduleWakeUpEvent(tXS);
        }
    }
}


bool
ChannelDRAMInterface::allRanksDrained() const
{
    // true until proven false
    bool all_ranks_drained = true;
    for (auto r : ranks) {
        // then verify that the power state is IDLE ensuring all banks are
        // closed and rank is not in a low power state. Also verify that rank
        // is idle from a refresh point of view.
        all_ranks_drained = r->inPwrIdleState() && r->inRefIdleState() &&
            all_ranks_drained;
    }
    return all_ranks_drained;
}


void
ChannelDRAMInterface::suspend()
{
    for (auto r : ranks) {
        r->suspend();
    }
}


std::pair<std::vector<uint32_t>, bool>
ChannelDRAMInterface::minBankPrep(const MemPacketQueue& queue,
                      Tick min_col_at) const
{
    Tick min_act_at = MaxTick;
    std::vector<uint32_t> bank_mask(ranksPerChannel, 0);

    // latest Tick for which ACT can occur without incurring additoinal
    // delay on the data bus
    const Tick hidden_act_max = std::max(min_col_at - tRCD, curTick());

    // Flag condition when burst can issue back-to-back with previous burst
    bool found_seamless_bank = false;

    // Flag condition when bank can be opened without incurring additional
    // delay on the data bus
    bool hidden_bank_prep = false;

    // determine if we have queued transactions targetting the
    // bank in question
    std::vector<bool> got_waiting(ranksPerChannel * banksPerRank, false);
    for (const auto& p : queue) {
        if (p->isDram() && ranks[p->rank]->inRefIdleState())
            got_waiting[p->bankId] = true;
    }

    // Find command with optimal bank timing
    // Will prioritize commands that can issue seamlessly.
    for (int i = 0; i < ranksPerChannel; i++) {
        for (int j = 0; j < banksPerRank; j++) {
            uint16_t bank_id = i * banksPerRank + j;

            // if we have waiting requests for the bank, and it is
            // amongst the first available, update the mask
            if (got_waiting[bank_id]) {
                // make sure this rank is not currently refreshing.
                assert(ranks[i]->inRefIdleState());
                // simplistic approximation of when the bank can issue
                // an activate, ignoring any rank-to-rank switching
                // cost in this calculation
                Tick act_at = ranks[i]->banks[j].openRow == Bank::NO_ROW ?
                    std::max(ranks[i]->banks[j].actAllowedAt, curTick()) :
                    std::max(ranks[i]->banks[j].preAllowedAt, curTick()) + tRP;

                // When is the earliest the R/W burst can issue?
                const Tick col_allowed_at = channelCtrl->
                                              inReadBusState(false) ?
                                              ranks[i]->banks[j].rdAllowedAt :
                                              ranks[i]->banks[j].wrAllowedAt;
                Tick col_at = std::max(col_allowed_at, act_at + tRCD);

                // bank can issue burst back-to-back (seamlessly) with
                // previous burst
                bool new_seamless_bank = col_at <= min_col_at;

                // if we found a new seamless bank or we have no
                // seamless banks, and got a bank with an earlier
                // activate time, it should be added to the bit mask
                if (new_seamless_bank ||
                    (!found_seamless_bank && act_at <= min_act_at)) {
                    // if we did not have a seamless bank before, and
                    // we do now, reset the bank mask, also reset it
                    // if we have not yet found a seamless bank and
                    // the activate time is smaller than what we have
                    // seen so far
                    if (!found_seamless_bank &&
                        (new_seamless_bank || act_at < min_act_at)) {
                        std::fill(bank_mask.begin(), bank_mask.end(), 0);
                    }

                    found_seamless_bank |= new_seamless_bank;

                    // ACT can occur 'behind the scenes'
                    hidden_bank_prep = act_at <= hidden_act_max;

                    // set the bit corresponding to the available bank
                    replaceBits(bank_mask[i], j, j, 1);
                    min_act_at = act_at;
                }
            }
        }
    }

    return std::make_pair(bank_mask, hidden_bank_prep);
}

void
ChannelDRAMInterface::ChannelDRAMStats::resetStats()
{
    dram.lastStatsResetTick = curTick();
}

// Fixme fine channel number
ChannelDRAMInterface::ChannelDRAMStats::
                ChannelDRAMStats(MinirankDRAMInterface &_dram)
    : statistics::Group(&_dram,
            csprintf("channel%d", 0).c_str()),
    dram(_dram),

    ADD_STAT(readBursts, statistics::units::Count::get(),
             "Number of DRAM read bursts"),
    ADD_STAT(writeBursts, statistics::units::Count::get(),
             "Number of DRAM write bursts"),

    ADD_STAT(perBankRdBursts, statistics::units::Count::get(),
             "Per bank write bursts"),
    ADD_STAT(perBankWrBursts, statistics::units::Count::get(),
             "Per bank write bursts"),

    ADD_STAT(totQLat, statistics::units::Tick::get(),
                "Total ticks spent queuing"),
    ADD_STAT(totBusLat, statistics::units::Tick::get(),
             "Total ticks spent in databus transfers"),
    ADD_STAT(totMemAccLat, statistics::units::Tick::get(),
             "Total ticks spent from burst creation until serviced "
             "by the DRAM"),

    ADD_STAT(avgQLat, statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(),
             "Average queueing delay per DRAM burst"),
    ADD_STAT(avgBusLat, statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(),
             "Average bus latency per DRAM burst"),
    ADD_STAT(avgMemAccLat, statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(),
             "Average memory access latency per DRAM burst"),

    ADD_STAT(readRowHits, statistics::units::Count::get(),
             "Number of row buffer hits during reads"),
    ADD_STAT(writeRowHits, statistics::units::Count::get(),
             "Number of row buffer hits during writes"),
    ADD_STAT(readRowHitRate, statistics::units::Ratio::get(),
             "Row buffer hit rate for reads"),
    ADD_STAT(writeRowHitRate, statistics::units::Ratio::get(),
             "Row buffer hit rate for writes"),

    ADD_STAT(bytesPerActivate, statistics::units::Byte::get(),
             "Bytes accessed per row activation"),
    ADD_STAT(bytesRead, statistics::units::Byte::get(),
             "Total number of bytes read from DRAM"),
    ADD_STAT(bytesWritten, statistics::units::Byte::get(),
            "Total number of bytes written to DRAM"),
    ADD_STAT(avgRdBW, statistics::units::Rate<
                statistics::units::Byte, statistics::units::Second>::get(),
             "Average DRAM read bandwidth in MiBytes/s"),
    ADD_STAT(avgWrBW, statistics::units::Rate<
                statistics::units::Byte, statistics::units::Second>::get(),
             "Average DRAM write bandwidth in MiBytes/s"),
    ADD_STAT(peakBW,  statistics::units::Rate<
                statistics::units::Byte, statistics::units::Second>::get(),
             "Theoretical peak bandwidth in MiByte/s"),

    ADD_STAT(busUtil, statistics::units::Ratio::get(),
             "Data bus utilization in percentage"),
    ADD_STAT(busUtilRead, statistics::units::Ratio::get(),
             "Data bus utilization in percentage for reads"),
    ADD_STAT(busUtilWrite, statistics::units::Ratio::get(),
             "Data bus utilization in percentage for writes"),

    ADD_STAT(pageHitRate, statistics::units::Ratio::get(),
             "Row buffer hit rate, read and write combined")

{
}

void
ChannelDRAMInterface::ChannelDRAMStats::regStats()
{
    using namespace statistics;

    avgQLat.precision(2);
    avgBusLat.precision(2);
    avgMemAccLat.precision(2);

    readRowHitRate.precision(2);
    writeRowHitRate.precision(2);

    perBankRdBursts.init(dram.banksPerRank * dram.ranksPerChannel);
    perBankWrBursts.init(dram.banksPerRank * dram.ranksPerChannel);

    bytesPerActivate
        .init(dram.maxAccessesPerRow ?
              dram.maxAccessesPerRow : dram.rowBufferSize)
        .flags(nozero);

    peakBW.precision(2);
    busUtil.precision(2);
    busUtilWrite.precision(2);
    busUtilRead.precision(2);

    pageHitRate.precision(2);

    // Formula stats
    avgQLat = totQLat / readBursts;
    avgBusLat = totBusLat / readBursts;
    avgMemAccLat = totMemAccLat / readBursts;

    readRowHitRate = (readRowHits / readBursts) * 100;
    writeRowHitRate = (writeRowHits / writeBursts) * 100;

    avgRdBW = (bytesRead / 1000000) / simSeconds;
    avgWrBW = (bytesWritten / 1000000) / simSeconds;
    peakBW = (sim_clock::Frequency / dram.burstDelay()) *
              dram.bytesPerBurst() / 1000000;

    busUtil = (avgRdBW + avgWrBW) / peakBW * 100;
    busUtilRead = avgRdBW / peakBW * 100;
    busUtilWrite = avgWrBW / peakBW * 100;

    pageHitRate = (writeRowHits + readRowHits) /
        (writeBursts + readBursts) * 100;
}

std::pair<MemPacketQueue::iterator, Tick>
ChannelDRAMInterface::chooseNextFRFCFS(MemPacketQueue& queue,
        Tick min_col_at) const
{
    std::vector<uint32_t> earliest_banks(ranksPerChannel, 0);

    // Has minBankPrep been called to populate earliest_banks?
    bool filled_earliest_banks = false;
    // can the PRE/ACT sequence be done without impacting utlization?
    bool hidden_bank_prep = false;

    // search for seamless row hits first, if no seamless row hit is
    // found then determine if there are other packets that can be issued
    // without incurring additional bus delay due to bank timing
    // Will select closed rows first to enable more open row possibilies
    // in future selections
    bool found_hidden_bank = false;

    // remember if we found a row hit, not seamless, but bank prepped
    // and ready
    bool found_prepped_pkt = false;

    // if we have no row hit, prepped or not, and no seamless packet,
    // just go for the earliest possible
    bool found_earliest_pkt = false;

    Tick selected_col_at = MaxTick;
    auto selected_pkt_it = queue.end();

    for (auto i = queue.begin(); i != queue.end() ; ++i) {
        MemPacket* pkt = *i;

        // select optimal DRAM packet in Q
        if (pkt->isDram()) {
            const Bank& bank = ranks[pkt->rank]->banks[pkt->bank];
            const Tick col_allowed_at = pkt->isRead() ? bank.rdAllowedAt :
                                                        bank.wrAllowedAt;

            DPRINTF(ChannelDRAM,
                    "%s checking DRAM packet in bank %d, row %d\n",
                    __func__, pkt->bank, pkt->row);

            // check if rank is not doing a refresh and thus is available,
            // if not, jump to the next packet
            if (burstReady(pkt)) {

                DPRINTF(ChannelDRAM,
                        "%s bank %d - Rank %d available\n", __func__,
                        pkt->bank, pkt->rank);

                // check if it is a row hit
                if (bank.openRow == pkt->row) {
                    // no additional rank-to-rank or same bank-group
                    // delays, or we switched read/write and might as well
                    // go for the row hit
                    if (col_allowed_at <= min_col_at) {
                        // FCFS within the hits, giving priority to
                        // commands that can issue seamlessly, without
                        // additional delay, such as same rank accesses
                        // and/or different bank-group accesses
                        DPRINTF(ChannelDRAM, "%s Seamless buffer hit\n",
                                __func__);
                        selected_pkt_it = i;
                        selected_col_at = col_allowed_at;
                        // no need to look through the remaining queue entries
                        break;
                    } else if (!found_hidden_bank && !found_prepped_pkt) {
                        // if we did not find a packet to a closed row
                        // that can issue the bank commands without
                        // incurring delay, and
                        // did not yet find a packet to a prepped row,
                        // remember the current one
                        selected_pkt_it = i;
                        selected_col_at = col_allowed_at;
                        found_prepped_pkt = true;
                        DPRINTF(ChannelDRAM, "%s Prepped row buffer hit\n",
                                __func__);
                    }
                } else if (!found_earliest_pkt) {
                    // if we have not initialised the bank status, do it
                    // now, and only once per scheduling decisions
                    if (!filled_earliest_banks) {
                        // determine entries with earliest bank delay
                        std::tie(earliest_banks, hidden_bank_prep) =
                            minBankPrep(queue, min_col_at);
                        filled_earliest_banks = true;
                    }

                    // bank is amongst first available banks
                    // minBankPrep will give priority to packets that can
                    // issue seamlessly
                    if (bits(earliest_banks[pkt->rank],
                             pkt->bank, pkt->bank)) {
                        found_earliest_pkt = true;
                        found_hidden_bank = hidden_bank_prep;

                        // give priority to packets that can issue
                        // bank commands 'behind the scenes'
                        // any additional delay if any will be due to
                        // col-to-col command requirements
                        if (hidden_bank_prep || !found_prepped_pkt) {
                            selected_pkt_it = i;
                            selected_col_at = col_allowed_at;
                        }
                    }
                }
            } else {
                DPRINTF(ChannelDRAM, "%s bank %d - Rank %d not available\n",
                        __func__, pkt->bank, pkt->rank);
            }
        }
    }

    if (selected_pkt_it == queue.end()) {
        DPRINTF(ChannelDRAM, "%s no available DRAM ranks found\n", __func__);
    }

    return std::make_pair(selected_pkt_it, selected_col_at);
}

ChannelDRAMInterface::ChannelRank::ChannelRank(
        const MinirankDRAMInterfaceParams &_p, int _rank,
        ChannelDRAMInterface& _dram, bool is_raim)
    : Rank(_p, _rank, _dram, is_raim),
      writeDoneEvent([this]{ processWriteDoneEvent(); }, name()),
      activateEvent([this]{ processActivateEvent(); }, name()),
      prechargeEvent([this]{ processPrechargeEvent(); }, name()),
      refreshEvent([this]{ processRefreshEvent(); }, name()),
      powerEvent([this]{ processPowerEvent(); }, name()),
      wakeUpEvent([this]{ processWakeUpEvent(); }, name())
{
            // nothing to add

}

} // namespce memory

} // namespace gem5
