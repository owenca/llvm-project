//===- bolt/Rewrite/DebugNames.cpp -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "bolt/Core/DebugNames.h"
#include "bolt/Core/BinaryContext.h"
#include "llvm/DebugInfo/DWARF/DWARFTypeUnit.h"
#include "llvm/DebugInfo/DWARF/LowLevel/DWARFExpression.h"
#include "llvm/Support/EndianStream.h"
#include "llvm/Support/LEB128.h"
#include <cstdint>
#include <optional>

namespace llvm {
namespace bolt {
DWARF5AcceleratorTable::DWARF5AcceleratorTable(
    const bool CreateDebugNames, BinaryContext &BC,
    DebugStrWriter &MainBinaryStrWriter)
    : BC(BC), MainBinaryStrWriter(MainBinaryStrWriter) {
  NeedToCreate = CreateDebugNames || BC.getDebugNamesSection();
  if (!NeedToCreate)
    return;
  FullTableBuffer = std::make_unique<DebugStrBufferVector>();
  FullTableStream = std::make_unique<raw_svector_ostream>(*FullTableBuffer);
  StrBuffer = std::make_unique<DebugStrBufferVector>();
  StrStream = std::make_unique<raw_svector_ostream>(*StrBuffer);
  EntriesBuffer = std::make_unique<DebugStrBufferVector>();
  Entriestream = std::make_unique<raw_svector_ostream>(*EntriesBuffer);
  AugStringBuffer = std::make_unique<DebugStrBufferVector>();
  AugStringtream = std::make_unique<raw_svector_ostream>(*AugStringBuffer);

  // Binary has split-dwarf CUs.
  // Even thought for non-skeleton-cu all names are in .debug_str.dwo section,
  // for the .debug_names contributions they are in .debug_str section.
  if (BC.getNumDWOCUs()) {
    DataExtractor StrData(BC.DwCtx->getDWARFObj().getStrSection(),
                          BC.DwCtx->isLittleEndian(), 0);
    uint64_t Offset = 0;
    uint64_t StrOffset = 0;
    while (StrData.isValidOffset(Offset)) {
      Error Err = Error::success();
      const char *CStr = StrData.getCStr(&Offset, &Err);
      if (Err) {
        NeedToCreate = false;
        BC.errs() << "BOLT-WARNING: [internal-dwarf-error]: Could not extract "
                     "string from .debug_str section at offset: "
                  << Twine::utohexstr(StrOffset) << ".\n";
        return;
      }
      auto R = StrCacheToOffsetMap.try_emplace(
          llvm::hash_value(llvm::StringRef(CStr)), StrOffset);
      if (!R.second)
        BC.errs()
            << "BOLT-WARNING: [internal-dwarf-error]: collision occured on "
            << CStr << " at offset : 0x" << Twine::utohexstr(StrOffset)
            << ". Previous string offset is: 0x"
            << Twine::utohexstr(R.first->second) << ".\n";
      StrOffset = Offset;
    }
  }
}

void DWARF5AcceleratorTable::setCurrentUnit(DWARFUnit &Unit,
                                            const uint64_t UnitStartOffset) {
  CurrentUnit = nullptr;
  CurrentUnitOffset = UnitStartOffset;
  std::optional<uint64_t> DWOID = Unit.getDWOId();
  // We process skeleton CUs after DWO Units for it.
  // Patching offset in CU list to correct one.
  if (!Unit.isDWOUnit() && DWOID) {
    auto Iter = CUOffsetsToPatch.find(*DWOID);
    // Check in case no entries were added from non skeleton DWO section.
    if (Iter != CUOffsetsToPatch.end())
      CUList[Iter->second] = UnitStartOffset;
  }
}

void DWARF5AcceleratorTable::addUnit(DWARFUnit &Unit,
                                     const std::optional<uint64_t> &DWOID) {
  constexpr uint32_t BADCUOFFSET = 0xBADBAD;
  StrSection = Unit.getStringSection();
  if (Unit.isTypeUnit()) {
    if (DWOID) {
      // We adding an entry for a DWO TU. The DWO CU might not have any entries,
      // so need to add it to the list pre-emptively.
      auto Iter = CUOffsetsToPatch.insert({*DWOID, CUList.size()});
      if (Iter.second)
        CUList.push_back(BADCUOFFSET);
      const uint64_t TUHash = cast<DWARFTypeUnit>(&Unit)->getTypeHash();
      if (!TUHashToIndexMap.count(TUHash)) {
        TUHashToIndexMap.insert({TUHash, ForeignTUList.size()});
        ForeignTUList.push_back(TUHash);
      }
    } else {
      LocalTUList.push_back(CurrentUnitOffset);
    }
  } else {
    if (DWOID) {
      // This is a path for split dwarf without type units.
      // We process DWO Units before Skeleton CU. So at this point we don't know
      // the offset of Skeleton CU. Adding CULit index to a map to patch later
      // with the correct offset.
      auto Iter = CUOffsetsToPatch.insert({*DWOID, CUList.size()});
      if (Iter.second)
        CUList.push_back(BADCUOFFSET);
    } else {
      CUList.push_back(CurrentUnitOffset);
    }
  }
}

// Returns true if DW_TAG_variable should be included in .debug-names based on
// section 6.1.1.1 for DWARF5 spec.
static bool shouldIncludeVariable(const DWARFUnit &Unit, const DIE &Die) {
  const DIEValue LocAttrInfo =
      Die.findAttribute(dwarf::Attribute::DW_AT_location);
  if (!LocAttrInfo)
    return false;
  if (!(doesFormBelongToClass(LocAttrInfo.getForm(), DWARFFormValue::FC_Exprloc,
                              Unit.getVersion()) ||
        doesFormBelongToClass(LocAttrInfo.getForm(), DWARFFormValue::FC_Block,
                              Unit.getVersion())))
    return false;
  std::vector<uint8_t> Sblock;
  auto constructVect =
      [&](const DIEValueList::const_value_range &Iter) -> void {
    for (const DIEValue &Val : Iter)
      Sblock.push_back(Val.getDIEInteger().getValue());
  };
  if (doesFormBelongToClass(LocAttrInfo.getForm(), DWARFFormValue::FC_Exprloc,
                            Unit.getVersion()))
    constructVect(LocAttrInfo.getDIELoc().values());
  else
    constructVect(LocAttrInfo.getDIEBlock().values());
  ArrayRef<uint8_t> Expr = ArrayRef<uint8_t>(Sblock);
  DataExtractor Data(StringRef((const char *)Expr.data(), Expr.size()),
                     Unit.getContext().isLittleEndian(), 0);
  DWARFExpression LocExpr(Data, Unit.getAddressByteSize(),
                          Unit.getFormParams().Format);
  for (const DWARFExpression::Operation &Expr : LocExpr)
    if (Expr.getCode() == dwarf::DW_OP_addrx ||
        Expr.getCode() == dwarf::DW_OP_form_tls_address ||
        Expr.getCode() == dwarf::DW_OP_GNU_push_tls_address)
      return true;
  return false;
}

bool static canProcess(const DWARFUnit &Unit, const DIE &Die,
                       std::string &NameToUse, const bool TagsOnly) {
  if (Die.findAttribute(dwarf::Attribute::DW_AT_declaration))
    return false;
  switch (Die.getTag()) {
  case dwarf::DW_TAG_base_type:
  case dwarf::DW_TAG_class_type:
  case dwarf::DW_TAG_enumeration_type:
  case dwarf::DW_TAG_imported_declaration:
  case dwarf::DW_TAG_pointer_type:
  case dwarf::DW_TAG_structure_type:
  case dwarf::DW_TAG_typedef:
  case dwarf::DW_TAG_unspecified_type:
  case dwarf::DW_TAG_union_type:
    if (TagsOnly || Die.findAttribute(dwarf::Attribute::DW_AT_name))
      return true;
    return false;
  case dwarf::DW_TAG_namespace:
    // According to DWARF5 spec namespaces without DW_AT_name needs to have
    // "(anonymous namespace)"
    if (!Die.findAttribute(dwarf::Attribute::DW_AT_name))
      NameToUse = "(anonymous namespace)";
    return true;
  case dwarf::DW_TAG_inlined_subroutine:
  case dwarf::DW_TAG_label:
  case dwarf::DW_TAG_subprogram:
    if (TagsOnly || Die.findAttribute(dwarf::Attribute::DW_AT_low_pc) ||
        Die.findAttribute(dwarf::Attribute::DW_AT_high_pc) ||
        Die.findAttribute(dwarf::Attribute::DW_AT_ranges) ||
        Die.findAttribute(dwarf::Attribute::DW_AT_entry_pc))
      return true;
    return false;
  case dwarf::DW_TAG_variable:
    return TagsOnly || shouldIncludeVariable(Unit, Die);
  default:
    break;
  }
  return false;
}

bool DWARF5AcceleratorTable::canGenerateEntryWithCrossCUReference(
    const DWARFUnit &Unit, const DIE &Die,
    const DWARFAbbreviationDeclaration::AttributeSpec &AttrSpec) {
  if (!isCreated())
    return false;
  std::string NameToUse = "";
  if (!canProcess(Unit, Die, NameToUse, true))
    return false;
  return (AttrSpec.Attr == dwarf::Attribute::DW_AT_abstract_origin ||
          AttrSpec.Attr == dwarf::Attribute::DW_AT_specification) &&
         AttrSpec.Form == dwarf::DW_FORM_ref_addr;
}
/// Returns name offset in String Offset section.
static uint64_t getNameOffset(BinaryContext &BC, DWARFUnit &Unit,
                              const uint64_t Index) {
  const DWARFSection &StrOffsetsSection = Unit.getStringOffsetSection();
  const std::optional<StrOffsetsContributionDescriptor> &Contr =
      Unit.getStringOffsetsTableContribution();
  if (!Contr) {
    BC.errs() << "BOLT-WARNING: [internal-dwarf-warning]: Could not get "
                 "StringOffsetsTableContribution for unit at offset: "
              << Twine::utohexstr(Unit.getOffset()) << ".\n";
    return 0;
  }

  const uint8_t DwarfOffsetByteSize = Contr->getDwarfOffsetByteSize();
  return support::endian::read32le(StrOffsetsSection.Data.data() + Contr->Base +
                                   Index * DwarfOffsetByteSize);
}

static uint64_t getEntryID(const BOLTDWARF5AccelTableData &Entry) {
  return reinterpret_cast<uint64_t>(&Entry);
}

uint32_t DWARF5AcceleratorTable::getUnitID(const DWARFUnit &Unit,
                                           const std::optional<uint64_t> &DWOID,
                                           bool &IsTU) {
  IsTU = Unit.isTypeUnit();
  if (IsTU) {
    if (DWOID) {
      const uint64_t TUHash = cast<DWARFTypeUnit>(&Unit)->getTypeHash();
      auto Iter = TUHashToIndexMap.find(TUHash);
      assert(Iter != TUHashToIndexMap.end() && "Could not find TU hash in map");
      return Iter->second;
    }
    return LocalTUList.size() - 1;
  }
  return CUList.size() - 1;
}

std::optional<std::string> DWARF5AcceleratorTable::getName(
    DWARFUnit &Unit, const std::optional<uint64_t> &DWOID,
    const std::string &NameToUse, DIEValue ValName) {
  if ((!ValName || ValName.getForm() == dwarf::DW_FORM_string) &&
      NameToUse.empty())
    return std::nullopt;
  std::string Name = "";
  uint64_t NameIndexOffset = 0;
  if (NameToUse.empty()) {
    NameIndexOffset = ValName.getDIEInteger().getValue();
    if (ValName.getForm() != dwarf::DW_FORM_strp)
      NameIndexOffset = getNameOffset(BC, Unit, NameIndexOffset);
    // Counts on strings end with '\0'.
    Name = std::string(&StrSection.data()[NameIndexOffset]);
  } else {
    Name = NameToUse;
  }
  auto &It = Entries[Name];
  if (It.Values.empty()) {
    if (DWOID && NameToUse.empty()) {
      // For DWO Unit the offset is in the .debug_str.dwo section.
      // Need to find offset for the name in the .debug_str section.
      llvm::hash_code Hash = llvm::hash_value(llvm::StringRef(Name));
      auto ItCache = StrCacheToOffsetMap.find(Hash);
      if (ItCache == StrCacheToOffsetMap.end())
        NameIndexOffset = MainBinaryStrWriter.addString(Name);
      else
        NameIndexOffset = ItCache->second;
    }
    if (!NameToUse.empty())
      NameIndexOffset = MainBinaryStrWriter.addString(Name);
    It.StrOffset = NameIndexOffset;
    // This is the same hash function used in DWARF5AccelTableData.
    It.HashValue = caseFoldingDjbHash(Name);
  }
  return Name;
}

std::optional<BOLTDWARF5AccelTableData *> DWARF5AcceleratorTable::addEntry(
    DWARFUnit &DU, const DIE &CurrDie, const std::optional<uint64_t> &DWOID,
    const std::optional<BOLTDWARF5AccelTableData *> &Parent,
    const std::optional<std::string> &Name,
    const uint32_t NumberParentsInChain) {
  if (!Name)
    return std::nullopt;

  auto &It = Entries[*Name];
  bool IsTU = false;
  uint32_t DieTag = CurrDie.getTag();
  uint32_t UnitID = getUnitID(DU, DWOID, IsTU);
  std::optional<unsigned> SecondIndex = std::nullopt;
  if (IsTU && DWOID) {
    auto Iter = CUOffsetsToPatch.find(*DWOID);
    if (Iter == CUOffsetsToPatch.end())
      BC.errs() << "BOLT-WARNING: [internal-dwarf-warning]: Could not find "
                   "DWO ID in CU offsets for second Unit Index "
                << *Name << ". For DIE at offset: "
                << Twine::utohexstr(CurrentUnitOffset + CurrDie.getOffset())
                << ".\n";
    SecondIndex = Iter->second;
  }
  std::optional<uint64_t> ParentOffset =
      (Parent ? std::optional<uint64_t>(getEntryID(**Parent)) : std::nullopt);
  // This will be only populated in writeEntry, in order to keep only the parent
  // entries, and keep the footprint down.
  if (ParentOffset)
    EntryRelativeOffsets.insert({*ParentOffset, 0});
  bool IsParentRoot = false;
  // If there is no parent and no valid Entries in parent chain this is a root
  // to be marked with a flag.
  if (!Parent && !NumberParentsInChain)
    IsParentRoot = true;
  It.Values.push_back(new (Allocator) BOLTDWARF5AccelTableData(
      CurrDie.getOffset(), ParentOffset, DieTag, UnitID, IsParentRoot, IsTU,
      SecondIndex));
  return It.Values.back();
}

std::optional<BOLTDWARF5AccelTableData *>
DWARF5AcceleratorTable::processReferencedDie(
    DWARFUnit &Unit, const DIE &Die, const std::optional<uint64_t> &DWOID,
    const std::optional<BOLTDWARF5AccelTableData *> &Parent,
    const std::string &NameToUse, const uint32_t NumberParentsInChain,
    const dwarf::Attribute &Attr) {
  DIEValue Value = Die.findAttribute(Attr);
  if (!Value)
    return std::nullopt;
  auto getReferenceDie = [&](const DIEValue &Value, const DIE *RefDieUsed)
      -> std::optional<std::pair<DWARFUnit *, const DIE *>> {
    if (!Value)
      return std::nullopt;
    if (Value.getForm() == dwarf::DW_FORM_ref_addr) {
      auto Iter = CrossCUDies.find(Value.getDIEInteger().getValue());
      if (Iter == CrossCUDies.end()) {
        BC.errs() << "BOLT-WARNING: [internal-dwarf-warning]: Could not find "
                     "referenced DIE in CrossCUDies for "
                  << Twine::utohexstr(Value.getDIEInteger().getValue())
                  << ".\n";
        return std::nullopt;
      }
      return Iter->second;
    }
    const DIEEntry &DIEENtry = Value.getDIEEntry();
    return {{&Unit, &DIEENtry.getEntry()}};
  };

  DIEValue AttrValLinkageName;
  DIEValue AttrValName = Die.findAttribute(dwarf::Attribute::DW_AT_name);
  DWARFUnit *RefUnit = &Unit;
  const DIE *RefDieUsed = &Die;
  // It is possible to have DW_TAG_subprogram only with  DW_AT_linkage_name that
  // DW_AT_abstract_origin/DW_AT_specification point to.
  while (!AttrValName) {
    std::optional<std::pair<DWARFUnit *, const DIE *>> RefDUDie =
        getReferenceDie(Value, RefDieUsed);
    if (!RefDUDie)
      break;
    RefUnit = RefDUDie->first;
    const DIE &RefDie = *RefDUDie->second;
    RefDieUsed = &RefDie;
    if (!AttrValLinkageName)
      AttrValLinkageName =
          RefDie.findAttribute(dwarf::Attribute::DW_AT_linkage_name);
    AttrValName = RefDie.findAttribute(dwarf::Attribute::DW_AT_name);
    Value = RefDie.findAttribute(dwarf::Attribute::DW_AT_abstract_origin);
    if (!Value)
      Value = RefDie.findAttribute(dwarf::Attribute::DW_AT_specification);
  }
  addEntry(Unit, Die, DWOID, Parent,
           getName(*RefUnit, DWOID, NameToUse, AttrValLinkageName),
           NumberParentsInChain);
  return addEntry(Unit, Die, DWOID, Parent,
                  getName(*RefUnit, DWOID, NameToUse, AttrValName),
                  NumberParentsInChain);
}

std::optional<BOLTDWARF5AccelTableData *>
DWARF5AcceleratorTable::addAccelTableEntry(
    DWARFUnit &Unit, const DIE &Die, const std::optional<uint64_t> &DWOID,
    const uint32_t NumberParentsInChain,
    std::optional<BOLTDWARF5AccelTableData *> &Parent) {
  if (Unit.getVersion() < 5 || !NeedToCreate)
    return std::nullopt;
  std::string NameToUse = "";

  if (!canProcess(Unit, Die, NameToUse, false))
    return std::nullopt;

  // Adds a Unit to either CU, LocalTU or ForeignTU list the first time we
  // encounter it.
  // Invoking it here so that we don't add Units that don't have any entries.
  if (&Unit != CurrentUnit) {
    CurrentUnit = &Unit;
    addUnit(Unit, DWOID);
  }

  // Minor optimization not to add entry twice for DW_TAG_namespace if it has no
  // DW_AT_name.
  std::optional<BOLTDWARF5AccelTableData *> LinkageEntry = std::nullopt;
  DIEValue NameVal = Die.findAttribute(dwarf::Attribute::DW_AT_name);
  DIEValue LinkageNameVal =
      Die.findAttribute(dwarf::Attribute::DW_AT_linkage_name);
  if (!(Die.getTag() == dwarf::DW_TAG_namespace && !NameVal))
    LinkageEntry = addEntry(Unit, Die, DWOID, Parent,
                            getName(Unit, DWOID, NameToUse, LinkageNameVal),
                            NumberParentsInChain);

  std::optional<BOLTDWARF5AccelTableData *> NameEntry =
      addEntry(Unit, Die, DWOID, Parent,
               getName(Unit, DWOID, NameToUse, NameVal), NumberParentsInChain);
  if (NameEntry)
    return NameEntry;

  // The DIE doesn't have DW_AT_name or DW_AT_linkage_name, so we need to see if
  // we can follow other attributes to find them. For the purposes of
  // determining whether a debug information entry has a particular
  // attribute (such as DW_AT_name), if debug information entry A has a
  // DW_AT_specification or DW_AT_abstract_origin attribute pointing to another
  // debug information entry B, any attributes of B are considered to be
  // part of A.
  if (std::optional<BOLTDWARF5AccelTableData *> Entry = processReferencedDie(
          Unit, Die, DWOID, Parent, NameToUse, NumberParentsInChain,
          dwarf::Attribute::DW_AT_abstract_origin))
    return *Entry;
  if (std::optional<BOLTDWARF5AccelTableData *> Entry = processReferencedDie(
          Unit, Die, DWOID, Parent, NameToUse, NumberParentsInChain,
          dwarf::Attribute::DW_AT_specification))
    return *Entry;

  // This point can be hit by DW_TAG_varialbe that has no DW_AT_name.
  return std::nullopt;
}

/// Algorithm from llvm implementation.
void DWARF5AcceleratorTable::computeBucketCount() {
  // First get the number of unique hashes.
  std::vector<uint32_t> Uniques;
  Uniques.reserve(Entries.size());
  for (const auto &E : Entries)
    Uniques.push_back(E.second.HashValue);
  array_pod_sort(Uniques.begin(), Uniques.end());
  std::vector<uint32_t>::iterator P = llvm::unique(Uniques);

  UniqueHashCount = std::distance(Uniques.begin(), P);

  if (UniqueHashCount > 1024)
    BucketCount = UniqueHashCount / 4;
  else if (UniqueHashCount > 16)
    BucketCount = UniqueHashCount / 2;
  else
    BucketCount = std::max<uint32_t>(UniqueHashCount, 1);
}

/// Bucket code as in: AccelTableBase::finalize()
void DWARF5AcceleratorTable::finalize() {
  if (!NeedToCreate)
    return;
  // Figure out how many buckets we need, then compute the bucket contents and
  // the final ordering. The hashes and offsets can be emitted by walking these
  // data structures.
  computeBucketCount();

  // Compute bucket contents and final ordering.
  Buckets.resize(BucketCount);
  for (auto &E : Entries) {
    uint32_t Bucket = E.second.HashValue % BucketCount;
    Buckets[Bucket].push_back(&E.second);
  }

  // Sort the contents of the buckets by hash value so that hash collisions end
  // up together. Stable sort makes testing easier and doesn't cost much more.
  for (HashList &Bucket : Buckets) {
    llvm::stable_sort(Bucket, [](const HashData *LHS, const HashData *RHS) {
      return LHS->HashValue < RHS->HashValue;
    });
    for (HashData *H : Bucket)
      llvm::stable_sort(H->Values, [](const BOLTDWARF5AccelTableData *LHS,
                                      const BOLTDWARF5AccelTableData *RHS) {
        return LHS->getDieOffset() < RHS->getDieOffset();
      });
  }

  CUIndexForm = DIEInteger::BestForm(/*IsSigned*/ false, CUList.size() - 1);
  TUIndexForm = DIEInteger::BestForm(
      /*IsSigned*/ false, LocalTUList.size() + ForeignTUList.size() - 1);
  const dwarf::FormParams FormParams{5, 4, dwarf::DwarfFormat::DWARF32, false};
  CUIndexEncodingSize = *dwarf::getFixedFormByteSize(CUIndexForm, FormParams);
  TUIndexEncodingSize = *dwarf::getFixedFormByteSize(TUIndexForm, FormParams);
}

std::optional<DWARF5AccelTable::UnitIndexAndEncoding>
DWARF5AcceleratorTable::getIndexForEntry(
    const BOLTDWARF5AccelTableData &Value) const {
  // The foreign TU list immediately follows the local TU list and they both
  // use the same index, so that if there are N local TU entries, the index for
  // the first foreign TU is N.
  if (Value.isTU())
    return {{(Value.getSecondUnitID() ? (unsigned)LocalTUList.size() : 0) +
                 Value.getUnitID(),
             {dwarf::DW_IDX_type_unit, TUIndexForm}}};
  if (CUList.size() > 1)
    return {{Value.getUnitID(), {dwarf::DW_IDX_compile_unit, CUIndexForm}}};
  return std::nullopt;
}

std::optional<DWARF5AccelTable::UnitIndexAndEncoding>
DWARF5AcceleratorTable::getSecondIndexForEntry(
    const BOLTDWARF5AccelTableData &Value) const {
  if (Value.isTU() && CUList.size() > 1 && Value.getSecondUnitID())
    return {
        {*Value.getSecondUnitID(), {dwarf::DW_IDX_compile_unit, CUIndexForm}}};
  return std::nullopt;
}

void DWARF5AcceleratorTable::populateAbbrevsMap() {
  for (auto &Bucket : getBuckets()) {
    for (DWARF5AcceleratorTable::HashData *Hash : Bucket) {
      for (BOLTDWARF5AccelTableData *Value : Hash->Values) {
        const std::optional<DWARF5AccelTable::UnitIndexAndEncoding> EntryRet =
            getIndexForEntry(*Value);
        // For entries that need to refer to the foreign type units and to
        // the CU.
        const std::optional<DWARF5AccelTable::UnitIndexAndEncoding>
            SecondEntryRet = getSecondIndexForEntry(*Value);
        DebugNamesAbbrev Abbrev(Value->getDieTag());
        if (EntryRet)
          Abbrev.addAttribute(EntryRet->Encoding);
        if (SecondEntryRet)
          Abbrev.addAttribute(SecondEntryRet->Encoding);
        Abbrev.addAttribute({dwarf::DW_IDX_die_offset, dwarf::DW_FORM_ref4});
        if (std::optional<uint64_t> Offset = Value->getParentDieOffset())
          Abbrev.addAttribute({dwarf::DW_IDX_parent, dwarf::DW_FORM_ref4});
        else if (Value->isParentRoot())
          Abbrev.addAttribute(
              {dwarf::DW_IDX_parent, dwarf::DW_FORM_flag_present});
        FoldingSetNodeID ID;
        Abbrev.Profile(ID);
        void *InsertPos;
        if (DebugNamesAbbrev *Existing =
                AbbreviationsSet.FindNodeOrInsertPos(ID, InsertPos)) {
          Value->setAbbrevNumber(Existing->getNumber());
          continue;
        }
        DebugNamesAbbrev *NewAbbrev =
            new (Alloc) DebugNamesAbbrev(std::move(Abbrev));
        AbbreviationsVector.push_back(NewAbbrev);
        NewAbbrev->setNumber(AbbreviationsVector.size());
        AbbreviationsSet.InsertNode(NewAbbrev, InsertPos);
        Value->setAbbrevNumber(NewAbbrev->getNumber());
      }
    }
  }
}

void DWARF5AcceleratorTable::writeEntry(BOLTDWARF5AccelTableData &Entry) {
  const uint64_t EntryID = getEntryID(Entry);
  if (EntryRelativeOffsets.find(EntryID) != EntryRelativeOffsets.end())
    EntryRelativeOffsets[EntryID] = EntriesBuffer->size();

  const std::optional<DWARF5AccelTable::UnitIndexAndEncoding> EntryRet =
      getIndexForEntry(Entry);
  // For forgeign type (FTU) units that need to refer to the FTU and to the CU.
  const std::optional<DWARF5AccelTable::UnitIndexAndEncoding> SecondEntryRet =
      getSecondIndexForEntry(Entry);
  const unsigned AbbrevIndex = Entry.getAbbrevNumber() - 1;
  assert(AbbrevIndex < AbbreviationsVector.size() &&
         "Entry abbrev index is outside of abbreviations vector range.");
  const DebugNamesAbbrev *Abbrev = AbbreviationsVector[AbbrevIndex];
  encodeULEB128(Entry.getAbbrevNumber(), *Entriestream);
  auto writeIndex = [&](uint32_t Index, uint32_t IndexSize) -> void {
    switch (IndexSize) {
    default:
      llvm_unreachable("Unsupported Index Size!");
      break;
    case 1:
      support::endian::write(*Entriestream, static_cast<uint8_t>(Index),
                             llvm::endianness::little);
      break;
    case 2:
      support::endian::write(*Entriestream, static_cast<uint16_t>(Index),
                             llvm::endianness::little);
      break;
    case 4:
      support::endian::write(*Entriestream, static_cast<uint32_t>(Index),
                             llvm::endianness::little);
      break;
    };
  };

  for (const DebugNamesAbbrev::AttributeEncoding &AttrEnc :
       Abbrev->getAttributes()) {
    switch (AttrEnc.Index) {
    default: {
      llvm_unreachable("Unexpected index attribute!");
      break;
    }
    case dwarf::DW_IDX_compile_unit: {
      const unsigned CUIndex =
          SecondEntryRet ? SecondEntryRet->Index : EntryRet->Index;
      writeIndex(CUIndex, CUIndexEncodingSize);
      break;
    }
    case dwarf::DW_IDX_type_unit: {
      writeIndex(EntryRet->Index, TUIndexEncodingSize);
      break;
    }
    case dwarf::DW_IDX_die_offset: {
      assert(AttrEnc.Form == dwarf::DW_FORM_ref4);
      support::endian::write(*Entriestream,
                             static_cast<uint32_t>(Entry.getDieOffset()),
                             llvm::endianness::little);
      break;
    }
    case dwarf::DW_IDX_parent: {
      assert(
          (AttrEnc.Form == dwarf::DW_FORM_ref4 && Entry.getParentDieOffset()) ||
          AttrEnc.Form == dwarf::DW_FORM_flag_present);
      if (std::optional<uint64_t> ParentOffset = Entry.getParentDieOffset()) {
        Entry.setPatchOffset(EntriesBuffer->size());
        support::endian::write(*Entriestream, static_cast<uint32_t>(UINT32_MAX),
                               llvm::endianness::little);
      }
      break;
    }
    }
  }
}

void DWARF5AcceleratorTable::writeEntries() {
  for (auto &Bucket : getBuckets()) {
    for (DWARF5AcceleratorTable::HashData *Hash : Bucket) {
      Hash->EntryOffset = EntriesBuffer->size();
      for (BOLTDWARF5AccelTableData *Value : Hash->Values) {
        writeEntry(*Value);
      }
      support::endian::write(*Entriestream, static_cast<uint8_t>(0),
                             llvm::endianness::little);
    }
  }
  // Patching parent offsets.
  for (auto &Bucket : getBuckets()) {
    for (DWARF5AcceleratorTable::HashData *Hash : Bucket) {
      for (BOLTDWARF5AccelTableData *Entry : Hash->Values) {
        std::optional<uint64_t> ParentOffset = Entry->getParentDieOffset();
        if (!ParentOffset)
          continue;
        if (const auto Iter = EntryRelativeOffsets.find(*ParentOffset);
            Iter != EntryRelativeOffsets.end()) {
          const uint64_t PatchOffset = Entry->getPatchOffset();
          uint32_t *Ptr =
              reinterpret_cast<uint32_t *>(&EntriesBuffer->data()[PatchOffset]);
          *Ptr = Iter->second;
        } else {
          BC.errs() << "BOLT-WARNING: [internal-dwarf-warning]: Could not find "
                       "entry with offset "
                    << *ParentOffset << "\n";
        }
      }
    }
  }
}

void DWARF5AcceleratorTable::writeAugmentationString() {
  // String needs to be multiple of 4 bytes.
  *AugStringtream << "BOLT";
  AugmentationStringSize = AugStringBuffer->size();
}

/// Calculates size of .debug_names header without Length field.
static constexpr uint32_t getDebugNamesHeaderSize() {
  constexpr uint16_t VersionLength = sizeof(uint16_t);
  constexpr uint16_t PaddingLength = sizeof(uint16_t);
  constexpr uint32_t CompUnitCountLength = sizeof(uint32_t);
  constexpr uint32_t LocalTypeUnitCountLength = sizeof(uint32_t);
  constexpr uint32_t ForeignTypeUnitCountLength = sizeof(uint32_t);
  constexpr uint32_t BucketCountLength = sizeof(uint32_t);
  constexpr uint32_t NameCountLength = sizeof(uint32_t);
  constexpr uint32_t AbbrevTableSizeLength = sizeof(uint32_t);
  constexpr uint32_t AugmentationStringSizeLenght = sizeof(uint32_t);
  return VersionLength + PaddingLength + CompUnitCountLength +
         LocalTypeUnitCountLength + ForeignTypeUnitCountLength +
         BucketCountLength + NameCountLength + AbbrevTableSizeLength +
         AugmentationStringSizeLenght;
}

void DWARF5AcceleratorTable::emitHeader() const {
  constexpr uint32_t HeaderSize = getDebugNamesHeaderSize();
  // Header Length
  support::endian::write(*FullTableStream,
                         static_cast<uint32_t>(HeaderSize + StrBuffer->size() +
                                               AugmentationStringSize),
                         llvm::endianness::little);
  // Version
  support::endian::write(*FullTableStream, static_cast<uint16_t>(5),
                         llvm::endianness::little);
  // Padding
  support::endian::write(*FullTableStream, static_cast<uint16_t>(0),
                         llvm::endianness::little);
  // Compilation Unit Count
  support::endian::write(*FullTableStream, static_cast<uint32_t>(CUList.size()),
                         llvm::endianness::little);
  // Local Type Unit Count
  support::endian::write(*FullTableStream,
                         static_cast<uint32_t>(LocalTUList.size()),
                         llvm::endianness::little);
  // Foreign Type Unit Count
  support::endian::write(*FullTableStream,
                         static_cast<uint32_t>(ForeignTUList.size()),
                         llvm::endianness::little);
  // Bucket Count
  support::endian::write(*FullTableStream, static_cast<uint32_t>(BucketCount),
                         llvm::endianness::little);
  // Name Count
  support::endian::write(*FullTableStream,
                         static_cast<uint32_t>(Entries.size()),
                         llvm::endianness::little);
  // Abbrev Table Size
  support::endian::write(*FullTableStream,
                         static_cast<uint32_t>(AbbrevTableSize),
                         llvm::endianness::little);
  // Augmentation String Size
  support::endian::write(*FullTableStream,
                         static_cast<uint32_t>(AugmentationStringSize),
                         llvm::endianness::little);

  emitAugmentationString();
  FullTableStream->write(StrBuffer->data(), StrBuffer->size());
}

void DWARF5AcceleratorTable::emitCUList() const {
  for (const uint32_t CUID : CUList)
    support::endian::write(*StrStream, CUID, llvm::endianness::little);
}
void DWARF5AcceleratorTable::emitTUList() const {
  for (const uint32_t TUID : LocalTUList)
    support::endian::write(*StrStream, TUID, llvm::endianness::little);

  for (const uint64_t TUID : ForeignTUList)
    support::endian::write(*StrStream, TUID, llvm::endianness::little);
}
void DWARF5AcceleratorTable::emitBuckets() const {
  uint32_t Index = 1;
  for (const auto &Bucket : enumerate(getBuckets())) {
    const uint32_t TempIndex = Bucket.value().empty() ? 0 : Index;
    support::endian::write(*StrStream, TempIndex, llvm::endianness::little);
    Index += Bucket.value().size();
  }
}
void DWARF5AcceleratorTable::emitHashes() const {
  for (const auto &Bucket : getBuckets()) {
    for (const DWARF5AcceleratorTable::HashData *Hash : Bucket)
      support::endian::write(*StrStream, Hash->HashValue,
                             llvm::endianness::little);
  }
}
void DWARF5AcceleratorTable::emitStringOffsets() const {
  for (const auto &Bucket : getBuckets()) {
    for (const DWARF5AcceleratorTable::HashData *Hash : Bucket)
      support::endian::write(*StrStream, static_cast<uint32_t>(Hash->StrOffset),
                             llvm::endianness::little);
  }
}
void DWARF5AcceleratorTable::emitOffsets() const {
  for (const auto &Bucket : getBuckets()) {
    for (const DWARF5AcceleratorTable::HashData *Hash : Bucket)
      support::endian::write(*StrStream,
                             static_cast<uint32_t>(Hash->EntryOffset),
                             llvm::endianness::little);
  }
}
void DWARF5AcceleratorTable::emitAbbrevs() {
  const uint32_t AbbrevTableStart = StrBuffer->size();
  for (const auto *Abbrev : AbbreviationsVector) {
    encodeULEB128(Abbrev->getNumber(), *StrStream);
    encodeULEB128(Abbrev->getDieTag(), *StrStream);
    for (const auto &AttrEnc : Abbrev->getAttributes()) {
      encodeULEB128(AttrEnc.Index, *StrStream);
      encodeULEB128(AttrEnc.Form, *StrStream);
    }
    encodeULEB128(0, *StrStream);
    encodeULEB128(0, *StrStream);
  }
  encodeULEB128(0, *StrStream);
  AbbrevTableSize = StrBuffer->size() - AbbrevTableStart;
}
void DWARF5AcceleratorTable::emitData() {
  StrStream->write(EntriesBuffer->data(), EntriesBuffer->size());
}
void DWARF5AcceleratorTable::emitAugmentationString() const {
  FullTableStream->write(AugStringBuffer->data(), AugStringBuffer->size());
}
void DWARF5AcceleratorTable::emitAccelTable() {
  if (!NeedToCreate)
    return;
  finalize();
  populateAbbrevsMap();
  writeEntries();
  writeAugmentationString();
  emitCUList();
  emitTUList();
  emitBuckets();
  emitHashes();
  emitStringOffsets();
  emitOffsets();
  emitAbbrevs();
  emitData();
  emitHeader();
}
} // namespace bolt
} // namespace llvm
