#ifndef VALUE_CHECKER_H
#define VALUE_CHECKER_H

#include "ast_nodes.h"
#include "lexer.h"
#include "expression_attributes.h"

namespace SingNames {

enum class VarStatus { ZERO, NONZERO, UNKNOWN, LOCAL_ADD };

//
// invertible: 
// - single / multiple checks chained by && between them or with non relevant checks are not invertible. 
// - condition var == const where const is != 0 is not invertible. (x ==5 inverted becomes x != 5 which doesn't imply x == 0 or x != 0) 
//
struct KnownState {
    KnownState() { invertible_ = false; valid_after_inversion_ = false; }

    const VarDeclaration    *var_;
    VarStatus               status_;
    bool                    invertible_;            // != 0 inverts to == 0, != 5 inverts to unknown
    bool                    valid_after_inversion_; // condition of type x || y || ..  
};

enum class IfReturnPattern { INIT, NOPE, DETECTED };

struct FlowBranch {

    // indices into states_
    int conditions_start_;  // list of inherited conditions from previous if-else cases
    int curr_conditions_;   // conditions of this branch
    int branch_start_;      // states detected into the branch  

    int dirty_;             // index in dirty_vars_
    bool isloop_;
    IfReturnPattern if_return_pattern_;
};

enum class TypeOfCheck { ZERO_DIVISOR, NULL_DEREFERENCE };

struct DeferredCheck {
    const VarDeclaration    *var_;
    const IAstExpNode       *location_; 
    TypeOfCheck             type_;  
};

class ValueChecker {
public:
    bool zeroDivisionIsSafe(const AstBinop *division_exp);
    bool pointerDereferenceIsSafe(const AstUnop *op);
    bool pointerToMemberOpIsSafe(const AstBinop *op);
    const DeferredCheck *getError(void);
    const char *GetErrorString(TypeOfCheck check);

    // events
    void onVariableAccess(const VarDeclaration *var, const ExpressionAttributes *attr, const ExpressionUsage usage);
    void onAssignment(const ExpressionAttributes &left, const ExpressionAttributes &right);

    void onIf(const IAstExpNode *exp);
    void onElseIf(const IAstExpNode *exp);
    void onEndIf(void);
    void onBreak(void);

    void onWhile(const IAstExpNode *exp);
    void onFor(void);
    void onLoopEnd(void);

    void onFunctionStart(void);

private:
    vector<KnownState>      states_;
    vector<FlowBranch>      stack_;
    vector<KnownState>      dirty_vars_;    // accessed into a child block.
    vector<DeferredCheck>   deferred_;
    int                     deferred_scan_;

    void initNewBranch(FlowBranch *stt);
    void extractConditions(const IAstExpNode *exp);
    void extractAndedConditions(const IAstExpNode *exp);
    void extractOredContditions(const IAstExpNode *exp);
    void extractRelationalCondition(const AstBinop *op);
    void invertLastCondition(const FlowBranch *stt);
    void saveDirtyVars(const FlowBranch *stt);
    void pasteDirtyVars(const FlowBranch *stt);
    bool isNonZero(const VarDeclaration *var);
};

} // namespace

#endif

