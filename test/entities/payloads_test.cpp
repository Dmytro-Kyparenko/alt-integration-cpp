#include <gtest/gtest.h>

#include "util/literals.hpp"
#include "veriblock/entities/payloads.hpp"

using namespace VeriBlock;

static const PublicationData publicationData{
    0, "header bytes"_v, "payout info bytes"_v, "context info bytes"_v};

static const auto defaultSignature =
    "30440220398b74708dc8f8aee68fce0c47b8959e6fce6354665da3ed87a83f708e62a"
    "a6b02202e6c00c00487763c55e92c7b8e1dd538b7375d8df2b2117e75acbb9db7deb3"
    "c7"_unhex;

static const auto defaultPublicKey =
    "3056301006072a8648ce3d020106052b8104000a03420004de4ee8300c3cd99e91353"
    "6cf53c4add179f048f8fe90e5adf3ed19668dd1dbf6c2d8e692b1d36eac7187950620"
    "a28838da60a8c9dd60190c14c59b82cb90319e"_unhex;

static const VbkTx defaultTx{
    NetworkBytePair{false, 0, (uint8_t)TxType::VBK_TX},
    Address::fromString("V5Ujv72h4jEBcKnALGc4fKqs6CDAPX"),
    Coin(1000),
    std::vector<Output>{},
    7,
    publicationData,
    defaultSignature,
    defaultPublicKey};

static const VbkMerklePath defaultPath{
    1,
    0,
    uint256(
        "1fec8aa4983d69395010e4d18cd8b943749d5b4f575e88a375debdc5ed22531c"_unhex),
    std::vector<uint256>{
        uint256(
            "0000000000000000000000000000000000000000000000000000000000000000"_unhex),
        uint256(
            "0000000000000000000000000000000000000000000000000000000000000000"_unhex)}};

static const VbkBlock defaultVbkBlock{5000,
                                      2,
                                      "449c60619294546ad825af03"_unhex,
                                      "b0935637860679ddd5"_unhex,
                                      "5ee4fd21082e18686e"_unhex,
                                      "26bbfda7d5e4462ef24ae02d67e47d78"_unhex,
                                      1553699059,
                                      16842752,
                                      1};

static const ATV defaultAtv{
    defaultTx, defaultPath, defaultVbkBlock, std::vector<VbkBlock>{}};

static const AltBlock defaultContainingBlock{
    "1fec8aa4983d69395010e4d18cd8b943749d5b4f575e88a375debdc5ed22531c"_unhex,
    156,
    1466};
static const AltBlock defaultEndorsedBlock{
    "449c60619294546ad825af03"_unhex, 124, 1246};

static const AltProof defaultAltProof{
    defaultEndorsedBlock, defaultContainingBlock, defaultAtv};

TEST(AltProof, RoundTrip) {
  std::vector<uint8_t> bytes = defaultAltProof.toVbkEncoding();
  AltProof deserialized =
      AltProof::fromVbkEncoding(std::string(bytes.begin(), bytes.end()));

  EXPECT_EQ(deserialized.endorsed.hash, defaultEndorsedBlock.hash);
  EXPECT_EQ(deserialized.endorsed.height, defaultEndorsedBlock.height);
  EXPECT_EQ(deserialized.endorsed.timestamp, defaultEndorsedBlock.timestamp);

  EXPECT_EQ(deserialized.containing.hash, defaultContainingBlock.hash);
  EXPECT_EQ(deserialized.containing.height, defaultContainingBlock.height);
  EXPECT_EQ(deserialized.containing.timestamp,
            defaultContainingBlock.timestamp);

  EXPECT_EQ(deserialized.atv.toVbkEncoding(), defaultAtv.toVbkEncoding());
}