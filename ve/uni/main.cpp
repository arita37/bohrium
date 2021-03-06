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

#include <cassert>

#include <bh_component.hpp>
#include <bh_extmethod.hpp>
#include <bh_idmap.hpp>

#include "kernel.hpp"
#include "block.hpp"
#include "instruction.hpp"
#include "type.hpp"
#include "store.hpp"

using namespace bohrium;
using namespace component;
using namespace std;

namespace {
class Impl : public ComponentImpl {
  private:
    Store _store;
    map<bh_opcode, extmethod::ExtmethodFace> extmethods;
  public:
    Impl(int stack_level) : ComponentImpl(stack_level), _store(config) {}
    ~Impl() {}; // NB: a destructor implementation must exist
    void execute(bh_ir *bhir);
    void extmethod(const string &name, bh_opcode opcode) {
        // ExtmethodFace does not have a default or copy constructor thus
        // we have to use its move constructor.
        extmethods.insert(make_pair(opcode, extmethod::ExtmethodFace(config, name)));
    }
};
}

extern "C" ComponentImpl* create(int stack_level) {
    return new Impl(stack_level);
}
extern "C" void destroy(ComponentImpl* self) {
    delete self;
}

// Returns the views with the greatest number of dimensions
vector<const bh_view*> max_ndim_views(int64_t nviews, const bh_view view_list[]) {
    // Find the max ndim
    int64_t ndim = 0;
    for (int64_t i=0; i < nviews; ++i) {
        const bh_view *view = &view_list[i];
        if (not bh_is_constant(view)) {
            if (view->ndim > ndim)
                ndim = view->ndim;
        }
    }
    vector<const bh_view*> ret;
    for (int64_t i=0; i < nviews; ++i) {
        const bh_view *view = &view_list[i];
        if (not bh_is_constant(view)) {
            if (view->ndim == ndim) {
                ret.push_back(view);
            }
        }
    }
    return ret;
}

// Returns the shape of the view with the greatest number of dimensions
// if equal, the greatest shape is returned
vector<int64_t> shape_of_views(int64_t nviews, const bh_view *view_list) {
    vector<const bh_view*> views = max_ndim_views(nviews, view_list);
    vector<int64_t > shape;
    for(const bh_view *view: views) {
        for (int64_t j=0; j < view->ndim; ++j) {
            if (shape.size() > (size_t)j) {
                if (shape[j] < view->shape[j])
                    shape[j] = view->shape[j];
            } else {
                shape.push_back(view->shape[j]);
            }
        }
    }
    return shape;
}

namespace {
void spaces(stringstream &out, int num) {
    for (int i = 0; i < num; ++i) {
        out << " ";
    }
}
}

void write_block(const IdMap<bh_base*> &base_ids, const Block &block, stringstream &out) {
    spaces(out, 4 + block.rank*4);
    if (block._instr != NULL) {
        write_instr(base_ids, *block._instr, out);
    } else {

        // If this block is sweeped, we will "peel" the for-loop such that the
        // sweep instruction is replaced with BH_IDENTITY in the first iteration
        if (block._sweeps.size() > 0) {
            Block peeled_block(block);
            vector<bh_instruction> sweep_instr_list(block._sweeps.size());
            {
                size_t i = 0;
                for (const bh_instruction *instr: block._sweeps) {
                    Block *sweep_instr_block = peeled_block.findInstrBlock(instr);
                    assert(sweep_instr_block != NULL);
                    bh_instruction *sweep_instr = &sweep_instr_list[i++];
                    sweep_instr->opcode = BH_IDENTITY;
                    sweep_instr->operand[1] = instr->operand[1]; // The input is the same as in the sweep
                    sweep_instr->operand[0] = instr->operand[0];
                    // But the output needs an extra dimension when we are reducing to a non-scalar
                    if (bh_opcode_is_reduction(instr->opcode) and instr->operand[1].ndim > 1) {
                        sweep_instr->operand[0].insert_dim(instr->constant.get_int64(), 1, 0);
                    }
                    sweep_instr_block->_instr = sweep_instr;
                }
            }
            string itername;
            {stringstream t; t << "i" << block.rank; itername = t.str();}
            out << "{ // Peeled loop, 1. iteration" << endl;
            spaces(out, 8 + block.rank*4);
            out << "uint64_t " << itername << " = 0;" << endl;
            for (const Block &b: peeled_block._block_list) {
                write_block(base_ids, b, out);
            }
            spaces(out, 4 + block.rank*4);
            out << "}" << endl;
            spaces(out, 4 + block.rank*4);
        }

        string itername;
        {stringstream t; t << "i" << block.rank; itername = t.str();}
        out << "for(uint64_t " << itername;
        if (block._sweeps.size() > 0) // If the for-loop has been peeled, we should that at 1
            out << "=1; ";
        else
            out << "=0; ";
        out << itername << " < " << block.size << "; ++" << itername << ") {" << endl;
        for (const Block &b: block._block_list) {
            write_block(base_ids, b, out);
        }
        spaces(out, 4 + block.rank*4);
        out << "}" << endl;
    }
}

Kernel fuser_singleton(vector<bh_instruction> &instr_list) {

    // Creates the block_list based on the instr_list
    Kernel ret;
    for(const bh_instruction &instr: instr_list) {
        int nop = bh_noperands(instr.opcode);
        if (nop == 0)
            continue; // Ignore noop instructions such as BH_NONE or BH_TALLY

        if (bh_opcode_is_system(instr.opcode)) {
            continue; // Ignore system instructions, we will have BH_FREE later
        }

        int sweep_rank = sweep_axis(instr);
        vector<int64_t> shape = shape_of_views(nop, instr.operand);
        Block root;
        if (sweep_rank == 0) {
            root._sweeps.insert(&instr);
        }
        root.rank = 0;
        root.size = shape[0];
        Block *parent = &root;
        Block *bottom = &root;
        for(int i=1; i < (int)shape.size(); ++i) {
            Block b;
            if (sweep_rank == i) {
                b._sweeps.insert(&instr);
                shape_of_views(nop, instr.operand);
            }
            b.rank = i;
            b.size = shape[i];
            parent->_block_list.push_back(b);
            bottom = &parent->_block_list[0];
            parent = bottom;
        }
        Block instr_block;
        instr_block._instr = &instr;
        instr_block.rank = (int)shape.size();
        bottom->_block_list.push_back(instr_block);
        ret.block_list.push_back(root);
    }

    // Find all frees and kernel flags such as 'useRandom'
    for(bh_instruction &instr: instr_list) {
        if (instr.opcode == BH_RANDOM) {
            ret.useRandom = true;
        } else if (instr.opcode == BH_FREE) {
            ret.frees.insert(instr.operand[0].base);
        }
    }
    return ret;
}

void Impl::execute(bh_ir *bhir) {

    // Assign IDs to all base arrays
    IdMap<bh_base *> base_ids;
    // NB: by assigning the IDs in the order they appear in the 'instr_list',
    //     we kernels can better be reused
    for(const bh_instruction &instr: bhir->instr_list) {
        const int nop = bh_noperands(instr.opcode);
        for(int i=0; i<nop; ++i) {
            const bh_view &v = instr.operand[i];
            if (not bh_is_constant(&v)) {
                base_ids.insert(v.base);
            }
        }
    }
    // Do we have anything to do?
    if (base_ids.size() == 0)
        return;

    // Let's fuse
    Kernel kernel = fuser_singleton(bhir->instr_list);

    // Debug print
    //cout << block_list;

    // Code generation
    stringstream ss;

    // Make sure all arrays are allocated
    for(bh_base *base: base_ids.getKeys()) {
        bh_data_malloc(base);
    }

    // Write the need includes
    ss << "#include <stdint.h>" << endl;
    ss << "#include <stdlib.h>" << endl;
    ss << "#include <stdbool.h>" << endl;
    ss << "#include <complex.h>" << endl;
    ss << "#include <tgmath.h>" << endl;
    ss << "#include <math.h>" << endl;
    ss << "#include <bh_memory.h>" << endl;
    ss << "#include <bh_type.h>" << endl;
    ss << endl;

    if (kernel.useRandom) { // Write the random function
        ss << "#include <Random123/philox.h>" << endl;
        ss << "uint64_t random123(uint64_t start, uint64_t key, uint64_t index) {" << endl;
        ss << "    union {philox2x32_ctr_t c; uint64_t ul;} ctr, res; " << endl;
        ss << "    ctr.ul = start + index; " << endl;
        ss << "    res.c = philox2x32(ctr.c, (philox2x32_key_t){{key}}); " << endl;
        ss << "    return res.ul; " << endl;
        ss << "} " << endl;
    }
    ss << endl;

    // Write the header of the execute function
    ss << "void execute(";
    for(size_t id=0; id < base_ids.size(); ++id) {
        const bh_base *b = base_ids.getKeys()[id];
        ss << write_type(b->type) << " a" << id << "[]";
        if (id+1 < base_ids.size()) {
            ss << ", ";
        }
    }
    ss << ") {" << endl;

    // Write the blocks that makes up the body of 'execute()'
    for(const Block &block: kernel.block_list) {
        write_block(base_ids, block, ss);
    }

    ss << "}" << endl << endl;

    // Write the launcher function, which will convert the data_list of void pointers
    // to typed arrays and call the execute function
    {
        ss << "void launcher(void* data_list[]) {" << endl;
        size_t i=0;
        for (bh_base *b: base_ids.getKeys()) {
            spaces(ss, 4);
            ss << write_type(b->type) << " *a" << base_ids[b];
            ss << " = data_list[" << i << "];" << endl;
            ++i;
        }
        spaces(ss, 4);
        ss << "execute(";
        for(size_t id=0; id < base_ids.size(); ++id) {
            ss << "a" << id;
            if (id+1 < base_ids.size()) {
                ss << ", ";
            }
        }
        ss << ");" << endl;
        ss << "}" << endl;
    }

  //  cout << ss.str();

    KernelFunction func = _store.getFunction(ss.str());
    assert(func != NULL);

    // Create a 'data_list' of data pointers
    vector<void*> data_list;
    data_list.reserve(base_ids.size());
    for(bh_base *base: base_ids.getKeys()) {
        assert(base->data != NULL);
        data_list.push_back(base->data);
    }

    // Call the launcher function with the 'data_list', which will execute the kernel
    func(&data_list[0]);
    // Finally, let's cleanup
    for(bh_base *base: kernel.frees) {
        bh_data_free(base);
    }
}

