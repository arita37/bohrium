#include <iomanip>

#include "engine.hpp"
#include "timevault.hpp"
#include "kp_rt.h"
#include "kp_vcache.h"

using namespace std;
using namespace kp::core;

namespace kp{
namespace engine{

const char Engine::TAG[] = "Engine";

Engine::Engine(
    const kp_thread_binding binding,
    const size_t vcache_size,
    const bool preload,
    const bool jit_enabled,
    const bool jit_dumpsrc,
    const bool jit_fusion,
    const bool jit_contraction,
    const bool jit_offload,
    const string compiler_cmd,
    const string compiler_inc,
    const string compiler_lib,
    const string compiler_flg,
    const string compiler_ext,
    const string object_directory,
    const string template_directory,
    const string kernel_directory
    )
:   rt_(NULL),
    preload_(preload),
    jit_enabled_(jit_enabled),
    jit_dumpsrc_(jit_dumpsrc),
    jit_fusion_(jit_fusion),
    jit_contraction_(jit_contraction),
    jit_offload_(jit_offload),
    jit_offload_devid_(jit_offload-1),
    storage_(object_directory, kernel_directory),
    plaid_(template_directory),
    compiler_(compiler_cmd, compiler_inc, compiler_lib, compiler_flg, compiler_ext)
{
    if (preload_) {                     // Object storage
        storage_.preload();
    }

    rt_ = kp_rt_init(vcache_size);      // Initialize CAPE C-runtime
    kp_rt_bind_threads(rt_, binding);   // Bind threads on host PUs

    DEBUG(TAG, text());                 // Print the engine configuration
}

Engine::~Engine()
{   
    kp_rt_shutdown(rt_);                // Shut down the CAPE C-runtime
}

size_t Engine::vcache_size(void)
{
    return kp_rt_vcache_size(rt_);
}

bool Engine::preload(void)
{
    return preload_;
}

bool Engine::jit_enabled(void)
{
    return jit_enabled_;
}

bool Engine::jit_dumpsrc(void)
{
    return jit_dumpsrc_;
}

bool Engine::jit_fusion(void)
{
    return jit_fusion_;
}

bool Engine::jit_contraction(void)
{
    return jit_contraction_;
}

bool Engine::jit_offload(void)
{
    return jit_offload_;
}

int Engine::jit_offload_devid(void)
{
    return jit_offload_devid_;
}

string Engine::text()
{
    stringstream ss;
    ss << "Engine {" << endl;
    ss << "  vcache_size = "        << kp_rt_vcache_size(rt_) << endl;
    ss << "  preload = "            << this->preload_ << endl;    
    ss << "  jit_enabled = "        << this->jit_enabled_ << endl;    
    ss << "  jit_dumpsrc = "        << this->jit_dumpsrc_ << endl;
    ss << "  jit_fusion = "         << this->jit_fusion_ << endl;
    ss << "  jit_contraction = "    << this->jit_contraction_ << endl;
    ss << "  jit_offload = "        << this->jit_offload_ << endl;
    ss << "}" << endl;
    
    ss << storage_.text() << endl;
    ss << compiler_.text() << endl;
    ss << plaid_.text() << endl;

    return ss.str();    
}

bh_error Engine::process_block(Program &tac_program,
                               SymbolTable &symbol_table,
                               Block &block
)
{
    bool consider_jit = jit_enabled_ and (block.narray_tacs() > 0);

    if (!block.symbolize()) {                       // Update block-symbol
        fprintf(stderr, "Engine::process_block(...) == Failed creating symbol.\n");
        return BH_ERROR;
    }

    DEBUG(TAG, "PROCESSING " << block.symbol());

    //
    // JIT-compile: generate source and compile code
    //
    if (consider_jit && \
        (!storage_.symbol_ready(block.symbol()))) {   
        DEBUG(TAG, "JITTING " << block.text());
                                                        // Genereate source
        string sourcecode = codegen::Kernel(plaid_, block).generate_source(jit_offload_);

        bool compile_res;
        if (jit_dumpsrc_==1) {                          // Compile via file
            core::write_file(                           // Dump to file
                storage_.src_abspath(block.symbol()),
                sourcecode.c_str(), 
                sourcecode.size()
            );
            compile_res = compiler_.compile(            // Compile
                storage_.obj_abspath(block.symbol()),
                storage_.src_abspath(block.symbol())
            );
        } else {                                        // Compile via stdin
            compile_res = compiler_.compile(            // Compile
                storage_.obj_abspath(block.symbol()),
                sourcecode.c_str(), 
                sourcecode.size()
            );
        }
        if (!compile_res) {
            fprintf(stderr, "Engine::execute(...) == Compilation failed.\n");

            return BH_ERROR;
        }
        storage_.add_symbol(                            // Inform storage
            block.symbol(),
            storage_.obj_filename(block.symbol())
        );
    }

    //
    // Load the compiled code
    //
    if ((block.narray_tacs() > 0) && \
        (!storage_.symbol_ready(block.symbol())) && \
        (!storage_.load(block.symbol()))) {             // Need but cannot load

        fprintf(stderr, "Engine::execute(...) == Failed loading object.\n");
        return BH_ERROR;
    }

    //
    // Grab the kernel function
    kp_krnl_func func = NULL;
    if (block.narray_tacs() > 0) {
        func = storage_.funcs[block.symbol()];
    }

    // Now on with the execution
    bool llexec = kp_rt_execute(rt_, &tac_program.meta(), &symbol_table.meta(), &block.meta(), func);
    if (llexec) {
        return BH_SUCCESS;
    } else {
        return BH_ERROR;
    }
}

}}

