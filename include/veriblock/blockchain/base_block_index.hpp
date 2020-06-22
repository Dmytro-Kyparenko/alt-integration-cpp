// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef ALT_INTEGRATION_INCLUDE_VERIBLOCK_BLOCKCHAIN_BASE_BLOCK_INDEX_HPP_
#define ALT_INTEGRATION_INCLUDE_VERIBLOCK_BLOCKCHAIN_BASE_BLOCK_INDEX_HPP_

#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include "veriblock/arith_uint256.hpp"
#include "veriblock/blockchain/command.hpp"
#include "veriblock/blockchain/command_group.hpp"
#include "veriblock/entities/endorsements.hpp"
#include "veriblock/logger.hpp"
#include "veriblock/validation_state.hpp"
#include "veriblock/write_stream.hpp"

namespace altintegration {

enum BlockStatus : uint8_t {
  //! default state for validity - validity state is unknown
  BLOCK_VALID_UNKNOWN = 0,
  //! acceptBlock succeded. All parents are at leasI'm probably making a novice
  //! mistake t at this state.
  BLOCK_VALID_TREE = 1,
  //! addPayloads succeded. All parents are at least BLOCK_VALID_TREE
  BLOCK_VALID_PAYLOADS = 2,
  //! all validity flags
  BLOCK_VALID_MASK = BLOCK_VALID_TREE | BLOCK_VALID_PAYLOADS,
  //! block is statelessly valid, but we marked it as failed
  BLOCK_FAILED_BLOCK = 4,
  //! block failed POP validation
  BLOCK_FAILED_POP = 8,
  //! block is state{lessly,fully} valid, but some of previous blocks is invalid
  BLOCK_FAILED_CHILD = 16,
  //! all invalidity flags
  BLOCK_FAILED_MASK = BLOCK_FAILED_CHILD | BLOCK_FAILED_POP | BLOCK_FAILED_BLOCK
};

//! Store block
template <typename Block>
struct BaseBlockIndex {
  virtual ~BaseBlockIndex() = default;

  using block_t = Block;
  using hash_t = typename block_t::hash_t;
  using prev_hash_t = typename block_t::prev_hash_t;
  using height_t = typename block_t::height_t;

  //! (memory only) pointer to a previous block
  BaseBlockIndex* pprev = nullptr;

  //! (memory only) a set of pointers for forward iteration
  std::set<BaseBlockIndex*> pnext{};

  //! total amount of work in the chain up to and including this
  //! block
  ArithUint256 chainWork = 0;

  //! height of the entry in the chain
  height_t height = 0;

  //! block header
  std::shared_ptr<Block> header{};

  //! contains status flags
  uint8_t status = 0;  // unknown validity

  //! reference counter for fork resolution
  uint32_t refCounter = 0;

  bool isValid(enum BlockStatus upTo = BLOCK_VALID_TREE) const {
    VBK_ASSERT(!(upTo & ~BLOCK_VALID_MASK));  // Only validity flags allowed.
    if ((status & BLOCK_FAILED_MASK) != 0u) {
      // block failed
      return false;
    }
    return ((status & BLOCK_VALID_MASK) >= upTo);
  }

  virtual void setNull() {
    this->pprev = nullptr;
    this->pnext.clear();
    this->chainWork = 0;
    this->height = 0;
    this->status = 0;
    this->refCounter = 0;
  }

  bool raiseValidity(enum BlockStatus upTo) {
    VBK_ASSERT(!(upTo & ~BLOCK_VALID_MASK));  // Only validity flags allowed.
    if ((status & BLOCK_FAILED_MASK) != 0u) {
      return false;
    }
    if ((status & BLOCK_VALID_MASK) < upTo) {
      status = (status & ~BLOCK_VALID_MASK) | upTo;
      return true;
    }
    return false;
  }

  void setFlag(enum BlockStatus s) { this->status |= s; }

  void unsetFlag(enum BlockStatus s) { this->status &= ~s; }

  bool hasFlags(enum BlockStatus s) const { return this->status & s; }

  hash_t getHash() const { return header->getHash(); }
  uint32_t getBlockTime() const { return header->getBlockTime(); }
  uint32_t getDifficulty() const { return header->getDifficulty(); }

  bool isValidTip() const {
    // can be a valid tip iff there're no next blocks or all next blocks are
    // invalid
    return isValid() &&
           (pnext.empty() ||
            std::all_of(pnext.begin(), pnext.end(), [](BaseBlockIndex* index) {
              return !index->isValid();
            }));
  }

  const BaseBlockIndex* getAncestorBlocksBehind(height_t steps) const {
    if (steps < 0 || steps > this->height + 1) {
      return nullptr;
    }

    return this->getAncestor(this->height + 1 - steps);
  }

  BaseBlockIndex* getAncestor(height_t _height) const {
    if (_height < 0 || _height > this->height) {
      return nullptr;
    }

    // TODO: this algorithm is not optimal. for O(n) seek backwards until we hit
    // valid height. also it assumes whole blockchain is in memory (pprev is
    // valid until given height)
    BaseBlockIndex* index = const_cast<BaseBlockIndex*>(this);
    while (index != nullptr) {
      if (index->height > _height) {
        index = index->pprev;
      } else if (index->height == _height) {
        return index;
      } else {
        return nullptr;
      }
    }

    return nullptr;
  }

  std::string toShortPrettyString() const {
    return fmt::sprintf("%s:%d:%s", Block::name(), height, HexStr(getHash()));
  }

  void toRaw(WriteStream& stream) const {
    stream.writeBE<uint32_t>(height);
    header.toRaw(stream);
  }

  std::vector<uint8_t> toRaw() const {
    WriteStream stream;
    toRaw(stream);
    return stream.data();
  }

  static BaseBlockIndex fromRaw(ReadStream& stream) {
    BaseBlockIndex index{};
    index.height = stream.readBE<uint32_t>();
    index.header = Block::fromRaw(stream);
    return index;
  }

  static BaseBlockIndex fromRaw(const std::string& bytes) {
    ReadStream stream(bytes);
    return fromRaw(stream);
  }

  friend bool operator==(const BaseBlockIndex& a, const BaseBlockIndex& b) {
    return *a.header == *b.header;
  }

  friend bool operator!=(const BaseBlockIndex& a, const BaseBlockIndex& b) {
    return !operator==(a, b);
  }
};

template <typename Block>
void PrintTo(const BaseBlockIndex<Block>& b, ::std::ostream* os) {
  *os << b.toPrettyString();
}

template <typename JsonValue, typename Block>
JsonValue ToJSON(const BaseBlockIndex<Block>& i) {
  auto obj = json::makeEmptyObject<JsonValue>();
  json::putStringKV(obj, "chainWork", i.chainWork.toHex());
  json::putIntKV(obj, "height", i.height);
  json::putKV(obj, "header", ToJSON<JsonValue>(*i.header));
  json::putIntKV(obj, "status", i.status);
  json::putIntKV(obj, "ref", i.refCounter);
  return obj;
}

}  // namespace altintegration
#endif  // ALT_INTEGRATION_INCLUDE_VERIBLOCK_BLOCKCHAIN_BLOCK_INDEX_HPP_