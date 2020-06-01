// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
#include <gtest/gtest.h>

#include "util/pop_test_fixture.hpp"

using namespace altintegration;

struct PopFrInvalidVbkChainTest : public ::testing::Test,
                                  public PopTestFixture {
  BlockIndex<VbkBlock>* tipA;
  BlockIndex<VbkBlock>* tipB;

  VbkBlock vtbcontaining;

  PopFrInvalidVbkChainTest() {
    // prepare:
    // tipA is 40 blocks long
    tipA = popminer->mineVbkBlocks(40);
    // tipB forks at 10, and has 28 more blocks. tipA is best chain
    tipB = popminer->mineVbkBlocks(*tipA->getAncestor(10), 28);
    EXPECT_EQ(*popminer->vbk().getBestChain().tip(), *tipA);

    // next block in B endorses block number 25 in its chain twice, and
    // containing is block 39
    auto vbkpoptx1 = popminer->endorseVbkBlock(
        *tipB->getAncestor(25)->header, getLastKnownBtcBlock(), state);
    auto vbkpoptx2 = popminer->endorseVbkBlock(
        *tipB->getAncestor(25)->header, getLastKnownBtcBlock(), state);
    popminer->vbkmempool.push_back(vbkpoptx1);
    popminer->vbkmempool.push_back(vbkpoptx2);

    auto vbkcontaining = popminer->mineVbkBlocks(*tipB, 1);
    EXPECT_EQ(vbkcontaining->height, 39);
    tipB = vbkcontaining;
    vtbcontaining = *vbkcontaining->header;

    // since chain B contains an endorsement of KS period 20-40, now it has to
    // be active
    EXPECT_EQ(*popminer->vbk().getBestChain().tip(), *tipB);
  }

  std::vector<AltBlock> chain{altparam.getBootstrapBlock()};
};

TEST_F(PopFrInvalidVbkChainTest, SendInvalidVTBtoAlternativeVBKchain) {
  // endorse block 26 in chain B, containing is B40
  auto missingVbkBlock = popminer->mineVbkBlocks(*tipB, 1);
  tipB = missingVbkBlock;
  auto vbkpoptx = popminer->endorseVbkBlock(
      *tipB->getAncestor(26)->header, getLastKnownBtcBlock(), state);
  popminer->vbkmempool.push_back(vbkpoptx);
  tipB = popminer->mineVbkBlocks(*tipB, 1);
  ASSERT_EQ(tipB->height, 41);

  // mine 10 alt blocks
  mineAltBlocks(10, chain);

  // endorse ALT5
  auto vbktx1 =
      popminer->createVbkTxEndorsingAltBlock(generatePublicationData(chain[5]));
  auto atv1 = popminer->generateATV(vbktx1, getLastKnownVbkBlock(), state);
  ASSERT_EQ(atv1.containingBlock.height, 42);

  // mine 10 more blocks on top of tipB
  tipB = popminer->mineVbkBlocks(*tipB, 10);

  //! act: add ATV1, VTB1 to ALT9. should be valid.
  auto vtb1 = popminer->vbkPayloads[vtbcontaining.getHash()][0];
  AltPayloads p1;
  p1.endorsed = chain[5];
  p1.containingBlock = chain[9];
  p1.popData.hasAtv = true;
  p1.popData.atv = atv1;
  p1.popData.vtbs = {vtb1};
  fillVbkContext(p1.popData.vbk_context,
                 getLastKnownVbkBlock(),
                 atv1.containingBlock.getHash(),
                 popminer->vbk());

  ASSERT_TRUE(alttree.addPayloads(chain[9], {p1}, state));
  ASSERT_TRUE(alttree.setState(chain[9].hash, state)) << state.toString();

  //! act: add ATV2, VTB2 to ALT10
  // endorse ALT5
  auto vbktx2 =
      popminer->createVbkTxEndorsingAltBlock(generatePublicationData(chain[5]));
  auto atv2 = popminer->generateATV(vbktx1, getLastKnownVbkBlock(), state);
  ASSERT_EQ(atv1.containingBlock.height, 42);

  auto vtb2 = popminer->vbkPayloads[vtbcontaining.getHash()][1];
  AltPayloads p2;
  p2.endorsed = chain[5];
  p2.containingBlock = chain[10];
  p2.popData.hasAtv = true;
  p2.popData.atv = atv2;
  // break VTB2: break hash of containing block
  vtb2.containingBlock.previousBlock = uint96::fromHex("abcdef");
  p2.popData.vtbs = {vtb2};

  ASSERT_TRUE(alttree.addPayloads(chain[10], {p2}, state));
  ASSERT_FALSE(alttree.setState(chain[10].hash, state));
}