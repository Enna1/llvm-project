//===- bolt/Passes/BinaryPasses.cpp - Binary-level passes -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements multiple passes for binary optimization and analysis.
//
//===----------------------------------------------------------------------===//

#include "bolt/Passes/BinaryPasses.h"
#include "bolt/Core/FunctionLayout.h"
#include "bolt/Core/ParallelUtilities.h"
#include "bolt/Passes/ReorderAlgorithm.h"
#include "bolt/Passes/ReorderFunctions.h"
#include "bolt/Utils/CommandLineOpts.h"
#include "llvm/Support/CommandLine.h"
#include <atomic>
#include <mutex>
#include <numeric>
#include <vector>

#define DEBUG_TYPE "bolt-opts"

using namespace llvm;
using namespace bolt;

static const char *dynoStatsOptName(const bolt::DynoStats::Category C) {
  assert(C > bolt::DynoStats::FIRST_DYNO_STAT &&
         C < DynoStats::LAST_DYNO_STAT && "Unexpected dyno stat category.");

  static std::string OptNames[bolt::DynoStats::LAST_DYNO_STAT + 1];

  OptNames[C] = bolt::DynoStats::Description(C);

  llvm::replace(OptNames[C], ' ', '-');

  return OptNames[C].c_str();
}

namespace opts {

extern cl::OptionCategory BoltCategory;
extern cl::OptionCategory BoltOptCategory;

extern cl::opt<unsigned> Verbosity;
extern cl::opt<bool> EnableBAT;
extern cl::opt<unsigned> ExecutionCountThreshold;
extern cl::opt<bool> UpdateDebugSections;
extern cl::opt<bolt::ReorderFunctions::ReorderType> ReorderFunctions;

enum DynoStatsSortOrder : char {
  Ascending,
  Descending
};

static cl::opt<DynoStatsSortOrder> DynoStatsSortOrderOpt(
    "print-sorted-by-order",
    cl::desc("use ascending or descending order when printing functions "
             "ordered by dyno stats"),
    cl::init(DynoStatsSortOrder::Descending), cl::cat(BoltOptCategory));

cl::list<std::string>
HotTextMoveSections("hot-text-move-sections",
  cl::desc("list of sections containing functions used for hugifying hot text. "
           "BOLT makes sure these functions are not placed on the same page as "
           "the hot text. (default=\'.stub,.mover\')."),
  cl::value_desc("sec1,sec2,sec3,..."),
  cl::CommaSeparated,
  cl::ZeroOrMore,
  cl::cat(BoltCategory));

bool isHotTextMover(const BinaryFunction &Function) {
  for (std::string &SectionName : opts::HotTextMoveSections) {
    if (Function.getOriginSectionName() &&
        *Function.getOriginSectionName() == SectionName)
      return true;
  }

  return false;
}

static cl::opt<bool> MinBranchClusters(
    "min-branch-clusters",
    cl::desc("use a modified clustering algorithm geared towards minimizing "
             "branches"),
    cl::Hidden, cl::cat(BoltOptCategory));

static cl::list<Peepholes::PeepholeOpts> Peepholes(
    "peepholes", cl::CommaSeparated, cl::desc("enable peephole optimizations"),
    cl::value_desc("opt1,opt2,opt3,..."),
    cl::values(clEnumValN(Peepholes::PEEP_NONE, "none", "disable peepholes"),
               clEnumValN(Peepholes::PEEP_DOUBLE_JUMPS, "double-jumps",
                          "remove double jumps when able"),
               clEnumValN(Peepholes::PEEP_TAILCALL_TRAPS, "tailcall-traps",
                          "insert tail call traps"),
               clEnumValN(Peepholes::PEEP_USELESS_BRANCHES, "useless-branches",
                          "remove useless conditional branches"),
               clEnumValN(Peepholes::PEEP_ALL, "all",
                          "enable all peephole optimizations")),
    cl::ZeroOrMore, cl::cat(BoltOptCategory));

static cl::opt<unsigned>
    PrintFuncStat("print-function-statistics",
                  cl::desc("print statistics about basic block ordering"),
                  cl::init(0), cl::cat(BoltOptCategory));

static cl::opt<bool> PrintLargeFunctions(
    "print-large-functions",
    cl::desc("print functions that could not be overwritten due to excessive "
             "size"),
    cl::init(false), cl::cat(BoltOptCategory));

static cl::list<bolt::DynoStats::Category>
    PrintSortedBy("print-sorted-by", cl::CommaSeparated,
                  cl::desc("print functions sorted by order of dyno stats"),
                  cl::value_desc("key1,key2,key3,..."),
                  cl::values(
#define D(name, description, ...)                                              \
  clEnumValN(bolt::DynoStats::name, dynoStatsOptName(bolt::DynoStats::name),   \
             description),
                      REAL_DYNO_STATS
#undef D
                          clEnumValN(bolt::DynoStats::LAST_DYNO_STAT, "all",
                                     "sorted by all names")),
                  cl::ZeroOrMore, cl::cat(BoltOptCategory));

static cl::opt<bool>
    PrintUnknown("print-unknown",
                 cl::desc("print names of functions with unknown control flow"),
                 cl::cat(BoltCategory), cl::Hidden);

static cl::opt<bool>
    PrintUnknownCFG("print-unknown-cfg",
                    cl::desc("dump CFG of functions with unknown control flow"),
                    cl::cat(BoltCategory), cl::ReallyHidden);

// Please MSVC19 with a forward declaration: otherwise it reports an error about
// an undeclared variable inside a callback.
extern cl::opt<bolt::ReorderBasicBlocks::LayoutType> ReorderBlocks;
cl::opt<bolt::ReorderBasicBlocks::LayoutType> ReorderBlocks(
    "reorder-blocks", cl::desc("change layout of basic blocks in a function"),
    cl::init(bolt::ReorderBasicBlocks::LT_NONE),
    cl::values(
        clEnumValN(bolt::ReorderBasicBlocks::LT_NONE, "none",
                   "do not reorder basic blocks"),
        clEnumValN(bolt::ReorderBasicBlocks::LT_REVERSE, "reverse",
                   "layout blocks in reverse order"),
        clEnumValN(bolt::ReorderBasicBlocks::LT_OPTIMIZE, "normal",
                   "perform optimal layout based on profile"),
        clEnumValN(bolt::ReorderBasicBlocks::LT_OPTIMIZE_BRANCH,
                   "branch-predictor",
                   "perform optimal layout prioritizing branch "
                   "predictions"),
        clEnumValN(bolt::ReorderBasicBlocks::LT_OPTIMIZE_CACHE, "cache",
                   "perform optimal layout prioritizing I-cache "
                   "behavior"),
        clEnumValN(bolt::ReorderBasicBlocks::LT_OPTIMIZE_CACHE_PLUS, "cache+",
                   "perform layout optimizing I-cache behavior"),
        clEnumValN(bolt::ReorderBasicBlocks::LT_OPTIMIZE_EXT_TSP, "ext-tsp",
                   "perform layout optimizing I-cache behavior"),
        clEnumValN(bolt::ReorderBasicBlocks::LT_OPTIMIZE_SHUFFLE,
                   "cluster-shuffle", "perform random layout of clusters")),
    cl::ZeroOrMore, cl::cat(BoltOptCategory),
    cl::callback([](const bolt::ReorderBasicBlocks::LayoutType &option) {
      if (option == bolt::ReorderBasicBlocks::LT_OPTIMIZE_CACHE_PLUS) {
        errs() << "BOLT-WARNING: '-reorder-blocks=cache+' is deprecated, please"
               << " use '-reorder-blocks=ext-tsp' instead\n";
        ReorderBlocks = bolt::ReorderBasicBlocks::LT_OPTIMIZE_EXT_TSP;
      }
    }));

static cl::opt<unsigned> ReportBadLayout(
    "report-bad-layout",
    cl::desc("print top <uint> functions with suboptimal code layout on input"),
    cl::init(0), cl::Hidden, cl::cat(BoltOptCategory));

static cl::opt<bool>
    ReportStaleFuncs("report-stale",
                     cl::desc("print the list of functions with stale profile"),
                     cl::Hidden, cl::cat(BoltOptCategory));

enum SctcModes : char {
  SctcAlways,
  SctcPreserveDirection,
  SctcHeuristic
};

static cl::opt<SctcModes>
SctcMode("sctc-mode",
  cl::desc("mode for simplify conditional tail calls"),
  cl::init(SctcAlways),
  cl::values(clEnumValN(SctcAlways, "always", "always perform sctc"),
    clEnumValN(SctcPreserveDirection,
      "preserve",
      "only perform sctc when branch direction is "
      "preserved"),
    clEnumValN(SctcHeuristic,
      "heuristic",
      "use branch prediction data to control sctc")),
  cl::ZeroOrMore,
  cl::cat(BoltOptCategory));

static cl::opt<unsigned>
StaleThreshold("stale-threshold",
    cl::desc(
      "maximum percentage of stale functions to tolerate (default: 100)"),
    cl::init(100),
    cl::Hidden,
    cl::cat(BoltOptCategory));

static cl::opt<unsigned> TSPThreshold(
    "tsp-threshold",
    cl::desc(
        "maximum number of hot basic blocks in a function for which to use "
        "a precise TSP solution while re-ordering basic blocks"),
    cl::init(10), cl::Hidden, cl::cat(BoltOptCategory));

static cl::opt<unsigned> TopCalledLimit(
    "top-called-limit",
    cl::desc("maximum number of functions to print in top called "
             "functions section"),
    cl::init(100), cl::Hidden, cl::cat(BoltCategory));

// Profile density options, synced with llvm-profgen/ProfileGenerator.cpp
static cl::opt<int> ProfileDensityCutOffHot(
    "profile-density-cutoff-hot", cl::init(990000),
    cl::desc("Total samples cutoff for functions used to calculate "
             "profile density."));

static cl::opt<double> ProfileDensityThreshold(
    "profile-density-threshold", cl::init(60),
    cl::desc("If the profile density is below the given threshold, it "
             "will be suggested to increase the sampling rate."),
    cl::Optional);

} // namespace opts

namespace llvm {
namespace bolt {

bool BinaryFunctionPass::shouldOptimize(const BinaryFunction &BF) const {
  return BF.isSimple() && BF.getState() == BinaryFunction::State::CFG &&
         !BF.isIgnored();
}

bool BinaryFunctionPass::shouldPrint(const BinaryFunction &BF) const {
  return BF.isSimple() && !BF.isIgnored();
}

void NormalizeCFG::runOnFunction(BinaryFunction &BF) {
  uint64_t NumRemoved = 0;
  uint64_t NumDuplicateEdges = 0;
  uint64_t NeedsFixBranches = 0;
  for (BinaryBasicBlock &BB : BF) {
    if (!BB.empty())
      continue;

    if (BB.isEntryPoint() || BB.isLandingPad())
      continue;

    // Handle a dangling empty block.
    if (BB.succ_size() == 0) {
      // If an empty dangling basic block has a predecessor, it could be a
      // result of codegen for __builtin_unreachable. In such case, do not
      // remove the block.
      if (BB.pred_size() == 0) {
        BB.markValid(false);
        ++NumRemoved;
      }
      continue;
    }

    // The block should have just one successor.
    BinaryBasicBlock *Successor = BB.getSuccessor();
    assert(Successor && "invalid CFG encountered");

    // Redirect all predecessors to the successor block.
    while (!BB.pred_empty()) {
      BinaryBasicBlock *Predecessor = *BB.pred_begin();
      if (Predecessor->hasJumpTable())
        break;

      if (Predecessor == Successor)
        break;

      BinaryBasicBlock::BinaryBranchInfo &BI = Predecessor->getBranchInfo(BB);
      Predecessor->replaceSuccessor(&BB, Successor, BI.Count,
                                    BI.MispredictedCount);
      // We need to fix branches even if we failed to replace all successors
      // and remove the block.
      NeedsFixBranches = true;
    }

    if (BB.pred_empty()) {
      BB.removeAllSuccessors();
      BB.markValid(false);
      ++NumRemoved;
    }
  }

  if (NumRemoved)
    BF.eraseInvalidBBs();

  // Check for duplicate successors. Do it after the empty block elimination as
  // we can get more duplicate successors.
  for (BinaryBasicBlock &BB : BF)
    if (!BB.hasJumpTable() && BB.succ_size() == 2 &&
        BB.getConditionalSuccessor(false) == BB.getConditionalSuccessor(true))
      ++NumDuplicateEdges;

  // fixBranches() will get rid of duplicate edges and update jump instructions.
  if (NumDuplicateEdges || NeedsFixBranches)
    BF.fixBranches();

  NumDuplicateEdgesMerged += NumDuplicateEdges;
  NumBlocksRemoved += NumRemoved;
}

Error NormalizeCFG::runOnFunctions(BinaryContext &BC) {
  ParallelUtilities::runOnEachFunction(
      BC, ParallelUtilities::SchedulingPolicy::SP_BB_LINEAR,
      [&](BinaryFunction &BF) { runOnFunction(BF); },
      [&](const BinaryFunction &BF) { return !shouldOptimize(BF); },
      "NormalizeCFG");
  if (NumBlocksRemoved)
    BC.outs() << "BOLT-INFO: removed " << NumBlocksRemoved << " empty block"
              << (NumBlocksRemoved == 1 ? "" : "s") << '\n';
  if (NumDuplicateEdgesMerged)
    BC.outs() << "BOLT-INFO: merged " << NumDuplicateEdgesMerged
              << " duplicate CFG edge"
              << (NumDuplicateEdgesMerged == 1 ? "" : "s") << '\n';
  return Error::success();
}

void EliminateUnreachableBlocks::runOnFunction(BinaryFunction &Function) {
  BinaryContext &BC = Function.getBinaryContext();
  unsigned Count;
  uint64_t Bytes;
  Function.markUnreachableBlocks();
  LLVM_DEBUG({
    for (BinaryBasicBlock &BB : Function) {
      if (!BB.isValid()) {
        dbgs() << "BOLT-INFO: UCE found unreachable block " << BB.getName()
               << " in function " << Function << "\n";
        Function.dump();
      }
    }
  });
  BinaryContext::IndependentCodeEmitter Emitter =
      BC.createIndependentMCCodeEmitter();
  std::tie(Count, Bytes) = Function.eraseInvalidBBs(Emitter.MCE.get());
  DeletedBlocks += Count;
  DeletedBytes += Bytes;
  if (Count) {
    auto L = BC.scopeLock();
    Modified.insert(&Function);
    if (opts::Verbosity > 0)
      BC.outs() << "BOLT-INFO: removed " << Count
                << " dead basic block(s) accounting for " << Bytes
                << " bytes in function " << Function << '\n';
  }
}

Error EliminateUnreachableBlocks::runOnFunctions(BinaryContext &BC) {
  ParallelUtilities::WorkFuncTy WorkFun = [&](BinaryFunction &BF) {
    runOnFunction(BF);
  };

  ParallelUtilities::PredicateTy SkipPredicate = [&](const BinaryFunction &BF) {
    return !shouldOptimize(BF) || BF.getLayout().block_empty();
  };

  ParallelUtilities::runOnEachFunction(
      BC, ParallelUtilities::SchedulingPolicy::SP_CONSTANT, WorkFun,
      SkipPredicate, "elimininate-unreachable");

  if (DeletedBlocks)
    BC.outs() << "BOLT-INFO: UCE removed " << DeletedBlocks << " blocks and "
              << DeletedBytes << " bytes of code\n";
  return Error::success();
}

bool ReorderBasicBlocks::shouldPrint(const BinaryFunction &BF) const {
  return (BinaryFunctionPass::shouldPrint(BF) &&
          opts::ReorderBlocks != ReorderBasicBlocks::LT_NONE);
}

bool ReorderBasicBlocks::shouldOptimize(const BinaryFunction &BF) const {
  // Apply execution count threshold
  if (BF.getKnownExecutionCount() < opts::ExecutionCountThreshold)
    return false;

  return BinaryFunctionPass::shouldOptimize(BF);
}

Error ReorderBasicBlocks::runOnFunctions(BinaryContext &BC) {
  if (opts::ReorderBlocks == ReorderBasicBlocks::LT_NONE)
    return Error::success();

  std::atomic_uint64_t ModifiedFuncCount(0);
  std::mutex FunctionEditDistanceMutex;
  DenseMap<const BinaryFunction *, uint64_t> FunctionEditDistance;

  ParallelUtilities::WorkFuncTy WorkFun = [&](BinaryFunction &BF) {
    SmallVector<const BinaryBasicBlock *, 0> OldBlockOrder;
    if (opts::PrintFuncStat > 0)
      llvm::copy(BF.getLayout().blocks(), std::back_inserter(OldBlockOrder));

    const bool LayoutChanged =
        modifyFunctionLayout(BF, opts::ReorderBlocks, opts::MinBranchClusters);
    if (LayoutChanged) {
      ModifiedFuncCount.fetch_add(1, std::memory_order_relaxed);
      if (opts::PrintFuncStat > 0) {
        const uint64_t Distance = BF.getLayout().getEditDistance(OldBlockOrder);
        std::lock_guard<std::mutex> Lock(FunctionEditDistanceMutex);
        FunctionEditDistance[&BF] = Distance;
      }
    }
  };

  ParallelUtilities::PredicateTy SkipFunc = [&](const BinaryFunction &BF) {
    return !shouldOptimize(BF);
  };

  ParallelUtilities::runOnEachFunction(
      BC, ParallelUtilities::SchedulingPolicy::SP_BB_LINEAR, WorkFun, SkipFunc,
      "ReorderBasicBlocks");
  const size_t NumAllProfiledFunctions =
      BC.NumProfiledFuncs + BC.NumStaleProfileFuncs;

  BC.outs() << "BOLT-INFO: basic block reordering modified layout of "
            << format(
                   "%zu functions (%.2lf%% of profiled, %.2lf%% of total)\n",
                   ModifiedFuncCount.load(std::memory_order_relaxed),
                   100.0 * ModifiedFuncCount.load(std::memory_order_relaxed) /
                       NumAllProfiledFunctions,
                   100.0 * ModifiedFuncCount.load(std::memory_order_relaxed) /
                       BC.getBinaryFunctions().size());

  if (opts::PrintFuncStat > 0) {
    raw_ostream &OS = BC.outs();
    // Copy all the values into vector in order to sort them
    std::map<uint64_t, BinaryFunction &> ScoreMap;
    auto &BFs = BC.getBinaryFunctions();
    for (auto It = BFs.begin(); It != BFs.end(); ++It)
      ScoreMap.insert(std::pair<uint64_t, BinaryFunction &>(
          It->second.getFunctionScore(), It->second));

    OS << "\nBOLT-INFO: Printing Function Statistics:\n\n";
    OS << "           There are " << BFs.size() << " functions in total. \n";
    OS << "           Number of functions being modified: "
       << ModifiedFuncCount.load(std::memory_order_relaxed) << "\n";
    OS << "           User asks for detailed information on top "
       << opts::PrintFuncStat << " functions. (Ranked by function score)"
       << "\n\n";
    uint64_t I = 0;
    for (std::map<uint64_t, BinaryFunction &>::reverse_iterator Rit =
             ScoreMap.rbegin();
         Rit != ScoreMap.rend() && I < opts::PrintFuncStat; ++Rit, ++I) {
      BinaryFunction &Function = Rit->second;

      OS << "           Information for function of top: " << (I + 1) << ": \n";
      OS << "             Function Score is: " << Function.getFunctionScore()
         << "\n";
      OS << "             There are " << Function.size()
         << " number of blocks in this function.\n";
      OS << "             There are " << Function.getInstructionCount()
         << " number of instructions in this function.\n";
      OS << "             The edit distance for this function is: "
         << FunctionEditDistance.lookup(&Function) << "\n\n";
    }
  }
  return Error::success();
}

bool ReorderBasicBlocks::modifyFunctionLayout(BinaryFunction &BF,
                                              LayoutType Type,
                                              bool MinBranchClusters) const {
  if (BF.size() == 0 || Type == LT_NONE)
    return false;

  BinaryFunction::BasicBlockOrderType NewLayout;
  std::unique_ptr<ReorderAlgorithm> Algo;

  // Cannot do optimal layout without profile.
  if (Type != LT_REVERSE && !BF.hasValidProfile())
    return false;

  if (Type == LT_REVERSE) {
    Algo.reset(new ReverseReorderAlgorithm());
  } else if (BF.size() <= opts::TSPThreshold && Type != LT_OPTIMIZE_SHUFFLE) {
    // Work on optimal solution if problem is small enough
    LLVM_DEBUG(dbgs() << "finding optimal block layout for " << BF << "\n");
    Algo.reset(new TSPReorderAlgorithm());
  } else {
    LLVM_DEBUG(dbgs() << "running block layout heuristics on " << BF << "\n");

    std::unique_ptr<ClusterAlgorithm> CAlgo;
    if (MinBranchClusters)
      CAlgo.reset(new MinBranchGreedyClusterAlgorithm());
    else
      CAlgo.reset(new PHGreedyClusterAlgorithm());

    switch (Type) {
    case LT_OPTIMIZE:
      Algo.reset(new OptimizeReorderAlgorithm(std::move(CAlgo)));
      break;

    case LT_OPTIMIZE_BRANCH:
      Algo.reset(new OptimizeBranchReorderAlgorithm(std::move(CAlgo)));
      break;

    case LT_OPTIMIZE_CACHE:
      Algo.reset(new OptimizeCacheReorderAlgorithm(std::move(CAlgo)));
      break;

    case LT_OPTIMIZE_EXT_TSP:
      Algo.reset(new ExtTSPReorderAlgorithm());
      break;

    case LT_OPTIMIZE_SHUFFLE:
      Algo.reset(new RandomClusterReorderAlgorithm(std::move(CAlgo)));
      break;

    default:
      llvm_unreachable("unexpected layout type");
    }
  }

  Algo->reorderBasicBlocks(BF, NewLayout);

  return BF.getLayout().update(NewLayout);
}

Error FixupBranches::runOnFunctions(BinaryContext &BC) {
  for (auto &It : BC.getBinaryFunctions()) {
    BinaryFunction &Function = It.second;
    if (!BC.shouldEmit(Function) || !Function.isSimple())
      continue;

    Function.fixBranches();
  }
  return Error::success();
}

Error FinalizeFunctions::runOnFunctions(BinaryContext &BC) {
  std::atomic<bool> HasFatal{false};
  ParallelUtilities::WorkFuncTy WorkFun = [&](BinaryFunction &BF) {
    if (!BF.finalizeCFIState()) {
      if (BC.HasRelocations) {
        BC.errs() << "BOLT-ERROR: unable to fix CFI state for function " << BF
                  << ". Exiting.\n";
        HasFatal = true;
        return;
      }
      BF.setSimple(false);
      return;
    }

    BF.setFinalized();

    // Update exception handling information.
    BF.updateEHRanges();
  };

  ParallelUtilities::PredicateTy SkipPredicate = [&](const BinaryFunction &BF) {
    return !BC.shouldEmit(BF);
  };

  ParallelUtilities::runOnEachFunction(
      BC, ParallelUtilities::SchedulingPolicy::SP_CONSTANT, WorkFun,
      SkipPredicate, "FinalizeFunctions");
  if (HasFatal)
    return createFatalBOLTError("finalize CFI state failure");
  return Error::success();
}

Error CheckLargeFunctions::runOnFunctions(BinaryContext &BC) {
  if (BC.HasRelocations)
    return Error::success();

  // If the function wouldn't fit, mark it as non-simple. Otherwise, we may emit
  // incorrect meta data.
  ParallelUtilities::WorkFuncTy WorkFun = [&](BinaryFunction &BF) {
    uint64_t HotSize, ColdSize;
    std::tie(HotSize, ColdSize) =
        BC.calculateEmittedSize(BF, /*FixBranches=*/false);
    uint64_t MainFragmentSize = HotSize;
    if (BF.hasIslandsInfo()) {
      MainFragmentSize +=
          offsetToAlignment(BF.getAddress() + MainFragmentSize,
                            Align(BF.getConstantIslandAlignment()));
      MainFragmentSize += BF.estimateConstantIslandSize();
    }
    if (MainFragmentSize > BF.getMaxSize()) {
      if (opts::PrintLargeFunctions)
        BC.outs() << "BOLT-INFO: " << BF << " size of " << MainFragmentSize
                  << " bytes exceeds allocated space by "
                  << (MainFragmentSize - BF.getMaxSize()) << " bytes\n";
      BF.setSimple(false);
    }
  };

  ParallelUtilities::PredicateTy SkipFunc = [&](const BinaryFunction &BF) {
    return !shouldOptimize(BF);
  };

  ParallelUtilities::runOnEachFunction(
      BC, ParallelUtilities::SchedulingPolicy::SP_INST_LINEAR, WorkFun,
      SkipFunc, "CheckLargeFunctions");

  return Error::success();
}

bool CheckLargeFunctions::shouldOptimize(const BinaryFunction &BF) const {
  // Unlike other passes, allow functions in non-CFG state.
  return BF.isSimple() && !BF.isIgnored();
}

Error LowerAnnotations::runOnFunctions(BinaryContext &BC) {
  // Convert GnuArgsSize annotations into CFIs.
  for (BinaryFunction *BF : BC.getAllBinaryFunctions()) {
    for (FunctionFragment &FF : BF->getLayout().fragments()) {
      // Reset at the start of the new fragment.
      int64_t CurrentGnuArgsSize = 0;

      for (BinaryBasicBlock *const BB : FF) {
        for (auto II = BB->begin(); II != BB->end(); ++II) {
          if (!BF->usesGnuArgsSize() || !BC.MIB->isInvoke(*II))
            continue;

          const int64_t NewGnuArgsSize = BC.MIB->getGnuArgsSize(*II);
          assert(NewGnuArgsSize >= 0 && "Expected non-negative GNU_args_size.");
          if (NewGnuArgsSize == CurrentGnuArgsSize)
            continue;

          auto InsertII = BF->addCFIInstruction(
              BB, II,
              MCCFIInstruction::createGnuArgsSize(nullptr, NewGnuArgsSize));
          CurrentGnuArgsSize = NewGnuArgsSize;
          II = std::next(InsertII);
        }
      }
    }
  }
  return Error::success();
}

// Check for dirty state in MCSymbol objects that might be a consequence
// of running calculateEmittedSize() in parallel, during split functions
// pass. If an inconsistent state is found (symbol already registered or
// already defined), clean it.
Error CleanMCState::runOnFunctions(BinaryContext &BC) {
  MCContext &Ctx = *BC.Ctx;
  for (const auto &SymMapEntry : Ctx.getSymbols()) {
    const MCSymbol *S = SymMapEntry.getValue().Symbol;
    if (!S)
      continue;
    if (S->isDefined()) {
      LLVM_DEBUG(dbgs() << "BOLT-DEBUG: Symbol \"" << S->getName()
                        << "\" is already defined\n");
      const_cast<MCSymbol *>(S)->setUndefined();
    }
    if (S->isRegistered()) {
      LLVM_DEBUG(dbgs() << "BOLT-DEBUG: Symbol \"" << S->getName()
                        << "\" is already registered\n");
      const_cast<MCSymbol *>(S)->setIsRegistered(false);
    }
    LLVM_DEBUG(if (S->isVariable()) {
      dbgs() << "BOLT-DEBUG: Symbol \"" << S->getName() << "\" is variable\n";
    });
  }
  return Error::success();
}

// This peephole fixes jump instructions that jump to another basic
// block with a single jump instruction, e.g.
//
// B0: ...
//     jmp  B1   (or jcc B1)
//
// B1: jmp  B2
//
// ->
//
// B0: ...
//     jmp  B2   (or jcc B2)
//
static uint64_t fixDoubleJumps(BinaryFunction &Function, bool MarkInvalid) {
  uint64_t NumDoubleJumps = 0;

  MCContext *Ctx = Function.getBinaryContext().Ctx.get();
  MCPlusBuilder *MIB = Function.getBinaryContext().MIB.get();
  for (BinaryBasicBlock &BB : Function) {
    auto checkAndPatch = [&](BinaryBasicBlock *Pred, BinaryBasicBlock *Succ,
                             const MCSymbol *SuccSym,
                             std::optional<uint32_t> Offset) {
      // Ignore infinite loop jumps or fallthrough tail jumps.
      if (Pred == Succ || Succ == &BB)
        return false;

      if (Succ) {
        const MCSymbol *TBB = nullptr;
        const MCSymbol *FBB = nullptr;
        MCInst *CondBranch = nullptr;
        MCInst *UncondBranch = nullptr;
        bool Res = Pred->analyzeBranch(TBB, FBB, CondBranch, UncondBranch);
        if (!Res) {
          LLVM_DEBUG(dbgs() << "analyzeBranch failed in peepholes in block:\n";
                     Pred->dump());
          return false;
        }
        Pred->replaceSuccessor(&BB, Succ);

        // We must patch up any existing branch instructions to match up
        // with the new successor.
        assert((CondBranch || (!CondBranch && Pred->succ_size() == 1)) &&
               "Predecessor block has inconsistent number of successors");
        if (CondBranch && MIB->getTargetSymbol(*CondBranch) == BB.getLabel()) {
          MIB->replaceBranchTarget(*CondBranch, Succ->getLabel(), Ctx);
        } else if (UncondBranch &&
                   MIB->getTargetSymbol(*UncondBranch) == BB.getLabel()) {
          MIB->replaceBranchTarget(*UncondBranch, Succ->getLabel(), Ctx);
        } else if (!UncondBranch) {
          assert(Function.getLayout().getBasicBlockAfter(Pred, false) != Succ &&
                 "Don't add an explicit jump to a fallthrough block.");
          Pred->addBranchInstruction(Succ);
        }
      } else {
        // Succ will be null in the tail call case.  In this case we
        // need to explicitly add a tail call instruction.
        MCInst *Branch = Pred->getLastNonPseudoInstr();
        if (Branch && MIB->isUnconditionalBranch(*Branch)) {
          assert(MIB->getTargetSymbol(*Branch) == BB.getLabel());
          Pred->removeSuccessor(&BB);
          Pred->eraseInstruction(Pred->findInstruction(Branch));
          Pred->addTailCallInstruction(SuccSym);
          if (Offset) {
            MCInst *TailCall = Pred->getLastNonPseudoInstr();
            assert(TailCall);
            MIB->setOffset(*TailCall, *Offset);
          }
        } else {
          return false;
        }
      }

      ++NumDoubleJumps;
      LLVM_DEBUG(dbgs() << "Removed double jump in " << Function << " from "
                        << Pred->getName() << " -> " << BB.getName() << " to "
                        << Pred->getName() << " -> " << SuccSym->getName()
                        << (!Succ ? " (tail)\n" : "\n"));

      return true;
    };

    if (BB.getNumNonPseudos() != 1 || BB.isLandingPad())
      continue;

    MCInst *Inst = BB.getFirstNonPseudoInstr();
    const bool IsTailCall = MIB->isTailCall(*Inst);

    if (!MIB->isUnconditionalBranch(*Inst) && !IsTailCall)
      continue;

    // If we operate after SCTC make sure it's not a conditional tail call.
    if (IsTailCall && MIB->isConditionalBranch(*Inst))
      continue;

    const MCSymbol *SuccSym = MIB->getTargetSymbol(*Inst);
    BinaryBasicBlock *Succ = BB.getSuccessor();

    if (((!Succ || &BB == Succ) && !IsTailCall) || (IsTailCall && !SuccSym))
      continue;

    std::vector<BinaryBasicBlock *> Preds = {BB.pred_begin(), BB.pred_end()};

    for (BinaryBasicBlock *Pred : Preds) {
      if (Pred->isLandingPad())
        continue;

      if (Pred->getSuccessor() == &BB ||
          (Pred->getConditionalSuccessor(true) == &BB && !IsTailCall) ||
          Pred->getConditionalSuccessor(false) == &BB)
        if (checkAndPatch(Pred, Succ, SuccSym, MIB->getOffset(*Inst)) &&
            MarkInvalid)
          BB.markValid(BB.pred_size() != 0 || BB.isLandingPad() ||
                       BB.isEntryPoint());
    }
  }

  return NumDoubleJumps;
}

bool SimplifyConditionalTailCalls::shouldRewriteBranch(
    const BinaryBasicBlock *PredBB, const MCInst &CondBranch,
    const BinaryBasicBlock *BB, const bool DirectionFlag) {
  if (BeenOptimized.count(PredBB))
    return false;

  const bool IsForward = BinaryFunction::isForwardBranch(PredBB, BB);

  if (IsForward)
    ++NumOrigForwardBranches;
  else
    ++NumOrigBackwardBranches;

  if (opts::SctcMode == opts::SctcAlways)
    return true;

  if (opts::SctcMode == opts::SctcPreserveDirection)
    return IsForward == DirectionFlag;

  const ErrorOr<std::pair<double, double>> Frequency =
      PredBB->getBranchStats(BB);

  // It's ok to rewrite the conditional branch if the new target will be
  // a backward branch.

  // If no data available for these branches, then it should be ok to
  // do the optimization since it will reduce code size.
  if (Frequency.getError())
    return true;

  // TODO: should this use misprediction frequency instead?
  const bool Result = (IsForward && Frequency.get().first >= 0.5) ||
                      (!IsForward && Frequency.get().first <= 0.5);

  return Result == DirectionFlag;
}

uint64_t SimplifyConditionalTailCalls::fixTailCalls(BinaryFunction &BF) {
  // Need updated indices to correctly detect branch' direction.
  BF.getLayout().updateLayoutIndices();
  BF.markUnreachableBlocks();

  MCPlusBuilder *MIB = BF.getBinaryContext().MIB.get();
  MCContext *Ctx = BF.getBinaryContext().Ctx.get();
  uint64_t NumLocalCTCCandidates = 0;
  uint64_t NumLocalCTCs = 0;
  uint64_t LocalCTCTakenCount = 0;
  uint64_t LocalCTCExecCount = 0;
  std::vector<std::pair<BinaryBasicBlock *, const BinaryBasicBlock *>>
      NeedsUncondBranch;

  // Will block be deleted by UCE?
  auto isValid = [](const BinaryBasicBlock *BB) {
    return (BB->pred_size() != 0 || BB->isLandingPad() || BB->isEntryPoint());
  };

  for (BinaryBasicBlock *BB : BF.getLayout().blocks()) {
    // Locate BB with a single direct tail-call instruction.
    if (BB->getNumNonPseudos() != 1)
      continue;

    MCInst *Instr = BB->getFirstNonPseudoInstr();
    if (!MIB->isTailCall(*Instr) || MIB->isConditionalBranch(*Instr))
      continue;

    const MCSymbol *CalleeSymbol = MIB->getTargetSymbol(*Instr);
    if (!CalleeSymbol)
      continue;

    // Detect direction of the possible conditional tail call.
    const bool IsForwardCTC = BF.isForwardCall(CalleeSymbol);

    // Iterate through all predecessors.
    for (BinaryBasicBlock *PredBB : BB->predecessors()) {
      BinaryBasicBlock *CondSucc = PredBB->getConditionalSuccessor(true);
      if (!CondSucc)
        continue;

      ++NumLocalCTCCandidates;

      const MCSymbol *TBB = nullptr;
      const MCSymbol *FBB = nullptr;
      MCInst *CondBranch = nullptr;
      MCInst *UncondBranch = nullptr;
      bool Result = PredBB->analyzeBranch(TBB, FBB, CondBranch, UncondBranch);

      // analyzeBranch() can fail due to unusual branch instructions, e.g. jrcxz
      if (!Result) {
        LLVM_DEBUG(dbgs() << "analyzeBranch failed in SCTC in block:\n";
                   PredBB->dump());
        continue;
      }

      assert(Result && "internal error analyzing conditional branch");
      assert(CondBranch && "conditional branch expected");

      // Skip dynamic branches for now.
      if (BF.getBinaryContext().MIB->isDynamicBranch(*CondBranch))
        continue;

      // It's possible that PredBB is also a successor to BB that may have
      // been processed by a previous iteration of the SCTC loop, in which
      // case it may have been marked invalid.  We should skip rewriting in
      // this case.
      if (!PredBB->isValid()) {
        assert(PredBB->isSuccessor(BB) &&
               "PredBB should be valid if it is not a successor to BB");
        continue;
      }

      // We don't want to reverse direction of the branch in new order
      // without further profile analysis.
      const bool DirectionFlag = CondSucc == BB ? IsForwardCTC : !IsForwardCTC;
      if (!shouldRewriteBranch(PredBB, *CondBranch, BB, DirectionFlag))
        continue;

      // Record this block so that we don't try to optimize it twice.
      BeenOptimized.insert(PredBB);

      uint64_t Count = 0;
      if (CondSucc != BB) {
        // Patch the new target address into the conditional branch.
        MIB->reverseBranchCondition(*CondBranch, CalleeSymbol, Ctx);
        // Since we reversed the condition on the branch we need to change
        // the target for the unconditional branch or add a unconditional
        // branch to the old target.  This has to be done manually since
        // fixupBranches is not called after SCTC.
        NeedsUncondBranch.emplace_back(PredBB, CondSucc);
        Count = PredBB->getFallthroughBranchInfo().Count;
      } else {
        // Change destination of the conditional branch.
        MIB->replaceBranchTarget(*CondBranch, CalleeSymbol, Ctx);
        Count = PredBB->getTakenBranchInfo().Count;
      }
      const uint64_t CTCTakenFreq =
          Count == BinaryBasicBlock::COUNT_NO_PROFILE ? 0 : Count;

      // Annotate it, so "isCall" returns true for this jcc
      MIB->setConditionalTailCall(*CondBranch);
      // Add info about the conditional tail call frequency, otherwise this
      // info will be lost when we delete the associated BranchInfo entry
      auto &CTCAnnotation =
          MIB->getOrCreateAnnotationAs<uint64_t>(*CondBranch, "CTCTakenCount");
      CTCAnnotation = CTCTakenFreq;
      // Preserve Offset annotation, used in BAT.
      // Instr is a direct tail call instruction that was created when CTCs are
      // first expanded, and has the original CTC offset set.
      if (std::optional<uint32_t> Offset = MIB->getOffset(*Instr))
        MIB->setOffset(*CondBranch, *Offset);

      // Remove the unused successor which may be eliminated later
      // if there are no other users.
      PredBB->removeSuccessor(BB);
      // Update BB execution count
      if (CTCTakenFreq && CTCTakenFreq <= BB->getKnownExecutionCount())
        BB->setExecutionCount(BB->getExecutionCount() - CTCTakenFreq);
      else if (CTCTakenFreq > BB->getKnownExecutionCount())
        BB->setExecutionCount(0);

      ++NumLocalCTCs;
      LocalCTCTakenCount += CTCTakenFreq;
      LocalCTCExecCount += PredBB->getKnownExecutionCount();
    }

    // Remove the block from CFG if all predecessors were removed.
    BB->markValid(isValid(BB));
  }

  // Add unconditional branches at the end of BBs to new successors
  // as long as the successor is not a fallthrough.
  for (auto &Entry : NeedsUncondBranch) {
    BinaryBasicBlock *PredBB = Entry.first;
    const BinaryBasicBlock *CondSucc = Entry.second;

    const MCSymbol *TBB = nullptr;
    const MCSymbol *FBB = nullptr;
    MCInst *CondBranch = nullptr;
    MCInst *UncondBranch = nullptr;
    PredBB->analyzeBranch(TBB, FBB, CondBranch, UncondBranch);

    // Find the next valid block.  Invalid blocks will be deleted
    // so they shouldn't be considered fallthrough targets.
    const BinaryBasicBlock *NextBlock =
        BF.getLayout().getBasicBlockAfter(PredBB, false);
    while (NextBlock && !isValid(NextBlock))
      NextBlock = BF.getLayout().getBasicBlockAfter(NextBlock, false);

    // Get the unconditional successor to this block.
    const BinaryBasicBlock *PredSucc = PredBB->getSuccessor();
    assert(PredSucc && "The other branch should be a tail call");

    const bool HasFallthrough = (NextBlock && PredSucc == NextBlock);

    if (UncondBranch) {
      if (HasFallthrough)
        PredBB->eraseInstruction(PredBB->findInstruction(UncondBranch));
      else
        MIB->replaceBranchTarget(*UncondBranch, CondSucc->getLabel(), Ctx);
    } else if (!HasFallthrough) {
      MCInst Branch;
      MIB->createUncondBranch(Branch, CondSucc->getLabel(), Ctx);
      PredBB->addInstruction(Branch);
    }
  }

  if (NumLocalCTCs > 0) {
    NumDoubleJumps += fixDoubleJumps(BF, true);
    // Clean-up unreachable tail-call blocks.
    const std::pair<unsigned, uint64_t> Stats = BF.eraseInvalidBBs();
    DeletedBlocks += Stats.first;
    DeletedBytes += Stats.second;

    assert(BF.validateCFG());
  }

  LLVM_DEBUG(dbgs() << "BOLT: created " << NumLocalCTCs
                    << " conditional tail calls from a total of "
                    << NumLocalCTCCandidates << " candidates in function " << BF
                    << ". CTCs execution count for this function is "
                    << LocalCTCExecCount << " and CTC taken count is "
                    << LocalCTCTakenCount << "\n";);

  NumTailCallsPatched += NumLocalCTCs;
  NumCandidateTailCalls += NumLocalCTCCandidates;
  CTCExecCount += LocalCTCExecCount;
  CTCTakenCount += LocalCTCTakenCount;

  return NumLocalCTCs > 0;
}

Error SimplifyConditionalTailCalls::runOnFunctions(BinaryContext &BC) {
  if (!BC.isX86())
    return Error::success();

  for (auto &It : BC.getBinaryFunctions()) {
    BinaryFunction &Function = It.second;

    if (!shouldOptimize(Function))
      continue;

    if (fixTailCalls(Function)) {
      Modified.insert(&Function);
      Function.setHasCanonicalCFG(false);
    }
  }

  if (NumTailCallsPatched)
    BC.outs() << "BOLT-INFO: SCTC: patched " << NumTailCallsPatched
              << " tail calls (" << NumOrigForwardBranches << " forward)"
              << " tail calls (" << NumOrigBackwardBranches << " backward)"
              << " from a total of " << NumCandidateTailCalls
              << " while removing " << NumDoubleJumps << " double jumps"
              << " and removing " << DeletedBlocks << " basic blocks"
              << " totalling " << DeletedBytes
              << " bytes of code. CTCs total execution count is "
              << CTCExecCount << " and the number of times CTCs are taken is "
              << CTCTakenCount << "\n";
  return Error::success();
}

uint64_t ShortenInstructions::shortenInstructions(BinaryFunction &Function) {
  uint64_t Count = 0;
  const BinaryContext &BC = Function.getBinaryContext();
  for (BinaryBasicBlock &BB : Function) {
    for (MCInst &Inst : BB) {
      // Skip shortening instructions with Size annotation.
      if (BC.MIB->getSize(Inst))
        continue;

      MCInst OriginalInst;
      if (opts::Verbosity > 2)
        OriginalInst = Inst;

      if (!BC.MIB->shortenInstruction(Inst, *BC.STI))
        continue;

      if (opts::Verbosity > 2) {
        BC.scopeLock();
        BC.outs() << "BOLT-INFO: shortening:\nBOLT-INFO:    ";
        BC.printInstruction(BC.outs(), OriginalInst, 0, &Function);
        BC.outs() << "BOLT-INFO: to:";
        BC.printInstruction(BC.outs(), Inst, 0, &Function);
      }

      ++Count;
    }
  }

  return Count;
}

Error ShortenInstructions::runOnFunctions(BinaryContext &BC) {
  std::atomic<uint64_t> NumShortened{0};
  if (!BC.isX86())
    return Error::success();

  ParallelUtilities::runOnEachFunction(
      BC, ParallelUtilities::SchedulingPolicy::SP_INST_LINEAR,
      [&](BinaryFunction &BF) { NumShortened += shortenInstructions(BF); },
      nullptr, "ShortenInstructions");

  if (NumShortened)
    BC.outs() << "BOLT-INFO: " << NumShortened
              << " instructions were shortened\n";
  return Error::success();
}

void Peepholes::addTailcallTraps(BinaryFunction &Function) {
  MCPlusBuilder *MIB = Function.getBinaryContext().MIB.get();
  for (BinaryBasicBlock &BB : Function) {
    MCInst *Inst = BB.getLastNonPseudoInstr();
    if (Inst && MIB->isTailCall(*Inst) && MIB->isIndirectBranch(*Inst)) {
      MCInst Trap;
      MIB->createTrap(Trap);
      BB.addInstruction(Trap);
      ++TailCallTraps;
    }
  }
}

void Peepholes::removeUselessCondBranches(BinaryFunction &Function) {
  for (BinaryBasicBlock &BB : Function) {
    if (BB.succ_size() != 2)
      continue;

    BinaryBasicBlock *CondBB = BB.getConditionalSuccessor(true);
    BinaryBasicBlock *UncondBB = BB.getConditionalSuccessor(false);
    if (CondBB != UncondBB)
      continue;

    const MCSymbol *TBB = nullptr;
    const MCSymbol *FBB = nullptr;
    MCInst *CondBranch = nullptr;
    MCInst *UncondBranch = nullptr;
    bool Result = BB.analyzeBranch(TBB, FBB, CondBranch, UncondBranch);

    // analyzeBranch() can fail due to unusual branch instructions,
    // e.g. jrcxz, or jump tables (indirect jump).
    if (!Result || !CondBranch)
      continue;

    BB.removeDuplicateConditionalSuccessor(CondBranch);
    ++NumUselessCondBranches;
  }
}

Error Peepholes::runOnFunctions(BinaryContext &BC) {
  const char Opts =
      std::accumulate(opts::Peepholes.begin(), opts::Peepholes.end(), 0,
                      [](const char A, const PeepholeOpts B) { return A | B; });
  if (Opts == PEEP_NONE)
    return Error::success();

  for (auto &It : BC.getBinaryFunctions()) {
    BinaryFunction &Function = It.second;
    if (shouldOptimize(Function)) {
      if (Opts & PEEP_DOUBLE_JUMPS)
        NumDoubleJumps += fixDoubleJumps(Function, false);
      if (Opts & PEEP_TAILCALL_TRAPS)
        addTailcallTraps(Function);
      if (Opts & PEEP_USELESS_BRANCHES)
        removeUselessCondBranches(Function);
      assert(Function.validateCFG());
    }
  }
  BC.outs() << "BOLT-INFO: Peephole: " << NumDoubleJumps
            << " double jumps patched.\n"
            << "BOLT-INFO: Peephole: " << TailCallTraps
            << " tail call traps inserted.\n"
            << "BOLT-INFO: Peephole: " << NumUselessCondBranches
            << " useless conditional branches removed.\n";
  return Error::success();
}

bool SimplifyRODataLoads::simplifyRODataLoads(BinaryFunction &BF) {
  BinaryContext &BC = BF.getBinaryContext();
  MCPlusBuilder *MIB = BC.MIB.get();

  uint64_t NumLocalLoadsSimplified = 0;
  uint64_t NumDynamicLocalLoadsSimplified = 0;
  uint64_t NumLocalLoadsFound = 0;
  uint64_t NumDynamicLocalLoadsFound = 0;

  for (BinaryBasicBlock *BB : BF.getLayout().blocks()) {
    for (MCInst &Inst : *BB) {
      unsigned Opcode = Inst.getOpcode();
      const MCInstrDesc &Desc = BC.MII->get(Opcode);

      // Skip instructions that do not load from memory.
      if (!Desc.mayLoad())
        continue;

      // Try to statically evaluate the target memory address;
      uint64_t TargetAddress;

      if (MIB->hasPCRelOperand(Inst)) {
        // Try to find the symbol that corresponds to the PC-relative operand.
        MCOperand *DispOpI = MIB->getMemOperandDisp(Inst);
        assert(DispOpI != Inst.end() && "expected PC-relative displacement");
        assert(DispOpI->isExpr() &&
               "found PC-relative with non-symbolic displacement");

        // Get displacement symbol.
        const MCSymbol *DisplSymbol;
        uint64_t DisplOffset;

        std::tie(DisplSymbol, DisplOffset) =
            MIB->getTargetSymbolInfo(DispOpI->getExpr());

        if (!DisplSymbol)
          continue;

        // Look up the symbol address in the global symbols map of the binary
        // context object.
        BinaryData *BD = BC.getBinaryDataByName(DisplSymbol->getName());
        if (!BD)
          continue;
        TargetAddress = BD->getAddress() + DisplOffset;
      } else if (!MIB->evaluateMemOperandTarget(Inst, TargetAddress)) {
        continue;
      }

      // Get the contents of the section containing the target address of the
      // memory operand. We are only interested in read-only sections.
      ErrorOr<BinarySection &> DataSection =
          BC.getSectionForAddress(TargetAddress);
      if (!DataSection || DataSection->isWritable())
        continue;

      if (BC.getRelocationAt(TargetAddress) ||
          BC.getDynamicRelocationAt(TargetAddress))
        continue;

      uint32_t Offset = TargetAddress - DataSection->getAddress();
      StringRef ConstantData = DataSection->getContents();

      ++NumLocalLoadsFound;
      if (BB->hasProfile())
        NumDynamicLocalLoadsFound += BB->getExecutionCount();

      if (MIB->replaceMemOperandWithImm(Inst, ConstantData, Offset)) {
        ++NumLocalLoadsSimplified;
        if (BB->hasProfile())
          NumDynamicLocalLoadsSimplified += BB->getExecutionCount();
      }
    }
  }

  NumLoadsFound += NumLocalLoadsFound;
  NumDynamicLoadsFound += NumDynamicLocalLoadsFound;
  NumLoadsSimplified += NumLocalLoadsSimplified;
  NumDynamicLoadsSimplified += NumDynamicLocalLoadsSimplified;

  return NumLocalLoadsSimplified > 0;
}

Error SimplifyRODataLoads::runOnFunctions(BinaryContext &BC) {
  for (auto &It : BC.getBinaryFunctions()) {
    BinaryFunction &Function = It.second;
    if (shouldOptimize(Function) && simplifyRODataLoads(Function))
      Modified.insert(&Function);
  }

  BC.outs() << "BOLT-INFO: simplified " << NumLoadsSimplified << " out of "
            << NumLoadsFound << " loads from a statically computed address.\n"
            << "BOLT-INFO: dynamic loads simplified: "
            << NumDynamicLoadsSimplified << "\n"
            << "BOLT-INFO: dynamic loads found: " << NumDynamicLoadsFound
            << "\n";
  return Error::success();
}

Error AssignSections::runOnFunctions(BinaryContext &BC) {
  for (BinaryFunction *Function : BC.getInjectedBinaryFunctions()) {
    if (!Function->isPatch()) {
      Function->setCodeSectionName(BC.getInjectedCodeSectionName());
      Function->setColdCodeSectionName(BC.getInjectedColdCodeSectionName());
    }
  }

  // In non-relocation mode functions have pre-assigned section names.
  if (!BC.HasRelocations)
    return Error::success();

  const bool UseColdSection =
      BC.NumProfiledFuncs > 0 ||
      opts::ReorderFunctions == ReorderFunctions::RT_USER;
  for (auto &BFI : BC.getBinaryFunctions()) {
    BinaryFunction &Function = BFI.second;
    if (opts::isHotTextMover(Function)) {
      Function.setCodeSectionName(BC.getHotTextMoverSectionName());
      Function.setColdCodeSectionName(BC.getHotTextMoverSectionName());
      continue;
    }

    if (!UseColdSection || Function.hasValidIndex())
      Function.setCodeSectionName(BC.getMainCodeSectionName());
    else
      Function.setCodeSectionName(BC.getColdCodeSectionName());

    if (Function.isSplit())
      Function.setColdCodeSectionName(BC.getColdCodeSectionName());
  }
  return Error::success();
}

Error PrintProfileStats::runOnFunctions(BinaryContext &BC) {
  double FlowImbalanceMean = 0.0;
  size_t NumBlocksConsidered = 0;
  double WorstBias = 0.0;
  const BinaryFunction *WorstBiasFunc = nullptr;

  // For each function CFG, we fill an IncomingMap with the sum of the frequency
  // of incoming edges for each BB. Likewise for each OutgoingMap and the sum
  // of the frequency of outgoing edges.
  using FlowMapTy = std::unordered_map<const BinaryBasicBlock *, uint64_t>;
  std::unordered_map<const BinaryFunction *, FlowMapTy> TotalIncomingMaps;
  std::unordered_map<const BinaryFunction *, FlowMapTy> TotalOutgoingMaps;

  // Compute mean
  for (const auto &BFI : BC.getBinaryFunctions()) {
    const BinaryFunction &Function = BFI.second;
    if (Function.empty() || !Function.isSimple())
      continue;
    FlowMapTy &IncomingMap = TotalIncomingMaps[&Function];
    FlowMapTy &OutgoingMap = TotalOutgoingMaps[&Function];
    for (const BinaryBasicBlock &BB : Function) {
      uint64_t TotalOutgoing = 0ULL;
      auto SuccBIIter = BB.branch_info_begin();
      for (BinaryBasicBlock *Succ : BB.successors()) {
        uint64_t Count = SuccBIIter->Count;
        if (Count == BinaryBasicBlock::COUNT_NO_PROFILE || Count == 0) {
          ++SuccBIIter;
          continue;
        }
        TotalOutgoing += Count;
        IncomingMap[Succ] += Count;
        ++SuccBIIter;
      }
      OutgoingMap[&BB] = TotalOutgoing;
    }

    size_t NumBlocks = 0;
    double Mean = 0.0;
    for (const BinaryBasicBlock &BB : Function) {
      // Do not compute score for low frequency blocks, entry or exit blocks
      if (IncomingMap[&BB] < 100 || OutgoingMap[&BB] == 0 || BB.isEntryPoint())
        continue;
      ++NumBlocks;
      const double Difference = (double)OutgoingMap[&BB] - IncomingMap[&BB];
      Mean += fabs(Difference / IncomingMap[&BB]);
    }

    FlowImbalanceMean += Mean;
    NumBlocksConsidered += NumBlocks;
    if (!NumBlocks)
      continue;
    double FuncMean = Mean / NumBlocks;
    if (FuncMean > WorstBias) {
      WorstBias = FuncMean;
      WorstBiasFunc = &Function;
    }
  }
  if (NumBlocksConsidered > 0)
    FlowImbalanceMean /= NumBlocksConsidered;

  // Compute standard deviation
  NumBlocksConsidered = 0;
  double FlowImbalanceVar = 0.0;
  for (const auto &BFI : BC.getBinaryFunctions()) {
    const BinaryFunction &Function = BFI.second;
    if (Function.empty() || !Function.isSimple())
      continue;
    FlowMapTy &IncomingMap = TotalIncomingMaps[&Function];
    FlowMapTy &OutgoingMap = TotalOutgoingMaps[&Function];
    for (const BinaryBasicBlock &BB : Function) {
      if (IncomingMap[&BB] < 100 || OutgoingMap[&BB] == 0)
        continue;
      ++NumBlocksConsidered;
      const double Difference = (double)OutgoingMap[&BB] - IncomingMap[&BB];
      FlowImbalanceVar +=
          pow(fabs(Difference / IncomingMap[&BB]) - FlowImbalanceMean, 2);
    }
  }
  if (NumBlocksConsidered) {
    FlowImbalanceVar /= NumBlocksConsidered;
    FlowImbalanceVar = sqrt(FlowImbalanceVar);
  }

  // Report to user
  BC.outs() << format("BOLT-INFO: Profile bias score: %.4lf%% StDev: %.4lf%%\n",
                      (100.0 * FlowImbalanceMean), (100.0 * FlowImbalanceVar));
  if (WorstBiasFunc && opts::Verbosity >= 1) {
    BC.outs() << "Worst average bias observed in "
              << WorstBiasFunc->getPrintName() << "\n";
    LLVM_DEBUG(WorstBiasFunc->dump());
  }
  return Error::success();
}

Error PrintProgramStats::runOnFunctions(BinaryContext &BC) {
  uint64_t NumRegularFunctions = 0;
  uint64_t NumStaleProfileFunctions = 0;
  uint64_t NumAllStaleFunctions = 0;
  uint64_t NumInferredFunctions = 0;
  uint64_t NumNonSimpleProfiledFunctions = 0;
  uint64_t NumUnknownControlFlowFunctions = 0;
  uint64_t TotalSampleCount = 0;
  uint64_t StaleSampleCount = 0;
  uint64_t InferredSampleCount = 0;
  std::vector<const BinaryFunction *> ProfiledFunctions;
  std::vector<std::pair<double, uint64_t>> FuncDensityList;
  const char *StaleFuncsHeader = "BOLT-INFO: Functions with stale profile:\n";
  for (auto &BFI : BC.getBinaryFunctions()) {
    const BinaryFunction &Function = BFI.second;

    // Ignore PLT functions for stats.
    if (Function.isPLTFunction())
      continue;

    // Adjustment for BAT mode: the profile for BOLT split fragments is combined
    // so only count the hot fragment.
    const uint64_t Address = Function.getAddress();
    bool IsHotParentOfBOLTSplitFunction = !Function.getFragments().empty() &&
                                          BAT && BAT->isBATFunction(Address) &&
                                          !BAT->fetchParentAddress(Address);

    ++NumRegularFunctions;

    // In BOLTed binaries split functions are non-simple (due to non-relocation
    // mode), but the original function is known to be simple and we have a
    // valid profile for it.
    if (!Function.isSimple() && !IsHotParentOfBOLTSplitFunction) {
      if (Function.hasProfile())
        ++NumNonSimpleProfiledFunctions;
      continue;
    }

    if (Function.hasUnknownControlFlow()) {
      if (opts::PrintUnknownCFG)
        Function.dump();
      else if (opts::PrintUnknown)
        BC.errs() << "function with unknown control flow: " << Function << '\n';

      ++NumUnknownControlFlowFunctions;
    }

    if (!Function.hasProfile())
      continue;

    uint64_t SampleCount = Function.getRawSampleCount();
    TotalSampleCount += SampleCount;

    if (Function.hasValidProfile()) {
      ProfiledFunctions.push_back(&Function);
      if (Function.hasInferredProfile()) {
        ++NumInferredFunctions;
        InferredSampleCount += SampleCount;
        ++NumAllStaleFunctions;
      }
    } else {
      if (opts::ReportStaleFuncs) {
        BC.outs() << StaleFuncsHeader;
        StaleFuncsHeader = "";
        BC.outs() << "  " << Function << '\n';
      }
      ++NumStaleProfileFunctions;
      StaleSampleCount += SampleCount;
      ++NumAllStaleFunctions;
    }

    if (opts::ShowDensity) {
      uint64_t Size = Function.getSize();
      // In case of BOLT split functions registered in BAT, executed traces are
      // automatically attributed to the main fragment. Add up function sizes
      // for all fragments.
      if (IsHotParentOfBOLTSplitFunction)
        for (const BinaryFunction *Fragment : Function.getFragments())
          Size += Fragment->getSize();
      double Density = (double)1.0 * Function.getSampleCountInBytes() / Size;
      FuncDensityList.emplace_back(Density, SampleCount);
      LLVM_DEBUG(BC.outs() << Function << ": executed bytes "
                           << Function.getSampleCountInBytes() << ", size (b) "
                           << Size << ", density " << Density
                           << ", sample count " << SampleCount << '\n');
    }
  }
  BC.NumProfiledFuncs = ProfiledFunctions.size();
  BC.NumStaleProfileFuncs = NumStaleProfileFunctions;

  const size_t NumAllProfiledFunctions =
      ProfiledFunctions.size() + NumStaleProfileFunctions;
  BC.outs() << "BOLT-INFO: " << NumAllProfiledFunctions << " out of "
            << NumRegularFunctions << " functions in the binary ("
            << format("%.1f", NumAllProfiledFunctions /
                                  (float)NumRegularFunctions * 100.0f)
            << "%) have non-empty execution profile\n";
  if (NumNonSimpleProfiledFunctions) {
    BC.outs() << "BOLT-INFO: " << NumNonSimpleProfiledFunctions << " function"
              << (NumNonSimpleProfiledFunctions == 1 ? "" : "s")
              << " with profile could not be optimized\n";
  }
  if (NumAllStaleFunctions) {
    const float PctStale =
        NumAllStaleFunctions / (float)NumAllProfiledFunctions * 100.0f;
    const float PctStaleFuncsWithEqualBlockCount =
        (float)BC.Stats.NumStaleFuncsWithEqualBlockCount /
        NumAllStaleFunctions * 100.0f;
    const float PctStaleBlocksWithEqualIcount =
        (float)BC.Stats.NumStaleBlocksWithEqualIcount /
        BC.Stats.NumStaleBlocks * 100.0f;
    auto printErrorOrWarning = [&]() {
      if (PctStale > opts::StaleThreshold)
        BC.errs() << "BOLT-ERROR: ";
      else
        BC.errs() << "BOLT-WARNING: ";
    };
    printErrorOrWarning();
    BC.errs() << NumAllStaleFunctions
              << format(" (%.1f%% of all profiled)", PctStale) << " function"
              << (NumAllStaleFunctions == 1 ? "" : "s")
              << " have invalid (possibly stale) profile."
                 " Use -report-stale to see the list.\n";
    if (TotalSampleCount > 0) {
      printErrorOrWarning();
      BC.errs() << (StaleSampleCount + InferredSampleCount) << " out of "
                << TotalSampleCount << " samples in the binary ("
                << format("%.1f",
                          ((100.0f * (StaleSampleCount + InferredSampleCount)) /
                           TotalSampleCount))
                << "%) belong to functions with invalid"
                   " (possibly stale) profile.\n";
    }
    BC.outs() << "BOLT-INFO: " << BC.Stats.NumStaleFuncsWithEqualBlockCount
              << " stale function"
              << (BC.Stats.NumStaleFuncsWithEqualBlockCount == 1 ? "" : "s")
              << format(" (%.1f%% of all stale)",
                        PctStaleFuncsWithEqualBlockCount)
              << " have matching block count.\n";
    BC.outs() << "BOLT-INFO: " << BC.Stats.NumStaleBlocksWithEqualIcount
              << " stale block"
              << (BC.Stats.NumStaleBlocksWithEqualIcount == 1 ? "" : "s")
              << format(" (%.1f%% of all stale)", PctStaleBlocksWithEqualIcount)
              << " have matching icount.\n";
    if (PctStale > opts::StaleThreshold) {
      return createFatalBOLTError(
          Twine("BOLT-ERROR: stale functions exceed specified threshold of ") +
          Twine(opts::StaleThreshold.getValue()) + Twine("%. Exiting.\n"));
    }
  }
  if (NumInferredFunctions) {
    BC.outs() << format(
        "BOLT-INFO: inferred profile for %d (%.2f%% of profiled, "
        "%.2f%% of stale) functions responsible for %.2f%% samples"
        " (%zu out of %zu)\n",
        NumInferredFunctions,
        100.0 * NumInferredFunctions / NumAllProfiledFunctions,
        100.0 * NumInferredFunctions / NumAllStaleFunctions,
        100.0 * InferredSampleCount / TotalSampleCount, InferredSampleCount,
        TotalSampleCount);
    BC.outs() << format(
        "BOLT-INFO: inference found an exact match for %.2f%% of basic blocks"
        " (%zu out of %zu stale) responsible for %.2f%% samples"
        " (%zu out of %zu stale)\n",
        100.0 * BC.Stats.NumExactMatchedBlocks / BC.Stats.NumStaleBlocks,
        BC.Stats.NumExactMatchedBlocks, BC.Stats.NumStaleBlocks,
        100.0 * BC.Stats.ExactMatchedSampleCount / BC.Stats.StaleSampleCount,
        BC.Stats.ExactMatchedSampleCount, BC.Stats.StaleSampleCount);
    BC.outs() << format(
        "BOLT-INFO: inference found an exact pseudo probe match for %.2f%% of "
        "basic blocks (%zu out of %zu stale) responsible for %.2f%% samples"
        " (%zu out of %zu stale)\n",
        100.0 * BC.Stats.NumPseudoProbeExactMatchedBlocks /
            BC.Stats.NumStaleBlocks,
        BC.Stats.NumPseudoProbeExactMatchedBlocks, BC.Stats.NumStaleBlocks,
        100.0 * BC.Stats.PseudoProbeExactMatchedSampleCount /
            BC.Stats.StaleSampleCount,
        BC.Stats.PseudoProbeExactMatchedSampleCount, BC.Stats.StaleSampleCount);
    BC.outs() << format(
        "BOLT-INFO: inference found a loose pseudo probe match for %.2f%% of "
        "basic blocks (%zu out of %zu stale) responsible for %.2f%% samples"
        " (%zu out of %zu stale)\n",
        100.0 * BC.Stats.NumPseudoProbeLooseMatchedBlocks /
            BC.Stats.NumStaleBlocks,
        BC.Stats.NumPseudoProbeLooseMatchedBlocks, BC.Stats.NumStaleBlocks,
        100.0 * BC.Stats.PseudoProbeLooseMatchedSampleCount /
            BC.Stats.StaleSampleCount,
        BC.Stats.PseudoProbeLooseMatchedSampleCount, BC.Stats.StaleSampleCount);
    BC.outs() << format(
        "BOLT-INFO: inference found a call match for %.2f%% of basic "
        "blocks"
        " (%zu out of %zu stale) responsible for %.2f%% samples"
        " (%zu out of %zu stale)\n",
        100.0 * BC.Stats.NumCallMatchedBlocks / BC.Stats.NumStaleBlocks,
        BC.Stats.NumCallMatchedBlocks, BC.Stats.NumStaleBlocks,
        100.0 * BC.Stats.CallMatchedSampleCount / BC.Stats.StaleSampleCount,
        BC.Stats.CallMatchedSampleCount, BC.Stats.StaleSampleCount);
    BC.outs() << format(
        "BOLT-INFO: inference found a loose match for %.2f%% of basic "
        "blocks"
        " (%zu out of %zu stale) responsible for %.2f%% samples"
        " (%zu out of %zu stale)\n",
        100.0 * BC.Stats.NumLooseMatchedBlocks / BC.Stats.NumStaleBlocks,
        BC.Stats.NumLooseMatchedBlocks, BC.Stats.NumStaleBlocks,
        100.0 * BC.Stats.LooseMatchedSampleCount / BC.Stats.StaleSampleCount,
        BC.Stats.LooseMatchedSampleCount, BC.Stats.StaleSampleCount);
  }

  if (const uint64_t NumUnusedObjects = BC.getNumUnusedProfiledObjects()) {
    BC.outs() << "BOLT-INFO: profile for " << NumUnusedObjects
              << " objects was ignored\n";
  }

  if (ProfiledFunctions.size() > 10) {
    if (opts::Verbosity >= 1) {
      BC.outs() << "BOLT-INFO: top called functions are:\n";
      llvm::sort(ProfiledFunctions,
                 [](const BinaryFunction *A, const BinaryFunction *B) {
                   return B->getExecutionCount() < A->getExecutionCount();
                 });
      auto SFI = ProfiledFunctions.begin();
      auto SFIend = ProfiledFunctions.end();
      for (unsigned I = 0u; I < opts::TopCalledLimit && SFI != SFIend;
           ++SFI, ++I)
        BC.outs() << "  " << **SFI << " : " << (*SFI)->getExecutionCount()
                  << '\n';
    }
  }

  if (!opts::PrintSortedBy.empty()) {
    std::vector<BinaryFunction *> Functions;
    std::map<const BinaryFunction *, DynoStats> Stats;

    for (auto &BFI : BC.getBinaryFunctions()) {
      BinaryFunction &BF = BFI.second;
      if (shouldOptimize(BF) && BF.hasValidProfile()) {
        Functions.push_back(&BF);
        Stats.emplace(&BF, getDynoStats(BF));
      }
    }

    const bool SortAll =
        llvm::is_contained(opts::PrintSortedBy, DynoStats::LAST_DYNO_STAT);

    const bool Ascending =
        opts::DynoStatsSortOrderOpt == opts::DynoStatsSortOrder::Ascending;

    std::function<bool(const DynoStats &, const DynoStats &)>
        DynoStatsComparator =
            SortAll ? [](const DynoStats &StatsA,
                         const DynoStats &StatsB) { return StatsA < StatsB; }
                    : [](const DynoStats &StatsA, const DynoStats &StatsB) {
                        return StatsA.lessThan(StatsB, opts::PrintSortedBy);
                      };

    llvm::stable_sort(Functions,
                      [Ascending, &Stats, DynoStatsComparator](
                          const BinaryFunction *A, const BinaryFunction *B) {
                        auto StatsItr = Stats.find(A);
                        assert(StatsItr != Stats.end());
                        const DynoStats &StatsA = StatsItr->second;

                        StatsItr = Stats.find(B);
                        assert(StatsItr != Stats.end());
                        const DynoStats &StatsB = StatsItr->second;

                        return Ascending ? DynoStatsComparator(StatsA, StatsB)
                                         : DynoStatsComparator(StatsB, StatsA);
                      });

    BC.outs() << "BOLT-INFO: top functions sorted by ";
    if (SortAll) {
      BC.outs() << "dyno stats";
    } else {
      BC.outs() << "(";
      bool PrintComma = false;
      for (const DynoStats::Category Category : opts::PrintSortedBy) {
        if (PrintComma)
          BC.outs() << ", ";
        BC.outs() << DynoStats::Description(Category);
        PrintComma = true;
      }
      BC.outs() << ")";
    }

    BC.outs() << " are:\n";
    auto SFI = Functions.begin();
    for (unsigned I = 0; I < 100 && SFI != Functions.end(); ++SFI, ++I) {
      const DynoStats Stats = getDynoStats(**SFI);
      BC.outs() << "  " << **SFI;
      if (!SortAll) {
        BC.outs() << " (";
        bool PrintComma = false;
        for (const DynoStats::Category Category : opts::PrintSortedBy) {
          if (PrintComma)
            BC.outs() << ", ";
          BC.outs() << dynoStatsOptName(Category) << "=" << Stats[Category];
          PrintComma = true;
        }
        BC.outs() << ")";
      }
      BC.outs() << "\n";
    }
  }

  if (!BC.TrappedFunctions.empty()) {
    BC.errs() << "BOLT-WARNING: " << BC.TrappedFunctions.size() << " function"
              << (BC.TrappedFunctions.size() > 1 ? "s" : "")
              << " will trap on entry. Use -trap-avx512=0 to disable"
                 " traps.";
    if (opts::Verbosity >= 1 || BC.TrappedFunctions.size() <= 5) {
      BC.errs() << '\n';
      for (const BinaryFunction *Function : BC.TrappedFunctions)
        BC.errs() << "  " << *Function << '\n';
    } else {
      BC.errs() << " Use -v=1 to see the list.\n";
    }
  }

  // Collect and print information about suboptimal code layout on input.
  if (opts::ReportBadLayout) {
    std::vector<BinaryFunction *> SuboptimalFuncs;
    for (auto &BFI : BC.getBinaryFunctions()) {
      BinaryFunction &BF = BFI.second;
      if (!BF.hasValidProfile())
        continue;

      const uint64_t HotThreshold =
          std::max<uint64_t>(BF.getKnownExecutionCount(), 1);
      bool HotSeen = false;
      for (const BinaryBasicBlock *BB : BF.getLayout().rblocks()) {
        if (!HotSeen && BB->getKnownExecutionCount() > HotThreshold) {
          HotSeen = true;
          continue;
        }
        if (HotSeen && BB->getKnownExecutionCount() == 0) {
          SuboptimalFuncs.push_back(&BF);
          break;
        }
      }
    }

    if (!SuboptimalFuncs.empty()) {
      llvm::sort(SuboptimalFuncs,
                 [](const BinaryFunction *A, const BinaryFunction *B) {
                   return A->getKnownExecutionCount() / A->getSize() >
                          B->getKnownExecutionCount() / B->getSize();
                 });

      BC.outs() << "BOLT-INFO: " << SuboptimalFuncs.size()
                << " functions have "
                   "cold code in the middle of hot code. Top functions are:\n";
      for (unsigned I = 0;
           I < std::min(static_cast<size_t>(opts::ReportBadLayout),
                        SuboptimalFuncs.size());
           ++I)
        SuboptimalFuncs[I]->print(BC.outs());
    }
  }

  if (NumUnknownControlFlowFunctions) {
    BC.outs() << "BOLT-INFO: " << NumUnknownControlFlowFunctions
              << " functions have instructions with unknown control flow";
    if (!opts::PrintUnknown)
      BC.outs() << ". Use -print-unknown to see the list.";
    BC.outs() << '\n';
  }

  if (opts::ShowDensity) {
    double Density = 0.0;
    llvm::sort(FuncDensityList);

    uint64_t AccumulatedSamples = 0;
    assert(opts::ProfileDensityCutOffHot <= 1000000 &&
           "The cutoff value is greater than 1000000(100%)");
    // Subtract samples in zero-density functions (no fall-throughs) from
    // TotalSampleCount (not used anywhere below).
    for (const auto [CurDensity, CurSamples] : FuncDensityList) {
      if (CurDensity != 0.0)
        break;
      TotalSampleCount -= CurSamples;
    }
    const uint64_t CutoffSampleCount =
        1.f * TotalSampleCount * opts::ProfileDensityCutOffHot / 1000000;
    // Process functions in decreasing density order
    for (const auto [CurDensity, CurSamples] : llvm::reverse(FuncDensityList)) {
      if (AccumulatedSamples >= CutoffSampleCount)
        break;
      AccumulatedSamples += CurSamples;
      Density = CurDensity;
    }
    if (Density == 0.0) {
      BC.errs() << "BOLT-WARNING: the output profile is empty or the "
                   "--profile-density-cutoff-hot option is "
                   "set too low. Please check your command.\n";
    } else if (Density < opts::ProfileDensityThreshold) {
      BC.errs()
          << "BOLT-WARNING: BOLT is estimated to optimize better with "
          << format("%.1f", opts::ProfileDensityThreshold / Density)
          << "x more samples. Please consider increasing sampling rate or "
             "profiling for longer duration to get more samples.\n";
    }

    BC.outs() << "BOLT-INFO: Functions with density >= "
              << format("%.1f", Density) << " account for "
              << format("%.2f",
                        static_cast<double>(opts::ProfileDensityCutOffHot) /
                            10000)
              << "% total sample counts.\n";
  }
  return Error::success();
}

Error InstructionLowering::runOnFunctions(BinaryContext &BC) {
  for (auto &BFI : BC.getBinaryFunctions())
    for (BinaryBasicBlock &BB : BFI.second)
      for (MCInst &Instruction : BB)
        BC.MIB->lowerTailCall(Instruction);
  return Error::success();
}

Error StripRepRet::runOnFunctions(BinaryContext &BC) {
  if (!BC.isX86())
    return Error::success();

  uint64_t NumPrefixesRemoved = 0;
  uint64_t NumBytesSaved = 0;
  for (auto &BFI : BC.getBinaryFunctions()) {
    for (BinaryBasicBlock &BB : BFI.second) {
      auto LastInstRIter = BB.getLastNonPseudo();
      if (LastInstRIter == BB.rend() || !BC.MIB->isReturn(*LastInstRIter) ||
          !BC.MIB->deleteREPPrefix(*LastInstRIter))
        continue;

      NumPrefixesRemoved += BB.getKnownExecutionCount();
      ++NumBytesSaved;
    }
  }

  if (NumBytesSaved)
    BC.outs() << "BOLT-INFO: removed " << NumBytesSaved
              << " 'repz' prefixes"
                 " with estimated execution count of "
              << NumPrefixesRemoved << " times.\n";
  return Error::success();
}

Error InlineMemcpy::runOnFunctions(BinaryContext &BC) {
  if (!BC.isX86())
    return Error::success();

  uint64_t NumInlined = 0;
  uint64_t NumInlinedDyno = 0;
  for (auto &BFI : BC.getBinaryFunctions()) {
    for (BinaryBasicBlock &BB : BFI.second) {
      for (auto II = BB.begin(); II != BB.end(); ++II) {
        MCInst &Inst = *II;

        if (!BC.MIB->isCall(Inst) || MCPlus::getNumPrimeOperands(Inst) != 1 ||
            !Inst.getOperand(0).isExpr())
          continue;

        const MCSymbol *CalleeSymbol = BC.MIB->getTargetSymbol(Inst);
        if (CalleeSymbol->getName() != "memcpy" &&
            CalleeSymbol->getName() != "memcpy@PLT" &&
            CalleeSymbol->getName() != "_memcpy8")
          continue;

        const bool IsMemcpy8 = (CalleeSymbol->getName() == "_memcpy8");
        const bool IsTailCall = BC.MIB->isTailCall(Inst);

        const InstructionListType NewCode =
            BC.MIB->createInlineMemcpy(IsMemcpy8);
        II = BB.replaceInstruction(II, NewCode);
        std::advance(II, NewCode.size() - 1);
        if (IsTailCall) {
          MCInst Return;
          BC.MIB->createReturn(Return);
          II = BB.insertInstruction(std::next(II), std::move(Return));
        }

        ++NumInlined;
        NumInlinedDyno += BB.getKnownExecutionCount();
      }
    }
  }

  if (NumInlined) {
    BC.outs() << "BOLT-INFO: inlined " << NumInlined << " memcpy() calls";
    if (NumInlinedDyno)
      BC.outs() << ". The calls were executed " << NumInlinedDyno
                << " times based on profile.";
    BC.outs() << '\n';
  }
  return Error::success();
}

bool SpecializeMemcpy1::shouldOptimize(const BinaryFunction &Function) const {
  if (!BinaryFunctionPass::shouldOptimize(Function))
    return false;

  for (const std::string &FunctionSpec : Spec) {
    StringRef FunctionName = StringRef(FunctionSpec).split(':').first;
    if (Function.hasNameRegex(FunctionName))
      return true;
  }

  return false;
}

std::set<size_t> SpecializeMemcpy1::getCallSitesToOptimize(
    const BinaryFunction &Function) const {
  StringRef SitesString;
  for (const std::string &FunctionSpec : Spec) {
    StringRef FunctionName;
    std::tie(FunctionName, SitesString) = StringRef(FunctionSpec).split(':');
    if (Function.hasNameRegex(FunctionName))
      break;
    SitesString = "";
  }

  std::set<size_t> Sites;
  SmallVector<StringRef, 4> SitesVec;
  SitesString.split(SitesVec, ':');
  for (StringRef SiteString : SitesVec) {
    if (SiteString.empty())
      continue;
    size_t Result;
    if (!SiteString.getAsInteger(10, Result))
      Sites.emplace(Result);
  }

  return Sites;
}

Error SpecializeMemcpy1::runOnFunctions(BinaryContext &BC) {
  if (!BC.isX86())
    return Error::success();

  uint64_t NumSpecialized = 0;
  uint64_t NumSpecializedDyno = 0;
  for (auto &BFI : BC.getBinaryFunctions()) {
    BinaryFunction &Function = BFI.second;
    if (!shouldOptimize(Function))
      continue;

    std::set<size_t> CallsToOptimize = getCallSitesToOptimize(Function);
    auto shouldOptimize = [&](size_t N) {
      return CallsToOptimize.empty() || CallsToOptimize.count(N);
    };

    std::vector<BinaryBasicBlock *> Blocks(Function.pbegin(), Function.pend());
    size_t CallSiteID = 0;
    for (BinaryBasicBlock *CurBB : Blocks) {
      for (auto II = CurBB->begin(); II != CurBB->end(); ++II) {
        MCInst &Inst = *II;

        if (!BC.MIB->isCall(Inst) || MCPlus::getNumPrimeOperands(Inst) != 1 ||
            !Inst.getOperand(0).isExpr())
          continue;

        const MCSymbol *CalleeSymbol = BC.MIB->getTargetSymbol(Inst);
        if (CalleeSymbol->getName() != "memcpy" &&
            CalleeSymbol->getName() != "memcpy@PLT")
          continue;

        if (BC.MIB->isTailCall(Inst))
          continue;

        ++CallSiteID;

        if (!shouldOptimize(CallSiteID))
          continue;

        // Create a copy of a call to memcpy(dest, src, size).
        MCInst MemcpyInstr = Inst;

        BinaryBasicBlock *OneByteMemcpyBB = CurBB->splitAt(II);

        BinaryBasicBlock *NextBB = nullptr;
        if (OneByteMemcpyBB->getNumNonPseudos() > 1) {
          NextBB = OneByteMemcpyBB->splitAt(OneByteMemcpyBB->begin());
          NextBB->eraseInstruction(NextBB->begin());
        } else {
          NextBB = OneByteMemcpyBB->getSuccessor();
          OneByteMemcpyBB->eraseInstruction(OneByteMemcpyBB->begin());
          assert(NextBB && "unexpected call to memcpy() with no return");
        }

        BinaryBasicBlock *MemcpyBB = Function.addBasicBlock();
        MemcpyBB->setOffset(CurBB->getInputOffset());
        InstructionListType CmpJCC =
            BC.MIB->createCmpJE(BC.MIB->getIntArgRegister(2), 1,
                                OneByteMemcpyBB->getLabel(), BC.Ctx.get());
        CurBB->addInstructions(CmpJCC);
        CurBB->addSuccessor(MemcpyBB);

        MemcpyBB->addInstruction(std::move(MemcpyInstr));
        MemcpyBB->addSuccessor(NextBB);
        MemcpyBB->setCFIState(NextBB->getCFIState());
        MemcpyBB->setExecutionCount(0);

        // To prevent the actual call from being moved to cold, we set its
        // execution count to 1.
        if (CurBB->getKnownExecutionCount() > 0)
          MemcpyBB->setExecutionCount(1);

        InstructionListType OneByteMemcpy = BC.MIB->createOneByteMemcpy();
        OneByteMemcpyBB->addInstructions(OneByteMemcpy);

        ++NumSpecialized;
        NumSpecializedDyno += CurBB->getKnownExecutionCount();

        CurBB = NextBB;

        // Note: we don't expect the next instruction to be a call to memcpy.
        II = CurBB->begin();
      }
    }
  }

  if (NumSpecialized) {
    BC.outs() << "BOLT-INFO: specialized " << NumSpecialized
              << " memcpy() call sites for size 1";
    if (NumSpecializedDyno)
      BC.outs() << ". The calls were executed " << NumSpecializedDyno
                << " times based on profile.";
    BC.outs() << '\n';
  }
  return Error::success();
}

void RemoveNops::runOnFunction(BinaryFunction &BF) {
  const BinaryContext &BC = BF.getBinaryContext();
  for (BinaryBasicBlock &BB : BF) {
    for (int64_t I = BB.size() - 1; I >= 0; --I) {
      MCInst &Inst = BB.getInstructionAtIndex(I);
      if (BC.MIB->isNoop(Inst) && BC.MIB->hasAnnotation(Inst, "NOP"))
        BB.eraseInstructionAtIndex(I);
    }
  }
}

Error RemoveNops::runOnFunctions(BinaryContext &BC) {
  ParallelUtilities::WorkFuncTy WorkFun = [&](BinaryFunction &BF) {
    runOnFunction(BF);
  };

  ParallelUtilities::PredicateTy SkipFunc = [&](const BinaryFunction &BF) {
    return BF.shouldPreserveNops();
  };

  ParallelUtilities::runOnEachFunction(
      BC, ParallelUtilities::SchedulingPolicy::SP_INST_LINEAR, WorkFun,
      SkipFunc, "RemoveNops");
  return Error::success();
}

} // namespace bolt
} // namespace llvm
