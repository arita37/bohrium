#include <sstream>
#include <string>
#include "codegen.hpp"
//
//  NOTE: This file is autogenerated based on the tac-definition.
//        You should therefore not edit it manually.
//
using namespace std;
using namespace bohrium::core;

namespace bohrium{
namespace engine{
namespace cpu{
namespace codegen{

string Walker::oper_neutral_element(OPERATOR oper, ETYPE etype)
{
    switch(oper) {
        case ADD:               return "0";
        case MULTIPLY:          return "1";
        case MAXIMUM:
            switch(etype) {
                case BOOL:      return "0";
                case INT8:      return "INT8_MIN";
                case INT16:     return "INT16_MIN";
                case INT32:     return "INT32_MIN";
                case INT64:     return "INT64_MIN";
                case UINT8:     return "UINT8_MIN";
                case UINT16:    return "UINT16_MIN";
                case UINT32:    return "UINT32_MIN";
                case UINT64:    return "UINT64_MIN";
                case FLOAT32:   return "FLT_MIN";
                case FLOAT64:   return "DBL_MIN";
                default:        return "UNKNOWN_NEUTRAL_FOR_MAXIMUM_OF_GIVEN_TYPE";
            }
        case MINIMUM:
            switch(etype) {
                case BOOL:      return "1";
                case INT8:      return "INT8_MAX";
                case INT16:     return "INT16_MAX";
                case INT32:     return "INT32_MAX";
                case INT64:     return "INT64_MAX";
                case UINT8:     return "UINT8_MAX";
                case UINT16:    return "UINT16_MAX";
                case UINT32:    return "UINT32_MAX";
                case UINT64:    return "UINT64_MAX";
                case FLOAT32:   return "FLT_MAX";
                case FLOAT64:   return "DBL_MAX";
                default:        return "UNKNOWN_NEUTRAL_FOR_MINIMUM_OF_GIVEN_TYPE";
            }
        case LOGICAL_AND:       return "1";
        case LOGICAL_OR:        return "0";
        case LOGICAL_XOR:       return "0";
        case BITWISE_AND:       return "1";
            switch(etype) {
                case BOOL:      return "1";
                case INT8:      return "-1";
                case INT16:     return "-1";
                case INT32:     return "-1";
                case INT64:     return "-1";
                case UINT8:     return "UINT8_MAX";
                case UINT16:    return "UINT16_MAX";
                case UINT32:    return "UINT32_MAX";
                case UINT64:    return "UINT64_MAX";
                default:        return "UNKNOWN_NEUTRAL_FOR_BITWISE_AND_OF_GIVEN_TYPE";
            }
        case BITWISE_OR:        return "0";
        case BITWISE_XOR:       return "0";
        default:                return "UNKNOWN_NEUTRAL_FOR_OPERATOR";
    }
}

string Walker::oper_description(tac_t tac)
{
    stringstream ss;
    ss << operator_text(tac.oper) << " (";
    switch(core::tac_noperands(tac)) {
        case 3:
            ss << layout_text(kernel_.operand_glb(tac.out).meta().layout);
            ss << ", ";
            ss << layout_text(kernel_.operand_glb(tac.in1).meta().layout);
            ss << ", ";
            ss << layout_text(kernel_.operand_glb(tac.in2).meta().layout);
            break;
        case 2:
            ss << layout_text(kernel_.operand_glb(tac.out).meta().layout);
            ss << ", ";
            ss << layout_text(kernel_.operand_glb(tac.in1).meta().layout);
            break;
        case 1:
            ss << layout_text(kernel_.operand_glb(tac.out).meta().layout);
            break;
        default:
            break;
    }
    ss << ")";
    return ss.str();
}

string Walker::oper(OPERATOR oper, ETYPE etype, string in1, string in2)
{
    switch(oper) {
        case ABSOLUTE:
            switch(etype) {
                case COMPLEX128:    return _cabs(in1);
                case COMPLEX64:     return _cabsf(in1);
                default:            return _abs(in1);
            }
        case ADD:                   return _add(in1, in2);
        case ARCCOS:            
            switch(etype) {
                case COMPLEX128:    return _cacos(in1);
                case COMPLEX64:     return _cacosf(in1);
                default:            return _acos(in1);
            }
        case ARCCOSH:
            switch(etype) {
                case COMPLEX128:    return _cacosh(in1);
                case COMPLEX64:     return _cacosf(in1);
                default:            return _acosh(in1);
            }
        case ARCSIN:
            switch(etype) {
                case COMPLEX128:    return _casin(in1);
                case COMPLEX64:     return _casinf(in1);
                default:            return _asin(in1);
            }
        case ARCSINH:
            switch(etype) {
                case COMPLEX128:    return _casinh(in1);
                case COMPLEX64:     return _casinhf(in1);
                default:            return _asinh(in1);
            }
        case ARCTAN:
            switch(etype) {
                case COMPLEX128:    return _catan(in1);
                case COMPLEX64:     return _catanf(in1);
                default:            return _atan(in1);
            }
        case ARCTAN2:               return _atan2(in1, in2);
        case ARCTANH:
            switch(etype) {
                case COMPLEX128:    return _catanh(in1);
                case COMPLEX64:     return _catanhf(in1);
                default:            return _atanh(in1);
            }
        case BITWISE_AND:           return _bitw_and(in1, in2);
        case BITWISE_OR:            return _bitw_or(in1, in2);
        case BITWISE_XOR:           return _bitw_xor(in1, in2);
        case CEIL:                  return _ceil(in1);
        case COS:
            switch(etype) {
                case COMPLEX128:    return _ccos(in1);
                case COMPLEX64:     return _ccosf(in1);
                default:            return _cos(in1);
            }
        case COSH:
            switch(etype) {
                case COMPLEX128:    return _ccosh(in1);
                case COMPLEX64:     return _ccoshf(in1);
                default:            return _cosh(in1);
            }
        case DISCARD:               break;  // TODO: Raise exception
        case DIVIDE:                return _div(in1, in2);
        case EQUAL:                 return _eq(in1, in2);
        case EXP:
            switch(etype) {
                case COMPLEX128:    return _cexp(in1);
                case COMPLEX64:     return _cexpf(in1);
                default:            return _exp(in1);
            }
        case EXP2:
            switch(etype) {
                case COMPLEX128:    return _cexp2(in1);
                case COMPLEX64:     return _cexp2f(in1);
                default:            return _exp2(in1);
            }
        case EXPM1:                 return _expm1(in1);
        case EXTENSION_OPERATOR:    break;  // TODO: Raise exception
        case FLOOD:                 break;  // TODO: Raise exception
        case FLOOR:                 return _floor(in1);
        case FREE:                  break;  // TODO: Raise exception
        case GREATER:               return _gt(in1, in2);
        case GREATER_EQUAL:         return _gteq(in1, in2);
        case IDENTITY:              return in1;
        case IMAG:
            switch(etype) {
                case FLOAT32:       return _cimagf(in1);
                default:            return _cimag(in1);
            }
        case INVERT:
            switch(etype) {
                case BOOL:          return _invertb(in1);
                default:            return _invert(in1);
            }
        case ISINF:                 return _isinf(in1);
        case ISNAN:                 return _isnan(in1);
        case LEFT_SHIFT:            return _bitw_leftshift(in1, in2);
        case LESS:                  return _lt(in1, in2);
        case LESS_EQUAL:            return _lteq(in1, in2);
        case LOG:
            switch(etype) {
                case COMPLEX128:    return _clog(in1);
                case COMPLEX64:     return _clogf(in1);
                default:            return _log(in1);
            }
        case LOG10:
            switch(etype) {
                case COMPLEX128:    return _clog10(in1);
                case COMPLEX64:     return _clog10f(in1);
                default:            return _log10(in1);
            }
        case LOG1P:                 return _log1p(in1);
        case LOG2:                  return _log2(in1);
        case LOGICAL_AND:           return _logic_and(in1, in2);
        case LOGICAL_NOT:           return _logic_not(in1);
        case LOGICAL_OR:            return _logic_or(in1, in2);
        case LOGICAL_XOR:           return _logic_xor(in1, in2);
        case MAXIMUM:               return _max(in1, in2);
        case MINIMUM:               return _min(in1, in2);
        case MOD:                   return _mod(in1, in2);
        case MULTIPLY:              return _mul(in1, in2);
        case NONE:                  break;  // TODO: Raise exception
        case NOT_EQUAL:             return _neq(in1, in2);
        case POWER:
            switch(etype) {
                case COMPLEX128:    return _cpow(in1, in2);
                case COMPLEX64:     return _cpowf(in1, in2);
                default:            return _pow(in1, in2);
            }
        case RANDOM:                return _random(in1, in2);
        case RANGE:                 return _range();
        case REAL:
            switch(etype) {
                case FLOAT32:       return _crealf(in1);
                default:            return _creal(in1);
            }
        case RIGHT_SHIFT:           return _bitw_rightshift(in1, in2);
        case RINT:                  return _rint(in1);
        case SIN:
            switch(etype) {
                case COMPLEX128:    return _csin(in1);
                case COMPLEX64:     return _csinf(in1);
                default:            return _sin(in1);
            }
        case SIGN:
            switch(etype) {
                case COMPLEX128:    return _div(in1, _cabs(in1));
                case COMPLEX64:     return _div(in1, _cabsf(in1));
                default:            return _sub(
                                            _parens(_gt(in1, "0")),
                                            _parens(_lt(in1, "0"))
                                           );
            }

        case SINH:
            switch(etype) {
                case COMPLEX128:    return _csinh(in1);
                case COMPLEX64:     return _csinhf(in1);
                default:            return _sinh(in1);
            }
        case SQRT:
            switch(etype) {
                case COMPLEX128:    return _csqrt(in1);
                case COMPLEX64:     return _csqrtf(in1);
                default:            return _sqrt(in1);
            }
        case SUBTRACT:              return _sub(in1, in2);
        case SYNC:                  break;  // TODO: Raise exception
        case TAN:
            switch(etype) {
                case COMPLEX128:    return _ctan(in1);
                case COMPLEX64:     return _ctanf(in1);
                default:            return _tan(in1);
            }
        case TANH:
            switch(etype) {
                case COMPLEX128:    return _ctanh(in1);
                case COMPLEX64:     return _ctanhf(in1);
                default:            return _tanh(in1);
            }
        case TRUNC:                 return _trunc(in1);
        default:                    return "NOT_IMPLEMENTED_YET";
    }
    return "NO NO< NO NO NO NO NONO NO NO NO NOTHERES NO LIMITS";
}

string Walker::synced_oper(OPERATOR operation, ETYPE etype, string out, string in1, string in2)
{
    stringstream ss;
    switch(operation) {
        case MAXIMUM:
        case MINIMUM:
        case LOGICAL_AND:
        case LOGICAL_OR:
        case LOGICAL_XOR:
            ss << _omp_critical(_assign(out, oper(operation, etype, in1, in2)), "accusync");
            break;
        default:
            switch(etype) {
                case COMPLEX64:
                case COMPLEX128:
                    ss << _omp_critical(_assign(out, oper(operation, etype, in1, in2)), "accusync");
                    break;
                default:
                    ss << _omp_atomic(_assign(out, oper(operation, etype, in1, in2)));
                    break;
            }
            break;
    }
    return ss.str();
}

}}}}
