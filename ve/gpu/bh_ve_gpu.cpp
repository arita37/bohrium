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

#include <iostream>
#include <stdexcept>
#include <bh.h>
#include "bh_ve_gpu.h"

bh_error bh_ve_gpu_init(bh_component* _component)
{
    component = _component;
    try {
        resourceManager = new ResourceManager(component);
        instructionScheduler = new InstructionScheduler(resourceManager);
    } 
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return BH_ERROR;
    }
    return BH_SUCCESS;
}

std::vector<bh_instruction*> inst_list;

static bh_error create_inst_list(bh_instruction* inst)
{
    inst_list.push_back(inst);
    return BH_SUCCESS;
}

bh_error bh_ve_gpu_execute(bh_ir* bhir)
{
    
    bh_ir_map_instr(bhir, &bhir->dag_list[0], &create_inst_list);
    
    bh_error ret_val = BH_ERROR;
    try
    { 
        ret_val =  instructionScheduler->schedule(inst_list);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    inst_list.clear();
    return ret_val;
}

bh_error bh_ve_gpu_shutdown()
{
    delete instructionScheduler;
    delete resourceManager;
    return BH_SUCCESS;
}

bh_error bh_ve_gpu_reg_func(char *fun, 
                            bh_intp *id)
{
    bh_userfunc_impl userfunc;
    bh_component_get_func(component, fun, &userfunc);
    if (userfunc != NULL)
    {
        instructionScheduler->registerFunction(*id, userfunc);
    	return BH_SUCCESS;
    }
    else
	    return BH_USERFUNC_NOT_SUPPORTED;
}
