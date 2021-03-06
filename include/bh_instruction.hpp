#ifndef __BH_INSTRUCTION_H
#define __BH_INSTRUCTION_H

#include <boost/serialization/is_bitwise_serializable.hpp>
#include <boost/serialization/array.hpp>
#include <set>

#include "bh_opcode.h"
#include <bh_array.hpp>
#include "bh_error.h"

// Forward declaration of class boost::serialization::access
namespace boost {namespace serialization {class access;}}

// Maximum number of operands in a instruction.
#define BH_MAX_NO_OPERANDS (3)

//Memory layout of the Bohrium instruction
struct bh_instruction
{
    //Opcode: Identifies the operation
    bh_opcode  opcode;
    //Id of each operand
    bh_view  operand[BH_MAX_NO_OPERANDS];
    //Constant included in the instruction (Used if one of the operands == NULL)
    bh_constant constant;

    bh_instruction(){}
    bh_instruction(const bh_instruction& instr)
    {
        opcode = instr.opcode;
        constant = instr.constant;
        std::memcpy(operand, instr.operand, bh_noperands(opcode) * sizeof(bh_view));
    }

    // Return a set of all bases used by the instruction
    std::set<bh_base*> get_bases();

    // Serialization using Boost
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & opcode;
        //We use make_array as a hack to make bh_constant BOOST_IS_BITWISE_SERIALIZABLE
        ar & boost::serialization::make_array(&constant, 1);
        const size_t nop = bh_noperands(opcode);
        for(size_t i=0; i<nop; ++i)
            ar & operand[i];
    }

    bool operator==(const bh_instruction& other) const
    {
        if (opcode != other.opcode) return false;
        if (constant != other.constant) return false;

        for (bh_intp i = 0; i < BH_MAX_NO_OPERANDS; ++i) {
            if (operand[i] != other.operand[i]) return false;
        }

        return true;
    }

    bool operator!=(const bh_instruction& other) const
    {
        return !(*this == other);
    }
};
BOOST_IS_BITWISE_SERIALIZABLE(bh_constant)

//Implements pprint of an instruction
DLLEXPORT std::ostream& operator<<(std::ostream& out, const bh_instruction& instr);

/* Retrive the operands of a instruction.
 *
 * @instruction  The instruction in question
 * @return The operand list
 */
DLLEXPORT bh_view *bh_inst_operands(bh_instruction *instruction);

/* Determines whether instruction 'a' depends on instruction 'b',
 * which is true when:
 *      'b' writes to an array that 'a' access
 *                        or
 *      'a' writes to an array that 'b' access
 *
 * @a The first instruction
 * @b The second instruction
 * @return The boolean answer
 */
DLLEXPORT bool bh_instr_dependency(const bh_instruction *a, const bh_instruction *b);


#endif
