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

#ifndef __BH_H
#define __BH_H

#include <cstddef>
#include "bh_error.h"
#include "bh_debug.h"
#include "bh_opcode.h"
#include "bh_type.h"
#include "bh_array.h"
#include "bh_instruction.h"
#include "bh_component.h"
#include "bh_pprint.h"
#include "bh_osx.h"
#include "bh_win.h"
#include "bh_memory.h"
#include "bh_ir.h"
#include "bh_mem_signal.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) >= (b) ? (a) : (b))

/* Find the base array for a given view
 *
 * @view   The view in question
 * @return The Base array
 */
#define bh_base_array(view) ((view)->base)

/* Number of non-broadcasted elements in a given view
 *
 * @view    The view in question.
 * @return  Number of elements.
 */
bh_index bh_nelements_nbcast(const bh_view *view);

/* Number of element in a given shape
 *
 * @ndim     Number of dimentions
 * @shape[]  Number of elements in each dimention.
 * @return   Number of element operations
 */
DLLEXPORT bh_index bh_nelements(bh_intp ndim,
                                const bh_index shape[]);
DLLEXPORT bh_index bh_nelements(const bh_view& view);

/* Size of the base array in bytes
 *
 * @base    The base in question
 * @return  The size of the base array in bytes
 */
bh_index bh_base_size(const bh_base *base);

/* Set the view stride to contiguous row-major
 *
 * @view    The view in question
 * @return  The total number of elements in view
 */
DLLEXPORT bh_intp bh_set_contiguous_stride(bh_view *view);

/* Updates the view with the complete base
 *
 * @view    The view to update (in-/out-put)
 * @base    The base assign to the view
 * @return  The total number of elements in view
 */
DLLEXPORT void bh_assign_complete_base(bh_view *view, bh_base *base);

/* Number of operands for operation
 *
 * @opcode Opcode for operation
 * @return Number of operands
 */
DLLEXPORT int bh_operands(bh_opcode opcode);

/* Number of operands in instruction
 * NB: this function handles user-defined function correctly
 * @inst Instruction
 * @return Number of operands
 */
DLLEXPORT int bh_operands_in_instruction(const bh_instruction *inst);

/* Text string for operation
 *
 * @opcode Opcode for operation
 * @return Text string.
 */
DLLEXPORT const char* bh_opcode_text(bh_opcode opcode);

/* Determines if the operation is a system operation
 *
 * @opcode The operation opcode
 * @return The boolean answer
 */
DLLEXPORT bool bh_opcode_is_system(bh_opcode opcode);

/* Determines if the operation is an elementwise operation
 *
 * @opcode The operation opcode
 * @return The boolean answer
 */
DLLEXPORT bool bh_opcode_is_elementwise(bh_opcode opcode);

/* Determines if the operation is a reduction operation
 *
 * @opcode The operation opcode
 * @return The boolean answer
 */
DLLEXPORT bool bh_opcode_is_reduction(bh_opcode opcode);

/* Determines if the operation is an accumulate operation
 *
 * @opcode The operation opcode
 * @return The boolean answer
 */
DLLEXPORT bool bh_opcode_is_accumulate(bh_opcode opcode);

/* Determines if the operation is performed elementwise
 *
 * @opcode Opcode for operation
 * @return TRUE if the operation is performed elementwise, FALSE otherwise
 */
DLLEXPORT bool bh_opcode_is_elementwise(bh_opcode opcode);

/* Determines if the types are acceptable for the operation
 *
 * @opcode Opcode for operation
 * @outtype The type of the output
 * @inputtype1 The type of the first input operand
 * @inputtype2 The type of the second input operand
 * @constanttype The type of the constant
 * @return TRUE if the types are accepted, FALSE otherwise
 */
DLLEXPORT bool bh_validate_types(bh_opcode opcode, bh_type outtype, bh_type inputtype1, bh_type inputtype2, bh_type constanttype);

/* Determines if the types are acceptable for the operation,
 * and provides a suggestion for converting them
 *
 * @opcode Opcode for operation
 * @outtype The type of the output
 * @inputtype1 The type of the first input operand
 * @inputtype2 The type of the second input operand
 * @constanttype The type of the constant
 * @return TRUE if the types can be converted, FALSE otherwise
 */
DLLEXPORT bool bh_get_type_conversion(bh_opcode opcode, bh_type outtype, bh_type* inputtype1, bh_type* inputtype2, bh_type* constanttype);

/**
 *  Deduct a numerical value representing the types of the given instruction.
 *
 *  @param instr The instruction for which to deduct a signature.
 *  @return The deducted signature.
 */
DLLEXPORT int bh_type_sig(bh_instruction *instr);

/**
 *  Determine whether the given typesig, in the coding produced by bh_typesig, is valid.
 *
 *  @param instr The instruction for which to deduct a signature.
 *  @return The deducted signature.
 */
DLLEXPORT bool bh_type_sig_check(int type_sig);

/* Byte size for type
 *
 * @type   Type code
 * @return Byte size
 */
DLLEXPORT int bh_type_size(bh_type type);


/* Text string for type
 *
 * @type   Type code.
 * @return Text string.
 */
DLLEXPORT const char* bh_type_text(bh_type type);


/* Text string for error code
 *
 * @error  Error code.
 * @return Text string.
 */
DLLEXPORT const char* bh_error_text(bh_error error);

/* Set the data pointer for the view.
 * Can only set to non-NULL if the data ptr is already NULL
 *
 * @view   The view in question
 * @data   The new data pointer
 * @return Error code (BH_SUCCESS, BH_ERROR)
 */
DLLEXPORT bh_error bh_data_set(bh_view* view, bh_data_ptr data);

/* Get the data pointer for the view.
 *
 * @view    The view in question
 * @result  Output data pointer
 * @return  Error code (BH_SUCCESS, BH_ERROR)
 */
DLLEXPORT bh_error bh_data_get(bh_view* view, bh_data_ptr* result);

/* Allocate data memory for the given base if not already allocated.
 * For convenience, the base is allowed to be NULL.
 *
 * @base    The base in question
 * @return  Error code (BH_SUCCESS, BH_ERROR, BH_OUT_OF_MEMORY)
 */
DLLEXPORT bh_error bh_data_malloc(bh_base* base);

/* Frees data memory for the given view.
 * For convenience, the view is allowed to be NULL.
 *
 * @base    The base in question
 * @return  Error code (BH_SUCCESS, BH_ERROR)
 */
bh_error bh_data_free(bh_base* base);

/* Retrive the operands of a instruction.
 *
 * @instruction  The instruction in question
 * @return The operand list
 */
DLLEXPORT bh_view *bh_inst_operands(bh_instruction *instruction);

/* Determines whether the view is a scalar or a broadcasted scalar.
 *
 * @view The view
 * @return The boolean answer
 */
DLLEXPORT bool bh_is_scalar(const bh_view* view);

/* Determines whether the operand is a constant
 *
 * @o The operand
 * @return The boolean answer
 */
DLLEXPORT bool bh_is_constant(const bh_view* o);

/* Flag operand as a constant
 *
 * @o      The operand
 */
DLLEXPORT void bh_flag_constant(bh_view* o);

/* Returns the simplest view (fewest dimensions) that access 
 * the same elements in the same pattern
 *
 * @view The view
 * @return The simplified view
 */
DLLEXPORT bh_view bh_view_simplify(const bh_view &view);

/* Simplifies, and removes repetisions (i.e. stride == 0)  
 * dimensions
 *
 * @view The view
 * @return The super_simplified view
 */
DLLEXPORT bh_view bh_view_super_simplify(const bh_view& view);

/* Simplifies the given view down to the given shape.
 * If that is not possible an std::invalid_argument exception is thrown 
 *
 * @view The view
 * @return The simplified view
 */
DLLEXPORT bh_view bh_view_simplify(const bh_view& view, const std::vector<bh_index>& shape);

/* Determines whether two views have same shape.
 *
 * @a The first view
 * @b The second view
 * @return The boolean answer
 */
DLLEXPORT bool bh_view_same_shape(const bh_view *a, const bh_view *b);

/* Determines whether two views are identical and points
 * to the same base array.
 *
 * @a The first view
 * @b The second view
 * @return The boolean answer
 */
DLLEXPORT bool bh_view_same(const bh_view *a, const bh_view *b);

/* Determines whether two views are aligned and points
 * to the same base array.
 *
 * @a The first view
 * @b The second view
 * @return The boolean answer
 */
DLLEXPORT bool bh_view_aligned(const bh_view *a, const bh_view *b);

/* Determines whether two views are aligned, points
 * to the same base array, and have same shape.
 *
 * @a The first view
 * @b The second view
 * @return The boolean answer
 */
DLLEXPORT bool bh_view_aligned_and_same_shape(const bh_view *a, const bh_view *b);

/* Determines whether two views access some of the same data points
 * NB: This functions may return True on non-overlapping views.
 *     But will always return False on overlapping views.
 *
 * @a The first view
 * @b The second view
 * @return The boolean answer
 */
DLLEXPORT bool bh_view_disjoint(const bh_view *a, const bh_view *b);

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

/* Determines whether the opcode is a sweep opcode 
 * i.e. either a reduction or an accumulate
 *
 * @opcode
 * @return The boolean answer
 */
DLLEXPORT bool bh_opcode_is_sweep(bh_opcode opcode);

template<typename E>
std::ostream& operator<<(std::ostream& out, const std::vector<E>& v) 
{
    out << "[";
    for (typename std::vector<E>::const_iterator i = v.cbegin();;)
    {
        out << *i;
        if (++i == v.cend())
            break;
        out << ", ";
    }
    out << "]";
    return out;
}

#endif

