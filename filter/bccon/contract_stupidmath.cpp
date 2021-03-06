/*
This file is part of Bohrium and copyright (c) 2012 the Bohrium
team <http://www.bh107.org>.

Bohrium is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3
of the License, or (at your option) any later version.

Bohrium is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the
GNU Lesser General Public License along with Bohrium.

If not, see <http://www.gnu.org/licenses/>.
*/
#include "contracter.hpp"

using namespace std;

namespace bohrium {
namespace filter {
namespace bccon {

static inline bool is_multiplying_by_one(const bh_instruction& instr)
{
    return instr.opcode == BH_MULTIPLY and
           instr.constant.get_double() == 1.0;
}

static inline bool is_dividing_by_one(const bh_instruction& instr)
{
    return instr.opcode == BH_DIVIDE and
           instr.constant.get_double() == 1.0;
}

static inline bool is_adding_zero(const bh_instruction& instr)
{
    return instr.opcode == BH_ADD and
           instr.constant.get_double() == 0.0;
}

static inline bool is_subtracting_zero(const bh_instruction& instr)
{
    return instr.opcode == BH_SUBTRACT and
           instr.constant.get_double() == 0.0;
}

static inline bool is_entire_view(const bh_instruction& instr)
{
    for(int i = 0; i < bh_noperands(instr.opcode); ++i) {
        if (bh_is_contiguous(&(instr.operand[i]))) {
            return true;
        }
    }

    return false;
}

static inline bool is_constant(const bh_instruction& instr)
{
    for(int i = 0; i < bh_noperands(instr.opcode); ++i) {
        if (bh_is_constant(&(instr.operand[i]))) {
            return true;
        }
    }

    return false;
}

static inline bool is_doing_stupid_math(const bh_instruction& instr)
{
    return bh_type_is_integer(instr.constant.type) and
           is_constant(instr) and
           (
               is_multiplying_by_one(instr) or
               is_dividing_by_one(instr) or
               is_adding_zero(instr) or
               is_subtracting_zero(instr)
           ) and
           is_entire_view(instr);
}

void Contracter::contract_stupidmath(bh_ir &bhir)
{
    for(size_t pc = 0; pc < bhir.instr_list.size(); ++pc) {
        bh_instruction& instr = bhir.instr_list[pc];

        if (is_doing_stupid_math(instr)) {
            verbose_print("[Stupid math] Is doing stupid math with a " + std::string(bh_opcode_text(instr.opcode)));

            // We could have the following:
            //   BH_ADD B A 0
            //   BH_FREE A
            //   BH_SYNC B
            // We want to find the add and replace A in all above with B, if A is created in this flush.
            // Then remove the free of A.

            // Output operand
            bh_view* B = &(instr.operand[0]);

            // The one operand, that isn't constant
            bh_view* A;
            if (bh_is_constant(&(instr.operand[1]))) {
                A = &(instr.operand[2]);
            } else {
                A = &(instr.operand[1]);
            }

            if (bh_view_same(A, B)) continue;

            bool freed = false;
            for (size_t pc_chain = 0; pc_chain < bhir.instr_list.size(); ++pc_chain) {
                bh_instruction& other_instr = bhir.instr_list[pc_chain];

                // Look for matching FREE for B
                if (other_instr.opcode == BH_FREE and bh_view_same(&(other_instr.operand[0]), B)) {
                    freed = true;
                    break;
                }
            }

            if (!freed) {
                verbose_print("[Stupid math] \tCan't rectify as it isn't freeing in same flush.");
                continue;
            }

            // Check that A is created by us.
            bool created_before = false;

            for (size_t pc_chain = 0; pc_chain < pc; ++pc_chain) {
                bh_instruction& other_instr = bhir.instr_list[pc_chain];

                if (bh_view_same(&(other_instr.operand[0]), A)) {
                    created_before = true;
                    break;
                }
            }

            // Only if we have created A in this flush, are we allowed to change it.
            if (!created_before) {
                verbose_print("[Stupid math] \tCan't rectify as other view isn't created in same flush.");
                continue;
            }

            for (size_t pc_chain = 0; pc_chain < bhir.instr_list.size(); ++pc_chain) {
                if (pc == pc_chain) continue;

                bh_instruction& other_instr = bhir.instr_list[pc_chain];

                // Look for matching FREE for A
                if (other_instr.opcode == BH_FREE and bh_view_same(&(other_instr.operand[0]), A)) {
                    verbose_print("[Stupid math] \tFound and removed FREE.");
                    other_instr.opcode = BH_NONE; // Remove instruction
                } else {
                    // Rewrite all uses of A to B
                    for (int idx = 0; idx < bh_noperands(other_instr.opcode); ++idx) {
                        if (bh_view_same(&(other_instr.operand[idx]), A)) {
                            verbose_print("[Stupid math] \tRewriting A to B.");
                            other_instr.operand[idx] = *B;
                        }
                    }
                }
            }

            // Remove self
            verbose_print("[Stupid math] \tRemoving " + std::string(bh_opcode_text(instr.opcode)));
            instr.opcode = BH_NONE;
        }
    }
}

}}}
