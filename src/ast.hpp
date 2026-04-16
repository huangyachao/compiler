#pragma once
#include <memory>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <variant>
#include <functional>

enum class Type
{
    Int,
    Register,
    Variable,
    String,
    Unknown,
};

class IRValue
{
private:
    Type type;
    // std::variant<int, std::string> data;
    std::string data;

public:
    IRValue() = default;
    IRValue(Type type, std::string data)
    {
        this->type = type;
        this->data = data;
    }

    std::string ToString()
    {
        return data;
    }

    bool IsConst()
    {
        return type == Type::Int;
    }

    bool IsVariable()
    {
        return type == Type::Variable;
    }

    int GetConstValue()
    {
        return std::stoi(data);
    }
};

enum class SymbolKind
{
    Const,
    Var,
    Unknown,
};
struct Symbol
{
    SymbolKind kind;
    std::string value;
};

class SymbolTable
{
private:
    std::vector<std::unordered_map<std::string, Symbol>> table_list;
    std::unordered_map<std::string, int> name_count;

public:
    SymbolTable()
    {
        table_list.push_back({});
    }

    void CreateNewSymbolTable()
    {
        table_list.push_back({});
    }

    void DestroySymbolTable()
    {
        table_list.pop_back();
    }

    void Insert(std::string name, Symbol value)
    {
        auto &table = table_list[table_list.size() - 1];
        // value.value += "_" + std::to_string(table_list.size());
        table[name] = value;
    }

    Symbol Get(std::string name)
    {
        for (int i = table_list.size() - 1; i >= 0; --i)
        {
            auto table = table_list[i];
            if (table.count(name) > 0)
            {
                return table[name];
            }
        }
        abort();
    }

    void AddCount(std::string name)
    {
        ++name_count[name];
    }

    int GetCount(std::string name)
    {
        return name_count[name];
    }
};

extern SymbolTable global_symbol_table;
class IRBuilder
{
private:
    std::string ir;
    int reg_cnt = 0;

    std::string LoadIfVariable(IRValue value)
    {
        if (value.IsVariable())
        {
            std::string first_arg = NewReg();
            Emit("  " + first_arg + " = load " + value.ToString() + "\n");
            return first_arg;
        }
        return value.ToString();
    }

    IRValue CreateBinaryOp(IRValue first_value, IRValue second_value,
                           const std::string &op_name,
                           std::function<int(int, int)> const_op)
    {
        if (first_value.IsConst() && second_value.IsConst())
        {
            int result = const_op(first_value.GetConstValue(), second_value.GetConstValue());
            return IRValue(Type::Int, std::to_string(result));
        }
        else
        {
            std::string first_arg = LoadIfVariable(first_value);
            std::string second_arg = LoadIfVariable(second_value);
            std::string reg = NewReg();
            Emit("  " + reg + " = " + op_name + " " + first_arg + ", " + second_arg + "\n");
            return IRValue(Type::Register, reg);
        }
    }

    IRValue CreateNotEQ(IRValue a, IRValue b)
    {
        return CreateBinaryOp(a, b, "ne", [](int x, int y)
                              { return x != y; });
    }

    IRValue CreateEQ(IRValue a, IRValue b)
    {
        return CreateBinaryOp(a, b, "eq", [](int x, int y)
                              { return x == y; });
    }

    IRValue CreateGT(IRValue a, IRValue b)
    {
        return CreateBinaryOp(a, b, "gt", [](int x, int y)
                              { return x > y; });
    }

    IRValue CreateLT(IRValue a, IRValue b)
    {
        return CreateBinaryOp(a, b, "lt", [](int x, int y)
                              { return x < y; });
    }

    IRValue CreateGE(IRValue a, IRValue b)
    {
        return CreateBinaryOp(a, b, "ge", [](int x, int y)
                              { return x >= y; });
    }

    IRValue CreateLE(IRValue a, IRValue b)
    {
        return CreateBinaryOp(a, b, "le", [](int x, int y)
                              { return x <= y; });
    }

    IRValue CreateAdd(IRValue a, IRValue b)
    {
        return CreateBinaryOp(a, b, "add", [](int x, int y)
                              { return x + y; });
    }
    IRValue CreateSub(IRValue a, IRValue b)
    {
        return CreateBinaryOp(a, b, "sub", [](int x, int y)
                              { return x - y; });
    }
    IRValue CreateMul(IRValue a, IRValue b)
    {
        return CreateBinaryOp(a, b, "mul", [](int x, int y)
                              { return x * y; });
    }

    IRValue CreateDiv(IRValue a, IRValue b)
    {
        return CreateBinaryOp(a, b, "div", [](int x, int y)
                              { return x / y; });
    }

    IRValue CreateMod(IRValue a, IRValue b)
    {
        return CreateBinaryOp(a, b, "mod", [](int x, int y)
                              { return x % y; });
    }

    IRValue CreateAnd(IRValue a, IRValue b)
    {
        if (a.IsConst() && b.IsConst())
        {
            int result = a.GetConstValue() && b.GetConstValue();
            return IRValue(Type::Int, std::to_string(result));
        }
        std::string first_arg = LoadIfVariable(a);
        std::string second_arg = LoadIfVariable(b);
        std::string reg1 = NewReg();
        Emit("  " + reg1 + " = ne " + first_arg + ", " + "0" + "\n");
        std::string reg2 = NewReg();
        Emit("  " + reg2 + " = ne " + second_arg + ", " + "0" + "\n");
        std::string reg3 = NewReg();
        Emit("  " + reg3 + " = and " + reg1 + ", " + reg2 + "\n");
        return IRValue(Type::Register, reg3);
    }

    IRValue CreateOr(IRValue a, IRValue b)
    {
        if (a.IsConst() && b.IsConst())
        {
            int result = a.GetConstValue() || b.GetConstValue();
            return IRValue(Type::Int, std::to_string(result));
        }
        std::string first_arg = LoadIfVariable(a);
        std::string second_arg = LoadIfVariable(b);
        std::string reg1 = NewReg();
        Emit("  " + reg1 + " = ne " + first_arg + ", " + "0" + "\n");
        std::string reg2 = NewReg();
        Emit("  " + reg2 + " = ne " + second_arg + ", " + "0" + "\n");
        std::string reg3 = NewReg();
        Emit("  " + reg3 + " = or " + reg1 + ", " + reg2 + "\n");
        return IRValue(Type::Register, reg3);
    }

public:
    std::string
    NewReg()
    {
        return "%" + std::to_string(reg_cnt++);
    }

    IRValue CreateBinary(std::string opt, IRValue first_value, IRValue second_value)
    {
        if (opt == "+")
        {
            return CreateAdd(first_value, second_value);
        }
        else if (opt == "-")
        {
            return CreateSub(first_value, second_value);
        }
        else if (opt == "*")
        {
            return CreateMul(first_value, second_value);
        }
        else if (opt == "/")
        {
            return CreateDiv(first_value, second_value);
        }
        else if (opt == "%")
        {
            return CreateMod(first_value, second_value);
        }
        else if (opt == "!=")
        {
            return CreateNotEQ(first_value, second_value);
        }
        else if (opt == "==")
        {
            return CreateEQ(first_value, second_value);
        }
        else if (opt == ">")
        {
            return CreateGT(first_value, second_value);
        }
        else if (opt == "<")
        {
            return CreateLT(first_value, second_value);
        }
        else if (opt == ">=")
        {
            return CreateGE(first_value, second_value);
        }
        else if (opt == "<=")
        {
            return CreateLE(first_value, second_value);
        }
        else if (opt == "&&")
        {
            return CreateAnd(first_value, second_value);
        }
        else if (opt == "||")
        {
            return CreateOr(first_value, second_value);
        }
        abort();
    }

    void CreateReturn(IRValue value)
    {
        std::string ret = LoadIfVariable(value);
        Emit("  ret " + ret + "\n");
    }

    void Emit(const std::string &s)
    {
        ir += s;
    }

    std::string GetIR()
    {
        return ir;
    }
};
// 所有 AST 的基类
class BaseAST
{
public:
    virtual ~BaseAST() = default;
    virtual void Dump() const = 0;
    virtual IRValue GenerateIRExpr(IRBuilder &) const { return IRValue(Type::Unknown, ""); }
    virtual void GenerateIRStmt(IRBuilder &) const {}
};

class BaseExpAST : public BaseAST
{
public:
    virtual ~BaseExpAST() = default;
    virtual void Dump() const = 0;
    virtual IRValue GenerateIRExpr(IRBuilder &) const = 0;
};

class BaseStmtAST : public BaseAST
{
public:
    virtual ~BaseStmtAST() = default;
    virtual void Dump() const = 0;
    virtual void GenerateIRStmt(IRBuilder &) const = 0;
};

// CompUnit 是 BaseStmtAST
class CompUnitAST : public BaseStmtAST
{
public:
    // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override
    {
        std::cout << "CompUnitAST { ";
        func_def->Dump();
        std::cout << " }";
    }

    void GenerateIRStmt(IRBuilder &builder) const override
    {
        func_def->GenerateIRStmt(builder);
    }
};

// FuncDef 也是 BaseStmtAST
class FuncDefAST : public BaseStmtAST
{
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void Dump() const override
    {
        std::cout << "FuncDefAST { ";
        func_type->Dump();
        std::cout << ", ";
        std::cout << ident << ", ";
        block->Dump();
        std::cout << " } ";
    }

    void GenerateIRStmt(IRBuilder &builder) const override
    {
        builder.Emit("fun @" + ident + "(): ");
        func_type->GenerateIRStmt(builder);
        builder.Emit(" {\n");
        builder.Emit("%entry:\n");
        block->GenerateIRStmt(builder);
        builder.Emit("}\n");
    }
};

class FuncTypeAST : public BaseStmtAST
{
public:
    std::string func_type;
    void Dump() const override
    {
        std::cout << "FuncTypeAST { ";
        std::cout << func_type;
        std::cout << " } ";
    }

    void GenerateIRStmt(IRBuilder &builder) const override
    {

        if (func_type == "int")
        {
            builder.Emit("i32");
        }
        else
            abort();
    }
};

class BlockAST : public BaseStmtAST
{
public:
    std::vector<std::unique_ptr<BaseAST>> block_items;
    void Dump() const override
    {
        std::cout << "BlockAST { ";
        for (const auto &item : block_items)
        {
            item->Dump();
        }
        std::cout << " } ";
    }

    void GenerateIRStmt(IRBuilder &builder) const override
    {
        global_symbol_table.CreateNewSymbolTable();
        for (const auto &item : block_items)
        {
            item->GenerateIRStmt(builder);
        }
        global_symbol_table.DestroySymbolTable();
    }
};

class BlockItemAST : public BaseStmtAST
{
public:
    std::unique_ptr<BaseAST> item;
    void Dump() const override
    {
        std::cout << "BlockItemAST { ";
        item->Dump();
        std::cout << " } ";
    }

    void GenerateIRStmt(IRBuilder &builder) const override
    {
        item->GenerateIRStmt(builder);
    }
};

class DeclAST : public BaseStmtAST
{
public:
    std::unique_ptr<BaseAST> decl;
    void Dump() const override
    {
        std::cout << "DeclAST { ";
        decl->Dump();
        std::cout << " } ";
    }

    void GenerateIRStmt(IRBuilder &builder) const override
    {
        decl->GenerateIRStmt(builder);
    }
};

class ConstDeclAST : public BaseStmtAST
{
public:
    std::unique_ptr<BaseAST> btype;
    std::vector<std::unique_ptr<BaseAST>> const_defs;
    void Dump() const override
    {
        std::cout << "DeclAST { const ";
        btype->Dump();
        for (const auto &const_def : const_defs)
        {
            const_def->Dump();
        }
        std::cout << " } ";
    }

    void GenerateIRStmt(IRBuilder &builder) const override
    {
        btype->GenerateIRStmt(builder);
        for (const auto &const_def : const_defs)
        {
            const_def->GenerateIRStmt(builder);
        }
    }
};

class BTypeAST : public BaseExpAST
{
public:
    std::string btype;
    void Dump() const override
    {
        std::cout << "BTypeAST { ";
        std::cout << btype;
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        return IRValue(Type::String, btype);
    }
};

class ConstDefAST : public BaseStmtAST
{
public:
    std::string ident;
    std::unique_ptr<BaseAST> const_init_val;
    void Dump() const override
    {
        std::cout << "ConstDefAST { ";
        std::cout << ident;
        std::cout << " = ";
        const_init_val->Dump();
        std::cout << " } ";
    }

    void GenerateIRStmt(IRBuilder &builder) const override
    {
        IRValue value = const_init_val->GenerateIRExpr(builder);
        if (!value.IsConst())
        {
            abort();
        }
        global_symbol_table.Insert(ident, {SymbolKind::Const, value.ToString()});
    }
};

class ConstInitValAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> const_exp;
    void Dump() const override
    {
        std::cout << "ConstInitValAST { ";
        const_exp->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        return const_exp->GenerateIRExpr(builder);
    }
};

class ConstExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> exp;
    void Dump() const override
    {
        std::cout << "ConstExpAST { ";
        exp->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        return exp->GenerateIRExpr(builder);
    }
};

class VarDeclAST : public BaseStmtAST
{
public:
    std::unique_ptr<BaseAST> btype;
    std::vector<std::unique_ptr<BaseAST>> var_defs;
    void Dump() const override
    {
        std::cout << "VarDeclAST { ";
        btype->Dump();
        for (const auto &var_def : var_defs)
        {
            var_def->Dump();
        }
        std::cout << " } ";
    }

    void GenerateIRStmt(IRBuilder &builder) const override
    {
        btype->GenerateIRStmt(builder);
        for (const auto &var_def : var_defs)
        {
            IRValue variable = var_def->GenerateIRExpr(builder);
            global_symbol_table.AddCount(variable.ToString());
            int count = global_symbol_table.GetCount(variable.ToString());
            std::string variable_name = "@" + variable.ToString() + "_" + std::to_string(count);
            builder.Emit("  " + variable_name + " = alloc i32\n");
            std::string value = global_symbol_table.Get(variable.ToString()).value;
            if (value != "")
            {
                builder.Emit("  store " + value + ", " + variable_name + "\n");
            }
            Symbol symbol = {SymbolKind::Var, variable_name};
            global_symbol_table.Insert(variable.ToString(), symbol);
        }
    }
};

class VarDefAST : public BaseExpAST
{
public:
    std::string ident;
    std::unique_ptr<BaseAST> init_val;
    void Dump() const override
    {
        std::cout << "VarDefAST { ";
        std::cout << ident;
        std::cout << " = ";
        if (init_val != nullptr)
        {
            init_val->Dump();
        }

        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {

        if (init_val != nullptr)
        {
            IRValue value = init_val->GenerateIRExpr(builder);
            global_symbol_table.Insert(ident, {SymbolKind::Var, value.ToString()});
        }
        else
        {
            global_symbol_table.Insert(ident, {SymbolKind::Var, ""});
        }

        return IRValue(Type::Variable, ident);
    }
};

class InitValAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> exp;
    void Dump() const override
    {
        std::cout << "InitValAST { ";
        exp->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        return exp->GenerateIRExpr(builder);
    }
};

class LValAST : public BaseExpAST
{
public:
    std::string ident;
    void Dump() const override
    {
        std::cout << "LValAST { ";
        std::cout << ident;
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        Symbol symbol = global_symbol_table.Get(ident);
        if (symbol.kind == SymbolKind::Const)
        {
            return IRValue(Type::Int, symbol.value);
        }
        else
        {
            return IRValue(Type::Variable, symbol.value);
        }
    }
};
class StmtAST : public BaseStmtAST
{
public:
    std::unique_ptr<BaseAST> lval;
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> block;
    bool is_return;
    void Dump() const override
    {
        if (lval != nullptr)
        {
            std::cout << "StmtAST { ";
            lval->Dump();
            std::cout << " = ";
            exp->Dump();
            std::cout << " } ";
        }
        else if (block != nullptr)
        {
            std::cout << "StmtAST { ";
            block->Dump();
            std::cout << " } ";
        }
        else if (exp != nullptr && is_return)
        {
            std::cout << "StmtAST { return ";
            exp->Dump();
            std::cout << " } ";
        }
        else if (exp != nullptr && !is_return)
        {
            std::cout << "StmtAST { ";
            exp->Dump();
            std::cout << " } ";
        }
        else if (exp == nullptr && is_return)
        {
            std::cout << "StmtAST { return ";
            std::cout << " } ";
        }
    }

    void GenerateIRStmt(IRBuilder &builder) const override
    {
        if (lval != nullptr)
        {
            IRValue variable = lval->GenerateIRExpr(builder);
            if (!variable.IsVariable())
            {
                abort();
            }
            IRValue exp_value = exp->GenerateIRExpr(builder);
            builder.Emit("  store " + exp_value.ToString() + ", " + variable.ToString() + "\n");
        }
        else if (block != nullptr)
        {
            block->GenerateIRStmt(builder);
        }
        else if (exp != nullptr && is_return)
        {
            builder.CreateReturn(exp->GenerateIRExpr(builder));
        }
        else if (exp != nullptr && !is_return)
        {
        }
        else if (exp == nullptr && is_return)
        {
        }
    }
};
class ExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> lor_exp;
    void Dump() const override
    {
        std::cout << "ExpAST { ";
        lor_exp->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        return lor_exp->GenerateIRExpr(builder);
    }
};

class MulExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> mul_exp;
    std::string op;
    std::unique_ptr<BaseAST> unar_exp;

    void Dump() const override
    {
        std::cout << "ExpAST { ";
        if (op != "")
        {
            mul_exp->Dump();
            std::cout << op << " ";
        }
        unar_exp->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return unar_exp->GenerateIRExpr(builder);
        }

        IRValue first_value = mul_exp->GenerateIRExpr(builder);
        IRValue second_value = unar_exp->GenerateIRExpr(builder);
        return builder.CreateBinary(op, first_value, second_value);
    }
};

class AddExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> add_exp;
    std::string op;
    std::unique_ptr<BaseAST> mul_exp;
    void Dump() const override
    {
        std::cout << "ExpAST { ";
        if (op != "")
        {
            add_exp->Dump();
            std::cout << op << " ";
        }
        mul_exp->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return mul_exp->GenerateIRExpr(builder);
        }
        IRValue first_value = add_exp->GenerateIRExpr(builder);
        IRValue second_value = mul_exp->GenerateIRExpr(builder);
        return builder.CreateBinary(op, first_value, second_value);
    }
};

class RelExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> rel_exp;
    std::string op;
    std::unique_ptr<BaseAST> add_exp;
    void Dump() const override
    {
        std::cout << "ExpAST { ";
        if (op != "")
        {
            rel_exp->Dump();
            std::cout << op << " ";
        }
        add_exp->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return add_exp->GenerateIRExpr(builder);
        }
        IRValue first_value = rel_exp->GenerateIRExpr(builder);
        IRValue second_value = add_exp->GenerateIRExpr(builder);
        return builder.CreateBinary(op, first_value, second_value);
    }
};

class EqExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> eq_exp;
    std::string op;
    std::unique_ptr<BaseAST> rel_exp;
    void Dump() const override
    {
        std::cout << "ExpAST { ";
        if (op != "")
        {
            eq_exp->Dump();
            std::cout << op << " ";
        }
        rel_exp->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return rel_exp->GenerateIRExpr(builder);
        }
        IRValue first_value = eq_exp->GenerateIRExpr(builder);
        IRValue second_value = rel_exp->GenerateIRExpr(builder);
        return builder.CreateBinary(op, first_value, second_value);
    }
};

class LAndExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> land_exp;
    std::string op;
    std::unique_ptr<BaseAST> eq_exp;
    void Dump() const override
    {
        std::cout << "ExpAST { ";
        if (op != "")
        {
            land_exp->Dump();
            std::cout << "&& ";
        }
        eq_exp->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return eq_exp->GenerateIRExpr(builder);
        }
        IRValue first_value = land_exp->GenerateIRExpr(builder);
        IRValue second_value = eq_exp->GenerateIRExpr(builder);
        return builder.CreateBinary(op, first_value, second_value);
    }
};

class LOrExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> lor_exp;
    std::string op;
    std::unique_ptr<BaseAST> land_exp;
    void Dump() const override
    {
        std::cout << "ExpAST { ";
        if (op != "")
        {
            lor_exp->Dump();
            std::cout << "or ";
        }
        land_exp->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return land_exp->GenerateIRExpr(builder);
        }
        IRValue first_value = lor_exp->GenerateIRExpr(builder);
        IRValue second_value = land_exp->GenerateIRExpr(builder);
        return builder.CreateBinary(op, first_value, second_value);
    }
};

class UnaryExpAST : public BaseExpAST
{
public:
    std::string unaryOp;
    std::unique_ptr<BaseAST> exp_ast;

    void Dump() const override
    {
        std::cout << "UnaryExp { ";
        std::cout << unaryOp;
        exp_ast->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        IRValue ir_value = exp_ast->GenerateIRExpr(builder);
        std::string value = ir_value.ToString();
        if (unaryOp == "+")
        {
            return ir_value;
        }
        else if (unaryOp == "-")
        {
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = sub 0, " + value + "\n");
            return IRValue(Type::Register, reg);
        }
        else if (unaryOp == "!")
        {
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = eq " + value + ", 0" + +"\n");
            return IRValue(Type::Register, reg);
        }
        else
        {
            if (unaryOp != "")
                abort();
            return ir_value;
        }
    }
};

class PrimaryExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> exp_ast;
    void Dump() const override
    {
        std::cout << "PrimaryExp { ";
        exp_ast->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        return exp_ast->GenerateIRExpr(builder);
    }
};

class ParenExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> exp_ast;
    void Dump() const override
    {
        std::cout << "ParenExp { ";
        std::cout << " ( ";
        exp_ast->Dump();
        std::cout << " ) ";
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        return exp_ast->GenerateIRExpr(builder);
    }
};

class NumberExpAST : public BaseExpAST
{
public:
    int number;
    void Dump() const override
    {
        std::cout << "NumberExp { ";
        std::cout << number;
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        return IRValue(Type::Int, std::to_string(number));
    }
};