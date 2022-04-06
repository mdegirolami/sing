#include "value_checker.h"
#include "ast_nodes.h"

namespace SingNames {

/*
    not usable as divisor / for dereference / for write dereference in pure functions
    
    VF_ISPOINTED        // don't track: any funcall / dereference of same type may be a write
    VF_ISARG /output
    VF_IS_REFERENCE
    VF_IS_ITERATED      // KO, is a vector

    usable, no need to track the writes

    VF_ISARG /input
    VF_ISFORINDEX       // the counter
    VF_ISFORITERATOR    // must NOT have VF_IS_REFERENCE
    no flags (global)   // OK if read only.

    usable, need to track writes

    VF_ISLOCAL          // OK.
*/
static bool VarFlagsAreRight(const VarDeclaration *var)
{
    // Local or read-only global/arg
    if (var->HasOneOfFlags(VF_ISPOINTED | VF_IS_REFERENCE | VF_IS_ITERATED)) return(false);
    if (!var->HasOneOfFlags(VF_ISFORINDEX | VF_ISFORITERATOR | VF_ISLOCAL | VF_READONLY)) return(false);
    return(true);
}

static bool VarIsObservable(const VarDeclaration *var, const ExpressionAttributes *attr)
{
    return(VarFlagsAreRight(var) && (attr->IsInteger() || attr->IsStrongPointer()));
}

static const VarDeclaration *observableVarFromExp(const IAstExpNode *exp, bool *isint, bool *isptr)
{
    *isint = *isptr = false;
    if (exp->GetType() == ANT_EXP_LEAF) {
        const AstExpressionLeaf *node = (const AstExpressionLeaf*)exp;
        if (node->subtype_ == TOKEN_NAME) {
            VarDeclaration *var = (VarDeclaration*)node->wp_decl_;
            if (var != nullptr && var->GetType() == ANT_VAR && VarFlagsAreRight(var)) {
                const ExpressionAttributes *attr = ((IAstExpNode *)exp)->GetAttr();
                if (attr == nullptr || attr->IsOnError()) {
                    return(nullptr);
                } else if (attr->IsInteger()) {
                    *isint = true;
                    return(var);                
                } else if (attr->IsStrongPointer()) {
                    *isptr = true;
                    return(var);
                }
            }
        }
    }
    return(nullptr);
}

static bool isIntegerCtc(const IAstExpNode *exp, bool *is_zero)
{
    *is_zero = false;
    const ExpressionAttributes *attr = ((IAstExpNode *)exp)->GetAttr();
    if (attr != nullptr && !attr->IsOnError() && attr->IsInteger() && attr->HasKnownValue()) {
        *is_zero = attr->GetValue()->GetSignedIntegerValue() == 0;
        return(true);                
    }
    return(false);
}

static bool isNull(const IAstExpNode *exp)
{
    const ExpressionAttributes *attr = ((IAstExpNode *)exp)->GetAttr();
    return(attr != nullptr && attr->IsLiteralNull());                
}

// called in CheckNamedLeaf
void ValueChecker::onVariableAccess(const VarDeclaration *var, const ExpressionAttributes *attr, const ExpressionUsage usage)
{
    if (usage == ExpressionUsage::READ || usage == ExpressionUsage::NONE || var->HasOneOfFlags(VF_READONLY)) return;
    if (VarIsObservable(var, attr)) {
        KnownState st;

        st.status_ = VarStatus::UNKNOWN;
        st.var_ = var;
        states_.push_back(st);
    }
}

void ValueChecker::onAssignment(const ExpressionAttributes &left, const ExpressionAttributes &right)
{
    if (left.IsOnError() || right.IsOnError()) {
        return;
    }

    // assigned must be a variable.
    if (!left.IsAVariable()) return;
    VarDeclaration *var = left.GetVariable();
    if (var == nullptr) return;
    if (!VarFlagsAreRight(var)) return;

    KnownState st;
    st.var_ = var;
    st.status_ = VarStatus::UNKNOWN;

    if (left.IsInteger()) {
        if (right.HasKnownValue()) {
            if (right.GetValue()->GetSignedIntegerValue() == 0) {
                st.status_ = VarStatus::ZERO;
            } else {
                st.status_ = VarStatus::NONZERO;
            }
        }
        states_.push_back(st);
    } else if (left.IsStrongPointer()) {
        bool writable;
        if (right.GetTypeFromAddressOperator(&writable) != nullptr) {
            VarDeclaration *pointed_var = right.GetVariable();
            if (pointed_var != nullptr) {
                st.status_ = VarStatus::LOCAL_ADD;
            }
        }
        states_.push_back(st);
    }
}

void ValueChecker::onIf(const IAstExpNode *exp)
{
    FlowBranch stt;

    initNewBranch(&stt);
    if (exp != nullptr) {
        extractConditions(exp);
        stt.branch_start_ = states_.size();
    }
    stack_.push_back(stt);
}

void ValueChecker::onElseIf(const IAstExpNode *exp)
{
    size_t stacklen = stack_.size();
    if (stacklen < 1) return;
    FlowBranch *stt = &stack_[stacklen - 1];
    saveDirtyVars(stt);
    invertLastCondition(stt);
    stt->curr_conditions_ = states_.size();
    if (exp != nullptr) {
        extractConditions(exp);
    }
    stt->branch_start_ = states_.size();
    stt->if_return_pattern_ = IfReturnPattern::NOPE;
}

void ValueChecker::onEndIf(void)
{
    size_t stacklen = stack_.size();
    if (stacklen < 1) return;
    FlowBranch *stt = &stack_[stacklen - 1];
    if (stt->if_return_pattern_ == IfReturnPattern::DETECTED) {
        states_.erase(stt->branch_start_, states_.size());        
        invertLastCondition(stt);
    } else {
        saveDirtyVars(stt);
        states_.erase(stt->conditions_start_, states_.size());
        pasteDirtyVars(stt);
    }
    stack_.pop_back();
}

void ValueChecker::onBreak(void)
{
    size_t stacklen = stack_.size();
    if (stacklen < 1) return;
    FlowBranch *stt = &stack_[stacklen - 1];
    if (stt->if_return_pattern_ == IfReturnPattern::INIT) {
        stt->if_return_pattern_ = IfReturnPattern::DETECTED;
    }
}

void ValueChecker::initNewBranch(FlowBranch *stt)
{
    stt->branch_start_ = stt->conditions_start_ = stt->curr_conditions_ = states_.size();
    stt->isloop_ = false;
    stt->if_return_pattern_ = IfReturnPattern::INIT;
    stt->dirty_ = dirty_vars_.size();
}

void ValueChecker::extractConditions(const IAstExpNode *exp)
{
    if (exp->GetType() != ANT_BINOP) return;
    const AstBinop *op = (AstBinop*)exp;
    if (op->subtype_ == TOKEN_LOGICAL_AND) {
        int base = states_.size();
        extractAndedConditions(op->operand_left_);
        extractAndedConditions(op->operand_right_);
        for (int ii = base; ii < states_.size(); ++ii) {
            states_[ii].invertible_ = false;
        }
    } else if (op->subtype_ == TOKEN_LOGICAL_OR) {
        int base = states_.size();
        extractOredContditions(op->operand_left_);
        extractOredContditions(op->operand_right_);
        for (int ii = base; ii < states_.size(); ++ii) {
            states_[ii].valid_after_inversion_ = true;
        }
    } else if (op->subtype_ == TOKEN_EQUAL || op->subtype_ == TOKEN_DIFFERENT) {
        extractRelationalCondition(op);
    }
}

void ValueChecker::extractAndedConditions(const IAstExpNode *exp)
{
    if (exp->GetType() != ANT_BINOP) return;
    const AstBinop *op = (AstBinop*)exp;
    if (op->subtype_ == TOKEN_LOGICAL_AND) {
        extractAndedConditions(op->operand_left_);
        extractAndedConditions(op->operand_right_);
    } else if (op->subtype_ == TOKEN_EQUAL || op->subtype_ == TOKEN_DIFFERENT) {
        extractRelationalCondition(op);
    }
}

void ValueChecker::extractOredContditions(const IAstExpNode *exp)
{
    if (exp->GetType() != ANT_BINOP) return;
    const AstBinop *op = (AstBinop*)exp;
    if (op->subtype_ == TOKEN_LOGICAL_OR) {
        extractOredContditions(op->operand_left_);
        extractOredContditions(op->operand_right_);
    } else if (op->subtype_ == TOKEN_EQUAL || op->subtype_ == TOKEN_DIFFERENT) {
        extractRelationalCondition(op);
    }
}

void ValueChecker::extractRelationalCondition(const AstBinop *op)
{
    bool isint;
    bool isptr;
    const IAstExpNode *constant;
    const VarDeclaration *var = observableVarFromExp(op->operand_left_, &isint, &isptr);
    if (var != nullptr) {
        constant = op->operand_right_;
    } else {
        var = observableVarFromExp(op->operand_right_, &isint, &isptr);
        constant = op->operand_left_;
    }
    if (var != nullptr) {
        KnownState st;
        st.var_ = var;
        st.status_ = VarStatus::UNKNOWN;
        st.invertible_ = true;
        bool iszero;
        if (isint && isIntegerCtc(constant, &iszero)) {
            if (op->subtype_ == TOKEN_DIFFERENT) {
                if (!iszero) return;
                st.status_ = VarStatus::NONZERO;
            } else {
                st.status_ = iszero ? VarStatus::ZERO : VarStatus::NONZERO;
                st.invertible_ = iszero;
            }
            states_.push_back(st);                
        } else if (isptr && isNull(constant)) {
            st.status_ = op->subtype_ == TOKEN_DIFFERENT ? VarStatus::NONZERO : VarStatus::ZERO;
            states_.push_back(st);                
        }
    }
}

void ValueChecker::invertLastCondition(const FlowBranch *stt)
{
    size_t dst = stt->curr_conditions_;
    for (int ii = dst; ii < stt->branch_start_; ++ii) {
        KnownState *state = &states_[ii];
        if (state->invertible_) {
            state->status_ = state->status_ == VarStatus::ZERO ? VarStatus::NONZERO : VarStatus::ZERO;
            state->invertible_ = false;
            state->valid_after_inversion_ = false;
            if (dst != ii) {
                states_[dst] = *state;
            }
            dst++;
        }
    }
    if (dst != stt->branch_start_) states_.erase(dst, stt->branch_start_);
}

void ValueChecker::saveDirtyVars(const FlowBranch *stt)
{
    for (int ii = stt->branch_start_; ii < states_.size(); ++ii) {
        KnownState *state = &states_[ii];
        if (state->status_ == VarStatus::ZERO || state->status_ == VarStatus::UNKNOWN) {
            dirty_vars_.push_back(*state);
        }
    }
    states_.erase(stt->branch_start_, states_.size());
}

void ValueChecker::pasteDirtyVars(const FlowBranch *stt)
{
    int count = (int)dirty_vars_.size() - stt->dirty_;
    if (count > 0) {
        states_.insert_range(states_.size(), count, &dirty_vars_[stt->dirty_]);
        dirty_vars_.erase(stt->dirty_, dirty_vars_.size());
    }
}

void ValueChecker::onWhile(const IAstExpNode *exp)
{
    FlowBranch stt;

    initNewBranch(&stt);
    stt.isloop_ = true;
    extractConditions(exp);
    stt.branch_start_ = states_.size();
    stack_.push_back(stt);
}

void ValueChecker::onLoopEnd(void)
{
    size_t stacklen = stack_.size();
    if (stacklen < 1) return;
    FlowBranch *stt = &stack_[stacklen - 1];
    saveDirtyVars(stt);
    invertLastCondition(stt);
    pasteDirtyVars(stt);
    stack_.pop_back();
}

void ValueChecker::onFor(void)
{
    FlowBranch stt;

    initNewBranch(&stt);
    stt.isloop_ = true;
    stack_.push_back(stt);
}

void ValueChecker::onFunctionStart(void)
{
    dirty_vars_.clear();
    stack_.clear();
    states_.clear();
    deferred_.clear();
    deferred_scan_ = 0;
}

const DeferredCheck *ValueChecker::getError(void)
{
    while (deferred_scan_ < deferred_.size()) {
        if (deferred_[deferred_scan_].var_->HasOneOfFlags(VF_ISPOINTED)) {
            return(&deferred_[deferred_scan_++]);
        }
        deferred_scan_++;
    }
    return(nullptr);
}

const char *ValueChecker::GetErrorString(TypeOfCheck check)
{
    if (check == TypeOfCheck::NULL_DEREFERENCE) {
        return("To prevent null dereferencing, you can only dereference pointers declared as local vars after checking their value against null in a place near enough to the usage location.");
    } else {
        return("To prevent integer division by 0, the divisor must be a compile time constant or a local variable/constant tested against 0 in a place near enough to the usage location.");
    }
}

bool ValueChecker::isNonZero(const VarDeclaration *var)
{
    // remember: if the var is not a constant, ignore what happened before a loop you are inside of.
    int bottom = 0;
    if (var->HasOneOfFlags(VF_IS_NOT_NULL)) {
        return(true);
    }
    if (!var->HasOneOfFlags(VF_READONLY)) {
        for (int ii = stack_.size() - 1; ii >= 0; --ii) {
            if (stack_[ii].isloop_) {
                bottom = stack_[ii].conditions_start_;
                break;
            }
        }
    }
    for (int ii = states_.size() - 1; ii >= bottom; --ii) {
        if (states_[ii].var_ == var && !states_[ii].valid_after_inversion_) {
            VarStatus status = states_[ii].status_;
            return(status == VarStatus::NONZERO || status == VarStatus::LOCAL_ADD);
        }
    }
    return(false);
}

bool ValueChecker::zeroDivisionIsSafe(const AstBinop *division_exp)
{
    const IAstExpNode *divisor_exp = division_exp->operand_right_;
    if (divisor_exp == nullptr) return(true);   // should never happen

    const ExpressionAttributes *attr = ((IAstExpNode *)divisor_exp)->GetAttr();
    if (attr == nullptr) return(true);          // should never happen

    if (!attr->IsInteger() || attr->HasKnownValue()) return(true);

    bool isint, isptr;
    const VarDeclaration *var = observableVarFromExp(divisor_exp, &isint, &isptr);
    if (var == nullptr || !isNonZero(var)) return(false);

    // in case it is an escaping local and we discover it only later.
    if (var->HasOneOfFlags(VF_ISLOCAL)) {
        DeferredCheck dc;
        dc.location_ = division_exp;
        dc.var_ = var;
        dc.type_ = TypeOfCheck::ZERO_DIVISOR;
        deferred_.push_back(dc);
    }

    return(true);
}

bool ValueChecker::pointerDereferenceIsSafe(const AstUnop *op)
{
    const IAstExpNode *pointer = op->operand_;
    if (pointer == nullptr) return(true);   // should never happen

    bool isint, isptr;
    const VarDeclaration *var = observableVarFromExp(pointer, &isint, &isptr);
    if (var == nullptr || !isNonZero(var)) return(false);

    // in case it is an escaping local and we discover it only later.
    if (var->HasOneOfFlags(VF_ISLOCAL)) {
        DeferredCheck dc;
        dc.location_ = op;
        dc.var_ = var;
        dc.type_ = TypeOfCheck::NULL_DEREFERENCE;
        deferred_.push_back(dc);
    }

    return(true);
}

bool ValueChecker::pointerToMemberOpIsSafe(const AstBinop *op)
{
    const IAstExpNode *pointer = op->operand_left_;
    if (pointer == nullptr) return(true);   // should never happen

    bool isint, isptr;
    const VarDeclaration *var = observableVarFromExp(pointer, &isint, &isptr);
    if (var == nullptr || !isNonZero(var)) return(false);

    // in case it is an escaping local and we discover it only later.
    if (var->HasOneOfFlags(VF_ISLOCAL)) {
        DeferredCheck dc;
        dc.location_ = op;
        dc.var_ = var;
        dc.type_ = TypeOfCheck::NULL_DEREFERENCE;
        deferred_.push_back(dc);
    }

    return(true);
}

/*
if (a) {
                // assumes a
} else if (b) {
                // assumes !a, b
} else {
                // assumes !a, !b
}
// assumes nothing about a,b

if (a || b || c) {
    // assumes nothing (because of the ||)
    ...
    unconditioned break; or continue; or return; 
}
// assumes !a, !b, !c 
// known states before the if are retained despite accesses in the if body

if (a && b) {
                // assumes a, b
} else if (c) {
                // assumes c
} else {
                // assumes !c
}

if (a || b) {
                // assumes nothing
} else if (c) {
                // assumes !a, !b, c
} else {
                // assumes !a, !b, !c
}

switch(intstuff_or_ptr) { 
case value: // assumes nothing
}
// assumes nothing

while(a) {
            // assumes a
} 
// assumes !a

while(a && b) {
            // assumes a, b
} 
// assumes nothing

while(a || b) {
            // assumes nothing
} 
// assumes !a, !b

for (count, a in range) {
                    // assumes nothing
}
// assumes nothing

entering a switch: previous knowledge is retained
exiting a switch: p. knowledge + all accessed vars in if blocks set as unknown.
entering a loop: prev. k. is ignored (except info about run time const).
exiting a loop: p. knowledge + all accessed vars in loop block set as unknown. 

RULE:
Locals: local non-const pointers in Pure functions must be inited and written with addresses of local vars. (don't even need to check for null)

while (src != null && last != null) {
    Node nn = *src;
    last.next = &nn;
    last = &nn;
}

exp to detect:

<ptr> write -> unknown
<ptr> = &local -> local
<ptr> [!]= null -> zero / nonzero

<int> write -> unknown
<int> = <cc> -> zero / nonzero
<int> [!]= <cc> -> zero / nonzero

detect from tree: <ptr>, <int>, <cc>, null

*/

} // namespace