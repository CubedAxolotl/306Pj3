// src/tools/calculator_eval.cpp — calc_evaluate (spec §4.6).
// Two-stack evaluation: `values` holds numbers, `ops` holds operators and '('.

#include "tools/calculator.h"

#include <stack>
#include <string>

using Kind = CalcToken::Kind;

static int precedence(Kind k) {
    if (k == Kind::Times || k == Kind::Divide) {
        return 2;
    }
    if (k == Kind::Plus  || k == Kind::Minus){
        return 1;
    }  
    return 0;
}

// Pop one operator and two numbers, then push.
static bool apply(std::stack<double>& values, std::stack<Kind>& ops, std::string* error) {
    if (ops.empty() || values.size() < 2) {          // check if operation is trash
        *error = "malformed expression";
        return false;
    }
    Kind op = ops.top();        
    ops.pop();
    double right = values.top(); //right comes first
    values.pop();      
    double left  = values.top(); 
    values.pop();

    if (op == Kind::Divide && right == 0.0) {//check for div by zero
        *error = "division by zero";
        return false;
    }
    if      (op == Kind::Plus)   values.push(left + right);
    else if (op == Kind::Minus)  values.push(left - right);
    else if (op == Kind::Times)  values.push(left * right);
    else if (op == Kind::Divide) values.push(left / right);
    else { *error = "malformed expression"; return false; }  
    return true;
}

bool calc_evaluate(const std::vector<CalcToken>& tokens, double* out, std::string* error) {
    std::stack<double> values;
    std::stack<Kind>   ops;

    for (const CalcToken& t : tokens) {
        if (t.kind == Kind::Number) {
            values.push(t.value);

        } else if (t.kind == Kind::LParen) {
            ops.push(t.kind);

        } else if (t.kind == Kind::RParen) {
            // Finish everything inside the parenthese
            while (!ops.empty() && ops.top() != Kind::LParen){
                if (!apply(values, ops, error)){
                     return false;
                }
            }
            if (ops.empty()) { 
                *error = "malformed expression"; return false; 
            }
            ops.pop();

        } else {   // + - * /

            while (!ops.empty() && precedence(ops.top()) >= precedence(t.kind)){
                if (!apply(values, ops, error)){ 
                    return false;
                }
            }
            ops.push(t.kind);
        }
    }

    // Out of tokens
    while (!ops.empty()){
        if (!apply(values, ops, error)){ 
            return false;
        }
    }
    // A correct expression leaves exactly one number left.
    if (values.size() != 1) { 
    *error = "malformed expression"; 
    return false; 
    }
    *out = values.top();
    return true;
}