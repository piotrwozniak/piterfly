# SPDX-License-Identifier: GPL-3.0-only
import unittest
from target_budget import cost


def branch(target, conditional=False):
    return ('if (r0 != 0) ' if conditional else '') + f'goto -1 <test: {target:08x} >'


class LoopCostTest(unittest.TestCase):
    def test_acyclic_cold_block_is_not_a_loop(self):
        code = [(0, branch(12)), (4, 'r0 = r1 + 1'), (8, branch(20)),
                (12, branch(4, True)), (16, 'r2 = r0'), (20, 'rets')]
        self.assertEqual(cost(code)['cost'], 0)

    def test_real_loop_is_still_counted(self):
        code = [(0, 'r0 = 0'), (4, branch(20, True)), (8, 'r0 += 1'), (12, branch(4)), (20, 'rets')]
        self.assertEqual(cost(code)['cost'], 3)

    def test_cold_branch_inside_an_outer_loop_is_not_a_nested_loop(self):
        code = [(0, 'r0 = 0'), (4, branch(16)), (8, 'r1 += 1'),
                (12, branch(24)), (16, branch(8, True)), (20, 'r2 = r1'),
                (24, branch(4, True)), (28, 'rets')]
        self.assertEqual(cost(code)['cost'], 6)

    def test_nested_loop_keeps_the_existing_weight(self):
        code = [(0, 'r0 = 0'), (4, 'r1 = 0'), (8, 'r2 += 1'),
                (12, branch(8, True)), (16, branch(4, True)), (20, 'rets')]
        self.assertEqual(cost(code)['cost'], 10)  # Two outer-only instructions and two at x4.

    def test_farther_acyclic_branch_does_not_hide_a_real_loop(self):
        code = [(0, branch(20, True)), (4, 'r1 += 1'), (8, branch(4, True)),
                (12, branch(24)), (20, branch(4)), (24, 'rets')]
        self.assertEqual(cost(code)['cost'], 2)

    def test_irreducible_cycle_keeps_conservative_estimate(self):
        code = [(0, branch(8, True)), (4, branch(12)), (8, branch(12)),
                (12, branch(4, True)), (16, 'rets')]
        self.assertEqual(cost(code)['cost'], 3)

    def test_unknown_indirect_edge_keeps_conservative_estimate(self):
        code = [(0, 'goto r0'), (4, 'r1 += 1'), (8, branch(4)), (12, 'rets')]
        self.assertEqual(cost(code)['cost'], 2)


if __name__ == '__main__':
    unittest.main()
